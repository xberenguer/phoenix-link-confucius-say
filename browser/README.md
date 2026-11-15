# Browser port

This static demo compiles the actual accepted C++ Engine, UI, fonts, artwork, cue bank and music worker. JavaScript supplies browser display/audio/input/storage I/O; it does not generate slot outcomes or payouts. The browser uses the same local estimated model as the firmware, not casino-calibrated odds.

Players open the built page. No installation, account, device, API key or server-side game is needed. Saved progress stays in this browser's IndexedDB. After the service worker caches a complete bundle, that copy can load offline.

## Build

Use official Emscripten 7.0.0. The pinned SDK manifest is commit 708dcbcc9ee23bc7d2dad44fc90db377e38b771c; the official 7.0.0 release build is 6afbd83493d647051772d491124187d4e9a0cda9. Follow https://emscripten.org/docs/getting_started/downloads.html and activate that version in your build shell.

```sh
python3 browser/build.py --source firmware/src --assets firmware/data --out browser/dist
python3 -m http.server --directory browser/dist 8080
```

Adjust the `browser/` prefix to this folder's location. `--empp` optionally selects an explicit em++/em++.py. The builder requires genuine accepted source metadata and retains the source version; it provides no trial-proof override. Only the two verified runtime files `feature_music.mp3` and `phoenix_atlas.zlib` are packaged. No research media, user pictures or personal checkpoint is needed.

The output directory is directly suitable for static GitHub Pages hosting, including a repository subpath. Publication belongs to the release owner; this folder does not publish or change repository/hosting settings. This package contains the actual version 102 source and matching browser bundle; see `demo/PROVENANCE.json` for its identity.

## Platform contract

The C++ financial outcome handler waits for a real IndexedDB write transaction and byte-for-byte readback before it returns durable success. Failed attempts retain immutable bytes for retry. A transactional compare-and-swap prevents a stale tab from silently replacing a newer saved game. Existing invalid or unsupported banks are retained rather than replaced with a fresh one. Browser eviction/private-storage policy can still remove site data; downloading a saved game provides a local copy.

The C++ music worker runs cooperatively through an Emscripten fiber. WebAudio owns a PCM copy for each accepted queue entry; the native C++ queue/channel/priority logic stays intact. Output mirrors the pinned M5 S16 digital gain law. Browser resampling, output latency, physical speakers and OS scheduling differ from the Stick. The game clock/audio context pause when the page is hidden. No pthread/cross-origin-isolation header is required.

Blue short selects/spins, hold toggles autoplay. Grey short opens the native wager panel in the game or advances a menu. Grey hold follows the native UI back/settings behavior. Double-grey uses the actual C++ GameBlackTap helper; the hold threshold is extracted from native Main. The page adds keyboard/touch delivery around those controls.

`PROVENANCE.json` binds the firmware source and packaged assets. It does not certify casino behavior or imply identical browser/device audio or every rendered frame.

For this release, the native source version is 0.2.102 (`build_version::firmware` adds the device-status `-candidate` suffix). The browser reports 0.2.102 and the same accepted runtime fingerprint. It uses the exact C++ UI seed API and WebCrypto; it does not run native Main or claim identical hardware entropy.
