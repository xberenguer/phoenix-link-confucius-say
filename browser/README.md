# Browser build

This port compiles the exact engineering-accepted 0.2.106 C++ Engine, renderer, audio decoder and music worker to WebAssembly. JavaScript supplies browser display, buttons, WebAudio and WebCrypto. It does not choose outcomes or payouts.

The game is session only: each reload starts with 1,000 denomination credits. There is no IndexedDB bank, old save import, bank export or saved RNG state. WebCrypto must be available in a secure context; an entropy failure stops play. Offline use requires the page and code/assets to have been cached previously in a secure browser context.

Use the pinned official Emscripten **7.0.0** SDK and Python 3:

```sh
python3 browser/build.py --source firmware/src --assets firmware/data --out demo --empp /path/to/emsdk/release-7.0.0/install/emscripten/em++.py
python3 browser/verify_bundle.py --source firmware/src --assets firmware/data --bundle demo
python3 -m http.server 8777
```

Open `http://localhost:8777/demo/` and press **Play with sound**. Source the SDK environment first, or supply its `EM_CONFIG` path. The build refuses any source or runtime asset differing from `SOURCE_BINDING.json`; there is no unaccepted-source override. A new release requires an explicitly reviewed new binding and fresh build.

Blue/Space spins or selects; hold to toggle autoplay. Grey/Enter/Right opens SELECT BET or advances a menu; hold for settings, double press in the game to flip. Escape opens settings. Hidden tabs pause gameplay. Browser timing and output differ from physical hardware.

Engineering source acceptance is not regulatory approval. The local estimated game model does not reproduce original casino PAR probabilities, and actual-CSPRNG infinite-horizon RTP remains unqualified. Maintained Mbed TLS 3.6.7 supplies AES-256 CTR_DRBG; a fresh WebCrypto entropy request is required for each financial action.

`PROVENANCE.json` binds every firmware file, both runtime assets and the adapter. `BUNDLE_MANIFEST.json` binds the generated bundle files. Run `verify_bundle.py` before Pages uploads; changing source, assets, adapter or checked-in WASM without rebuilding makes that gate fail.
