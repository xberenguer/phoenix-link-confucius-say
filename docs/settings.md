# Settings and play-money funding

Hold the large grey button to open Game Menu. Grey moves through entries; blue selects. Menu labels use “black” in some compact hints for the same large grey game-control button. The small power key is separate.

<img src="media/menu.png" alt="Game Menu with the version in its corner" width="720">

## Rules and wagers

Game Rules has six pages: how to play, features, Base symbol pays, Free symbol pays, win units and Phoenix awards. The in-game numeric pay pages use the currently active configuration and show unit credits before the bet multiplier. They should agree with the matching [compiled reference](payouts.md).

Select Bet opens the same cabinet screen as a short grey press in normal play. Drafting and cancelling are read-only; a red BET choice can commit and spin. Owed features prevent changing their wager.

## Display

| Option | Effect |
|---|---|
| Brightness | Cycles the implemented display brightness levels |
| Display | Changes cash/credits presentation, not the financial amount |
| Flip Screen | Rotates the game by 180 degrees |

The normal game-view grey double press also flips the screen. It does not spend a spin. The single grey opening waits through a 400 ms double-click window; the blue spin control remains immediate. Saved display preferences are restored independently from the game bank.

## Sound

Master sound, reel-bed sound and stops/landings have separate controls. Fine volume and three test entries make it possible to compare music without waiting for a bonus. See the complete [audio guide](audio.md) for event routing and limits.

<img src="media/sound.png" alt="Sound menu" width="720">

## Credits

Game Menu → ADD CREDITS opens the funding submenu. Its choices are:

| Choice | Behavior |
|---|---|
| Add $1,000 | Add to the current bankroll and return to the game |
| Add $10,000 | Add to the current bankroll and return to the game |
| Add $100,000 | Add to the current bankroll and return to the game |
| Auto Refill | Toggle restoring an unaffordable bankroll to the last supported added amount |
| Return to Menu | Leave the funding page |

Auto-refill remembers the last supported $1,000/$10,000/$100,000 selection; its initial default remains $10,000. It acts when another paid spin is unaffordable, including a positive balance smaller than the wager. It restores the remembered amount or the current stake, whichever is larger. The $1 million option is absent.

Funding is available at safe Ready/Base boundaries. It preserves the meter buckets, wager, game RNG and statistics. Each funding choice adds to the existing balance. Successful additions clear stale status text, return to gameplay and leave autoplay off. Earned awards and queued features remain part of the saved game.

## Battery display

There is no battery percentage in this build. The StickS3 API returns fine-grained integers calculated from battery voltage; it is not a measured remaining-charge counter. A one-percent numerical step does not establish the requested five-percentage-point state-of-charge accuracy. The [pinned M5Unified implementation](https://github.com/m5stack/M5Unified/blob/96576a14247ddc935149145aa6b51943464120f1/src/utility/Power_Class.inl) and [StickS3 battery example](https://docs.m5stack.com/en/arduino/m5sticks3/battery) describe the available reading without providing that error bound.

## Simulator Return

SIMULATOR RETURN shows six local-model choices for the current wager profile's return range. Option 3 is the default. Select a choice to review its return range, complementary model house-edge range and future Grand, then press blue to apply or grey to go back. Values depend on the current denomination, lines, multiplier and prize-cap profile; changing that context requires another review.

These choices change the future Grand amount. Reels, hit probabilities, other prizes and ordinary line prices remain unchanged. The ranges describe the simulator's fixed-bet, fixed-option, always-funded model without Lucky Chance. They are not recovered casino RTP, promises about a short session or bounds for every wager-switching strategy. The [math guide](math-and-evidence.md) explains the assumptions.

Changing the choice requires a closed Ready/Base boundary. An unfinished Free/Hold feature, queued work or storage/service block locks the setting and preserves existing prizes. Browsing, help and cancellation do not change the financial game or normalize an old saved intent.

Main confirms the exact canonical saved record before returning to manual gameplay. Identical owned choices are verified without a gratuitous generation. Failed or ambiguous saves retain one exact pending record for retry; release the controls before playing after recovery. See [saving](saving.md) for compatible owed rounds and persistence limits.

Implementation: [menus, funding and credit revisions](../firmware/src/ui.cpp), [persisted device options](../firmware/src/main.cpp).
