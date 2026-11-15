#!/usr/bin/env python3
"""Build the offline runtime filesystem using Espressif's portable generator."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
data=root/"data"
expected={"feature_music.mp3","phoenix_atlas.zlib"}
entries=list(data.iterdir())
if data.is_symlink() or {p.name for p in entries}!=expected or any(p.is_symlink() or not p.is_file() for p in entries):
    raise SystemExit("Runtime data must contain exactly feature_music.mp3 and phoenix_atlas.zlib as regular files")
out=root/"release/feature-assets.bin"
out.parent.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(root/"scripts/vendor/spiffsgen.py"),"0x180000",str(root/"data"),str(out),"--page-size","256","--obj-name-len","32","--meta-len","4","--use-magic","--use-magic-len"],check=True)
print(out)
