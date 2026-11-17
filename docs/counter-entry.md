# Counter-entry return

The simulator's counter-entry curve is **not calibrated to the real Phoenix Link machine**. Its rules include recorded observations, but its reel, prize, Hold-arrival and hidden-trigger probabilities remain modeled. These calculations describe practice within this game; they do not establish a casino entry point.

## What an entry return measures

Start at a displayed counter, keep the wager fixed, play until that counter first awards its free feature, and finish the entire owed round. This includes natural Holds, Free Games, retriggers and queued Holds along the way. Stop before placing another paid spin.

Entry return is **expected payout divided by expected paid stakes** over repeated entries. It is not the average of individual session percentages. The calculation assumes sufficient funds, default jackpots, Simulator Return **Option 3**, and no manual counter edits during the session.

A high starting counter can have positive conditional value while continuous play has a house edge. Paying to build the counter yourself includes the earlier costs. An expected return above 100% does not guarantee a winning session.

## Current calculated thresholds

| Wager | Long-run full return | First counter above 110%, all payouts | First above 110%, excluding Minor, Major and Grand |
|---|---:|---:|---:|
| 1¢ / 50 lines / 1×, $0.50 spin | 94.13% | 1,276 | 1,780 |
| Default $1 / 5 lines / 2×, $10 spin | 94.02% | 1,283 | 1,776 |

The excluded-prize column values **all** Minor, Major and Grand payments at zero, including doubled and tripled versions. Mini and ordinary cash remain. Outcomes and costs are still counted; losing sessions and exceptional events are not discarded. This is a payout-component diagnostic, not the same statistic as a guide that excludes only extraordinary wins.

[All 77 offered wagers](data/counter-entry-thresholds.csv) · [Return at selected counters](data/counter-entry-curves.csv)

Rare jackpots make a large contribution to the full-return curve. At the default wager's 1,283 entry, expected Grand payments contribute about $869 of the approximately $1,681 expected total payout. That dependence, and the nearly identical thresholds across bet multipliers, remain important calibration gaps.

## What the model still assumes

- The original virtual reel strips and stop frequencies are unrecovered; symbol weights and stacking are local choices.
- Each empty Hold cell currently has a 9.5% landing chance per respin. That figure is not a measured casino estimate.
- Prize frequencies and named-prize/multiplier chances are local weights, even where the prize amount itself is recorded.
- Reset values are uniform from 100 through 1,500; the hidden threshold is then uniform above that value through 1,887. Regional rules document limits, not those uniform distributions or their US applicability.
- A secure random generator supplies the draws. It does not authenticate the probabilities assigned to outcomes.

The [Engine](../firmware/src/phoenix_engine.h) defines these choices. The [counter guide](collection-counter.md) explains grouping, progress and queueing. [Math and evidence](math-and-evidence.md) distinguishes recovered observations from assumptions.

## What would improve real-machine fidelity

Counter-entry value depends jointly on the chance of triggering at each counter, counter ticks per paid round, the feature's expected payout, and ordinary returns while waiting. Matching a total RTP or a handful of entry thresholds does not identify those quantities separately.

Useful measurements retain every readable paid spin, non-triggering counter interval and complete bonus through final payment. Denomination, lines, multiplier, title and cabinet/program context must stay attached. Edited gaps and unfinished features remain censored observations rather than invented losses or wins. US observations are kept separate from regional rule variants.

A replacement probability model needs to fit these observations together, then predict different recordings that were not used for fitting. Entry thresholds need uncertainty intervals, especially for rare prizes. Authentic manufacturer probability/payset documentation would provide stronger identification than video estimates alone.
