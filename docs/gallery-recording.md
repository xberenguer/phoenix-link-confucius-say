# How the gallery is recorded

The [gameplay gallery](graphics.md) shows the playable Confucius Say recreation actually running. Every reel result comes from the current game's normal random source. The [symbol gallery](symbol-gallery.md) uses close-ups cut from those recordings.

## What you are looking at

- Full-screen pictures and clips show the game's 240 × 135 display enlarged by whole pixels.
- Symbol close-ups retain the pixels and background of their original reel cell. Each has a link back to its gameplay context.
- Clips are silent. Their frame timing follows the game's presentation clock; looping restarts the recorded excerpt.
- Menu pictures come from navigating the actual menus. A jackpot information screen shows the available prizes; it is separate from a recorded jackpot award.
- The gallery selects interesting moments from a longer session. It does not show how often those results occur.

The [capture index](gameplay-media.json) lists the source version, captured events, image hashes, frame times, crop rectangles and coverage. A feature without a recording is identified as not yet captured.

## Recording method

The native recorder compiles the repository's unchanged C++ game, renderer and production CSPRNG with a desktop display adapter. Fresh operating-system entropy feeds the production random service. The recorder navigates through public UI actions, funds play credits, and continues playing until events occur. It reads the display framebuffer and read-only game diagnostics.

The desktop adapter advances a monotonic presentation clock without waiting in real time. Captured sequences are sampled every 40 milliseconds and exported at 25 frames per second. This makes long capture sessions practical while preserving the elapsed time between their recorded frames. The adapter has silent audio output, so these clips do not establish audio or physical-device performance.

The recorder uses no chosen boards, fixture scenes, test random source, saved-state injection, altered prize probabilities or forced wins. It does not edit production source files. Source and runtime-asset hashes bind the capture to the published game.

## Reproduce a new session

On macOS with a C++17 compiler and Python installed, run from the repository root:

```sh
python3 tools/capture_gameplay.py --out ../recorded-session --actions 10000
```

Each run is a fresh session with fresh randomness. It will produce different results and may need more play to encounter a rare feature.

The output contains the input journal, frame journals, native RGB565 recordings and source-binding receipt. Convert selected recordings with the [gallery converter](../tools/build_gameplay_gallery.py); its Python environment needs Pillow. Keep the raw recordings while reviewing crops and sequences.

Before publishing, run:

```sh
python3 tools/verify_gameplay_gallery.py
python3 tools/verify_release.py
```

These checks validate the media, journal links and source identities. Review the recordings themselves as well. The [testing guide](testing.md) explains the scope of software, browser and physical-device checks.
