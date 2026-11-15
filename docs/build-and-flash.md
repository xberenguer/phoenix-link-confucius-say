# Build and flash

This page describes the current source layout. Firmware, runtime assets, implemented tables and gallery belong to the same release source. Build the application and required filesystem below; browser-building instructions are in [browser play](browser-play.md).

## Hardware and toolchain

The target is M5Stack **StickS3**, an ESP32-S3 device with 8 MB flash, 8 MB PSRAM and a 135 × 240 display used here in landscape. The ESP32-based StickC is a different target. See the [official hardware page](https://docs.m5stack.com/en/core/StickS3).

Install [PlatformIO Core](https://docs.platformio.org/en/stable/core/installation/index.html). The [project configuration](../firmware/platformio.ini) pins Espressif32 6.12.0, M5GFX 0.3.0 and explicit M5Unified/M5PM1 revisions. The first build needs internet to obtain those development dependencies. The installed game itself uses no network.

## Build both required images

From the repository root:

```sh
cd firmware
pio run -e m5stack-sticks3
python3 scripts/build_assets.py
```

The application contains the Engine, UI, compiled art and audio banks. The filesystem supplies exactly `feature_music.mp3` and `phoenix_atlas.zlib`. A successful app build does not replace a missing filesystem.

The portable [asset builder](../firmware/scripts/build_assets.py) uses bundled Espressif [spiffsgen.py](../firmware/scripts/vendor/spiffsgen.py). It avoids relying on an incompatible host `mkspiffs` executable. It rejects symlinks, missing files and unexpected data entries rather than packaging arbitrary personal files.

| Output under `firmware/release/` | Purpose | Flash offset |
|---|---|---|
| `phoenix-confucius-sticks3.bin` | Verified merged boot/application image for a fresh installation | `0x0` |
| `phoenix-confucius-app.bin` | Separate application image for a reviewed compatible app update | `0x10000` |
| `feature-assets.bin` | Required runtime filesystem | `0x670000` |

The [merge script](../firmware/scripts/merge_firmware.py) validates chip/image headers, partition bounds, overlap, merged bytes and erased OTA selection before installing its output filename.

## Enter download mode

Connect a USB data cable, hold the small power button until the green LED blinks, then release it. This is the procedure in the [M5Stack download-mode instructions](https://docs.m5stack.com/en/core/StickS3). The large grey game-control button is not the small power key.

Discover the serial port with `pio device list`; use that actual port in the commands below. Do not copy a developer's device identifier or assume any connected S3 board is the intended StickS3.

## Fresh installation

With the esptool version provided by the pinned toolchain, and while inside `firmware/`:

```sh
python3 -m esptool --chip esp32s3 --port PORT write_flash \
  0x0 release/phoenix-confucius-sticks3.bin \
  0x670000 release/feature-assets.bin
```

Replace `PORT` with the discovered port and invoke esptool from an environment in which it is installed. The pinned tool uses underscore command names such as `write_flash`.

**The merged fresh-install image is not a save-preserving update.** Its covered erased-fill ranges include the NVS area. Existing game data must be backed up and handled explicitly before using it on a device that already has a bank.

## Compatible app-only update

If the partition map and asset filesystem are already compatible, write only:

```sh
python3 -m esptool --chip esp32s3 --port PORT write_flash \
  0x10000 release/phoenix-confucius-app.bin
```

This range excludes NVS and the filesystem. A new layout or incompatible assets require a separate reviewed migration; an app-only write cannot fix a missing Phoenix atlas or music file. Do not use `erase_flash` as a routine update step.

## Verify the result

Check the expected chip/model and image hash before writing. Afterward, verify written bytes, boot the game, check the version in Game Menu, test both controls, check one sound audition and confirm that the expected saved bank/meter survived if this was a preserving update. Autoplay should be off after startup.

Those checks answer different questions: compiling proves a build, flash verification proves stored bytes, a frame proves rendering, and a restored checkpoint comparison proves data continuity. None establishes that the original machine's casino odds have been recovered.

Partition authority: [partitions-offline.csv](../firmware/partitions-offline.csv). Game data and recovery: [saving](saving.md). Common failures: [troubleshooting](troubleshooting.md).

## Repository checks and browser hosting

The build workflow compiles this firmware, creates the offline filesystem and compares freshly exported C++ tables with the documented CSVs. The Pages workflow deploys only `demo/`; choose GitHub Actions as the repository's Pages publishing source. See [GitHub's custom workflow guide](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages). Local validation is recorded separately from a successful hosted workflow run.
