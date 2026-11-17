"""Build the source-bound, engineering-accepted C++ firmware browser port."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, time

D = Path(__file__).resolve().parent
H = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
canonical = lambda value: json.dumps(value, sort_keys=True, separators=(',', ':')).encode()
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, required=True, help='Exact accepted firmware/src directory')
parser.add_argument('--assets', type=Path, required=True, help='Exact runtime firmware/data directory')
parser.add_argument('--out', type=Path, default=D / 'dist')
parser.add_argument('--empp', default=shutil.which('em++'), help='Pinned Emscripten 7.0.0 em++ or em++.py')
args = parser.parse_args()
assert args.empp, 'Emscripten 7.0.0 is required; see README.md'
binding = json.loads((D / 'SOURCE_BINDING.json').read_text())
assert binding['engineeringAccepted'] is True
original = args.source.resolve()
pins = {p.relative_to(original).as_posix(): H(p) for p in sorted(original.rglob('*')) if p.is_file()}
assert pins == binding['sourcePins'], 'Firmware source differs from the accepted release binding'
identity = (original / 'math_runtime_identity.h').read_text()
table = (original / 'static_grand_table.h').read_text()
assert re.search(r'\baccepted\s*=\s*true\s*;', identity)
assert 'rootAccepted=true' in table and 'hostTrial=false' in table
assert all(v == '3' for v in re.findall(r'Proof::[A-Za-z0-9_]+,(\d+)}', table))
runtime = re.search(r'fingerprint="([^"]+)"', identity)[1]
family = re.search(r'mathematicalFamily="([^"]+)"', identity)[1]
assert runtime == binding['runtime'] and family == binding['mathFamily']
body = {n: sha for n, sha in pins.items() if Path(n).suffix in ('.h', '.hpp', '.cpp', '.c', '.inc') and Path(n).name not in ('static_grand_table.h', 'math_runtime_identity.h')}
assert len(body) == binding['sourceBodyFiles']
assert hashlib.sha256(canonical({'family': family, 'sources': body})).hexdigest() == runtime
assert not (original / 'startup_nvs_guard.cpp').exists()
version = re.search(r'#define PHOENIX_BUILD_VERSION "([^"]+)"', (original / 'build_version.h').read_text())[1]
assert version == binding['version']
asset_source = args.assets.resolve()
assets = {p.name: {'sha256': H(p), 'bytes': p.stat().st_size} for p in sorted(asset_source.iterdir()) if p.is_file()}
assert assets == binding['assets'], 'Runtime assets differ from the accepted release binding'
assert set(assets) == {'feature_music.mp3', 'phoenix_atlas.zlib'}
expected_music = bytes(int(v, 16) for v in re.findall(r'0x([0-9a-f]{2})', (original / 'music_asset.h').read_text()))
assert hashlib.sha256((asset_source / 'feature_music.mp3').read_bytes()).digest() == expected_music
atlas = (asset_source / 'phoenix_atlas.zlib').read_bytes()
fnv = 2166136261
for byte in atlas:
    fnv = ((fnv ^ byte) * 16777619) & 0xffffffff
atlas_header = (original / 'phoenix_asset.h').read_text()
assert len(atlas) == int(re.search(r'compressedBytes=(\d+)', atlas_header)[1])
assert fnv == int(re.search(r'compressedFNV=(\d+)', atlas_header)[1])
source = D / 'build/source'
dist = args.out.resolve()
assert original != source.resolve() and source.resolve() not in original.parents
assert dist != original and original not in dist.parents and dist not in original.parents
if source.exists():
    shutil.rmtree(source)
source.mkdir(parents=True)
for name in pins:
    out = source / name
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(original / name, out)
# Refuse an output directory with unrelated files; never package stale artifacts.
dist.mkdir(parents=True, exist_ok=True)
allowed = {'phoenix.js', 'phoenix.wasm', 'phoenix.data', 'index.html', 'app.js', 'style.css', 'control-config.js', 'source-config.js', 'sw.js', 'PROVENANCE.json', 'BUNDLE_MANIFEST.json', 'THIRD_PARTY_LICENSES.txt'}
assert all(p.is_file() and p.name in allowed for p in dist.iterdir()), 'Output must be empty or a prior browser bundle'
for p in dist.iterdir():
    p.unlink()
empp = Path(args.empp).resolve()
compiler = ['python3', str(empp)] if empp.suffix == '.py' else [str(empp)]
version_result = subprocess.run(compiler + ['--version'], capture_output=True, text=True, check=True)
toolchain_version = re.search(r'emcc.*? (\d+\.\d+\.\d+)', version_result.stdout)[1]
assert toolchain_version == '7.0.0', 'Use pinned Emscripten 7.0.0'
units = ['phoenix_random', 'ui', 'entry_art', 'collect_art', 'fire_field', 'counter_hud', 'generated_assets', 'phoenix_art', 'phoenix_effects', 'clean_coins', 'coin_sampler', 'winged_phoenix', 'award_art', 'pdf_assets', 'phoenix_numerals', 'audio', 'audio_cues', 'audio_music', 'music_stream', 'twin_music42']
exports = ['pl_boot', 'pl_preferences', 'pl_tick', 'pl_frame', 'pl_input', 'pl_snapshot', 'pl_info', 'pl_game_view', 'pl_return_input_fenced', 'pl_pixels', 'pl_random_healthy', 'malloc', 'free']
prefix_flags = ['-ffile-prefix-map=' + str(D) + '=BROWSER', '-fmacro-prefix-map=' + str(D) + '=BROWSER']
crypto_objects = []
for stem in ['aes', 'ctr_drbg', 'platform_util']:
    obj = D / 'build' / (stem + '.o')
    subprocess.run(compiler + prefix_flags + ['-x', 'c', '-std=c99', '-O3', '-I', str(source), '-c', str(source / 'phoenix_crypto124/library' / (stem + '.c')), '-o', str(obj)], check=True)
    crypto_objects.append(str(obj))
cmd = compiler + ['-std=c++17', '-O3', '-g0', '-DARDUINO', '-I', str(D / 'platform'), '-I', str(source), *prefix_flags, str(D / 'bridge.cpp'), *[str(source / (n + '.cpp')) for n in units], *crypto_objects, '-sUSE_ZLIB=1', '-sASYNCIFY=1', '-sASYNCIFY_STACK_SIZE=262144', '-sSTACK_SIZE=1048576', '-sINITIAL_MEMORY=67108864', '-sALLOW_MEMORY_GROWTH=1', '-sMODULARIZE=1', '-sEXPORT_NAME=PhoenixModule', '-sENVIRONMENT=web,worker,node', '-sEXPORTED_FUNCTIONS=' + json.dumps(['_' + n for n in exports]), '-sEXPORTED_RUNTIME_METHODS=' + json.dumps(['ccall', 'UTF8ToString', 'FS']), '-sASSERTIONS=1', '-sFILESYSTEM=1', '--no-entry']
for name in assets:
    cmd += ['--preload-file', str(asset_source / name) + '@/firmware/data/' + name]
cmd += ['-o', 'phoenix.js']
env = dict(os.environ)
env.setdefault('EMCC_CORES', '4')
(D / 'BUILD_COMMAND.json').write_text(json.dumps(cmd, indent=2) + '\n')
began = time.time()
with (D / 'BUILD.log').open('w') as log:
    result = subprocess.run(cmd, cwd=dist, env=env, stdout=log, stderr=subprocess.STDOUT)
assert result.returncode == 0, (D / 'BUILD.log').read_text()[-6000:]
assert pins == {p.relative_to(original).as_posix(): H(p) for p in sorted(original.rglob('*')) if p.is_file()}, 'Input changed during build'
for p in (D / 'site').iterdir():
    if p.is_file():
        shutil.copyfile(p, dist / p.name)
hold_ms = int(re.search(r'kHoldMs\s*=\s*(\d+)', (source / 'main.cpp').read_text())[1])
double_ms = int(re.search(r'windowMs\s*=\s*(\d+)', (source / 'game_black_tap.h').read_text())[1])
(dist / 'control-config.js').write_text('export const CONTROL=Object.freeze(' + json.dumps({'holdMs': hold_ms, 'doubleMs': double_ms}) + ');\n')
(dist / 'source-config.js').write_text('export const SOURCE=Object.freeze(' + json.dumps({'version': version, 'runtime': runtime, 'mathFamily': family}) + ');\n')
adapter_files = [D / 'build.py', D / 'verify_bundle.py', D / 'SOURCE_BINDING.json', D / 'bridge.cpp', *sorted((D / 'platform').rglob('*.h')), *sorted((D / 'site').glob('*'))]
adapter_pins = {p.relative_to(D).as_posix(): H(p) for p in adapter_files if p.is_file()}
public = {'version': version, 'runtime': runtime, 'runtimeMeaning': 'Engineering-accepted firmware source fingerprint; browser platform is separately bound.', 'mathFamily': family, 'acceptedSource': True, 'acceptanceMeaning': binding['acceptanceMeaning'], 'sourcePins': pins, 'browserAdapterPins': adapter_pins, 'browserAdapterSHA256': hashlib.sha256(canonical(adapter_pins)).hexdigest(), 'sourceAcceptanceReceiptSHA256': binding['sourceAcceptanceReceiptSHA256'], 'assets': assets, 'persistence': 'Session only. Every reload starts with zero balance and denomination selection. No bank imports, exports or saved RNG state.', 'randomness': 'Maintained private Mbed TLS 3.6.7 AES-256 CTR_DRBG; fresh WebCrypto entropy for every financial action; no fallback.', 'gameRules': 'Actual local C++ model. Original casino PAR probabilities and actual-CSPRNG infinite-horizon RTP are unqualified.', 'audio': 'Actual C++ PCM, MP3 decoder and worker through WebAudio. Browser output is not physical speaker/DMA equivalence.', 'build': 'Emscripten ' + toolchain_version}
(dist / 'PROVENANCE.json').write_text(json.dumps(public, indent=2) + '\n')
files = ['./', './index.html', './app.js', './style.css', './control-config.js', './source-config.js', './phoenix.js', './phoenix.wasm', './phoenix.data', './PROVENANCE.json', './BUNDLE_MANIFEST.json', './THIRD_PARTY_LICENSES.txt']
worker = """const PREFIX = 'phoenix-cpp-demo:' + self.registration.scope + ':';
const CACHE = PREFIX + __CACHE_ID__;
const FILES = __FILES__;
self.addEventListener('install', event => {
  event.waitUntil((async () => {
    try {
      const cache = await caches.open(CACHE);
      await cache.addAll(FILES.map(file => new Request(new URL(file, self.registration.scope), { cache: 'reload' })));
      await self.skipWaiting();
    } catch (error) {
      await caches.delete(CACHE);
      throw error;
    }
  })());
});
self.addEventListener('activate', event => {
  event.waitUntil((async () => {
    const old = (await caches.keys()).filter(key => key.startsWith(PREFIX) && key !== CACHE);
    await Promise.all(old.map(key => caches.delete(key)));
    await self.clients.claim();
    if (old.length) {
      const windows = await self.clients.matchAll({ type: 'window' });
      for (const client of windows) {
        if (client.url.startsWith(self.registration.scope)) {
          // Starting navigation without awaiting it lets activation finish;
          // navigation fetches are held until this event has settled.
          client.navigate(client.url).catch(() => {});
        }
      }
    }
  })());
});
self.addEventListener('fetch', event => {
  if (event.request.method === 'GET' && event.request.url.startsWith(self.registration.scope)) {
    event.respondWith((async () => {
      const cache = await caches.open(CACHE);
      return (await cache.match(event.request)) || fetch(event.request);
    })());
  }
});
"""
# Worker changes also create a new cache identity, including a logic-only fix.
cache = hashlib.sha256(worker.encode() + b''.join(p.name.encode() + hashlib.sha256(p.read_bytes()).digest() for p in sorted(dist.iterdir()) if p.is_file())).hexdigest()[:20]
(dist / 'sw.js').write_text(worker.replace('__CACHE_ID__', json.dumps(cache)).replace('__FILES__', json.dumps(files)))
for p in dist.iterdir():
    if p.is_file():
        data = p.read_bytes()
        assert str(Path.home()).encode() not in data and b'/var/folders/' not in data, 'Private path in ' + p.name
artifacts = {p.name: {'bytes': p.stat().st_size, 'sha256': H(p)} for p in sorted(dist.iterdir()) if p.is_file()}
manifest = {'version': version, 'runtime': runtime, 'artifacts': artifacts, 'note': 'This manifest excludes itself to avoid a circular digest. The deployment gate verifies all other bundle files and source/adapter pins.'}
(dist / 'BUNDLE_MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
record = {'status': 'PASS_ENGINEERING_ACCEPTED_SOURCE_BROWSER_BUILD_NOT_PUBLISHED', 'source': str(original), 'sourcePins': pins, 'adapterPins': adapter_pins, 'runtime': runtime, 'sourceAccepted': True, 'buildSeconds': time.time() - began, 'artifacts': {p.name: {'bytes': p.stat().st_size, 'sha256': H(p)} for p in sorted(dist.iterdir()) if p.is_file()}, 'output': str(dist), 'noFirmwareSourceEdits': True, 'noExternalOrDeviceWrites': True}
(D / 'BUILD_RESULT.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps({k: v for k, v in record.items() if k not in ('sourcePins', 'adapterPins', 'artifacts')}, indent=2), flush=True)
