# Playing in a browser

[Play the browser game](https://xberenguer.github.io/phoenix-link-confucius-say/). The current source release is **0.2.106**, with the build adapter in `browser/` and the hosted bundle in `demo/`. You can also run `python3 -m http.server --directory demo 8080` from the repository root and open the local page.

The browser compiles the C++ Engine, UI and audio programs to WebAssembly. Its buttons represent the Stick controls around the 240 × 135 game. No connected Stick, account or game server is needed.

## Controls

Blue spins/selects; holding blue toggles autoplay. Grey opens the wager panel or moves menu focus. Holding grey returns from the wager panel to the game, and opens settings from gameplay. Double-grey flips the screen. See [controls](controls.md).

A click or tap may be required before browser sound can start. The Sound menu controls master sound, stops/landings and volume. Hiding a tab pauses presentation/audio; returning to that still-open page continues its live session.

## Fresh sessions

Each reload, new page instance or new build starts manual play with **1,000 credits at the default $1 denomination ($1,000)**. Progress is held in that live page only. There is no saved-game upload, download or restore and no shared bank with a Stick or another tab.

Static asset caching is separate from game state. Once the complete bundle is cached, its files can be available offline; reloading those files still creates a new game. The first load needs the bundle to be available. Online updates install a complete new bundle, then refresh older game tabs. Offline play retains the last complete cached release.

## Randomness and errors

WebCrypto supplies fresh entropy to the same AES-256 CTR_DRBG service used by native code. If the API is missing or fails, play stops without a new committed outcome. No clock or weak random fallback is used. A reload starts a new session rather than recovering the interrupted round. See [fresh sessions](saving.md) and [math](math-and-evidence.md).

## Audio and platform scope

WebAudio submits the C++ cue/music banks through the browser adapter. Browser mixing, output latency, resampling, speakers and scheduling can differ from the Stick. This is not a claim of recovered manufacturer audio or identical physical acoustics.

Current checks cover real WebCrypto startup, controls, fresh reload and random-source failure behavior. Host/Wasm provider and Engine fixtures agree, and the target build passes; physical hardware claims are listed separately in [current release notes](../CHANGELOG.md#validation-status). Browsers/devices outside the exercised tests remain unverified.

## Hosting and building

GitHub Pages serves the static HTML, JavaScript, Wasm and assets. The README's preview links to this page because GitHub Markdown does not run the game scripts. See [GitHub Pages](https://docs.github.com/en/pages/getting-started-with-github-pages/what-is-github-pages) and the [browser build README](../browser/README.md).

The bundle's `PROVENANCE.json` records its code/asset identity. Match it to the current release rather than inferring a version from an older browser cache. Build validation and hosted deployment are separate steps.
