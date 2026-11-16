# Settings and play-money funding

Hold the large grey button during gameplay for Game Menu. Grey moves and blue selects. The small power key is separate. Inside the wager panel, hold-grey returns directly to Game.

<img src="media/menu.png" alt="Game Menu" width="720">

## Rules and wagers

Game Rules has six pages covering play, features, Base pays, Free pays, units and Phoenix awards. Pay pages use the current configuration and show unit credits before the bet multiplier. See [payouts](payouts.md).

Select Bet opens the cabinet panel. Browsing/cancelling does not stake money. Selecting a funded red BET choice commits its wager and starts immediately. Unfinished features prevent changing their wager.

## Display

Brightness cycles the implemented levels. Amount Units switches between cash and denomination credits without changing the money balance. Flip Screen rotates 180 degrees. Double-grey in gameplay opens the live Jackpots board. A new session starts with its default display state.

## Sound

Master sound and stops/landings have separate controls. Volume adjusts the level; Test Free-Games Intro, Test Hold Music and Test Award Jingle play their named programs. See [audio](audio.md) for current routing and acoustic limits.

<img src="media/sound.png" alt="Sound menu" width="720">

## Credits

Every boot/reload starts with **1,000 denomination credits**, or $1,000 at the default $1 denomination. Game Menu → CREDITS offers:

| Choice | Effect |
|---|---|
| Add $1,000 | Add to the current money balance, return to Game with autoplay off |
| Add $10,000 | Add to the current money balance, return to Game with autoplay off |
| Add $100,000 | Add to the current money balance, return to Game with autoplay off |
| Clear Balance | Set the money balance to zero and return to Game with autoplay off |
| Auto Refill | Toggle refill when another paid spin is unaffordable |
| Back to Menu | Return to Game Menu |

Auto Refill uses the last supported addition size for this session; its initial default is $10,000. It sets an unaffordable balance to that amount or the current full stake, whichever is larger. The manual Add choices are additive. Clear Balance is available at a closed Ready/Base boundary; unfinished Free/Hold/queued work and service or selector faults block it. It preserves the live wager, meter buckets and statistics, and does not change the Auto Refill preference. A later unaffordable paid spin can therefore refill the balance if that option is on. There is no $1 million choice.

<img src="media/credits.png" alt="Credits menu with balance and automatic refill controls" width="720">

## Feature counter and jackpot amounts

Open **Game Menu → Counter & Jackpots**. **Set Feature Counter** edits the active wager's counter from **0 to 1886**. **Jackpot Amounts** lets you select Mini, Minor, Major or Grand and enter an amount from **$0.01 to $30,000,000.00**.

Grey moves between digits, Apply and Cancel; blue changes the selected digit or confirms the selected action. Hold grey to go back without applying. A successful Apply returns to the regular game with autoplay off. The balance and statistics are unchanged. Finish any current spin or owed feature before editing.

<img src="media/game-values.png" alt="Counter and jackpot settings" width="720">

<img src="media/counter-editor.png" alt="Feature counter digit editor" width="720">

Jackpot amounts are absolute cash values: changing denomination or wager does not rescale an edited amount. They apply to future prizes; already landed prizes retain their values. **Restore Default Jackpots** restores all four tiers for the current configuration, including the selected return option's Grand. Boot or reload starts fresh defaults.

<img src="media/jackpot-editor.png" alt="Jackpot cash amount editor" width="720">

Double-grey during gameplay opens the jackpot board. Blue or grey closes it without spinning. It shows the amounts available for future prizes at the current configuration.

## Battery display

Battery percentage is not shown. See the [M5Unified implementation](https://github.com/m5stack/M5Unified/blob/96576a14247ddc935149145aa6b51943464120f1/src/utility/Power_Class.inl) and [StickS3 battery example](https://docs.m5stack.com/en/arduino/m5sticks3/battery).

## Simulator Return

SIMULATOR RETURN shows six options for the current profile; **Option 3 is the default**. Review the ideal-model return range, complementary edge range and future Grand, then press blue to apply or grey to go back. Context includes denomination, lines, multiplier and prize-cap profile.

Applying a choice changes future Grand only. Reel/hit probabilities, ordinary prices and other prizes do not change. The ranges are ideal-reference-only under fixed-profile, fixed-option, always-funded, no-Lucky play. They do not establish original casino RTP or cover arbitrary wager switching, faults or short-session results. See [math](math-and-evidence.md#return-and-house-edge).

Custom jackpot amounts lock this page and hide its preset return ranges. Restore Default Jackpots to use the preset choices again. Editing amounts changes payouts without changing their arrival probabilities.

A closed Ready/Base boundary is required. Free/Hold/queued work and service/capacity faults lock the choice. Browsing/help/cancellation do not alter the live round. Applied choices belong to this session; boot or reload starts fresh.

Implementation: [UI](../firmware/src/ui.cpp), [selector](../firmware/src/return_selector86.h), [native startup](../firmware/src/main.cpp).
