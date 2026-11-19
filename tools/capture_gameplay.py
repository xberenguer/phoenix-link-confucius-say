#!/usr/bin/env python3
"""Build and run a native recorder of unchanged, genuinely played game states.

macOS host adapter only. It records silent native RGB565 frames, action
journals and event metadata. No test, fixture, replay or outcome APIs are used.
Run from the repository root so runtime artwork loads from firmware/data.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path

def main():
    root=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--actions',type=int,default=5000)
    args=parser.parse_args()
    out=args.out.resolve()
    if out.exists() and any(out.iterdir()):parser.error('--out must be new or empty')
    out.mkdir(parents=True,exist_ok=True)
    build=out/'build';build.mkdir()
    source=root/'firmware/src'
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    pins={p.relative_to(source).as_posix():sha(p) for p in sorted(source.rglob('*')) if p.is_file()}
    assets={p.name:sha(p) for p in sorted((root/'firmware/data').iterdir()) if p.is_file()}
    units=['phoenix_random','ui','entry_art','collect_art','fire_field','counter_hud','generated_assets','phoenix_art','phoenix_effects','clean_coins','coin_sampler','winged_phoenix','award_art','pdf_assets','phoenix_numerals','audio','audio_cues','audio_music','music_stream','twin_music42']
    objects=[]
    for unit in ['aes','ctr_drbg','platform_util']:
        target=build/(unit+'.o')
        subprocess.run(['cc','-std=c99','-O3','-I',str(source),'-c',str(source/'phoenix_crypto124/library'/(unit+'.c')),'-o',str(target)],check=True)
        objects.append(str(target))
    command=['c++','-std=c++17','-O3','-g0','-DARDUINO','-I',str(root/'browser/platform'),'-I',str(source),str(root/'tools/capture_gameplay.cpp'),*[str(source/(unit+'.cpp')) for unit in units],*objects,'-lz','-o',str(build/'capture-gameplay')]
    subprocess.run(command,check=True,cwd=root)
    receipt={'source_sha256':pins,'runtime_asset_sha256':assets,'recorder_cpp_sha256':sha(root/'tools/capture_gameplay.cpp'),'recorder_py_sha256':sha(Path(__file__)),'build_command':command,'arguments':{'actions':args.actions},'scope':'Real production actions, OS entropy; native visual capture, silent audio output. No hardware timing claim.'}
    (out/'capture-provenance.json').write_text(json.dumps(receipt,indent=2)+'\n')
    subprocess.run([str(build/'capture-gameplay'),str(out),str(args.actions)],check=True,cwd=root)
    assert pins=={p.relative_to(source).as_posix():sha(p) for p in sorted(source.rglob('*')) if p.is_file()},'Production source changed during capture'
    assert assets=={p.name:sha(p) for p in sorted((root/'firmware/data').iterdir()) if p.is_file()},'Runtime assets changed during capture'

if __name__=='__main__':main()
