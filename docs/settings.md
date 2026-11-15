# Settings and play-money funding

Hold the large grey button during gameplay for Game Menu. Grey moves and blue selects. Compact hints may call that grey control “black”; the small power key is separate. Inside the wager panel, hold-grey returns directly to Game.

<img src="media/menu.png" alt="Game Menu" width="720">

## Rules and wagers

Game Rules has six pages covering play, features, Base pays, Free pays, units and Phoenix awards. Pay pages use the current configuration and show unit credits before the bet multiplier. See [payouts](payouts.md).

Select Bet opens the cabinet panel. Browsing/cancelling does not stake money. Selecting a funded red BET choice commits its wager and starts immediately. Unfinished features prevent changing their wager.

## Display

Brightness cycles the implemented levels. Display changes cash/credit presentation, not the money balance. Flip Screen rotates 180 degrees; double-grey also flips normal gameplay without spending a spin. A new session starts with its default display state.

## Sound

Master sound, reel bed and stops/landings have separate controls. Fine volume and the three test entries let you audition the existing programs. See [audio](audio.md) for current routing and acoustic limits.

<img src="media/sound.png" alt="Sound menu" width="720">

## Credits

Every boot/reload starts with **1,000 denomination credits**, or $1,000 at the default $1 denomination. Game Menu → ADD CREDITS offers:

| Choice | Effect |
|---|---|
| Add $1,000 | Add to the current money balance, return to Game with autoplay off |
| Add $10,000 | Add to the current money balance, return to Game with autoplay off |
| Add $100,000 | Add to the current money balance, return to Game with autoplay off |
| Auto Refill | Toggle refill when another paid spin is unaffordable |
| Return to Menu | Leave this submenu |

Auto Refill uses the last supported addition size for this session; its initial default is $10,000. It sets an unaffordable balance to that amount or the current full stake, whichever is larger. The manual Add choices are additive. Funding is available only at safe Ready/Base boundaries and does not reset the live session's wager, meters or statistics. There is no $1 million choice.

## Battery display

There is no battery percentage in this build. The available voltage-derived reading does not establish a five-percentage-point charge-accuracy guarantee. See the [M5Unified implementation](https://github.com/m5stack/M5Unified/blob/96576a14247ddc935149145aa6b51943464120f1/src/utility/Power_Class.inl) and [StickS3 battery example](https://docs.m5stack.com/en/arduino/m5sticks3/battery).

## Simulator Return

SIMULATOR RETURN shows six options for the current profile; **Option 3 is the default**. Review the ideal-model return range, complementary edge range and future Grand, then press blue to apply or grey to go back. Context includes denomination, lines, multiplier and prize-cap profile.

Applying a choice changes future Grand only. Reel/hit probabilities, ordinary prices and other prizes do not change. The ranges are ideal-reference-only under fixed-profile, fixed-option, always-funded, no-Lucky play. They do not establish original casino RTP or cover arbitrary wager switching, faults or short-session results. See [math](math-and-evidence.md#return-and-house-edge).

A closed Ready/Base boundary is required. Free/Hold/queued work and service/capacity faults lock the choice. Browsing/help/cancellation do not alter the live round. Applied choices belong to this session; boot or reload starts fresh.

Implementation: [UI](../firmware/src/ui.cpp), [selector](../firmware/src/return_selector86.h), [native startup](../firmware/src/main.cpp).
