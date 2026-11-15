"""Build and verify the single StickS3 image flashed at address 0x0.

Read flash settings from this build's ESP32-S3 bootloader: Arduino uses a
DIO image header even when the app enables QIO flash and octal PSRAM.
Do not silently substitute settings or publish a partially written image.
"""

import hashlib
import os
from pathlib import Path
import struct
import subprocess
import tempfile

_MODE = {0: "qio", 1: "qout", 2: "dio", 3: "dout"}
_FREQ = {0x0: "40m", 0x1: "26m", 0x2: "20m", 0xF: "80m"}
_SIZE = {0: "1MB", 1: "2MB", 2: "4MB", 3: "8MB", 4: "16MB", 5: "32MB"}
_FLASH_BYTES = 8 * 1024 * 1024
_APP_OFFSET = 0x10000
_OTA_OFFSET = 0xE000
_OTA_SIZE = 0x2000


def _image_header(data, name):
    if len(data) < 24 or data[0] != 0xE9 or not 1 <= data[1] <= 16:
        raise ValueError("%s: malformed ESP image header" % name)
    mode, freq, size = data[2], data[3] & 0x0F, data[3] >> 4
    if mode not in _MODE or freq not in _FREQ or size not in _SIZE:
        raise ValueError("%s: unsupported flash mode/frequency/size" % name)
    if size != 3:
        raise ValueError("%s: expected an 8MB flash image" % name)
    if struct.unpack_from("<H", data, 12)[0] != 9:
        raise ValueError("%s: image is not for ESP32-S3" % name)
    if data[23] not in (0, 1):
        raise ValueError("%s: invalid image hash flag" % name)
    return _MODE[mode], _FREQ[freq], _SIZE[size]


def _partition_layout(data, app_size):
    if not data or len(data) > 0x1000 or len(data) % 32:
        raise ValueError("invalid partition table length")
    entries = []
    have_md5 = False
    for offset in range(0, len(data), 32):
        entry = data[offset : offset + 32]
        if entry == b"\xFF" * 32:
            if any(byte != 0xFF for byte in data[offset:]):
                raise ValueError("unexpected data after partition table")
            break
        magic = struct.unpack_from("<H", entry)[0]
        if magic == 0xEBEB:
            if have_md5 or entry[2:16] != b"\xFF" * 14:
                raise ValueError("malformed partition table MD5 entry")
            if entry[16:] != hashlib.md5(data[:offset]).digest():
                raise ValueError("partition table MD5 mismatch")
            have_md5 = True
            continue
        if magic != 0x50AA or have_md5:
            raise ValueError("invalid partition table entry")
        _, kind, subtype, start, size, _, _ = struct.unpack("<HBBII16sI", entry)
        if not size or start < 0x9000 or start + size > _FLASH_BYTES:
            raise ValueError("partition outside the StickS3 flash layout")
        entries.append((kind, subtype, start, size))
    if not entries or not have_md5:
        raise ValueError("partition table is missing entries or MD5")
    previous_end = 0x9000
    for _, _, start, size in sorted(entries, key=lambda entry: entry[2]):
        if start < previous_end:
            raise ValueError("overlapping partitions")
        previous_end = start + size
    if not any(kind == 0 and subtype == 0x10 and start == _APP_OFFSET and
               size >= app_size for kind, subtype, start, size in entries):
        raise ValueError("app0 partition does not fit firmware at 0x10000")
    if not any(kind == 1 and subtype == 0 and start == _OTA_OFFSET and
               size == _OTA_SIZE for kind, subtype, start, size in entries):
        raise ValueError("expected otadata partition at 0xe000")
    if any(kind == 0 and subtype == 0 for kind, subtype, _, _ in entries):
        raise ValueError("unexpected factory partition changes erased-OTA boot selection")


