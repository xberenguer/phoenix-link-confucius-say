"""Reject stale browser artifacts when source, assets or adapter have changed."""
from pathlib import Path
import argparse, hashlib, json, re

D = Path(__file__).resolve().parent
H = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--assets', type=Path, required=True)
parser.add_argument('--bundle', type=Path, required=True)
args = parser.parse_args()
source, assets, bundle = args.source.resolve(), args.assets.resolve(), args.bundle.resolve()
binding = json.loads((D / 'SOURCE_BINDING.json').read_text())
assert binding['engineeringAccepted'] is True
source_pins = {p.relative_to(source).as_posix(): H(p) for p in source.rglob('*') if p.is_file()}
assert source_pins == binding['sourcePins'], 'Source is not the bound engineering-accepted release'
assert not any(p.is_symlink() for p in source.rglob('*')), 'Source symlinks are forbidden'
asset_pins = {p.name: {'sha256': H(p), 'bytes': p.stat().st_size} for p in assets.iterdir() if p.is_file()}
assert asset_pins == binding['assets']
provenance = json.loads((bundle / 'PROVENANCE.json').read_text())
assert provenance['acceptedSource'] is True
for field in ['version', 'runtime', 'mathFamily']:
    assert provenance[field] == binding[field], field
assert provenance['sourcePins'] == source_pins
assert provenance['assets'] == asset_pins
assert provenance['sourceAcceptanceReceiptSHA256'] == binding['sourceAcceptanceReceiptSHA256']
identity = (source / 'math_runtime_identity.h').read_text()
assert re.search(r'\baccepted\s*=\s*true\s*;', identity)
assert re.search(r'fingerprint="([^"]+)"', identity)[1] == binding['runtime']
assert re.search(r'mathematicalFamily="([^"]+)"', identity)[1] == binding['mathFamily']
for name, expected in provenance['browserAdapterPins'].items():
    p = D / name
    assert p.is_file() and not p.is_symlink() and H(p) == expected, 'Adapter changed: ' + name
required_adapter = {'build.py', 'verify_bundle.py', 'SOURCE_BINDING.json', 'bridge.cpp', *['site/' + n for n in ['app.js', 'index.html', 'style.css', 'THIRD_PARTY_LICENSES.txt']]}
assert required_adapter <= set(provenance['browserAdapterPins'])
assert hashlib.sha256(json.dumps(provenance['browserAdapterPins'], sort_keys=True, separators=(',', ':')).encode()).hexdigest() == provenance['browserAdapterSHA256']
manifest = json.loads((bundle / 'BUNDLE_MANIFEST.json').read_text())
assert manifest['version'] == binding['version'] and manifest['runtime'] == binding['runtime']
actual = {p.name: {'bytes': p.stat().st_size, 'sha256': H(p)} for p in bundle.iterdir() if p.is_file() and p.name != 'BUNDLE_MANIFEST.json'}
assert actual == manifest['artifacts'], 'Browser bundle changed or contains stale/extra files'
assert all(p.is_file() and not p.is_symlink() for p in bundle.iterdir()), 'Unexpected bundle directory or symlink'
assert {'phoenix.js', 'phoenix.wasm', 'phoenix.data', 'source-config.js', 'sw.js', 'PROVENANCE.json'} <= set(actual)
config = (bundle / 'source-config.js').read_text()
assert all(value in config for value in [binding['version'], binding['runtime'], binding['mathFamily']])
assert b'\x00asm' == (bundle / 'phoenix.wasm').read_bytes()[:4]
print(json.dumps({'status': 'PASS_SOURCE_ASSET_ADAPTER_BUNDLE_BINDING', 'version': binding['version'], 'runtime': binding['runtime'], 'sourceFiles': len(source_pins), 'assetFiles': len(asset_pins), 'bundleFiles': len(actual) + 1, 'meaning': 'Reproducible provenance gate; not regulatory or casino-math certification.'}, indent=2))
