# Testing and evidence

Correct math, faithful presentation and reliable physical operation need different evidence. This guide describes the checks used during development and the public reproduction paths; it does not claim certification or ship the private research harnesses as game data.

## What each kind of check proves

| Check | Useful conclusion | Practical limit |
|---|---|---|
| C++ evaluator export | Exact implemented price/path/deck rows | Does not recover original casino probabilities |
| Fixed-seed Engine replay | Same inputs produce the same saved result/PCG | Predictable replay is not cryptographic security |
| Engine/UI joined action | Real result reaches the expected presentation without extra draws | Does not prove durable NVS unless the real storage owner participates |
| Main/UI with simulated NVS failures | Retry, commit/readback and restore logic follow the exercised paths | Not physical flash latency, power-cut or endurance measurement |
| ASAN/UBSAN runtime checks | Exercised code paths avoid detected memory/undefined-behavior errors | Not proof that every possible path is safe |
| Exact raster comparison | An optimized path matches its reference pixels | The reference can still differ from the original machine |
| Source frame/event comparison | A stated observation matches a filmed configuration | Does not establish universal rules or unseen frames |
| Native build and readback | Target image builds/fits and the written bytes match | Not proof of audible output or restored saved data |
| Native frame and control test | Actual hardware renders/responds | A single still does not verify every animation or acoustic event |

## Reproduce the public tables

Use the command in [payouts](payouts.md#regenerating-the-reference). The exporter includes zero-pay entries, both Base/Free modes and internal compatibility profiles. Check its metadata and hashes against the source you actually built.

Do not relabel an old CSV as the current version. A table generated from the current code is an implementation reference, not a PAR sheet.

## Deterministic visual fixtures

The [gallery](graphics.md) is rendered from a fixed version and explicit synthetic fixtures. Its [manifest](gallery-manifest.json) distinguishes scripted display examples from naturally generated spins. Render-time snapshots are checked to remain financially unchanged. These images are suitable for UI documentation; they are not observations of a casino's event frequency.

For future comparisons, preserve native 240 × 135 frames and actual timestamps before resizing. Inspect crop edges, transparent alpha, row margins, title/number alignment, Gold/Sage seams, cell crossing, final stop identity, fire coverage and particle layers. A GIF's frame cadence is only a preview, not proof of physical refresh timing.

## Regression groups worth retaining

- Every symbol, win-animation state and partial reel-strip boundary.
- Manual, autoplay, quick-stop and delayed-frame presentation of the same financial result.
- Initial/retrigger/final Free state, both Twin pairs and nested Hold return.
- Natural/meter/chained Hold, zero-new revival, named awards and full-board settlement.
- Same-wager meter continuity across funding, wager switches, save and restart.
- Old owed contexts, unknown preferences, storage failures, numeric capacity and wraparound clocks.
- Audio start/stop/mute, independent reel switches, finite tails and missing assets.

## Checkpoint barrier testing

Version 102 retains the synchronous Main-owned outcome barrier introduced in 93. Standalone UI tests explicitly register a volatile fixture owner so their presentation can run; that acknowledgement is not durable storage. Joined tests retain Main's actual callback and simulate write/commit/readback failures, old/new durable cuts and cold restarts.

That separation keeps renderer checks distinct from persistence evidence. Actual accepted100 Main/UI qualification exercises 130 processes and 5,097 assertions with hardware/storage mocks. The full renderer, transport, credit/audio and target builds are checked separately; physical power-cut/endurance and acoustic identity remain unproven.

## A short hardware acceptance pass

After a deliberate update, check version, startup autoplay off, controls, one ordinary spin, one sound audition and a saved-state comparison. Exercise feature fixtures only through a test path that preserves/restores the player's state or clearly uses a separate synthetic bank.

Readback, native frames, audio auditions and saved-data continuity should be recorded separately. No test in this wiki makes the reconstruction a licensed or casino-calibrated machine.

## Version 102 qualification

The accepted source was previously tested with 192 focused startup/restore cases, 130 actual Main cases / 5097 assertions, the UI/USB/credit-audio/target suites, and a 1015-frame native rendering benchmark. Those hardware/source results are reused only after byte-for-byte source binding; public staging does not repeat a physical flash. The added 80 repeated-flight samples are offscreen raster plus saved LCD transfer, not full-game FPS.

The browser and synthetic gallery are rebuilt separately for this public102 tree. Their new results are stated in the browser guide and gallery manifest; earlier100 tests remain historical evidence, not automatically new102 tests. Native Main/NVS is not compiled into WebAssembly.
