# Sources, evidence labels and version scope

This guide accompanies version 102 source, tables and deterministic gallery. It distinguishes official material, first-hand machine footage, independent commentary and local implementation. Similar theme names or software numbers do not prove identical casino settings. Unknown original probabilities and source-specific configuration limits remain explicit.

## Public primary references

| Reference | Supports | Does not establish |
|---|---|---|
| [Aristocrat US Phoenix Link / Confucius Say](https://www.aristocratgaming.com/us/games/us-phoenix-link) | Twin Spins, Hold & Spin, scaling named jackpots and a Phoenix collection feature | Original reel strips, probabilities, a US casino RTP setting or original asset files |
| [Aristocrat Australia Phoenix Link](https://www.aristocratgaming.com/aus/games/aus-phoenix-link) | Regional description of paired 1+2 / 4+5 reels and Double/Triple Phoenix mechanics | Identity of Australian and US mathematical programs |
| [Loto-Québec official rules PDF](https://casinos.lotoquebec.com/dam/jcr:4add9be1-336a-4efc-b4c9-e41696b876b2/Phoenix%20Link%20Confucius%20Say.pdf) | Four-page regional rule description, software 1.01-69279 | Complete numeric paytables, reel weights or a universal US program |
| [Royal River Casino feature description](https://www.royalrivercasino.com/blog/new-slot-feature-phoenix-link-brings-the-fire) | Operator description of the four themes, six-game scatter entry and the mystery feature | Precise event frequencies or this simulator's certification |
| [Maryland October 2024 listing](https://www.mdgaming.com/wp-content/uploads/2024/10/Commission-Spreadsheet-October-2024-Final.pdf) | Listing context for a named/versioned game | Approval of this independent reconstruction or matching property settings |
| [M5Stack StickS3 documentation](https://docs.m5stack.com/en/core/StickS3) | Hardware, download-mode procedure and speaker power notes | Performance or validation of this particular game |
| [Espressif ESP32-S3 random API, IDF 4.4.7](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/random.html) | Conditions for hardware entropy in the pinned-generation platform | A recovered casino RNG or cryptographic qualification of PCG |
| [PlatformIO installation](https://docs.platformio.org/en/stable/core/installation/index.html) | Development environment setup | A prebuilt or published version-100 game |

The Québec PDF was reviewed as complete pages, including its symbol illustrations. It explicitly distinguishes Base and Free paytables, wager-specific counters, revival and queued meter Holds. It specifies +6 Free retriggers, while the targeted US implementation uses +3 from US observations. It also acknowledges version-dependent Phoenix prize rules. Those differences are retained instead of treating the PDF as a complete US PAR sheet.

## Scoped first-hand footage

These are original machine recordings used for specific visible facts. Source timeline anchors are given where established; they are not claims that all footage was watched continuously or that a bonus-selected recording is an unbiased statistical sample. No raw video or camera screenshot is included in the gallery.

| Recording | Scope used in this guide/source |
|---|---|
| [mBcjPK63KW4](https://www.youtube.com/watch?v=mBcjPK63KW4) | Recorded Palms denomination/line/BET deck; $1 PLAY 10 red row remains unqualified |
| [HzpqxpE-Ht8](https://www.youtube.com/watch?v=HzpqxpE-Ht8) | Original Confucius symbol layout and configuration-specific line captions; 5¢/25/5× and 10¢/25/3× examples stay scoped |
| [5BUJOWxIBN4 near 10:10](https://www.youtube.com/watch?v=5BUJOWxIBN4&t=610s) | Initial three-Wisdom announcement, START/Twin transition and lower-WIN timing at its visible configuration |
| [3pfZnZUWGhI](https://www.youtube.com/watch?v=3pfZnZUWGhI) | Independent Wisdom entries and active Twin audio; meter entry around 22:10 |
| [hmyKdYjMjAQ at 8:29](https://www.youtube.com/watch?v=hmyKdYjMjAQ&t=509s) | New queued meter Hold begins empty after the preceding feature |
| [hmyKdYjMjAQ at 25:40](https://www.youtube.com/watch?v=hmyKdYjMjAQ&t=1540s) | Twin symbol stops can carry different Phoenix amounts/prizes |
| [mQZbc-_wjO4](https://www.youtube.com/watch?v=mQZbc-_wjO4) | Target US Free/Hold observations and visible meter increments/wager switching |
| [YfbaaFXhln0](https://www.youtube.com/watch?v=YfbaaFXhln0) | Scoped 50¢/25/1× line chart, prices and Free/collection sequences; no blanket extension to every profile |
| [wO4wC7v3Ouw](https://www.youtube.com/watch?v=wO4wC7v3Ouw) | Chronological raw-board audit across four visible wager profiles; two 10¢/25/1× Base-price corrections are retained from version 99 in version 102, scoped to native Line 11/Line 22 captions |
| [GclIM2tutCg](https://www.youtube.com/watch?v=GclIM2tutCg) | Version-92 Base price corrections at 1¢/50/10× |
| [Zfc-LBjwXQA](https://www.youtube.com/watch?v=Zfc-LBjwXQA) | Version-92 Base price corrections at 10¢/100/1× |
| [qQ4gPanpkdU near 1:27](https://www.youtube.com/watch?v=qQ4gPanpkdU&t=87s) | Clean reference program/celebration passage; timestamp is not a universal feature-event rule |

Price annotations remain next to their exact branches in the [Engine](../firmware/src/phoenix_engine.h). The [compiled tables](payouts.md) record what the software does, including unresolved local fallback values. A visible payout establishes a price at its observed context; it does not estimate the event's underlying probability.

## Secondary commentary

[Desolator Gaming's Phoenix guide](https://www.youtube.com/watch?v=BJ4HAXsPE1k) is useful for hypotheses and terminology. AP thresholds, extrapolated returns and narrated claims are not authenticated original math. At [8:09](https://www.youtube.com/watch?v=BJ4HAXsPE1k&t=489s), he describes needing 100,000 to one million spins and lacking the resources to collect them; the following explanation describes estimating entry points and comparing YouTube sessions. That is not a claim to possess that many raw recorded spins. Private paid-community material is not republished here, and the guide does not claim access to hidden vendor/server data.

## Unrecovered original data

The original joint reel/stop law, virtual weights, full pay programs, prize/multiplier schedules, property RTP settings, meter hazard/reset weights and complete original graphics/audio banks remain unrecovered. Aggregate denomination returns cannot identify them. See [math](math-and-evidence.md), [graphics](graphics.md) and [audio](audio.md) for the practical consequences.

## Documentation media

All included screenshots/GIFs are synthetic, deterministic renders from the game's own renderer. The [gallery manifest](gallery-manifest.json) binds their version, dimensions and hashes. They are display demonstrations, not casino frequency samples or a player's device state. Third-party attribution is in [attribution](attribution.md).
