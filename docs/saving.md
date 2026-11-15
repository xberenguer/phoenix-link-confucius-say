# Saving, continuity and recovery

This page describes the storage ownership introduced by the **0.2.96 source** and retained by version 102. Version 98 also uses the Main-owned barrier for return selections. The outcome barrier shipped in version 93; version 92's older timing limit is retained below as history. The version-102 source and browser are bound to their accepted runtime; source, durable-storage and physical-device checks remain separate qualification scopes.

## What the game record contains

The canonical checkpoint binds the financial Engine body to its scene and math preference. It retains the bank, wager, statistics, all meter buckets, hidden active thresholds, owed Free/Hold work, locked Phoenix prizes, pending meter features and deterministic RNG continuation. The scene preserves relevant held-board and Phoenix-animation context.

Device options such as volume, sound switches, rotation, brightness, cash/credit display and refill preference are persisted separately. Autoplay is intentionally restored **off** so updating or restarting does not silently resume betting.

Code: [Engine save/restore](../firmware/src/phoenix_engine.h), [canonical bundle](../firmware/src/checkpoint_bundle.h), [scene binding](../firmware/src/scene_checkpoint.h), [Main storage owner](../firmware/src/main.cpp).

## Credits and meters

Add-credit and auto-refill use funding operations rather than resetting the whole Engine. They preserve progressive buckets and RNG state. The player menu offers additive $1k, $10k and $100k choices; each returns to manual gameplay. Selecting a different wager changes which bucket is visible; returning selects the stored bucket again.

An owed feature cannot be discarded merely because a new build has different future payout metadata. Its stored prizes and committed awards remain financial liabilities until the round is safely complete.

## Older saved math contexts

Version 92 introduces neutral saved schedule slots while keeping the same 48-byte preference size. Known older contexts can remain exact through owed Free/Hold/chained actions and repeated cold restores. They normalize only at a qualified Ready/Base boundary for future play.

An unknown valid context is not silently repriced or converted into a fresh bank. It can remain exportable while paid Base is blocked. This distinction protects existing obligations without claiming that every historical or externally modified record is a supported future configuration.

The same principle applies to named-prize values: correcting a future Mini/Minor lookup must not rewrite an already locked Phoenix or count an old payment a second time.

## Outcome ownership from version 93 onward

Before a real paid, Free or Hold result can be presented, the UI asks the actual Main storage owner to commit its exact canonical record. The record binds the prepared outcome, owed state, RNG continuation, scene and math context. Quick-stop and autoplay use that same boundary. Missing ownership prevents a new draw.

Main writes, commits and reads back that record before acknowledging it. A failed or ambiguous operation retains one immutable pending record behind the storage page. Retry works on that record rather than generating another outcome. Conflicting stored material is preserved and requires recovery instead of an automatic new bank.

Presentation time pauses across storage latency, so a slow write does not consume the reel lead-in or silently skip an animation segment. The storage retry clock remains independent of presentation time.

This barrier covers game outcomes. Funding changes and some non-outcome options still use deferred persistence; Main's ordinary change delay is one second. A newly displayed credit addition alone is therefore not an immediate-durability acknowledgement. The prepared native credits check waits for the exact canonical bank before reporting preservation, but that menu exercise has not yet run on the device. This remaining distinction should not be hidden behind a general claim that every menu change is synchronous.

Version 98's Simulator Return selection explicitly uses the Main-owned barrier. A changed choice and any earlier pending funding are included in the same canonical transaction. A fully identical owned financial/raw-preference/service tuple is freshly read and verified without an extra generation. Ambiguous failures retain the exact prepared record; recovery requires input release before another action. This menu path does not make the other deferred settings synchronous.

The actual Main/UI callback is exercised with simulated write, commit, readback and cold-start failures. Native reboot preservation and firmware readback are separate release checks. Neither host tests nor a successful flash establishes hardware certification, exhaustive physical power-cut coverage or flash endurance.

## Version 92's save timing limit

Version 92 used revision/action notices and deferred checkpointing, with a one-second change delay and a longer periodic checkpoint condition. It did **not** require each displayed paid result to become durable before exposure. Sudden power loss in an unsaved interval could return to the previous committed record and deterministic RNG position. Version 93 added the outcome ownership barrier described above; this historical limitation must not be described as the current outcome path.

## Recovery states

| Situation | Intended response |
|---|---|
| Proven absent game record | Initialize a fresh play-money game through the storage owner |
| Valid current record | Restore its bank, owed work and scene binding |
| Known older valid record | Preserve owed state and migrate only at the documented safe boundary |
| Unreadable/uncertain storage | Pause while recovery/read/retry can establish the record |
| Conflicting or unsupported financial context | Service/export path; no automatic reset |
| Capacity overflow risk | Reject the action without consuming a game draw; preserve the existing obligation |

An animation replay, self-test pass or firmware readback alone does not prove that a fresh bank survived a physical reboot. Native saved-data validation is a separate release check.

## Updating

An app-only update at the correct application offset leaves NVS and the compatible asset filesystem outside the written range. That is different from erasing flash or restoring a full device backup. A layout-changing update needs an explicit migration plan.

Use [build and flash](build-and-flash.md) for image roles. Keep a verified record before deliberate maintenance. Storage service messages should be investigated with the read-only diagnostics; do not use an erase as the default response to a game that correctly refuses an unknown record.
