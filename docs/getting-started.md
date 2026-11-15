# Your first game

This guide describes the current simulator. Images are deterministic demonstrations of the current game.

The game starts directly at the reels. Once installed, play requires no Wi-Fi, phone, browser or computer. Each boot or browser reload starts manual play with 1,000 credits at the default $1 denomination ($1,000). Progress lasts for the live session. Credit controls add funds without clearing that session’s collection meter.

## Start a paid spin

1. Short-press the large grey side button. Its single-click action waits briefly for the double-click window, then opens the cabinet-style wager screen.
2. Grey moves the focus and blue selects. Choose a denomination, a line control and a red bet button. **Selecting a funded red bet button starts a spin immediately.** Merely browsing a denomination or line does not stake credits.
3. During ordinary play, short-press blue for the next spin. A press during motion can accelerate the presentation; it cannot select a different result.

The stake is `denomination × selected lines × bet multiplier`. The red buttons show the resulting credit amount. See [wagers](wagers.md) for the actual offered rows, including $1/5-line and $1/10-line choices.

## Read the screen

| Area | Meaning |
|---|---|
| Five columns, three rows | The reel window; line wins start at the leftmost column |
| Upper-right green number | The active Phoenix collection meter for this session |
| Upper announcements | Free-game progress, remaining Hold spins or collection information |
| CREDIT | Current bankroll; a presentation may still be transferring a committed win |
| BET | The current full stake, not the amount per individual line |
| WIN | The amount being shown for the current result or feature |
| Lower-right denomination | Value of one credit; qualified cabinet profiles also show MIN BET |

A successful action commits before its count-up is drawn. A rising WIN display is an animation of that result, not a second payment. A random-source failure commits no new action; see [session and action safety](saving.md).

## Recognize the three main feature routes

- **Three or more red Wisdom scatters:** six Twin free games. They use paired outer reels and a distinct paytable.
- **Six to fourteen Phoenixes:** Hold & Spin. The Phoenixes lock, empty cells respin and new arrivals reset the remaining spins to three.
- **The 1888 collection meter:** a separate free Hold & Spin may trigger before the limit. It is not the Wisdom free-games feature.

Fifteen Phoenixes award the Grand plus their displayed prizes in the simulator. The [feature guide](hold-and-spin.md) explains queueing, revival and nested free games.

## Autoplay and settings

Hold blue for about 0.85 seconds to toggle autoplay. Hold it again to stop. A short blue press while autoplay is running also turns autoplay off. There is no on-screen autoplay badge. Every new session starts with autoplay off.

Hold the large grey button for Game Menu. Sound, fine volume, display, credit funding and game-rule pages are there. Double-press grey in the normal game view to rotate the screen. Full details are in [controls](controls.md) and [settings](settings.md).

Implementation: [Main button dispatch](../firmware/src/main.cpp), [UI](../firmware/src/ui.cpp), [Engine](../firmware/src/phoenix_engine.h).
