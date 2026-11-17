# Build and flash

Build the current firmware and its required asset filesystem. Browser instructions are in [browser play](browser-play.md). Every boot after installation opens a fresh zero-balance denomination chooser; existing game progress is not imported.

## Install the prebuilt game

Download `phoenix-confucius-sticks3.bin`, `feature-assets.bin` and `SHA256SUMS.txt` from the [current GitHub release](https://github.com/xberenguer/phoenix-link-confucius-say/releases/latest). These are the game image and its required offline graphics/music filesystem; no previous installation is required.

Install esptool in a Python environment (`python3 -m pip install esptool==4.9.0`), enter download mode as described below, then run from the download directory:

```sh
python3 -m esptool --chip esp32s3 --port PORT write_flash \
  0x0 phoenix-confucius-sticks3.bin \
  0x670000 feature-assets.bin
```

Replace `PORT` with your Stick's USB port. The checksums describe the exact release files. The source-build route below produces the same two image types.

## Hardware and toolchain

The target is M5Stack **StickS3**, with ESP32-S3, 8 MB flash, 8 MB PSRAM and a 135 × 240 display used in landscape. [Official hardware documentation](https://docs.m5stack.com/en/core/StickS3) describes the board and download mode. Purchase links: [M5Stack shop](https://shop.m5stack.com/products/m5sticks3-esp32s3-mini-iot-dev-kit), [Amazon](https://www.amazon.com/dp/B0GWHN8HK3).

Install [PlatformIO Core](https://docs.platformio.org/en/stable/core/installation/index.html). The [project configuration](../firmware/platformio.ini) pins the target/dependencies. The first build may download development tools; the installed game runs offline. The CSPRNG's private maintained MbedTLS subset is included in the source.

## Build both required images

From the repository root:

```sh
cd firmware
pio run -e m5stack-sticks3
python3 scripts/build_assets.py
```

The app contains the game, compiled art/audio and random service. The filesystem supplies `feature_music.mp3` and `phoenix_atlas.zlib`. An app build cannot replace a missing filesystem.

The portable [asset builder](../firmware/scripts/build_assets.py) uses bundled [spiffsgen.py](../firmware/scripts/vendor/spiffsgen.py) and checks the allowed files. The [merge script](../firmware/scripts/merge_firmware.py) validates image headers, bounds and overlaps.

| Output under `firmware/release/` | Purpose | Offset |
|---|---|---|
| `phoenix-confucius-sticks3.bin` | Merged boot/application installation image | `0x0` |
| `phoenix-confucius-app.bin` | Application image when layout/assets already match | `0x10000` |
| `feature-assets.bin` | Required asset filesystem | `0x670000` |

## Enter download mode

Use a USB data cable. Hold the small power button until the green LED blinks, then release, following [M5Stack's instructions](https://docs.m5stack.com/en/core/StickS3). The large grey gameplay button is a different control. Discover the actual port with `pio device list` and confirm the intended board.

## Installation

From `firmware/`, with the pinned esptool available:

```sh
python3 -m esptool --chip esp32s3 --port PORT write_flash \
  0x0 release/phoenix-confucius-sticks3.bin \
  0x670000 release/feature-assets.bin
```

Replace `PORT` with the discovered port. The installation writes those image ranges; there is no game-bank recovery step.

## Application-only write

When the partition layout and asset filesystem already match:

```sh
python3 -m esptool --chip esp32s3 --port PORT write_flash \
  0x10000 release/phoenix-confucius-app.bin
```

This excludes the filesystem; it cannot repair missing/mismatched assets. Use the [partition map](../firmware/partitions-offline.csv) as authority. Both installation methods boot into the same fresh-session policy.

## Verify the result

Verify image hashes/written bytes, current menu version, a zero balance and the startup denomination chooser and autoplay off. Exercise controls, an ordinary action and an audio audition. A build, a verified write, a native frame and physical sound/entropy behavior establish different things. Current completed validation is in [release notes](../CHANGELOG.md#validation).

See [troubleshooting](troubleshooting.md) for assets, USB and random-source errors. The firmware workflow and [table exporter](payouts.md#regenerating-the-reference) check current-source builds/data. A Pages workflow publishes the static `demo/` folder; [GitHub documents custom Pages workflows](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages).
