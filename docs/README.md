# Game guide

The screen contains five reels with three visible positions per reel. Regular symbols can pay along selected lines. Wisdom symbols start the Free Games feature, and Phoenix symbols carry values into Hold & Spin. The persistent collection meter can award a separate feature.

The top bar shows the game title, selected line/bet information and active collection meter. The bottom bar shows credit, bet, win and denomination. Larger cabinet announcements are adapted to the middle of the small display where needed.

## Choose what to read

| Topic | Reference |
|---|---|
| Buttons, autoplay, settings and screen rotation | [Controls](controls.md) |
| Hardware-free play, browser controls and separate saved progress | [Browser play](browser-play.md) |
| First spin, reading the display and recognizing feature routes | [Getting started](getting-started.md) |
| Actual offered denomination, line and multiplier combinations | [Wagers](wagers.md) |
| All twelve symbols, Wild substitution and combination categories | [Symbols and combinations](symbols-and-combinations.md) |
| All implemented line-pay entries and their evidence scopes | [Payouts](payouts.md) |
| Wisdom trigger, Twin outer pairs, retriggers and nested bonuses | [Free games](free-games.md) |
| Locked Phoenix values, respin reset, revival and 15-cell Grand | [Hold & Spin](hold-and-spin.md) |
| Which meter is active, what increments it and what persists | [Collection counter](collection-counter.md) |
| Every symbol, animated state and screen transition | [Graphics](graphics.md) |
| Landing cues, feature music, collection and volume controls | [Audio](audio.md) |
| Current model, measurements, return assumptions and unknowns | [Math and evidence](math-and-evidence.md) |
| Credits, meters, owed features, restarts and safe recovery | [Saving](saving.md) |
| Building, installing and updating offline | [Build and flash](build-and-flash.md) |
| Funding, fine volume, display and Simulator Return choices | [Settings](settings.md) |
| Architecture, source modules and change boundaries | [Developer reference](developer-reference.md) |
| Validation methods and what each can prove | [Testing](testing.md) |
| Missing sound, unavailable wagers, data recovery or USB trouble | [Troubleshooting](troubleshooting.md) |
| Short definitions of meter, Free, Hold, RTP and other terms | [Glossary](glossary.md) |
| Public references and their configuration limits | [Sources](sources.md) |
| Third-party art, audio, code and licenses | [Attribution](attribution.md) |
| Release history | [Version history](../CHANGELOG.md) |

## Evidence labels

**Source-observed** means the behavior or number was read in identified footage or operator material. **Configuration-specific** restricts that observation to its visible denomination, line count, bet multiplier and program context. **Simulator rule** describes what this source actually does. **Unresolved** means the original value or behavior has not been established.

The documentation keeps those distinctions next to the relevant rules and numbers. An animation match does not prove a probability match.

## Two meanings of “free feature”

Wisdom gives a **Twin free-games session**. The persistent Phoenix counter gives a **free Hold & Spin**. A Twin session can contain a natural Hold, and another meter Hold can be queued during it. These obligations all belong to their existing round; the captions and feature music do not charge another normal wager.

## Version and completeness

This guide describes version **0.2.102**. Its game source, implemented payout tables and synthetic gallery are synchronized. The [browser guide](browser-play.md) separates native financial-code parity, browser storage and platform-specific limitations. Historical changes remain in the [version history](../CHANGELOG.md).

The exhaustive table export documents **implemented** values, including local fallbacks and compatibility profiles. It does not make the original casino odds known. Unrecovered original strips, probabilities, font/asset banks and universal event timing remain explicit [math](math-and-evidence.md) and [graphics](graphics.md) limits.
