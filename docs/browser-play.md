# Playing in a browser

This package includes the version **0.2.102** browser game in `demo/` and its portable build adapter in `browser/`. The intended [playable Pages URL](https://xberenguer.github.io/phoenix-link-confucius-say/) is pending live deployment verification; deployment status also appears in the repository's Pages environment. You can also serve `demo/` locally with `python3 -m http.server --directory demo 8080`.

The browser build compiles the same C++ Engine, UI renderer and audio programs to WebAssembly. It displays the firmware's 240 × 135 game, scaled to the available window. The surrounding buttons represent the Stick's controls. It needs no connected Stick, account or game server.

## Controls

Blue spins or selects a menu choice; holding blue toggles autoplay. Grey opens wager selection from the game and moves focus in menus. Holding grey opens settings, or returns to the game when wager selection is open. Double-grey flips the screen in normal play. The port uses the firmware's button helper and timing constants; see [controls](controls.md).

A browser may require a click or tap before it allows sound. The first game interaction supplies that gesture. Sound, reel and landing preferences remain available in the regular Sound menu.

## Saved progress

Browser progress lives in that browser's IndexedDB storage and is separate from a Stick's bank. The browser adapter uses the actual canonical game/scene/preference encoder. Its outcome owner waits for an IndexedDB transaction to complete and reads the bytes back before allowing the outcome animation to proceed. A failed write must enter recovery rather than acknowledge a volatile result as saved.

Reload starts with autoplay off. Clearing site data, changing the page path or using a different browser profile changes which saved bank is available. The download control exports the last acknowledged canonical bank. Browser storage can be removed by the user or browser; it is not a hardware NVS backup.

After the first complete online load, the static bundle can reload offline from its service-worker cache. The actual version 100 bundle passed a cold page/Wasm reload with Chromium network disabled under a repository subpath. A first visit still needs the game files to be downloaded.

## Current evidence and remaining checks

An earlier version 96 port passed 12,000 deterministic Engine actions byte for byte against native C++, including Base, Free and Hold actions. That checks the port against the local simulator, not against original casino probabilities.

Real-browser checks cover initial boot, native grey-button gestures, autoplay start/stop, reload and a complete selected Hold. A selected six-Free session, including nested Holds, completed across a cold restore with autoplay off. The selected feature cases are private tests, not frequency measurements or forced outcomes included in the public game.

Storage checks include a delayed acknowledgement with repeated-click fencing, aborted initial/existing-bank transactions with exact pending-record retry, retention of empty/invalid stored records and refusal of stale-tab overwrites. A separate Root review exercised ordinary unseeded play and menu controls in a fresh Chrome context, then checked bank, meter, wager and action continuity after reload.

Chromium touch/DPR2 and portrait/landscape layouts pass, including offline cold loading under a repository subpath. Physical phones, Safari and Firefox remain unverified. Hidden tabs pause the browser presentation clock and audio; that is an explicit browser adaptation.

The audio adapter follows the pinned Stick speaker driver's digital volume law and uses the same PCM/music banks. Chrome received a complete test cue and the first eight live Twin worker chunks sample for sample from the C++ bank, with nonzero unclipped graph output. This demonstrates transport of the simulator's bank, not recovery of original machine audio or identical acoustic playback from different speakers. Browser resampling, mixing and scheduling can differ from the Stick.

## Hosting

GitHub Pages serves the static HTML, JavaScript, WebAssembly and asset files. The repository README can show an image or animation linked to that page; its markup renderer removes scripts, so the interactive game belongs on the Pages site. [GitHub Pages documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/what-is-github-pages), [GitHub markup rendering](https://github.com/github/markup).

Historical version 100 qualification passed 421 Chromium assertions and 12 fresh native record-pair checks, including oldG/currentH records, both corrected paid outcomes, durable retry, process restart, repository-path offline loading and exact saved-game download. Native Main/NVS is not compiled into the browser; the canonical IndexedDB owner is validated separately.

The build toolchain, private saved banks, raw research footage and internal test artifacts are excluded from the public payload. The local source build and browser tests are not a claim that the GitHub Actions workflows or hosted site have already run.

Version 102 is rebuilt from the accepted102 C++ source, not a relabeled100 Wasm bundle. The actual102 bundle passed the six existing Chromium suites (421 assertions), 12 current-header native canonical/paid-record pairs, and 18 additional checks for zero, high-word and maximum64-bit WebCrypto seeds plus exact cold restore. These are isolated browser/source checks, not physical phone, speaker or native NVS certification. WebCrypto seed material and restored canonical PCG continuation are different operations; saved progress is not replaced by a fresh seed.
