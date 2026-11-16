#!/usr/bin/env python3
"""Compile real Engine/MbedTLS metamorphic tests; never opens a device.

The deterministic input source exists only in the test executable. No runtime
crypto, outcome law, funding policy or production source is patched by this tool.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    root=Path(__file__).resolve().parents[1]
    parser.add_argument('--source',type=Path,default=root/'firmware/src')
    parser.add_argument('--out',type=Path,required=True,help='New private build/result directory')
    parser.add_argument('--expect-pins',type=Path,help='Optional exact source-binding receipt')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args()
    source=args.source.resolve()
    test_source=root/'tests/outcome_integrity.cpp'
    test_hash=sha(test_source)
    runner_hash=sha(Path(__file__))
    if not (source/'phoenix_engine.h').is_file():parser.error('Actual firmware source is required')
    if args.out.exists():parser.error('--out must be a new directory')
    if args.expect_pins:
        pinned=json.loads(args.expect_pins.read_text())['sourcePins']
        for name,expected in pinned.items():
            if sha(source/name)!=expected:raise RuntimeError('Source pin mismatch: '+name)
    args.out.mkdir(parents=True)
    common=['-O1' if args.sanitize else '-O2','-g0','-DPHOENIX_RNG_TESTING','-I',str(source)]
    if args.sanitize:common+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    commands=[]
    def invoke(command):
        start=time.monotonic()
        result=subprocess.run(command,capture_output=True,text=True)
        commands.append({'command':command,'returnCode':result.returncode,
                         'seconds':round(time.monotonic()-start,6),
                         'stdout':result.stdout,'stderr':result.stderr})
        (args.out/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
        if result.returncode:raise RuntimeError(result.stderr or result.stdout or 'Child test failed')
        return result
    objects=[]
    for unit in ['aes','ctr_drbg','platform_util']:
        target=args.out/(unit+'.o')
        invoke([os.environ.get('CC','cc'),'-std=c99',*common,'-MMD','-MF',str(target.with_suffix('.d')),
                '-c',str(source/'phoenix_crypto124/library'/(unit+'.c')),'-o',str(target)])
        objects.append(str(target))
    binary=args.out/'outcome-integrity'
    invoke([os.environ.get('CXX','c++'),'-std=c++17',*common,'-MMD','-MF',str(args.out/'cpp.d'),
            str(test_source),str(source/'phoenix_random.cpp'),*objects,'-o',str(binary)])
    result=invoke([str(binary.resolve())])
    value=json.loads(result.stdout)
    if value['status']!='PASS':raise RuntimeError('Test did not report PASS')
    actual={str(p.relative_to(source)):sha(p) for p in source.rglob('*') if p.is_file()}
    if test_hash!=sha(test_source) or runner_hash!=sha(Path(__file__)):
        raise RuntimeError('Test/runner changed during compilation or execution')
    if args.expect_pins and actual!=pinned:raise RuntimeError('Source tree changed or differs from pinned tree')
    report={'testResult':value,'sanitizers':'address,undefined' if args.sanitize else 'none',
            'sourcePins':actual,'testSha256':test_hash,
            'runnerSha256':runner_hash,'binarySha256':sha(binary),
            'scope':'Host actual Engine + private MbedTLS; test-only entropy. No native/hardware/regulatory claim.'}
    (args.out/'RESULT158.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(value,indent=2))


if __name__=='__main__':main()
