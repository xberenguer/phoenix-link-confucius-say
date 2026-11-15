# Math, RNG and what the evidence establishes

The current release uses local model H, ID `0x00020011`. Numeric tables are compiled from the included [Engine](../firmware/src/phoenix_engine.h); [static_grand.h](../firmware/src/static_grand.h), [static_grand_table.h](../firmware/src/static_grand_table.h) and [math_runtime_identity.h](../firmware/src/math_runtime_identity.h) bind its 7,200 Grand schedule entries and actual source.

**This is a local simulator model, not a recovered casino pay program.** Its results are reproducible and inspectable, but original reel strips, virtual stops, complete paysets, prize weights, meter distributions and authentic configured RTP remain unrecovered. Source acceptance qualifies this implementation; it does not establish original casino odds.

## Four different kinds of fact

| Category | Example | Scope |
|---|---|---|
| Official rule | Separate Twin Free and Hold features | Named regional/operator or manufacturer source |
| Source-observed value | A visible line caption pays a particular amount | Exact theme, denomination, lines, multiplier and mode |
| Local model assumption | Symbol weights or uniform hidden meter threshold | What this simulator does; no original-odds claim |
| Engineering protection | Capacity checks and unknown-save service | Protects this implementation; not a casino rule |

The [compiled payout reference](payouts.md) includes both observed corrections and fallback values. More table rows do not mean more original data has been recovered.

## What generates outcomes?

The Engine uses PCG32 with a 64-bit internal state and an odd stream increment. A bounded draw rejects the small overhang before applying the remainder, avoiding ordinary modulo bias. A weighted draw then walks an integer cumulative distribution.

This is deterministic pseudorandom generation, useful for exact replay and regression checks. It is not a CSPRNG, a recovered Aristocrat RNG or proof of unpredictable licensed operation. Native startup temporarily enables the offline hardware entropy source, captures eight bytes, disables it, then starts the application M5 peripherals. The full 64-bit value, including zero, reaches the exact UI seed API. This happens after framework startup, not before every SDK subsystem. Saved Engine state includes the PCG state and increment, so restored owed sessions continue without reseeding. The browser uses WebCrypto instead of emulating the hardware entropy source. Statistical draw law and seed unpredictability are different questions. Espressif documents the conditions for hardware entropy in its [ESP32-S3 random API](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/random.html).

Rendering, coin particles, flame phases and audio do not consume the game's outcome RNG. An animation skip therefore cannot choose a different committed outcome.

## The current reel and feature model

Ordinary symbols are selected from local weights, with column-specific Wild eligibility. A correlated three-position Sage-run construction increases adjacent Sage runs while preserving the modeled per-row Wild marginal. Free games additionally copy symbol stops across the two outer pairs; Phoenix prizes on those matching stops are generated separately.

This is not a recovered physical or virtual strip model. Equal per-row symbol percentages would not establish the correct joint reel distribution: adjacent repeats, stacks, cross-reel correlation and rare combinations also matter. Current model calculations must not be presented as measured casino hit rates.

Hold uses a local 950/10,000 chance per vacant cell per respin. Phoenix prize weights, named-prize chances and named multiplier chances are also local. The collection counter uses a persistent hidden threshold with local uniform reset/threshold laws; only its limit/support and broad rules have operator evidence.

Those parameters are local modeling choices, constrained by observations where evidence is available. They were **not fitted from a complete unbiased Vegas spin census**. The current source does not adapt results to a player's recent wins or losses.

The compatibility Engine also contains Lucky Chance: an unaffordable remaining bank can be risked with success probability equal to remaining credits divided by the full stake. The regional rulebook describes that feature, but the current custom-schedule player UI disables this route and requests added credits instead. Its presence in an Engine API or test fixture is not a claim that Lucky Chance is available in the shipped default menu. The local return-bound scope explicitly excludes it.

## Return and house edge

