# Phoenix Hold & Spin, revival and jackpots

The simulator can enter this feature from a natural Phoenix result or the persistent collection meter. Those routes can occur together, so more than one Hold may be owed. The official [US description](https://www.aristocratgaming.com/us/games/us-phoenix-link) identifies Hold & Spin and the separate mystery counter; the [operator rules, pages 1–3](https://casinos.lotoquebec.com/dam/jcr:4add9be1-336a-4efc-b4c9-e41696b876b2/Phoenix%20Link%20Confucius%20Say.pdf) supply the regional lock/reset/revival rules. Current financial behavior is in the [Engine](../firmware/src/phoenix_engine.h).

## Natural entry and respins

Six through fourteen Phoenixes on a Base or Free result enter Hold & Spin. Their positions and displayed values lock. Each empty position becomes an individual bonus cell; the feature begins with three respins.

Each action consumes one remaining respin. Any new Phoenix locks and resets the remaining count to three. Already locked Phoenixes keep their prizes and do not count as new arrivals on every spin. Empty cells do not earn ordinary line awards during Hold.

The large top-of-cabinet information is adapted to the small screen: a live collected count appears above the reels, and collection WINNER/currency announcements can occupy the center. The count includes existing locks and new symbols whose visual impact has occurred, rather than revealing every precommitted landing at the start of motion.

<img src="media/hold.png" alt="Scripted Hold scene with synthetic demonstration credits" width="720">

## Revival

If the initial three respins produce **zero new Phoenixes**, the simulator restores three respins. The initial locked symbols do not satisfy this condition. Once at least one new Phoenix has arrived, a later three-spin drought ends the feature normally.

There is no invented finite revival cutoff. The numeric storage/capacity protections are engineering limits, not an assertion that a casino feature uses the same bounds. Revival changes the remaining respin count; it does not remove existing locked values or charge another paid bet.

## Phoenix values and named prizes

| Displayed prize | Settlement treatment |
|---|---|
| Ordinary credit or cash value | Pay that shown amount when collected |
| Mini | Pay the locked Mini value |
| Minor | Pay the locked Minor value |
| Major | Use its separate named-award presentation and pay its locked value |
| Double / Triple named prize | Apply 2× / 3× to that named Mini, Minor or Major |
| Grand from a full board | Add Grand to all Phoenix prizes; it does not replace them |

Do not multiply a shown Phoenix cash value by the stake again. Internally generated ordinary prizes use total-bet multiples, but the displayed number already represents the prize. Ordinary credit prizes do not receive the named-prize 2×/3× multiplier in this model.

Mini/Minor values and restrictions on named/high-cash combinations are configuration-dependent. The Engine contains two prize-cap contexts and scoped filmed Mini/Minor corrections. These are internal compatibility/model contexts, not additional invented buttons on the cabinet menu. The original complete prize-frequency and progressive-contribution schedules remain unknown.

## Multiplied Mini and Minor collection

Double and Triple Mini/Minor prizes collect in separate visible increments, with a new Phoenix flight from the same physical cell for each unit. The stacked label stays intact. The payout is already settled once; these graphical steps add no extra Engine payment. Existing audio remains once per physical cell, without an invented ordinary count-up tone for each unit. The fitted interval between units is 1067 ms; normal between-cell timing remains 1333 ms. Those are local presentation fits, not recovered universal manufacturer timing or named-prize audio.

<img src="media/multiplied-mini101.gif" alt="TripleMini repeated pickups, current renderer in a synthetic state" width="720">

## Fifteen Phoenixes and Grand

All fifteen occupied positions award Grand **plus** all fifteen values. A full fifteen-Phoenix ordinary result settles directly; a Hold settles when it becomes full or exhausts its respins after the revival condition has been satisfied.

Grand and Major celebrations have dedicated presentation paths. The small-screen version places the larger announcement over the reels instead of requiring a second display. Its artwork and timings remain reconstructions. A full board being possible does not establish its casino probability: the model's landing chance and prize weights are local assumptions.

## Count-up and ending order

The source-inspired sequence includes a collected announcement, fire/bird effects, collection of eligible individual values, a WIN transfer and return fire. Major/Grand, ordinary cash and large cash can take different branches. The centered WINNER field reads the existing collection amount; it does not add earlier line wins or a separate named award again.

Coins launch upward from below the screen, spread, then fall. A coin/award phrase is not assigned to every Hold indiscriminately. The cash fountain uses a **provisional 50× stake presentation threshold**, with named/Grand cases and a qualifying final-Free combined round handled separately. That threshold is a local visual/audio policy, not a recovered universal casino rule. A 14-Phoenix board alone therefore does not prove a fountain or that particular jingle should play.

Autoplay and manual play use the same award-event routing. Muting, a skipped presentation or a different award branch can affect what is heard; autoplay is not intended to remove an otherwise eligible jingle. See [audio](audio.md) for cue order and outstanding limits.

## Counter awards and chained features

If the meter triggers with fewer than six Phoenixes, the currently visible Phoenixes lock for its free Hold. If a natural Hold already takes priority, the extra meter award queues. A later queued Hold starts with a new empty bonus board, rather than copying the first Hold's prizes into a second payment. Multiple owed meter awards are counted separately.

This order is supported by [hmyKdYjMjAQ around 8:29](https://www.youtube.com/watch?v=hmyKdYjMjAQ&t=509s) and the regional queueing description. Details of actual casino hidden distributions remain unresolved. Read [the collection counter](collection-counter.md) for the persistent state behind the feature.

Code: [award director](../firmware/src/feature_award_director.h), [collection flight](../firmware/src/collection_motion.h), [center treatment](../firmware/src/center_render49.inc), [opening music gate](../firmware/src/hold_opening31.h), [coin motion](../firmware/src/coin_motion.h).
