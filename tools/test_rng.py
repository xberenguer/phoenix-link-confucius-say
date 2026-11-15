#!/usr/bin/env python3
"""Build the actual CSPRNG known-answer and Engine failure tests in a temp dir.

Deterministic entropy injection and upstream self-test vectors are compiled only
for this test executable. Neither is enabled in firmware or the browser demo.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true', help='Enable address/undefined-behavior sanitizers')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = root / 'firmware/src'
    common = ['-O1' if args.sanitize else '-O2', '-g0', '-DPHOENIX_RNG_TESTING', '-I', str(source)]
    if args.sanitize:
        common += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    with tempfile.TemporaryDirectory(prefix='phoenix-rng-test-') as temp:
        temp = Path(temp)
        objects = []
        for unit in ['aes', 'ctr_drbg', 'platform_util']:
            target = temp / (unit + '.o')
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', *common, '-c',
                            str(source / 'phoenix_crypto124/library' / (unit + '.c')),
                            '-o', str(target)], check=True)
            objects.append(str(target))
        binary = temp / 'rng-tests'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', *common,
                        str(root / 'tests/rng_core.cpp'), str(source / 'phoenix_random.cpp'),
                        *objects, '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    main()
