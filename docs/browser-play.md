# Playing in a browser

This package includes the version **0.2.102** browser game in `demo/` and its portable build adapter in `browser/`. [Play the live browser game](https://xberenguer.github.io/phoenix-link-confucius-say/). Its version 102 runtime, all ten hosted files, one paid action and saved-game restoration were checked after deployment. You can also serve `demo/` locally with `python3 -m http.server --directory demo 8080`.

The browser build compiles the same C++ Engine, UI renderer and audio programs to WebAssembly. It displays the firmware's 240 × 135 game, scaled to the available window. The surrounding buttons represent the Stick's controls. It needs no connected Stick, account or game server.

## Controls

Blue spins or selects a menu choice; holding blue toggles autoplay. Grey opens wager selection from the game and moves focus in menus. Holding grey opens settings, or returns to the game when wager selection is open. Double-grey flips the screen in normal play. The port uses the firmware's button helper and timing constants; see [controls](controls.md).

A browser may require a click or tap before it allows sound. The first game interaction supplies that gesture. Sound, reel and landing preferences remain available in the regular Sound menu.

## Saved progress

Browser progress lives in that browser's IndexedDB storage and is separate from a Stick's bank. The browser adapter uses the actual canonical game/scene/preference encoder. Its outcome owner waits for an IndexedDB transaction to complete and reads the bytes back before allowing the outcome animation to proceed. A failed write must enter recovery rather than acknowledge a volatile result as saved.

Reload starts with autoplay off. Clearing site data, changing the page path or using a different browser profile changes which saved bank is available. The download control exports the last acknowledged canonical bank. Browser storage can be removed by the user or browser; it is not a hardware NVS backup.

After the first complete online load, the static bundle can reload offline from its service-worker cache. The current bundle passed a cold page/Wasm reload with Chromium network disabled under a repository subpath. A first visit still needs the game files to be downloaded.

## Current evidence and remaining checks

Real-browser checks cover initial boot, native grey-button gestures, autoplay start/stop, reload and a complete selected Hold. A selected six-Free session, including nested Holds, completed across a cold restore with autoplay off. The selected feature cases are private tests, not frequency measurements or forced outcomes included in the public game.

Storage checks include a delayed acknowledgement with repeated-click fencing, aborted initial/existing-bank transactions with exact pending-record retry, retention of empty/invalid stored records and refusal of stale-tab overwrites. A separate Root review exercised ordinary unseeded play and menu controls in a fresh Chrome context, then checked bank, meter, wager and action continuity after reload.

Chromium touch/DPR2 and portrait/landscape layouts pass, including offline cold loading under a repository subpath. Physical phones, Safari and Firefox remain unverified. Hidden tabs pause the browser presentation clock and audio; that is an explicit browser adaptation.

The audio adapter follows the pinned Stick speaker driver's digital volume law and uses the same PCM/music banks. Chrome received a complete test cue and the first eight live Twin worker chunks sample for sample from the C++ bank, with nonzero unclipped graph output. This demonstrates transport of the simulator's bank, not recovery of original machine audio or identical acoustic playback from different speakers. Browser resampling, mixing and scheduling can differ from the Stick.

## Hosting

GitHub Pages serves the static HTML, JavaScript, WebAssembly and asset files. The repository README can show an image or animation linked to that page; its markup renderer removes scripts, so the interactive game belongs on the Pages site. [GitHub Pages documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/what-is-github-pages), [GitHub markup rendering](https://github.com/github/markup).

The current bundle passed 439 Chromium assertions and 12 native canonical/paid-record pair checks. Coverage includes recognized compatible and current-model records, two scoped paid-price fixtures, durable retry, process restart, repository-path offline loading, saved-game download, and zero/high-word/maximum 64-bit WebCrypto seeds with exact cold restore. These are simulator transport and persistence checks, not casino-probability measurements. Native Main/NVS is not compiled into the browser; its IndexedDB owner is validated separately.

The build toolchain, private saved banks, raw research footage and internal test artifacts are excluded from the public payload. Local source and browser checks are separate from hosted verification. The release commit passed the [fresh GitHub firmware build and payout-table checks](https://github.com/xberenguer/phoenix-link-confucius-say/actions/runs/43743990417) and [Pages deployment](https://github.com/xberenguer/phoenix-link-confucius-say/actions/runs/43744089473); the deployed game then passed 17 Chromium smoke checks.

The browser is built from the accepted current C++ source. Its checks do not certify physical phones, speakers or native NVS. WebCrypto initializes fresh play; a valid saved game restores its canonical PCG continuation instead of receiving a replacement seed.
