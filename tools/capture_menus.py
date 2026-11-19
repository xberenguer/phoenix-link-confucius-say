#!/usr/bin/env python3
"""Build and capture actual public menu navigation, without changing values."""
import argparse, hashlib, json, subprocess
from pathlib import Path

def main():
    root=Path(__file__).resolve().parents[1];parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();out=args.out.resolve()
    if out.exists() and any(out.iterdir()):parser.error('--out must be new or empty')
    out.mkdir(parents=True,exist_ok=True);build=out/'build';build.mkdir()
    source=root/'firmware/src';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    pins={p.relative_to(source).as_posix():sha(p) for p in sorted(source.rglob('*')) if p.is_file()};assets={p.name:sha(p) for p in sorted((root/'firmware/data').iterdir()) if p.is_file()}
    units=['phoenix_random','ui','entry_art','collect_art','fire_field','counter_hud','generated_assets','phoenix_art','phoenix_effects','clean_coins','coin_sampler','winged_phoenix','award_art','pdf_assets','phoenix_numerals','audio','audio_cues','audio_music','music_stream','twin_music42'];objects=[]
    for unit in ['aes','ctr_drbg','platform_util']:
        target=build/(unit+'.o');subprocess.run(['cc','-std=c99','-O3','-I',str(source),'-c',str(source/'phoenix_crypto124/library'/(unit+'.c')),'-o',str(target)],check=True);objects.append(str(target))
    command=['c++','-std=c++17','-O3','-g0','-DARDUINO','-I',str(root/'browser/platform'),'-I',str(source),str(root/'tools/capture_menus.cpp'),*[str(source/(unit+'.cpp')) for unit in units],*objects,'-lz','-o',str(build/'capture-menus')];subprocess.run(command,check=True,cwd=root)
    receipt={'source_sha256':pins,'runtime_asset_sha256':assets,'recorder_cpp_sha256':sha(root/'tools/capture_menus.cpp'),'recorder_py_sha256':sha(Path(__file__)),'recorder_dependencies_sha256':{'tools/capture_gameplay.cpp':sha(root/'tools/capture_gameplay.cpp')},'build_command':command,'scope':'Actual public button navigation. No counter/jackpot value editing, no return option application, no forced outcome. Native silent captures.'}
    (out/'capture-provenance.json').write_text(json.dumps(receipt,indent=2)+'\n');subprocess.run([str(build/'capture-menus'),str(out)],check=True,cwd=root)
    assert pins=={p.relative_to(source).as_posix():sha(p) for p in sorted(source.rglob('*')) if p.is_file()}
    assert assets=={p.name:sha(p) for p in sorted((root/'firmware/data').iterdir()) if p.is_file()}

if __name__=='__main__':main()
