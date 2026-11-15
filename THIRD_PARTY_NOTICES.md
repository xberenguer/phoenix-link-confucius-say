# Third-party notices

- Espressif `spiffsgen.py`: Apache License 2.0; copyright and license header retained in `firmware/scripts/vendor/spiffsgen.py`. License: https://www.apache.org/licenses/LICENSE-2.0
- `dr_mp3.h`: original author/license notices retained in `firmware/src/third_party/dr_mp3.h`.
- Arduino-ESP32, ESP-IDF and M5Stack dependencies retain their upstream licenses and are downloaded by PlatformIO rather than bundled here.

## MbedTLS crypto subset

AES, CTR_DRBG and platform zeroization use Mbed TLS 3.6.7, copyright the MbedTLS contributors. The source offers Apache-2.0 OR GPL-2.0-or-later; this project uses the Apache-2.0 option. Private include paths, symbol names and configuration wrappers isolate the subset from platform libraries. Original algorithm bodies and notices are retained. See the [included license](firmware/src/phoenix_crypto124/LICENSE) and [official upstream release](https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7).
