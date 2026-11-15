# Runtime dependencies

The port uses Emscripten/LLVM/libc++ and its generated runtime under their upstream MIT/University of Illinois/Apache/LLVM exception licenses; retain the upstream notices applicable to the compiled toolchain/runtime. Emscripten 7.0.0 source and license information: https://github.com/emscripten-core/emscripten/tree/7.0.0 .

The actual firmware includes dr_mp3; its license is embedded in `firmware/src/third_party/dr_mp3.h`. The zlib port is under the zlib license: https://zlib.net/zlib_license.html . Firmware artwork/audio provenance and project licensing remain those of the main project; this adapter does not supply a new license for those assets.

Mbed TLS3.6.7: the minimal privately namespaced AES/CTR_DRBG/platform_util sources use the upstream Apache-2.0 licensing alternative. The full source license is retained at firmware/src/phoenix_crypto124/LICENSE. Configuration and prefix changes are identified in that source subtree.
