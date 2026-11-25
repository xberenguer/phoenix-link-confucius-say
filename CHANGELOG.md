# Release notes

## 0.2.173

- Added the shared confirmation click to denomination, play lines, bet confirmation and voucher digit edits.
- Added a first-screen StickS3 grey-button hold to switch between 10% and 100%, with a brief percentage indicator and equal click gain in both directions.
- Kept the existing credit-in sound for actual credit insertion.

## 0.2.171

- The collection counter can now reach **1887**, matching the supplied General Tso cabinet photograph. The next collected Phoenix awards and resets before an idle **1888** can appear.
- Set Feature Counter now accepts **0–1887**, including its digit editor, wager buckets and boundary validation.
- The browser game uses the same corrected counter rule. Existing graphics/audio and retained Grand amounts are unchanged.
- Previous numerical return and counter-entry estimates are explicitly historical; the corrected boundary needs new calculations. The six local schedule choices remain playable.

### Validation

The sanitized boundary suite passed 47,931 checks; the outcome suite passed 102,877,481 checks across 77 wagers and six Grand schedules. Target firmware/assets and source-bound browser builds pass. Release/media/source checks pass. A fresh physical installation of this version has not yet been observed. Historical galleries retain their original 0.2.170 capture provenance.

## Unified visual gallery and complete replays — 18 November 2026

Combined gameplay, symbols, animations and menus into one [Gallery](docs/graphics.md), with screenshots beside inline recorded motion and jackpots at the bottom. The [full player guide](docs/README.md) now includes the first-game walkthrough and a visual symbol key.