def _verify_merged(data, images):
    expected_size = max(offset + len(image) for offset, image in images)
    if len(data) != expected_size or expected_size > _FLASH_BYTES:
        raise ValueError("merged image length does not match current build")
    cursor = 0
    for offset, image in images:
        if any(byte != 0xFF for byte in data[cursor:offset]):
            raise ValueError("merged image gap is not erased flash")
        if data[offset : offset + len(image)] != image:
            raise ValueError("merged bytes differ from current input at 0x%x" % offset)
        cursor = offset + len(image)
    # boot_app0.bin is not needed: default_8MB has no factory partition, so
    # fully erased OTA metadata selects app0. The merged file explicitly
    # writes this region, replacing any old app1 selection on the device.
    # https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32s3/api-reference/system/ota.html#ota-data-partition
    if data[_OTA_OFFSET : _OTA_OFFSET + _OTA_SIZE] != b"\xFF" * _OTA_SIZE:
        raise ValueError("OTA metadata is not fully erased")


def merge_firmware(source, target, env):
    build = Path(env.subst("$BUILD_DIR"))
    project = Path(env.subst("$PROJECT_DIR"))
    paths = [(0, build / "bootloader.bin"),
             (0x8000, build / "partitions.bin"),
             (_APP_OFFSET, build / "firmware.bin")]
    images = []
    for offset, path in paths:
        if not path.is_file():
            raise FileNotFoundError("merge_firmware: missing %s" % path)
        data = path.read_bytes()
        if not data:
            raise ValueError("merge_firmware: empty %s" % path)
        images.append((offset, data))
    mode, freq, size = _image_header(images[0][1], "bootloader.bin")
    _image_header(images[2][1], "firmware.bin")
    if len(images[0][1]) > 0x8000:
        raise ValueError("bootloader overlaps partition table")
    _partition_layout(images[1][1], len(images[2][1]))

    pkg_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
    if not pkg_dir:
        raise FileNotFoundError("merge_firmware: esptool package not installed")
    esptool = Path(pkg_dir) / "esptool.py"
    if not esptool.is_file():
        raise FileNotFoundError("merge_firmware: missing %s" % esptool)
    out = project / "release" / "phoenix-confucius-sticks3.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(prefix=".phoenix-confucius-sticks3-", suffix=".bin",
                                          dir=out.parent)
    os.close(fd)
    temporary = Path(temporary_name)
    try:
        cmd = [env.subst("$PYTHONEXE"), str(esptool), "--chip", "esp32s3",
               "merge_bin", "-o", str(temporary), "--flash_mode", mode,
               "--flash_freq", freq, "--flash_size", size]
        for offset, path in paths:
            cmd.extend(["0x%x" % offset, str(path)])
        print("Merging %s (%s %s %s)" % (out, mode, freq, size))
        subprocess.check_call(cmd)
        merged = temporary.read_bytes()
        _verify_merged(merged, images)
        # Reject source changes racing this merge instead of publishing an
        # image that no longer corresponds to the build files on disk.
        if any(path.read_bytes() != image for (_, path), (_, image)
               in zip(paths, images)):
            raise ValueError("build inputs changed during merge")
        with temporary.open("rb") as handle:
            os.fsync(handle.fileno())
        os.replace(temporary, out)
        print("verified %s (%d bytes; SHA256 %s)" %
              (out, len(merged), hashlib.sha256(merged).hexdigest()))
        # A separate, explicitly named application image supports a reviewed
        # app-only update at 0x10000 without overwriting NVS or the bootloader.
        app_out = out.parent / "phoenix-confucius-app.bin"
        app_fd, app_temporary_name = tempfile.mkstemp(prefix=".phoenix-confucius-app-",
                                                     suffix=".bin", dir=out.parent)
        app_temporary = Path(app_temporary_name)
        try:
            with os.fdopen(app_fd, "wb") as handle:
                handle.write(images[2][1])
                handle.flush()
                os.fsync(handle.fileno())
            if app_temporary.read_bytes() != images[2][1]:
                raise ValueError("application copy differs from verified build")
            os.replace(app_temporary, app_out)
            print("verified app-only %s (%d bytes; offset 0x10000; SHA256 %s)" %
                  (app_out, len(images[2][1]), hashlib.sha256(images[2][1]).hexdigest()))
        finally:
            app_temporary.unlink(missing_ok=True)
    finally:
        temporary.unlink(missing_ok=True)


# SCons supplies Import during a PlatformIO build. Ordinary import is also
# supported so failure-path tests can exercise the same implementation.
try:
    Import("env")
except NameError:
    env = None
if env is not None:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_firmware)
