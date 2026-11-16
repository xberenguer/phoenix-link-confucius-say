# Current release notes

## 0.2.107

- Added a short coin burst for cash-only Phoenix Hold awards from 25× to below 50× bet, based on the filmed Venetian 25× ending.
- The visible burst begins about 1.5 seconds before returning fire, with a brief tail at its first crest.
- Payouts, odds, music and the existing larger/named-award routes are unchanged.
- Updated the guide gallery and browser to the current source. Browser updates retain automatic replacement of older cached tabs.

<img src="docs/media/cash25-burst.webp" alt="Short Phoenix coin celebration" width="720">

The observed 25× event supports this reconstruction. The 25–49× visual range is a local policy; the exact original cutoff and particle law remain unrecovered.

## Validation status

The paired Main/UI/audio replay covers 34 scenarios and 48,350 full-viewport raster samples, with matching financial state, entropy captures, phase clocks, next-action timing and audio. First visible coin to return crest measures 1.518 seconds at the 33 ms replay cadence; the source observation is approximately 1.535 seconds. This comparison is a software timing check, not a full claim of identical original animation.

Native firmware and offline assets passed full write/readback verification and healthy fresh startup: current version, all 60 atlas frames, 1,000 credits and autoplay off. The source-bound browser passed 64 Chromium assertions, including two open 106 tabs upgrading to 107 and offline play. Physical speaker fidelity and original casino odds remain unqualified.

The game retains Mbed TLS 3.6.7 AES-256 CTR-DRBG and model I (`0x00020012`). See [testing](docs/testing.md), [math](docs/math-and-evidence.md) and [sources](docs/sources.md).