RTP is expected paid amount divided by expected stake over a defined long-run model. A short sample return is a random observation. Counter-conditioned EV, a fixed-profile long-run return and a casino's reported denomination-wide return are different quantities.

The current source retains locally reviewed schedule slots whose bound qualification is below 100% under stated **fixed-profile, fixed-option, always-funded, no-Lucky, ideal-draw** assumptions. That is a model-specific bound, not measured casino RTP, certification, a bound for every possible wager-switching strategy or a guarantee against short-term house loss. Simulator Return exposes these choices as neutral Options 1–6 with the current profile's outward-rounded model ranges; it does not label them as authentic casino settings.

Public Clark County or Strip denomination averages are useful context. They cannot identify a particular Confucius cabinet's configured return or determine this game's individual symbol/feature probabilities. No official aggregate percentage is used as if it were an exact Phoenix Link PAR sheet.

Grand schedules are local simulator amounts. The current Base lookup includes source-observed Bell, Jack, Queen, King and Book prices at explicitly qualified configurations. The [scoped payout reference](payouts.md#observed-base-prices) gives their unit credits, multiplied awards and source limits. Other entries can remain local fallbacks. A verified price does not establish symbol frequencies, feature probabilities or a full casino return model.

## What footage can and cannot estimate

A continuous, unselected recording with readable wager, mode, meter and outcomes can support a spin ledger. Useful fields include source time, denomination, lines, multiplier, visible stop window, scatter/Phoenix count, line captions, final win, feature entry/exit and counter movements. Edited gaps, bonus-only compilations and changing profiles require separate strata or exclusions.

Two entertaining videos can expose a rule or payout error. They are not enough to tightly estimate a rare full-board Grand or the complete hidden prize distribution. Sample requirements depend on event frequency and desired uncertainty, not only hours of video. Confidence intervals, independent sessions and held-out checks must accompany any future fitted estimate.

### Completed raw-board audit

A further audit retained the first 50 witnessed paid starts in each of two registered recording windows: [wO4wC7v3Ouw](https://www.youtube.com/watch?v=wO4wC7v3Ouw&t=205s) and [YfbaaFXhln0](https://www.youtube.com/watch?v=YfbaaFXhln0&t=970s). All 100 raw boards and 1,500 cells were recovered from native frames, including losing starts. Five visible wager configurations remain separate. An independent reader checked 24 boards; two cell transcription errors were corrected with both original readings retained. The other 76 boards have one reader.

These recordings were published entertainment footage, screened earlier for readability, with edits and outcome-oriented titles. They are not an unbiased casino sample. Terminal award amounts are known for only 71 starts, so this audit does not report a source RTP.

| Event per raw board | First recording, 50 starts | Second recording, 50 starts | Analytic prediction of the current local generator |
|---|---:|---:|---:|
| Ordinary symbol repeats within one reel | 0/50 | 0/50 | 77.66% |
| At least one full three-Sage reel | 8/50 | 4/50 | 2.73% |
| At least one Sage/Wild | 17/50 | 17/50 | 13.08% |
| At least one Phoenix | 31/50 | 23/50 | 86.35% |

The current generator differs substantially from those recordings. In particular, its 86.35% Phoenix-bearing-board prediction exceeds the observed window fractions of 62% and 46%. The limited and selected recordings do not establish the true casino rate, but they do show why the current probabilities must remain labeled as local assumptions.

Replacing the generator requires an integer sampler checked against its declared window law, fresh Base/Free/Hold/meter/prize return calculations, further independent sessions and safe continuation of recognized saved obligations. Existing return bounds cannot be relabeled for a changed generator. The raw Base-board audit supplies no vacant-position Hold denominator; that feature has a separate chronological exposure audit.

The [source ledger](sources.md) lists the specific evidence used here. AP commentary and paid community guides may suggest hypotheses, but are not authenticated manufacturer math. There is no claim of access to a private casino server or vendor PAR data.
