# Saving, continuity and recovery

The current game saves financial outcomes through an acknowledged storage owner. Native Main owns the Stick's canonical checkpoint; the browser uses an IndexedDB owner around the same game/scene/preference encoding. Outcome saving, ordinary settings saves and physical-device validation have distinct scopes.

## What the game record contains

The canonical checkpoint binds the financial Engine body to its scene and math preference. It retains the bank, wager, statistics, all meter buckets, hidden active thresholds, owed Free/Hold work, locked Phoenix prizes, pending meter features and deterministic RNG continuation. The scene preserves relevant held-board and Phoenix-animation context.

A valid saved game restores its PCG state and stream increment; fresh-start seed material does not replace that continuation. Device options such as volume, sound switches, rotation, brightness, cash/credit display and refill preference are persisted separately. Autoplay is restored **off**.

Code: [Engine save/restore](../firmware/src/phoenix_engine.h), [canonical bundle](../firmware/src/checkpoint_bundle.h), [scene binding](../firmware/src/scene_checkpoint.h), [Main storage owner](../firmware/src/main.cpp).

## Credits and meters

Add-credit and auto-refill use funding operations rather than resetting the Engine. They preserve progressive buckets and RNG state. The player menu offers additive $1,000, $10,000 and $100,000 choices; successful additions return to manual gameplay. Selecting a different wager changes which stored meter bucket is visible; returning to that wager selects its bucket again.

Stored prizes and committed awards remain owed until the round is safely complete. A future payout setting must not rewrite a locked Phoenix or count an existing payment twice.

<a id="older-saved-math-contexts"></a>

## Compatible saved math contexts

The 48-byte math preference supports recognized selector and neutral-slot records. A recognized saved context preserves its raw preference, stored prices and owed Free/Hold/chained work through cold restores. Adoption of the current future-play schedule waits for a qualified closed Ready/Base boundary.

An unknown valid context is not silently repriced or replaced with a fresh bank. It can remain exportable while paid Base is blocked. Compatibility with a stored obligation does not imply support for every externally modified record or future configuration.

<a id="outcome-ownership-from-version-93-onward"></a>

## Outcome ownership

Before a paid, Free or Hold result can be presented, the UI asks the actual storage owner to commit its exact canonical record. The record binds the prepared outcome, owed state, RNG continuation, scene and math context. Quick-stop and autoplay use that same boundary. Missing ownership prevents a new draw.

Native Main writes, commits and reads back the record before acknowledging it. The browser waits for its IndexedDB transaction and exact readback. A failed or ambiguous operation retains one immutable pending record behind the storage page. Retry uses those same bytes rather than generating another outcome. Conflicting stored material is preserved for recovery instead of being replaced automatically.

Presentation time pauses across storage latency, so a slow write does not consume the reel lead-in or silently skip an animation segment. The storage retry clock remains independent of presentation time.

<a id="version-92s-save-timing-limit"></a>

## Funding and settings acknowledgements

The outcome barrier does not make every menu change synchronous. Funding changes and some non-outcome options use deferred persistence; Main's ordinary change delay is one second. A newly displayed credit addition alone is not an immediate-durability acknowledgement. Confirm a committed checkpoint before relying on a just-changed bank surviving sudden power loss.

Simulator Return uses the acknowledged canonical transaction. A changed choice and any earlier pending funding are saved together. A fully identical financial/raw-preference/service tuple is freshly read and verified without an extra generation. Ambiguous failures retain the prepared record for retry, and recovery requires input release before another action. Other deferred settings keep their own save behavior.

The actual Main/UI callback is checked with simulated write, commit, readback and cold-start failures. Native reboot preservation and firmware readback are separate checks. Host tests and a successful flash do not establish hardware certification, exhaustive physical power-cut coverage or flash endurance.

## Recovery states

| Situation | Intended response |
|---|---|
| Proven absent game record | Initialize a fresh play-money game through the storage owner |
| Valid current record | Restore its bank, owed work and scene binding |
| Recognized compatible record | Preserve owed state and adopt future settings only at the qualified boundary |
| Unreadable/uncertain storage | Pause while recovery/read/retry establishes the record |
| Conflicting or unsupported financial context | Service/export path; no automatic reset |
| Capacity overflow risk | Reject the action without consuming a game draw; preserve the existing obligation |

An animation replay, self-test pass or firmware readback alone does not prove that a fresh bank survived a physical reboot. Native saved-data validation is a separate check.

## Updating

An app-only update at the correct application offset leaves NVS and the compatible asset filesystem outside the written range. That differs from erasing flash or restoring a full device backup. A layout-changing update needs an explicit migration plan.

Use [build and flash](build-and-flash.md) for image roles. Keep a verified record before deliberate maintenance. Investigate storage service messages with read-only diagnostics; do not use an erase as the default response to an unsupported record.
