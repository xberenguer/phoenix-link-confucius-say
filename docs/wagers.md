# Denominations, line counts and bets

[← Full player guide](README.md) · [Gallery](graphics.md)

**New to PLAY LINES?** Each line is a fixed route that reads one symbol from each of the five columns. Selecting more lines makes the game check more routes through the same result from one spin. [See the actual charts and follow a winning line step by step →](paylines.md)

The current wager menu offers these denomination, line and multiplier choices. Most controls follow a recorded Confucius Say cabinet; the $1/10-line paid row uses the separately qualified family adaptation described below. The menu is narrower than every configuration the internal evaluator can represent.

| Denomination | Line controls | Bet multipliers offered |
|---|---|---|
| 1¢ | 50 / 100 | 1, 2, 3, 5, 10 |
| 2¢ | 50 / 100 | 1, 2, 3, 5, 10 |
| 5¢ | 25 / 50 | 1, 2, 3, 5, 10 |
| 10¢ | 25 / 50 | 1, 2, 3, 5, 10 |
| $1 | 5 / 10 | 1, 2, 3, 5, 10 on 5 lines; 1, 2, 3, 4, 5 on 10 lines |
| $2 | 5 | 1, 2, 3, 4, 5 |

The standard menu has 55 selectable denomination/line/multiplier combinations. Prize-cap contexts are internal model settings, not extra wager buttons.

**Stake in credits = selected lines × bet multiplier.**

**Stake in money = stake in credits × denomination.**

For example, 1¢ × 50 lines × multiplier 3 costs 150 credits, or $1.50. Selecting 100 lines with the same multiplier changes the red amount to 300 credits and costs $3.00. At $1 × 5 lines × multiplier 2, the stake is 10 credits, or $10.

Changing the line count changes the stake when the denomination and multiplier stay fixed. The game still spins once. More lines do not guarantee a win or a profit.

The menu evidence is cabinet-specific. It does not establish a universal denomination deck, a recovered pay program, or identical odds across all offered settings.

The main deck is based on the filmed Palms configuration [mBcjPK63KW4](https://www.youtube.com/watch?v=mBcjPK63KW4). Its $1/10-line control is visible, but a complete selected Confucius paid row has not been recovered. The selectable $1/10-line row uses observed Phoenix Link family controls from General Tso: multipliers 1, 2, 3, 4, 5, with amounts calculated for ten selected lines. This is an explicitly labeled family adaptation, not proof of a universal Confucius $1 wager deck.

The exact implemented availability is defined by [cabinet_profile.h](../firmware/src/cabinet_profile.h); the generated [offered-wager CSV](data/offered-wagers.csv) enumerates all 77 choices across standard and high-limit modes. The red button commits its wager and begins a spin when funded; the ordinary funding check prevents an unaffordable spin.

## Denomination and MIN BET

<img src="media/gameplay/screens/menus/startup-standard.png" alt="The actual standard denomination chooser" width="720" height="405" loading="lazy">

<img src="media/gameplay/screens/menus/startup-bet.png" alt="The actual line and bet choices after selecting a standard denomination" width="720" height="405" loading="lazy">

The lower-right badge shows the value of a credit. For this source-qualified cabinet profile, MIN BET is the minimum **credit** wager associated with the denomination: 50 for 1¢/2¢, 25 for 5¢/10¢ and 5 for $1/$2. It is not simply the currently selected line count. Choosing 100 lines at 1¢ does not turn its source-profile MIN BET into 100.

The caption is a tiny above/below adaptation for the 240 × 135 display; physical readability remains a limitation. Internal profiles outside the offered menu are not extra selectable controls. Code: [denomination badge rules](../firmware/src/denomination_minbet46.h).

Focus starts at the first denomination, first PLAY LINES choice and first offered BET button, in that order. Existing choices stay marked until changed. Selecting lines updates the red credit amounts without spending credits. At startup, selecting BET opens the reels with no balance; blue then opens the voucher. During funded play, a red BET starts a spin.

Holding grey or choosing RETURN applies the selected denomination and lines without spending credits or starting a spin. The current multiplier is retained when offered; otherwise the first supported multiplier is selected. The green counter changes to that wager's existing bank. An unaffordable red BET still selects its wager and counter, then follows the normal insufficient-funds path. A new session starts at its default wager; no previous game is imported.

The [payout reference](payouts.md) exports the wider internal evaluator support with a selectable flag. Those internal rows must not be read as extra live cabinet buttons. See [controls](controls.md) for focus/navigation and [session rules](saving.md) for live feature/funding boundaries.

## High Limit

<img src="media/gameplay/screens/menus/startup-high-limit.png" alt="The actual High Limit denomination menu reached through the startup controls" width="720" height="405" loading="lazy">

Settings → High Limit changes the denomination chooser to $1, $2, $5, $10, $25, $50 and $100. The cash balance is unchanged when switching modes. The chooser starts at the first denomination. Holding blue on the startup chooser switches between standard and High Limit denominations. On the first StickS3 chooser screen, holding grey toggles the volume. After you select a denomination, holding grey returns to the chooser and another hold opens Game Menu. Turning High Limit off from a denomination unavailable in standard mode returns to $1.

The shared practice row uses 25 lines and bet multipliers 1, 2 and 3, giving 25, 50 and 75 credits. At $5, PLAY50 also offers the directly observed 3× wager: 150 credits, or $750. At $100, the middle button places a $5,000 wager. These are simulator choices, not a complete recovered factory high-limit deck.

The filmed high-limit General Tso cabinet directly shows all seven denominations, 25-line play, $100 wagers of $2,500/$5,000, and a $50 wager of $3,750. The shared three-button row across all seven denominations is an implementation adaptation of those observed wagers. The Confucius Say recording 5BUJOWxIBN4 directly verifies $5 / 50 lines / 3× / $750, so that selection is also available. Its other 50-line bet choices remain unrecovered. Other omitted buttons, a universal $5,000 machine maximum and the authentic high-limit pay program remain unverified.

The extra denominations retain the existing local model in credit units. Their reference return enclosures are transported from the $10 local credit law; this does not identify casino probabilities, jackpot seeds or linked progressive contributions. Default jackpot values remain local simulator values. Use Counter & Jackpots to match displayed cash amounts for practice; custom amounts do not retain the default return qualification.

References: [General Tso high-limit play](https://www.youtube.com/watch?v=FbBUqjjzN08), [Confucius Say $750 play](https://www.youtube.com/watch?v=5BUJOWxIBN4).

## Reel display when switching

Each denomination remembers its last settled reel display during this session. Changing lines or bet multiplier within that denomination keeps the displayed board until another spin lands. Returning to a denomination restores its icons, displayed Phoenix amounts and win paths; selection alone does not pay the displayed result or restart its feature.

The green counter uses a separate grouping: denomination plus total credit stake. A fresh, unplayed denomination has an unpaid cosmetic arrangement of the opening symbols; it is not an evaluated spin. Both display history and counter banks are session-only.
