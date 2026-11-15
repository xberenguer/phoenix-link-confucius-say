# Developer reference

The public tree contains the offline embedded game, required runtime assets, browser adapter and gameplay reference. These notes describe version 102. Source identity covers 133 runtime files; generated schedule/identity headers are checked separately to avoid a hash cycle. It is an independent reconstruction, not a manufacturer machine emulator or recovered casino program.

## Runtime map

| File or module | Responsibility |
|---|---|
| [phoenix_engine.h](../firmware/src/phoenix_engine.h) | Integer money, PCG state, symbol generation, pay evaluation, meter buckets, features, serializable financial state |
| [ui.cpp](../firmware/src/ui.cpp), [ui.h](../firmware/src/ui.h) | Presentation phases, display-only counters, inputs, screen composition, menus and diagnostics |
| [main.cpp](../firmware/src/main.cpp) | Hardware setup, button dispatch, storage ownership, update loop and USB service |
| [checkpoint_bundle.h](../firmware/src/checkpoint_bundle.h), [checkpoint_store.h](../firmware/src/checkpoint_store.h) | Canonical record structure and exact storage/readback operations |
| [scene_checkpoint.h](../firmware/src/scene_checkpoint.h) | Scene-to-bank binding and visual continuation context |
| [static_grand.h](../firmware/src/static_grand.h), [static_grand_table.h](../firmware/src/static_grand_table.h) | Neutral schedule selection and reviewed local metadata |
| [capacity_service.h](../firmware/src/capacity_service.h) | Preserve and report an obligation that cannot safely advance within numeric capacity |
| [generated_assets.h](../firmware/src/generated_assets.h) | Common image API and symbol-frame selection |
| [phoenix_art.cpp](../firmware/src/phoenix_art.cpp), [phoenix_asset.h](../firmware/src/phoenix_asset.h) | Runtime Phoenix atlas, verification and frame access |
| [audio.cpp](../firmware/src/audio.cpp), [audio_music.cpp](../firmware/src/audio_music.cpp) | Cue lanes, speaker submission, finite tails and streamed music worker |
| [audio_cues.cpp](../firmware/src/audio_cues.cpp), [twin_music42.cpp](../firmware/src/twin_music42.cpp) | Immutable cue/Twin data and registration |
| [platformio.ini](../firmware/platformio.ini), [partition map](../firmware/partitions-offline.csv) | Pinned build and fixed flash layout |

## Financial state versus presentation

The Engine has three modes: Base, Free and Hold. It commits a result, its award and the next owed state before the UI animates it. `Presentation` records the result needed for drawing: spin/held boards, masks, line wins, meter arrivals, feature transitions and award totals.

UI phases include Ready, Spin, Land, Expand, Revive, FreeIntro/FreeCounter, SpinAward, HoldWipe/HoldIntro, Jackpot, Collect, WinTransfer, ReturnWipe and Outro/AwardIntro. They are not additional independent financial games. The next Engine mode can already be Base while the final Free result is still on screen; audio and graphics must use presentation context where appropriate.

Money is stored as integer minor currency units, with integer credit prices converted using the denomination. Floating-point geometry must not become the money authority. Changing a font, particle, cue or count-up must leave Engine serialization and PCG continuation unchanged.

## Data layout and timing

Cells are column-major: `cellIndex(column,row) = column × 3 + row`, with zero-based rows. Masks use those fifteen positions. Selected paylines are row paths across five columns; the chart changes with the line-count profile. Do not substitute “243 ways” for the implemented lines.

Time comparisons use unsigned elapsed differences so wrapping millisecond clocks can be handled. Storage-pause presentation time is distinct from wall time. A UI action, queued sound, audible speaker onset and final settled visual frame are separate event boundaries.

The 240 × 135 RGB565 framebuffer occupies 64,800 bytes before other storage. PSRAM caches accelerate effects and immutable decoded artwork; fallbacks and cache parity need testing. Native stack usage matters: large temporary raster frames should not be placed casually on the loop task stack.

## Asset and audio changes

Keep source-derived event rules next to their scope. An observed cash caption can correct an exact price without proving a whole paytable or changing the reel distribution. A new cue family should not be applied to unobserved Twin/Hold/coalesced cases merely because it sounds plausible.

Static audio submitted to the speaker must remain immutable until its borrowed buffer finishes. The direct Twin PCM loop uses bounded chunks and queue depth rather than allocating a second mutable copy. Finite Major/Big tails have priority rules; repeated UI scope assertions must not restart them on every frame.

Never modify the outcome RNG from render or audio work. Demo screenshots should use a deterministic synthetic state, not a player's exported bank.

## Diagnostics and public tooling

The firmware exposes read-only status/metadata and explicit developer test APIs. Commands that pause, audition or replace a fixture have side effects and must be kept distinct from ordinary read-only status. A USB timeout alone does not prove the game is frozen.

The game-only public package does not include the private NVS exports, device receipts, video archive, desktop artifacts or full private validation harnesses. The public [table exporter](../tools/export_wiki_tables.cpp) regenerates the documented evaluator tables; its metadata must match the actual release source.

The Main-owned outcome checkpoint barrier introduced in 93 is present in100. Prepared outcomes wait for an exact canonical write, commit and readback acknowledgement before presentation continues. Standalone rendering fixtures explicitly use volatile test ownership; storage tests retain the real Main callback. Browser persistence has its own IndexedDB owner around the same financial/scene encoding.

See [testing](testing.md), [math](math-and-evidence.md), [saving](saving.md) and [build](build-and-flash.md) before changing those boundaries.

Version 101 adds `collection_steps101.h` for visual Mini/Minor unit pickups; settlement remains in the Engine. Version 102 gives `uiInit` a full-width exact seed API. Native Main captures eight offline entropy bytes before application peripherals and passes the exact value; browser initialization gets its own seed through WebCrypto. Restored PCG state wins over provisional fresh initialization.
