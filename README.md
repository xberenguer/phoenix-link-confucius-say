# Confucius Say

**Latest: 0.2.171** — the collection counter now reaches **1887**. [What changed](CHANGELOG.md#02171).

A play-money recreation of **Phoenix Link — Confucius Say**, with five reels, animated symbols, Twin Free Games and Phoenix Hold & Spin.

## [▶ Play in your browser](https://xberenguer.github.io/phoenix-link-confucius-say/)

**No installation, account or hardware needed.** Open the game on your phone, tablet or computer, then press **Play with sound**.

<a href="https://xberenguer.github.io/phoenix-link-confucius-say/">
  <img src="docs/media/gameplay/sessions/primary/action-000002-clip-001.webp" alt="A real gameplay session in Confucius Say; click to play in your browser" width="720" height="405">
</a>

## Take a look around

| What would you like to see? | Open |
|---|---|
| The game in playing order: screenshots, animated symbols, merges, bonus features, menus and jackpots | [Gallery](docs/graphics.md) |
| How to start playing, understand the screen and explore every feature | [Full player guide](docs/README.md) |

Gameplay pictures and clips in the gallery come from actual play. Individual symbol previews are crops of those captures. The gallery records what has been captured so far; a rare event without footage is described rather than illustrated with a staged result.

## What to watch for

- **Gold Confucius:** adjacent qualifying faces can join into a tall two- or three-row figure.
- **Red Wisdom symbols:** three or more start six Twin Free Games.
- **Phoenix symbols:** six or more can start Hold & Spin, where birds lock in place while the other positions respin.
- **The green collection meter:** follows its own route to a free Hold & Spin feature.

[Symbols and wins](docs/symbols-and-combinations.md) · [Twin Free Games](docs/free-games.md) · [Hold & Spin](docs/hold-and-spin.md) · [Collection meter](docs/collection-counter.md)

## Play on a StickS3

The same game also runs offline on the **M5Stack StickS3** handheld. [Installation and updates](docs/build-and-flash.md) · [StickS3 controls](docs/controls.md)

Get a StickS3: [M5Stack shop](https://shop.m5stack.com/products/m5sticks3-esp32s3-mini-iot-dev-kit) · [Amazon](https://www.amazon.com/dp/B0GWHN8HK3)

<details>
<summary><strong>Source code, builds and project details</strong></summary>

- [Developer reference](docs/developer-reference.md) and [browser build](browser/README.md)
- [Testing and validation](docs/testing.md)
- [How randomness works](docs/randomness.md)
- [Math, evidence and known differences](docs/math-and-evidence.md)
- [Release notes](CHANGELOG.md)
- [Sources](docs/sources.md) and [artwork, audio and code attribution](docs/attribution.md)

The browser and handheld use the same C++ game, renderer and audio programs. This is an independent recreation with a local estimated model; it does not reproduce the original casino odds or offer real-money play. Software checks and physical-device validation have separate scopes in the testing guide.

</details>
