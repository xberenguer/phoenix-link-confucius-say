# Math, RNG and what the evidence establishes

For a plain-language introduction to hardware noise, the secure generator and the game choices it controls, read [How randomness works](randomness.md).

The included [release identity](../RELEASE.json), [Engine](../firmware/src/phoenix_engine.h), [Grand table](../firmware/src/static_grand_table.h) and [runtime identity](../firmware/src/math_runtime_identity.h) identify the current local model and its reference calculations. Original reel strips, complete pay programs, prize/meter distributions and configured casino RTP remain unrecovered.

## Evidence labels

| Category | Meaning |
|---|---|
| Official rule | A manufacturer/operator description, scoped to its named program and region |
| Source-observed value | A visible caption at an exact theme, denomination, line count, multiplier and mode |
| Local assumption | A weight, probability or hidden process chosen for this simulator |
| Engineering protection | A capacity, entropy or failure guard in this implementation |

The [payout reference](payouts.md) distinguishes scoped observations from fallback values. A verified line price does not identify its frequency or a complete return model.

<a id="what-generates-outcomes"></a>

## Outcome generation

An opaque external service runs **MbedTLS 3.6.7 AES-256 CTR_DRBG**. Prediction resistance is enabled: fresh strong entropy is requested before every action's generation request. The service requests 48 entropy bytes and generates at most 1,024 action-local output bytes. Unused output is erased instead of carrying into another action. [MbedTLS documents prediction resistance and its entropy requirement](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/add-a-random-generator/).

Native startup and reseeding use bounded internal-entropy leases under the pinned ESP32-S3 configuration. Browser entropy comes from `crypto.getRandomValues`. Neither platform substitutes a clock, public seed or weak fallback when the source is unavailable. The [Espressif S3 documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/random.html) describes the hardware-source prerequisites; this project does not claim a native min-entropy measurement.

Integer bounded draws reject the overhang before applying the remainder. Rejection attempts and the action reservoir have finite limits. Any source, exhaustion or mapping fault stops play and discards the scratch action without a partial debit or award. Rendering, particle motion and audio do not consume outcome randomness. Generator state is not exported or restored.

Code: [service API](../firmware/src/phoenix_random.h), [provider](../firmware/src/phoenix_random.cpp), [private crypto config](../firmware/src/phoenix_crypto124/config.h), [platform entropy](../firmware/src/platform_entropy125.h).

## Standards and statistical testing

