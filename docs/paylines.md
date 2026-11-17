# Paylines

Hold grey to open **Game Menu**, then select **Paylines**. Each page shows up to six routes for the displayed line count. Grey advances to the next page; blue switches between the 5-, 10-, 15-, 25-, 50- and 100-line layouts. Hold grey to return to Game Menu. Your active layout opens first and is labelled **CURRENT**. Browsing does not change your bet. Blue visits the other layouts in ascending order after CURRENT, then returns to CURRENT.

<img src="media/paylines.png" alt="In-game Paylines viewer" width="720">

Each route selects one position on each of the five reels. A line win starts on reel one and follows that route from left to right. Different winning lines add together.

[Download every implemented route](data/implemented-paylines.csv). The CSV uses 0 for the top row, 1 for the middle and 2 for the bottom. Its evidence column identifies observed geometry that matches the implementation and provisional local routes. It describes the simulator; the complete original charts and program variants have not been recovered.

## Documented geometry coverage

| Layout | Implemented routes matching documented geometry | Scope |
|---|---:|---|
| 5 lines | 1 | Legacy scoped Line4 observation |
| 10 lines | 0 | Local fallback paths |
| 15 lines | 2 | Scoped Lines6 and8; legacy timing/program records remain incomplete |
| 25 lines | 25 | Retained US25-chart reconstruction; wider program transport is conditional |
| 50 lines | 42 of45 documented IDs | Five IDs remain unrecovered; three observed vectors are deferred |
| 100 lines | 13 of17 documented IDs | Four observed vectors are deferred; the other83 IDs lack documented geometry |

For50 lines, **31,33,39,45,48** are unrecovered. Observed **42,44,50** differ from the current implementation. All first25 implemented50-line vectors match the retained observations. For100 lines, the documented but deferred IDs are **15,36,37,43**. Their observed vectors remain evidence; adding them while holding other local placeholders/prices fixed does not yet reproduce all qualified round totals. No unobserved ID is assigned a factory path from a payout residual.

Evidence labels are attached to the actual count, numbered ID and vector. A50-line observation does not automatically establish the same ID in the100-line chart. Matching a filmed path at one mode/wager also does not authenticate every program variant. Source facts remain separate from the current playable chart and from random outcome frequencies.
