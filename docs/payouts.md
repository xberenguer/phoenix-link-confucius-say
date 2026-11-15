# Symbols, winning combinations and payouts

This page describes version **0.2.102**, local model H (`0x00020011`). The complete lookup tables are freshly exported from the included C++ evaluator. They contain source-observed corrections and local fallback values; they are not recovered casino PAR sheets.

## Symbol roles

| Symbol | Role |
|---|---|
| 9, 10, J, Q, K | Regular line-pay symbols |
| Bell, abacus, book, candle | Picture symbols with line pays and winning animations |
| Confucius / Sage | Wild substitute for regular symbols; adjacent awarded faces can visually join into a taller gold figure |
| Red Wisdom tile | Scatter; can award a scatter win and Free Games |
| Phoenix | Carries a cash or named-prize value, contributes to the active collection counter and can trigger Hold & Spin |

## Line wins

A line win starts on the leftmost reel and continues through adjacent reels on a selected active path. Wilds substitute for regular symbols. Phoenix and Wisdom do not act as regular line-pay substitutes. The highest compatible award pays once on a line; awards on different lines are added.

The prototype lookup takes the symbol, match count, Base/Free mode and exact denomination/line/multiplier configuration. Some source-observed corrections currently apply only to the filmed configuration; other lookups retain local fallback prices. These coverage boundaries do **not** establish that a real machine changes a symbol's unit price when only its multiplier changes. The operator rules state that line awards are multiplied by the bet multiplier, while the full numeric tables and matching US program identities remain incomplete.

**Line award in credits = unit-credit price × bet multiplier.**

**Line award in money = line-award credits × denomination.**

The number of active lines is already used to select and evaluate the paths. Do not multiply a single line award by the total number of lines again. Multiple paying lines add separately.

## Complete implemented combination reference

- [All implemented line-pay entries](data/implemented-line-pays.csv): 48,000 rows, including zero-pay entries, both modes, ten evaluator symbols and match counts 2–5 across 600 internal denomination/line/multiplier profiles.
- [All active selected-line paths](data/implemented-paylines.csv): 205 paths across the six internal line selections. Rows use top=0, middle=1, bottom=2.
- [The current menu's 55 offered wagers](data/offered-wagers.csv): denomination, lines, multiplier, stake credits and stake money.
- [Export metadata](data/reference-metadata.json): source version, input hashes, exporter hash and dataset hashes.

The compatibility engine can represent more profiles than the wager menu offers. The CSV marks whether each profile is currently selectable. Its complete internal data must not be mistaken for additional real-machine buttons.

A positive evaluator entry does not prove that every compatible board can naturally occur. Wild placement, correlated reels and the prototype generator constrain which patterns can be drawn. The original joint reel law remains under study.

### Reading a row

For Base, 1¢ / 50 lines / 10×, a three-Jack line has unit price 5. Its award is 50 credits, or $0.50. The stake for the whole spin is 500 credits, or $5.00. Additional winning lines add to the $0.50.

An entry with unit price 0 does not pay that combination in that particular implemented profile.

## Version 92's five scoped corrections

| Exact Base configuration | Combination | Previous unit credits | Current unit credits |
|---|---|---:|---:|
| 1¢ / 50 lines / 10× | Two Bells | 0 | 2 |
| 1¢ / 50 lines / 10× | Three Jacks | 8 | 5 |
| 10¢ / 100 lines / 1× | Two Bells | 0 | 2 |
| 10¢ / 100 lines / 1× | Three Jacks | 8 | 5 |
| 10¢ / 100 lines / 1× | Four Queens | 20 | 10 |

These corrections do not establish the same price for another denomination, line count, multiplier, regional program or Free Games table. The annotated [engine source](../firmware/src/phoenix_engine.h) retains the individual source contexts.

## Accepted version 97's two scoped corrections

| Exact Base configuration | Combination | Previous unit credits | Accepted unit credits | Award after multiplier |
|---|---|---:|---:|---:|
| 2¢ / 50 lines / 3× | Two Bells | 0 | 2 | 6 credits / $0.12 |
| 2¢ / 50 lines / 3× | Three Kings | 10 | 5 | 15 credits / $0.30 |

The amounts come from explicit line-win captions in identified Confucius Say footage. They do not change symbol weights, feature probabilities, Free prices or retained Grand integers. The version-97 export differed from the version-92 numeric lookup in exactly those two rows. Menu availability also includes the five $1/10-line choices added in version 95; selected-line paths are unchanged. These earlier corrections are retained in version 102 tables; installation history is separate from recovery of the original probability model.

## Version 99's two scoped corrections

| Exact Base configuration | Combination | Previous unit credits | Current unit credits | Award after multiplier |
|---|---|---:|---:|---:|
| 10¢ / 25 lines / 1× | Three Kings, including Wild substitutes | 10 | 5 | 5 credits / $0.50 |
| 10¢ / 25 lines / 1× | Five Books | 250 | 100 | 100 credits / $10.00 |

The original [King line caption near 4:50](https://www.youtube.com/watch?v=wO4wC7v3Ouw&t=290s) shows “Line 11 wins 5”; the [Book caption at 5:03](https://www.youtube.com/watch?v=wO4wC7v3Ouw&t=303s) shows “Line 22 wins 100”. These captions establish line prices at that filmed configuration, not the terminal round totals. They do not qualify another denomination, line count, multiplier, regional program or Free Games table.

Compared with version 98, exactly these two rows change in the 48,000-row export. Paths, menu availability, symbol/feature probabilities, Free prices and all retained Grand integers are unchanged. Version99 was built, installed and native-verified; version 102 retains these same two corrections. The generator remains a local reconstruction, without recovered casino calibration.

## Wisdom scatter awards

Scatter awards use the **total stake**, rather than a line's bet multiplier alone. The implemented Base and Free tables differ.

| Wisdom count used by the award lookup | Base: total-stake multiple | Free: total-stake multiple |
|---|---:|---:|
| 0–2 | 0 | 0 |
| 3 | 2× | 1× |
| 4 | 10× | 15× |
| 5 | 75× | 50× |

Some entries are observed, and others remain local assumptions; consult the source annotations before treating a number as a machine fact. The prototype caps its numeric lookup at the five-symbol row. The natural generator's support is a separate unresolved issue.

The scatter win and the award of Free Games are separate effects. See [Free Games](free-games.md) for entry, retrigger and continuation rules.

## Phoenix and jackpots

Phoenix values are collected under the feature rules, separately from ordinary line wins. Named Mini, Minor and Major symbols can carry their own multipliers. In the current implementation, these named multipliers do not multiply a plain cash Phoenix. A full 15-cell board can award Grand **in addition to** the displayed Phoenix values.

The actual amount and applicable prize rules depend on the active configuration and any cached award owed by a saved feature. Grand amounts are local simulator schedules, not live casino progressive values. See [Hold & Spin](hold-and-spin.md), [the collection counter](collection-counter.md) and [math and evidence](math-and-evidence.md).

## Regenerating the reference

From the repository root:

```sh
c++ -std=c++17 -O2 -Ifirmware/src tools/export_wiki_tables.cpp -o /tmp/export-wiki
/tmp/export-wiki docs/data
```

Regenerate and verify the metadata against the actual release source. Never copy a table from an older release and relabel it as a newer one.
