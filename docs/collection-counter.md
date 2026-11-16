# The 1888 collection counter

The upper-right green number tracks collected Phoenixes for the active wager context. It can award a free Hold & Spin before 1888; it is separate from the red Wisdom free-games trigger.

The [manufacturer](https://www.aristocratgaming.com/us/games/us-phoenix-link) identifies this mystery feature. The [operator rules, page 2](https://casinos.lotoquebec.com/dam/jcr:4add9be1-336a-4efc-b4c9-e41696b876b2/Phoenix%20Link%20Confucius%20Say.pdf) describe the wager-specific counters and a reset in 100–1500. Exact original trigger and reset probabilities are not published there.

## What changes the number

In the current [Engine](../firmware/src/phoenix_engine.h):

- Each newly appearing Phoenix adds one in Base, Free and Hold.
- A locked Phoenix is not re-collected on every later respin.
- A named 2×/3× label does not add two or three counts; that multiplier belongs to the prize.
- Meter arrivals are processed one at a time. Multiple hits can create multiple queued features.
- Reaching the hidden local threshold awards a meter feature and immediately resets that bucket for its next cycle.

The UI can show meter increments at visible landing events even though the action's financial result has already been committed. Its animated frame and number position are presentation; they do not affect the trigger draw.

<a id="which-wager-shares-a-counter"></a>

## Counter groups

The model keys the bucket by **denomination and total bet in credits**. Total bet credits are `lines × multiplier`. Two line/multiplier selections with the same denomination and same total-credit stake select the same modeled bucket. A denomination change selects a different bucket even when the cash stake happens to match.

For example, 1¢ / 50 lines / 2× and 1¢ / 100 lines / 1× both use a 100-credit stake and the same modeled counter. This is the simulator's grouping rule; the casino's internal bucket identities remain unknown.

The current small-screen menu offers a subset of the internal evaluator profiles. See [wagers](wagers.md) before treating every Engine-valid configuration as an available cabinet control.

## Continuity

Adding credits, clearing the balance and using automatic refill do not reset collection progress during the live session. Switching away from a wager and back selects its current bucket again. All initialized buckets and queued Holds belong to that live game.

Opening a menu does not reset the meter. Each boot or page reload starts fresh meter state with a new game; no prior record is loaded. See [fresh sessions](saving.md).

## Setting the counter

**Game Menu → Counter & Jackpots → Set Feature Counter** accepts **0 through 1886**. Apply changes only the current wager's bucket and returns to regular play. Other buckets, credits and statistics remain unchanged. Editing requires a completed Base round with no owed feature.

Changing the count also draws a new hidden threshold above that count using the local model below. Applying the same count leaves the existing threshold untouched. This is a simulator control; it does not recover a real cabinet's hidden threshold or expected return. See [the editor controls](settings.md#feature-counter-and-jackpot-amounts).

<a id="the-hidden-model-is-still-an-assumption"></a>

## Local meter model

After a hit, the simulator chooses a reset uniformly from 100 through 1500. It then chooses its hidden threshold uniformly from the next count through 1887. The **support** of the reset and must-hit limit has rulebook support; the **uniform distributions do not**. On later arrivals the stored threshold stays in force until the bucket hits.

That local mechanism can make late-meter play more valuable conditionally, but it does not identify the casino's hazard curve or a profitable entry point. Published AP thresholds, selected bonus videos and aggregate casino returns are not a replacement for original distributions. No particular counter value on this simulator proves positive expected value on a real machine.

## Entry and queueing

A meter award with fewer than six visible Phoenixes retains those symbols for its free Hold. A simultaneous natural Hold runs first and preserves the extra meter feature in a queue. A meter hit during an existing Hold also queues. Each queued Hold starts with an empty board and its own collected count; the preceding Hold has already paid.

The entry bird/ribbon and fire are fitted to filmed movement, including [3pfZnZUWGhI around 22:10](https://www.youtube.com/watch?v=3pfZnZUWGhI&t=1330s). On the Stick, only the number frame receives the calm gold flame treatment; the original large cabinet panel is not added around the whole top bar.

Code: [meter storage and action flow](../firmware/src/phoenix_engine.h), [counter artwork](../firmware/src/counter_hud.cpp), [gold animation adaptation](../firmware/src/counter_calm90.h), [entry trajectory](../firmware/src/feature_entry_motion.h).