Added a [beginner's payline walkthrough](docs/paylines.md) with actual chart photos, enlarged individual routes, a real three-Tens winning example, bet-cost calculations and all 37 chart pages. The guide explains lines before the first wager selection.

The [complete payline reference](docs/paylines.md#every-line) shows every implemented line individually: all 205 numbered entries across the 5-, 10-, 15-, 25-, 50- and 100-line layouts. Each diagram is enlarged from an actual menu capture and its five dot positions are checked against the game's route data.

Replaced fixed-length animation parts with complete recorded actions and declared replay pauses, closed periodic symbol loops on matching real frames, and selected fully visible win screenshots. The playable game and its runtime assets are unchanged.

## Player gallery update — 18 November 2026

- Replaced staged preview boards with recorded play from the unchanged 0.2.170 game.
- Added chronological gameplay, individual symbols and their real motion, both Confucius merges, all nine named-prize variants, actual menu screens and every payline chart page.
- Put jackpot pictures and award sequences at the bottom of the galleries.
- Added capture journals, source bindings and media verification. The game source, rules, runtime assets and playable browser bundle are unchanged.

[Browse current gameplay](docs/graphics.md) · [Symbols and animation](docs/graphics.md) · [Menus and line charts](docs/menu-gallery.md)

Historical validation results below describe their tagged releases.


## 0.2.170

- Major uses WINNER-style gold lettering and a yellow amount over the held reels, replacing the opaque blue backdrop.
- Custom Amount starts at the thousands digit with **$0,000.00**. Default sound volume is **10%**.
- Expanded graphics gallery: individual symbols and their available animations, two-/three-row Confucius fusions, named-prize variants, feature effects, menus and cabinet artwork.
- Firmware, browser demo, game guide, gallery and signed flasher are synchronized to this release. Gameplay probability weights are unchanged.

### Validation

Complete firmware/assets readback and 18 fresh-start checks passed on the original Stick. All 77 wager startup paths, 30 manual/autoplay tally cases, 103 browser checks, 12 native/Wasm Major raster comparisons and the current-source gallery checks passed.

## 0.2.160

- Startup opens denomination selection, then PLAY LINES and BET, then the reels at a zero balance. Blue opens the Stardust custom voucher when funding is needed.
- Hold blue on the initial denomination chooser to switch Standard and High Limit. Hold grey for the full Game Menu, including sound, brightness and screen flip; closing it returns to setup.
- One blue press during the final Phoenix tally collects the remaining amounts immediately, stops autoplay and preserves the normal ending sequence.
- Original manufacturer portrait, curved temple and cloud artwork replace the earlier courtyard reconstruction; the portrait cutout and sky seams are refined.
- Wager navigation starts at the first denomination, first line choice and first offered bet.
- Jackpot labels and award lettering use source-derived shapes with clearer outlines. The Grand ending uses the centered WINNER display instead of a separate invented title screen.
- Recorded 1¢ / 50-line / 5× Mini and Minor prices are $10 and $50. The affected return and counter-entry calculations are repriced; probability weights remain unchanged.
- Gallery screens, animations, the three-spin showcase, browser demo and numeric references are regenerated from this source.

### Validation

All 77 wager choices pass actual Main/UI startup and funding checks. Manual/autoplay collection tests preserve the financial snapshot, outcome entropy and ending effects. Native and browser startup pixels match. Source RNG/outcome checks and the current target build pass. Full firmware/assets readback and 17 healthy fresh-start checks passed on the original Stick. Original casino odds and complete original-asset fidelity remain unverified.

## 0.2.151

- Custom Amount drops unused leading zeros as you move right: **$1.00** instead of **$00,001.00**.
- Moving left reveals larger digit positions again.
- Updated browser build, voucher examples and signed-flasher release.

## 0.2.150

- Two-stage denomination and wager selection with publisher Confucius/temple artwork and original title.
- Each denomination restores its last settled reel display; changing lines/bet within it keeps that display until another spin.
- Balance is the first Game Menu entry; Custom Amount starts at $00,000.00 and confirms with INSERT CREDITS.
- High Limit offers $1/$2/$5/$10/$25/$50/$100 and the directly filmed $5/50-line/3x/$750 selection.
- Higher-denomination money/credit formatting and source-derived offer documentation.
- Source, browser, wiki tables and signed-flasher bundle synchronized for this release.


## 0.2.143

- Wager-screen Return and hold-grey apply the selected denomination and lines without spinning.
- Unaffordable red BET selects its counter before the normal funding check; equal-credit same-denomination bets retain their shared bank.
- Custom Amount comes first in Credits; holding blue moves to the digit on the left.
- Editable Stardust voucher uses matching varied decorative identifiers, written amounts and the cash-in sound.
- Small startup progress overlay retains the game behind it.
- Flat jackpot fields, larger topper/amounts and stationary letter-edge fire.
- Reconstructed red temple courtyard behind the original clean publisher figure.
- Jackpot amounts, outcome weights and CSPRNG provider are unchanged.


## 0.2.141

- Opening a menu pauses the feature and its music. Returning to play continues the tune from its paused position instead of replaying the opening bells.
- Sound-menu auditions use a separate playback context, preserving the paused game's music position.
- The same menu behavior is included in the Stick firmware and browser game.

## 0.2.140

- Seven-row Game Menu with Settings grouping Sound and Display. Button shortcuts for wagers, autoplay and the live Jackpots board remain available; rumble is not used.
- Paylines opens on the active layout and browses all six line selections without changing the wager.
- Grand uses one continuous WINNER count for Grand, the Major base component and cash, adapted for the single LCD.
- Current model T includes scoped price corrections and source-supported route updates. The guide distinguishes 42/45 documented 50-line matches and 13/17 documented 100-line matches from local fallbacks.
- Every boot or browser reload begins with 1,000 default-denomination credits, fresh opening reels and autoplay off.

<a id="validation"></a>
<a id="validation-status"></a>

Current source, browser, firmware and media identities are recorded in [release metadata](RELEASE.json). [Testing](docs/testing.md) distinguishes source checks, byte readback, startup, physical controls/audio and original-machine fidelity. Local return references are not recovered casino odds.

## 0.2.130

- New sessions open on freshly sampled reels. The opening display does not place a bet, pay a win, advance the counter, record a spin or start a bonus. Credits still start at 1,000 and autoplay starts off.
- Counter editing stays within 0–1886 throughout. The first digit alternates 0/1; subsequent digits follow the selected prefix. When a larger prefix makes the old remainder invalid, the editor fits it within the ceiling. Apply and Cancel retain their existing behavior; jackpot amount editing is unchanged.
- The browser game, player guide, gallery and signed firmware package describe the same final source.

### 0.2.130 validation

Final firmware and offline assets passed complete byte-exact readback on StickS3. Healthy startup verified 1,000 credits, zero actions, autoplay off and all 60 Phoenix animation frames. Browser checks covered three distinct unpaid startup boards, every counter boundary wrap, menus, exact cash values and offline/cache updates.

The startup regression compared 1,024 test entropy tapes and 8,192 following Base/Free/Hold actions with the prior source: financial states, entropy requests and the next random reservoir matched. Startup boards containing feature symbols remained idle and unawarded. All 7,548 legal counter value/focus transitions passed an independent oracle, and the actual menu path passed 2,511 checks with address/undefined-behavior sanitizers. These are software regressions, not measurements of casino frequencies.

Original casino probabilities and complete factory artwork/audio remain unrecovered. The [math reference](docs/math-and-evidence.md) explains the local model and its limits.
