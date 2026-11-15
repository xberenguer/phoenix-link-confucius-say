# Controls and menus

## During play

| Input | Action |
|---|---|
| Short blue press | Spin, continue a prompted feature, or advance the current animation where supported; while autoplay is active, turn it off |
| Hold blue | Toggle autoplay on or off |
| Short large grey press | Open the wager-selection screen, or queue it until a running sequence permits entry |
| Hold large grey | Open Game Menu |
| Double-press large grey | Flip the screen orientation |

There is no on-screen autoplay marker. A new flash or restored game starts with autoplay off. The small power button is separate from the large grey game-control button.

Stopping autoplay stops further paid automatic play; it does not discard the current result or an already owed Free/Hold session.

The implemented long-press threshold is 850 ms. In normal game view, grey's single-release action waits through a 400 ms double-click window; that intentional delay prevents the wager screen from opening before a screen flip. On menu pages, grey navigation is immediate. Double-grey is a game-view gesture, not a second blue spin command.

## Wager-selection screen

Short grey presses move focus; blue selects the highlighted choice. Choose a denomination and line count, then a red bet button. The red-button credit amounts depend on the selected lines and multiplier. Holding the large grey button on this screen returns directly to the game.

An unavailable row cannot start a paid spin. Owed features protect their existing wager; opening the screen does not change the paid round that is already in progress.

## Game Menu

| Entry | Purpose |
|---|---|
| Game Rules | Inspect the implemented rule and pay pages |
| Select Bet | Open the same wager-selection screen |
| Autoplay | Toggle automatic play |
| Sound | Configure sound effects, volume and test playback |
| Display | Configure display options and orientation |
| Add Credits | Add $1,000, $10,000 or $100,000; configure automatic refill |
| Simulator Return | Review and apply six local-model choices affecting future Grand |
| Return to Game | Resume the game view |

On menu pages, grey moves and blue selects. Simulator Return offers six local-model options at a qualified closed Base boundary. Applying a choice changes future Grand only and waits for the canonical save acknowledgement; it does not select an authentic casino RTP. See [Simulator Return](settings.md#simulator-return).

The bottom-right version mark appears in Game Menu. Some compact hints call the large grey game button BLACK; they do not mean the separate power key. See [settings](settings.md) for the full option list and [troubleshooting](troubleshooting.md) for apparent input delays.

## Credit refill

The funding page lists **Add $1,000**, **Add $10,000**, **Add $100,000**, Auto Refill and Return to Menu, in that order. A successful addition returns directly to the game with autoplay off. Every choice adds to the current balance.

Automatic refill uses the last supported $1,000, $10,000 or $100,000 amount added when another spin is unaffordable. If the selected wager exceeds that amount, it restores enough to fund the wager. Funding preserves collection-meter buckets, RNG state, wager and statistics. Owed features and blocked storage protect the bank from an unsafe addition. See [saving](saving.md) for its continuity and acknowledgement rules.

Implementation: [button timing/dispatch](../firmware/src/main.cpp), [single/double-grey helper](../firmware/src/game_black_tap.h), [UI control behavior](../firmware/src/ui.cpp).
