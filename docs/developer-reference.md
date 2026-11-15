# Developer reference

The current source is release 0.2.104, model I (`0x00020012`). Code/asset identity and ideal-reference qualification are bound by the included headers; original casino probabilities remain unknown.

## Runtime map

| Module | Responsibility |
|---|---|
| [phoenix_engine.h](../firmware/src/phoenix_engine.h) | Integer money, prices, generated results, features/meters and atomic scratch/commit |
| [phoenix_random.h](../firmware/src/phoenix_random.h), [phoenix_random.cpp](../firmware/src/phoenix_random.cpp) | Opaque noncopyable AES-256 CTR_DRBG service, action-local output and finite failure |
| [crypto config](../firmware/src/phoenix_crypto124/config.h), [license](../firmware/src/phoenix_crypto124/LICENSE) | Private maintained MbedTLS subset; isolated headers, symbols and configuration |
| [platform_entropy125.h](../firmware/src/platform_entropy125.h) | Guarded platform entropy admission |
| [ui.cpp](../firmware/src/ui.cpp), [ui.h](../firmware/src/ui.h) | Display phases, controls, menus and source-error page |
| [main.cpp](../firmware/src/main.cpp) | Hardware setup, fresh session, controls, update loop and USB service |
| [static_grand.h](../firmware/src/static_grand.h), [static_grand_table.h](../firmware/src/static_grand_table.h) | Six future-Grand options and ideal-model reference metadata |
| [math_runtime_identity.h](../firmware/src/math_runtime_identity.h) | Current accepted source/family identity |
| [generated_assets.h](../firmware/src/generated_assets.h) | Common image API and frame selection |
| [phoenix_art.cpp](../firmware/src/phoenix_art.cpp) | Runtime Phoenix atlas validation and access |
| [audio.cpp](../firmware/src/audio.cpp), [audio_music.cpp](../firmware/src/audio_music.cpp) | Cue lanes, speaker submission and music worker |
| [platformio.ini](../firmware/platformio.ini) | Target/toolchain configuration |

## Financial state and presentation

The Engine modes are Base, Free and Hold. A successful action computes its board, award and next mode before the UI animates it. Presentation phases and count-ups are views of that one result, not independent financial games. Use the recorded presentation context when routing a feature's sound or graphics.

Money uses integer minor units; credit prices are converted by denomination. A visual or audio change must not alter financial results. Cells are column-major: `column * 3 + row`, with 15-position masks. Selected paylines are explicit row paths across five columns.

## Random-service ownership

The service must outlive its Engine handles and is confined to the game thread. Copying an Engine copies a handle, never a key/context/reservoir. Every transaction gets a fresh prediction-resistant request; unused words are discarded. Renderer/audio work and synthetic gallery fixtures do not draw from that service.

There is no production seed/replay interface or saved-game restore. Compile-test-only entropy/vector fixtures are separate from production callbacks. Backend/source errors, finite mapping exhaustion and allocation failure stop play without committing scratch money/state. Fresh boot/reload gives 1,000 default denomination credits. See [session rules](saving.md) and [RNG scope](math-and-evidence.md).

## Timing and resources

Use wrap-safe elapsed-time comparisons. Game phases, cue submission, audible onset and settled visual frames are distinct events. The RGB565 framebuffer is 64,800 bytes; large raster temporaries must not casually enter the loop stack. Heap scratch avoids copying a complete Engine onto that stack. Actual target stack high-water and physical timing need their own measurements.

Immutable audio submitted to a worker stays valid until consumption. Finite completion tails and priority rules belong to the audio backend; do not restart them each frame. `collection_steps101.h` controls visual named-prize pickups; settlement remains in the Engine.

## Public tooling

The public [table exporter](../tools/export_wiki_tables.cpp) documents the actual evaluator. Regenerate data from the current source and check its metadata. Status/frame diagnostics and scripted synthetic fixtures have different side effects; a USB timeout alone does not show an application freeze.

See [testing](testing.md), [build](build-and-flash.md), [sources](sources.md) and [attribution](attribution.md).