[Nevada Technical Standard 1](https://www.gaming.nv.gov/siteassets/content/regs/technical-standard-1.pdf), revision February 26, 2026, defines CSPRNG in §1.010(9). Its §1.400(2)(b) applies to approvals on or after January 1, 2027 and requires a CSPRNG plus a recognized randomness-test battery. The 100Hz clause is in the earlier-approval branch, §1.400(2)(a).

[Nevada Regulation 14 §14.040](https://www.gaming.nv.gov/contentassets/d6342a488e944e6aa02337fa9d3c94e5/regulation-14-as-of-08-26.pdf) sets a 75% theoretical floor for each available wager. A casino-wide measured average does not identify a particular game's probabilities. [GLI-11 §4.7.2](https://gaminglabs.com/wp-content/uploads/2018/09/GLI-11-Gaming-Devices-V3-0.pdf) prohibits adaptive payout compensation and changing the selected result to display a different near miss. Documented persistent features, such as the collection meter, are distinct game rules. These are engineering references; original Phoenix probabilities still require separate evidence.

The first official **NIST STS 2.1.2** run used 1,000 sequences of 1,000,000 bits from the host provider. All 15 test families produced 188 report rows, with no aggregate proportion/uniformity flags and no inapplicable rows. Excursion rows use 608 applicable sequences. This is one retained first sample, not a search for a favorable seed. Individual sequence failures are part of the complete result.

That is software test evidence. It does not establish physical entropy quality, resistance to every side channel, original casino odds or Nevada approval. [NIST explicitly distinguishes statistical testing from cryptanalysis](https://www.nist.gov/publications/statistical-test-suite-random-and-pseudorandom-number-generators-cryptographic). The portable AES path is not presented as a qualified constant-time or hardware-AES implementation. [Upstream maintenance scope](https://github.com/Mbed-TLS/mbedtls/blob/development/BRANCHES.md) must also be considered when updating the crypto dependency.

## The local reel and feature law

The current mathematical model is **U (`0x0002001e`)**. Its numeric source/family/runtime identities and table bindings are recorded in the included release data.

Ordinary symbols use local integer weights and column-specific Wild eligibility. A correlated three-position Sage-run construction controls adjacent Wild runs. Free games pair the two outer reel groups; Phoenix values on paired stops are generated separately. These rules do not recover the original physical or virtual strips.

Retained US footage already shows differences in ordinary-symbol repeats, Wild-stack patterns and Phoenix-bearing boards. These are known calibration gaps in the current window model. A clearer reconstruction of a symbol or a qualified CSPRNG does not resolve its outcome frequency; the local weights still require independently counted, configuration-specific gameplay evidence.

These weights are fixed configuration constants. The generator does not inspect the player's funded balance or accumulated past wins/losses to tune the next result. A paid-action admission check can refuse an unaffordable or unsafe action; disclosed meter, mode and locked-feature state can legitimately affect what action is generated. [The balance/history tests](testing.md#check-balance-and-outcome-history-independence) exercise that separation, not casino calibration.

Hold uses a local 950/10,000 arrival gate per vacancy. Prize weights, named-prize chances and multipliers are local assumptions. The session's collection counter uses uniform reset/threshold laws; the published operator description supplies support/limits, not those distributions. These choices are not a fit to an unbiased casino spin census and do not adapt to the player's recent wins or losses.

A retained development ledger contains **143 funding-confirmed paid-Base slots: 141 complete boards and two partial/censored boards**, across three recordings and ten recording/wager groups. Sixteen funding-unknown slots remain separate. The complete-board counts are 40 from [3pf](https://www.youtube.com/watch?v=3pfZnZUWGhI), 53 from [Rio](https://www.youtube.com/watch?v=GclIM2tutCg) and 48 from [Cannery](https://www.youtube.com/watch?v=YoyplGW4DPM). These are exposed, clustered publication samples with subset independent checks—not 141 independent double-read casino trials.

No ordinary adjacent equal pair was read in the funded cells, while two separated Q–C–Q reel windows were retained. That differs materially from the current ordinary-residual construction and supports further reel-window work; it does not justify hard-zero probabilities or a recovered casino law. No alternative fitted law is deployed.

Readable continuous recordings can expose a rule, shape or price mismatch. Edited gaps, profile changes, fast stops and bonus-selected uploads must remain visible in the denominators. More symbol cells do not create more independent spins. Rare Grand/prize behavior and meter cycles need their own complete feature exposure. The [source guide](sources.md) keeps program/theme restrictions explicit.

## Return and house edge

The [counter-entry reference](counter-entry.md) gives conditional return calculations for the current model and explains why they are not validated casino entry thresholds.

SIMULATOR RETURN offers six options. Applying an option at a closed Ready/Base boundary changes **future Grand only**. It does not change reel weights, hit probabilities, ordinary prices or other prizes.

The displayed return/edge ranges are **ideal-reference-only** enclosures under a fixed profile, fixed option, always-funded play and no Lucky Chance. They assume ideal uniform draws in the unchanged local stochastic game. They do not include every source/runtime fault, restart rule, adaptive wager strategy or short-session fluctuation. Finite draw caps have nonzero failure mass and fail closed; their implementation is not silently equated with unlimited sampling.

These ranges require the default prize schedule. Edited jackpot amounts change payments while leaving arrival probabilities unchanged, so the return selector hides and locks its preset ranges until defaults are restored. Setting the feature counter changes the starting condition; the fixed-profile full-cycle range is not an estimate of the next spin's conditional value or repeated manual counter edits.

Use the range shown for the selected denomination, lines, multiplier, option and prize-cap context. Its reference calculation covers the complete local paid-round process, including triggered Free/Hold/meter/Grand continuations, under the stated ideal-uniform assumptions. It is not a casino measurement or just a Base-line estimate. The tiny display rounds the enclosure outward. A short sample can exceed 100% because rare awards fluctuate.

The current UI asks for added credits when a paid wager is unaffordable. The Engine's separately callable Lucky Chance rule is outside the return-reference scope and is not offered as ordinary custom-schedule play.

RTP is expected paid amount divided by expected stake under a specified process and policy. A short observed return, an EV conditional on a high counter and a fixed-profile long-run reference are different quantities. County denomination averages do not identify this game's pay program. Grand amounts are local schedules, not live casino progressive values.

Visible line captions and named-prize increments constrain individual prices at their filmed configurations. Numbered line markers constrain geometry. Neither type of observation identifies outcome frequencies. A price, route or probability-law change requires fresh return calculations for every affected profile; unchanged Grand amounts alone do not preserve a return bound. The [payout guide](payouts.md) separates observed prices, partial chart coverage and local fallbacks.

Replacing the modeled reel/feature law would require new Base/Free/Hold/meter/prize calculations, independent evidence and new source qualification; these ranges cannot simply be relabeled for a different law.
