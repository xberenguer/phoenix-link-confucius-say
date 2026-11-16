# Current release notes

## 0.2.110

- Phoenix collection birds keep their broad curl visible when middle- and bottom-row prizes fly into the central WINNER panel.
- Corrected the observed 50-line Line 22 path: top, top, bottom, top, top.
- Corrected the Base 10¢ / 50-line / 1× three-Jack award from eight to five credits ($0.50), with matching return-reference tables.
- Updated the browser game, guide, payout tables and gallery. Added an automated check that funded balances and previous outcome history do not change the next result.

<img src="docs/media/line22-jack3.png" alt="Line 22 and the five-credit three-Jack award" width="720">

<img src="docs/media/pickup-flight.webp" alt="Phoenix collection flight into the WINNER panel" width="720">

The price correction applies to the named configuration. The collection flight adapts the original upper-screen effect to the smaller display. See [payouts](docs/payouts.md#ten-cent-50-line-observation), [graphics](docs/graphics.md#collection-flight-visibility) and [math](docs/math-and-evidence.md).

## Validation

Firmware and offline assets passed full write/readback verification and healthy startup on StickS3. It starts with 1,000 denomination credits and autoplay off.

Software checks covered 31 Main/UI scenarios, 19,314 rendered frames and 102,454 paired game actions. Prize timing, sound sequences and outcome-service calls matched the preceding build. The browser passed 51 Chromium checks covering upgrades, offline play and failed-install recovery. See [release metadata](RELEASE.json) and [testing](docs/testing.md).

Original casino probabilities remain unrecovered. This release retains the local symbol, prize, Hold-arrival and meter distributions.
