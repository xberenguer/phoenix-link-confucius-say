# Game guide

The screen contains five reels with three visible positions per reel. Regular symbols can pay along selected lines. Wisdom symbols start the Free Games feature, and Phoenix symbols carry values into Hold & Spin. The collection meter can award a separate feature during the live session.

The top bar shows the game title, selected line/bet information and active collection meter. The bottom bar shows credit, bet, win and denomination. Larger cabinet announcements are adapted to the middle of the small display where needed.

## Choose what to read

| Topic | Reference |
|---|---|
| Buttons, autoplay, settings and screen rotation | [Controls](controls.md) |
| Hardware-free play, browser controls and fresh sessions | [Browser play](browser-play.md) |
| First spin, reading the display and recognizing feature routes | [Getting started](getting-started.md) |
| Actual offered denomination, line and multiplier combinations | [Wagers](wagers.md) |
| All twelve symbols, Wild substitution and combination categories | [Symbols and combinations](symbols-and-combinations.md) |
| All implemented line-pay entries and their evidence scopes | [Payouts](payouts.md) |
| Browse every line layout and distinguish observed geometry from fallbacks | [Paylines](paylines.md) |
| Wisdom trigger, Twin outer pairs, retriggers and nested bonuses | [Free games](free-games.md) |
| Locked Phoenix values, respin reset, revival and 15-cell Grand | [Hold & Spin](hold-and-spin.md) |
| Which meter is active, what increments it and its live-session buckets | [Collection counter](collection-counter.md) |
| Every symbol, animated state and screen transition | [Graphics](graphics.md) |
| Landing cues, feature music, collection and volume controls | [Audio](audio.md) |
| Current model, measurements, return assumptions and unknowns | [Math and evidence](math-and-evidence.md) |
| Fresh starting credits, live-round safety and restart behavior | [Fresh sessions](saving.md) |
| Building, installing and updating offline | [Build and flash](build-and-flash.md) |
| Funding, fine volume, display and Simulator Return choices | [Settings](settings.md) |
| Architecture, source modules and change boundaries | [Developer reference](developer-reference.md) |
| Validation methods and what each can prove | [Testing](testing.md) |
| Missing sound, unavailable wagers, random-source errors or USB trouble | [Troubleshooting](troubleshooting.md) |
| Short definitions of meter, Free, Hold, RTP and other terms | [Glossary](glossary.md) |
| Public references and their configuration limits | [Sources](sources.md) |
| Third-party art, audio, code and licenses | [Attribution](attribution.md) |
| Current release status | [Release notes](../CHANGELOG.md) |

## Evidence labels

**Source-observed** means the behavior or number was read in identified footage or operator material. **Configuration-specific** restricts that observation to its visible denomination, line count, bet multiplier and program context. **Simulator rule** describes what this source actually does. **Unresolved** means the original value or behavior has not been established.

The documentation keeps those distinctions next to the relevant rules and numbers. An animation match does not prove a probability match.

## Two meanings of “free feature”

Wisdom gives a **Twin free-games session**. The Phoenix counter gives a **free Hold & Spin**. A Twin session can contain a natural Hold, and another meter Hold can be queued during it. These obligations all belong to their existing round; the captions and feature music do not charge another normal wager.

## Version and completeness

This guide describes the current published game. Its tables and gallery are generated from the included source. The [browser guide](browser-play.md) describes the same session-only financial rules with browser-specific input, entropy and audio. See [current release notes](../CHANGELOG.md) for validation status.

The exhaustive table export documents **implemented** values, including local fallbacks and internal evaluator profiles. It does not make the original casino odds known. Unrecovered original strips, probabilities, font/asset banks and universal event timing remain explicit [math](math-and-evidence.md) and [graphics](graphics.md) limits.
