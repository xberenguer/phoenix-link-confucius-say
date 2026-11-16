# Sources and evidence scope

Manufacturer references, operator rules and gameplay recordings used to document the game. Rules and pay programs vary by region and cabinet configuration.

## Public primary references

| Reference | Information |
|---|---|
| [Aristocrat US Phoenix Link / Confucius Say](https://www.aristocratgaming.com/us/games/us-phoenix-link) | Twin Spins, Hold & Spin, scaling named jackpots and a Phoenix collection feature |
| [Aristocrat Australia Phoenix Link](https://www.aristocratgaming.com/aus/games/aus-phoenix-link) | Regional description of paired 1+2 / 4+5 reels and Double/Triple Phoenix mechanics |
| [Loto-Québec official rules PDF](https://casinos.lotoquebec.com/dam/jcr:4add9be1-336a-4efc-b4c9-e41696b876b2/Phoenix%20Link%20Confucius%20Say.pdf) | Four-page regional rule description, software 1.01-69279 |
| [Royal River Casino feature description](https://www.royalrivercasino.com/blog/new-slot-feature-phoenix-link-brings-the-fire) | Operator description of the four themes, six-game scatter entry and the mystery feature |
| [Maryland October 2024 listing](https://www.mdgaming.com/wp-content/uploads/2024/10/Commission-Spreadsheet-October-2024-Final.pdf) | Listing context for a named/versioned game |
| [M5Stack StickS3 documentation](https://docs.m5stack.com/en/core/StickS3) | Hardware, download-mode procedure and speaker power notes |
| [Espressif ESP32-S3 random API, IDF 4.4.7](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/random.html) | Conditions for hardware entropy in the pinned-generation platform |
| [Nevada Technical Standard 1](https://www.gaming.nv.gov/siteassets/content/regs/technical-standard-1.pdf) | CSPRNG definition and software-RNG testing branch for approvals from January 1, 2027 |
| [Nevada Regulation 14](https://www.gaming.nv.gov/contentassets/d6342a488e944e6aa02337fa9d3c94e5/regulation-14-as-of-08-26.pdf) | Per-wager theoretical return floor and accurate rules/outcome displays |
| [GLI-11 Gaming Devices v3.0](https://gaminglabs.com/wp-content/uploads/2018/09/GLI-11-Gaming-Devices-V3-0.pdf) | RNG outcome integrity and restrictions on adaptive payout compensation and substituted results |
| [NIST statistical-test scope](https://www.nist.gov/publications/statistical-test-suite-random-and-pseudorandom-number-generators-cryptographic) | Recognized test methodology and its limits |
| [MbedTLS random-data guide](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/add-a-random-generator/) | CTR_DRBG setup and prediction-resistant reseeding |
| [PlatformIO installation](https://docs.platformio.org/en/stable/core/installation/index.html) | Development environment setup |

The Québec program 1.01-69279 specifies separate Base/Free paytables, wager-specific counters, revival and queued meter Holds. Its Free retriggers award six additional games; the implemented US profile awards three. Phoenix prize rules also vary by version.

## Scoped first-hand footage

Gameplay recordings are indexed below by the features, prices and configurations they show.

| Recording | Scope used in this guide/source |
|---|---|
| [mBcjPK63KW4](https://www.youtube.com/watch?v=mBcjPK63KW4) | Recorded Palms denomination/line/BET deck; $1 PLAY 10 control is visible; its selectable paid row uses the family adaptation documented in the wager guide |
| [HzpqxpE-Ht8](https://www.youtube.com/watch?v=HzpqxpE-Ht8) | Multiple Phoenix Link themes; Confucius segments provide theme-specific references |
| [5BUJOWxIBN4 near 10:10](https://www.youtube.com/watch?v=5BUJOWxIBN4&t=610s) | Initial three-Wisdom announcement, START/Twin transition and lower-WIN timing at its visible configuration |
| [3pfZnZUWGhI](https://www.youtube.com/watch?v=3pfZnZUWGhI) | Wisdom entries, active Twin audio and scoped 2¢/50/3× Bell/King Base prices; meter entry around 22:10 |
| [hmyKdYjMjAQ at 8:29](https://www.youtube.com/watch?v=hmyKdYjMjAQ&t=509s) | New queued meter Hold begins empty after the preceding feature |
| [hmyKdYjMjAQ at 25:40](https://www.youtube.com/watch?v=hmyKdYjMjAQ&t=1540s) | Twin symbol stops can carry different Phoenix amounts/prizes |
| [mQZbc-_wjO4](https://www.youtube.com/watch?v=mQZbc-_wjO4) | Target US Free/Hold observations and visible meter increments/wager switching |
| [YfbaaFXhln0](https://www.youtube.com/watch?v=YfbaaFXhln0) | Scoped 50¢/25/1× line chart, prices and Free/collection sequences; no blanket extension to every profile |
| [wO4wC7v3Ouw](https://www.youtube.com/watch?v=wO4wC7v3Ouw) | Chronological raw-board audit across four visible wager profiles; current 10¢/25/1× King/Book Base prices are scoped to native Line 11/Line 22 captions |
| [GclIM2tutCg](https://www.youtube.com/watch?v=GclIM2tutCg) | Current source-scoped Bell/Jack Base prices at 1¢/50/10× |
| [Zfc-LBjwXQA](https://www.youtube.com/watch?v=Zfc-LBjwXQA) | Current source-scoped Bell/Jack/Queen Base prices at 10¢/100/1× |
| [qQ4gPanpkdU near 1:27](https://www.youtube.com/watch?v=qQ4gPanpkdU&t=87s) | Feature celebration music reference |

Price annotations remain next to their exact branches in the [Engine](../firmware/src/phoenix_engine.h). The [compiled tables](payouts.md) record what the software does, including unresolved local fallback values. A visible payout establishes a price at its observed context; it does not estimate the event's underlying probability.

## Unrecovered original data

The original joint reel/stop law, virtual weights, full pay programs, prize/multiplier schedules, property RTP settings, meter hazard/reset weights and complete original graphics/audio banks remain unrecovered. Aggregate denomination returns cannot identify them. See [math](math-and-evidence.md), [graphics](graphics.md) and [audio](audio.md) for the practical consequences.

## Documentation media

Included media comes from the simulator with demonstration credits. Scripted gallery fixtures and live-session previews are identified by their [provenance](gallery-manifest.json), version, dimensions and hashes. Third-party attribution is in [attribution](attribution.md).
