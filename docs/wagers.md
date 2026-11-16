# Denominations, line counts and bets

The current wager menu offers these denomination, line and multiplier choices. Most controls follow a recorded Confucius Say cabinet; the $1/10-line paid row uses the separately qualified family adaptation described below. The menu is narrower than every configuration the internal evaluator can represent.

| Denomination | Line controls | Bet multipliers offered |
|---|---|---|
| 1¢ | 50 / 100 | 1, 2, 3, 5, 10 |
| 2¢ | 50 / 100 | 1, 2, 3, 5, 10 |
| 5¢ | 25 / 50 | 1, 2, 3, 5, 10 |
| 10¢ | 25 / 50 | 1, 2, 3, 5, 10 |
| $1 | 5 / 10 | 1, 2, 3, 5, 10 on 5 lines; 1, 2, 3, 4, 5 on 10 lines |
| $2 | 5 | 1, 2, 3, 4, 5 |

There are 55 selectable denomination/line/multiplier combinations. Prize-cap contexts are internal model settings, not extra wager buttons.

**Stake in credits = selected lines × bet multiplier.**

**Stake in money = stake in credits × denomination.**

For example, 1¢ × 50 lines × multiplier 3 costs 150 credits, or $1.50. Selecting 100 lines with the same multiplier changes the red amount to 300 credits and costs $3.00. At $1 × 5 lines × multiplier 2, the stake is 10 credits, or $10.

The menu evidence is cabinet-specific. It does not establish a universal denomination deck, a recovered pay program, or identical odds across all offered settings.

The main deck is based on the filmed Palms configuration [mBcjPK63KW4](https://www.youtube.com/watch?v=mBcjPK63KW4). Its $1/10-line control is visible, but a complete selected Confucius paid row has not been recovered. The selectable $1/10-line row uses observed Phoenix Link family controls from General Tso: multipliers 1, 2, 3, 4, 5, with amounts calculated for ten selected lines. This is an explicitly labeled family adaptation, not proof of a universal Confucius $1 wager deck.

The exact implemented availability is defined by [cabinet_profile.h](../firmware/src/cabinet_profile.h); the generated [offered-wager CSV](data/offered-wagers.csv) enumerates all 55 selectable choices. The red button commits its wager and begins a funded spin, rather than merely highlighting a bet for a second confirmation.

## Denomination and MIN BET

The lower-right badge shows the value of a credit. For this source-qualified cabinet profile, MIN BET is the minimum **credit** wager associated with the denomination: 50 for 1¢/2¢, 25 for 5¢/10¢ and 5 for $1/$2. It is not simply the currently selected line count. Choosing 100 lines at 1¢ does not turn its source-profile MIN BET into 100.

The caption is a tiny above/below adaptation for the 240 × 135 display; physical readability remains a limitation. Internal profiles outside the offered menu are not extra selectable controls. Code: [denomination badge rules](../firmware/src/denomination_minbet46.h).

The screen opens with the current denomination focused. Selecting a denomination keeps the current valid line selection focused; selecting lines keeps the current offered bet multiplier focused. A valid available choice is used if the current selection is unavailable. This order also applies to denominations with one line control. Selecting lines updates the red credit amounts without spending credits.

<img src="media/wagers-lines.png" alt="Denomination selected with focus on a real PLAY LINES control" width="720">

Viewing or cancelling the wager screen does not change the live wager or spend credits. A new session starts at its default wager; no previous game is imported.

The [payout reference](payouts.md) exports the wider internal evaluator support with a selectable flag. Those internal rows must not be read as extra live cabinet buttons. See [controls](controls.md) for focus/navigation and [session rules](saving.md) for live feature/funding boundaries.
