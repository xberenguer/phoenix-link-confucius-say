# Your first game

This guide describes the current simulator. Open the [gameplay gallery](graphics.md) for a visual tour or [play in your browser](https://xberenguer.github.io/phoenix-link-confucius-say/) to try it yourself.

Startup opens denomination selection at zero balance. Once installed, the Stick needs no Wi-Fi, phone or computer. Choose a wager, then add a custom play-money voucher to begin. Progress lasts for the live session. Adding credits or clearing the balance leaves that session’s collection meter intact.

## Start a paid spin

1. Choose a denomination on the startup screen. Grey moves; blue selects. Holding blue switches standard/High Limit denominations. Hold grey for Game Menu, including sound and screen settings.
2. Choose PLAY LINES, then a red BET button. The reels open at **$0** with that wager selected.
3. Press blue to open the Stardust voucher. Grey moves through the amount; blue changes a digit, and hold-blue moves left. Select **INSERT CREDITS** to fund the game.
4. Press blue to spin. Short grey reopens wagers, hold-grey opens Game Menu, and double-grey opens Jackpots.

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

Hold the large grey button for Game Menu. Sound, fine volume, display, credit funding and game-rule pages are there. Double-press grey in the normal game view to open the live Jackpots board. Screen rotation is in Game Menu → Display → Flip Screen. Full details are in [controls](controls.md) and [settings](settings.md).

Implementation: [Main button dispatch](../firmware/src/main.cpp), [UI](../firmware/src/ui.cpp), [Engine](../firmware/src/phoenix_engine.h).

The opening reels vary between new sessions. This is an unpaid display: your starting credits and counters are unchanged by those symbols. Press blue to play the first spin.
