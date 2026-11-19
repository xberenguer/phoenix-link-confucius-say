# Testing and evidence boundaries

Current tests qualify specific software paths and source identities. They do not establish original casino odds or complete hardware approval. The current [release status](../CHANGELOG.md#validation) separates host/target results from physical validation.

| Check | Establishes | Limit |
|---|---|---|
| Published AES/CTR_DRBG vectors | Exercised primitive agrees with the known vectors | Does not measure platform entropy |
| Independent DRBG byte oracle | Seed/reseed/request/discard behavior agrees | Test entropy is not a production seed interface |
| Rejection mapping and price/path comparison | Mapping counts and implemented evaluator behavior | Does not recover casino strips/probabilities |
| Atomic Engine fault cases | Failed Base/Free/Hold/Lucky/wager/init actions commit no partial state | Does not prove every possible fault |
| Balance and outcome-history pairs | Matching test inputs produce matching results across different funded balances and past win/loss totals | Disclosed meter/feature state and admission limits can legitimately change behavior |
| Actual Main/UI cases | Fresh startup, controls, menu pauses and post-fault dispatch follow exercised paths | Hardware APIs are mocked in host tests |
| Music pause/context cases | Decoded PCM continues across pauses and Sound auditions without replaying the intro | Does not prove physical acoustic identity |
| ASAN/UBSAN | No detected errors on exercised paths | Not a universal memory-safety proof |
| Exact raster comparison | Optimized drawing agrees with its reference | Reference fidelity is a separate question |
| Host/Wasm parity | Same specified fixture gives the same output | Browser/device environments still differ |
| Target build | Code links/fits the target configuration | Does not prove a flash, physical entropy or speaker output |

## Recognized randomness battery

The first official NIST STS 2.1.2 run used 1,000 × 1,000,000 bits, all 15 default test families and 188 report rows. No aggregate uniformity/proportion flags or inapplicable rows were reported. Individual sequence results were retained, with no seed search or replacement run. This host output test is distinct from native entropy qualification and from game-return calculations. See [NIST's scope](https://www.nist.gov/publications/statistical-test-suite-random-and-pseudorandom-number-generators-cryptographic) and [math](math-and-evidence.md).

The production crypto build excludes deterministic self-test/input helpers. Those exist only in explicit test builds. Source/protocol/output hashes and the final API/configuration equivalence are reviewed separately from the statistical results.

## Reproduce public tables

Use [the exporter command](payouts.md#regenerating-the-reference) against the current source. Verify its source version, input hashes and dataset hashes. The internal evaluator includes more profiles than the 77 offered wager choices.

## Gameplay captures and engineering fixtures

The player-facing [gallery](graphics.md) is reserved for captures reached by playing the unchanged game. Do not substitute a forced board, chosen random result or isolated rendering fixture for gameplay. Symbol close-ups must retain a link to the gameplay frame or sequence they were cropped from. Capture provenance belongs beside the published media, including source identity, session/action references and any scaling or cropping.

Synthetic renderer fixtures remain useful for engineering comparisons, but cannot establish what occurred during play. A selected gameplay clip also does not measure event frequency, full-game FPS, physical display performance or speaker output. Review the gallery's documented capture coverage before assuming that every possible event or animation has been recorded.

Menu checks pause real feature presentations, keep the phase, money, meter and outcome-generator state fixed while browsing, and resume the existing music context. Browser checks also cover a real Hold respin, Sound auditions and automatic cached-build upgrades followed by offline play.

Useful cases include partial reel stops, awarded Sage joins, Twin pairs, natural/meter/chained Hold, initial zero-new revival, named collection, direct 15, source-failure UI, manual/autoplay quick-stop and wrapped clocks. Funding/wager changes and meter continuity are checked within a live session. Reload/boot checks require a new zero-balance denomination chooser with autoplay off.

## Physical acceptance

After an installation, verify the intended board and written bytes, current menu version, a fresh zero-balance denomination chooser, autoplay off, controls, an ordinary action and one audio audition. Record physical display, stack/timing, entropy-source behavior and audible output separately. Never infer physical operation from a build or a desktop frame.

## Run the CSPRNG source tests

From the repository root, run `python3 tools/test_rng.py --sanitize`. A C99 compiler and a C++17 compiler are required. The script builds the included Mbed TLS source and exercises published known-answer vectors, rejection boundaries, source failures, reentry, reservoir exhaustion and failed-action atomicity. Deterministic entropy and test hooks are compiled only into the temporary test program.

The [current RNG validation summary](data/rng-validation.json) records the first full NIST study and separates host results, native startup checks and unmeasured physical entropy quality.

## Check balance and outcome-history independence

Run `python3 tools/test_outcome_integrity.py --sanitize --out /tmp/phoenix-outcome-integrity` with a new output directory. The test uses two independent instances of the real Engine and Mbed TLS service with matching test-only entropy. It varies funded balances and past win/loss totals while keeping wagers, meters and owed features equal. It compares canonical financial state, committed presentation results and entropy requests across all 77 standard/high-limit menu wagers, six Grand selections, both prize-cap profiles and Base/Free/Hold modes.

Additional cases continue complete rounds and queued features, check accounting-capacity refusals, and confirm that changing legitimate meter/feature state can change results. These are finite software regressions, not measurements of casino odds, physical entropy or regulatory approval.

## Release documentation and media check

Run `python3 tools/verify_release.py` before publishing. CI runs the same check. It rejects mismatched release/data/demo versions, changed or stale gallery source pins, missing or altered images, unreadably small image files, broken local links and private development paths. The gallery and showcase media are catalogued with current source versions and SHA-256 hashes.

This catches synchronization errors; it does not establish original-machine fidelity or prove natural-play provenance by itself. Review the capture records, visible reel continuity, animation timing and guide wording as well.
