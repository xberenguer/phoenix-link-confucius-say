#!/usr/bin/env python3
"""Enlarge the five actual line diagrams from a captured in-game chart.

Only rectangular crops and exact integer scaling are used. The line paths,
numbers, colours and grid pixels come from the unchanged game's own menu.
"""
import argparse
import json
from pathlib import Path

from PIL import Image

from build_gameplay_gallery import sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--index", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    index = json.loads(args.index.read_text())
    source = next(item for item in index["outputs"] if item.get("still_name") == "paylines-5-1")
    raw = args.capture / "paylines-5-1.rgb565"
    assert sha(raw) == source["source_raw_sha256"], "The original recorded menu differs"
    native = Image.frombytes("RGB", (240, 135), raw.read_bytes(), "raw", "BGR;16")
    boxes = [(12, 50, 100, 18), (122, 50, 100, 18), (12, 72, 100, 18),
             (122, 72, 100, 18), (12, 94, 100, 18)]
    names = {f"artwork/paylines/line-{number}.png" for number in range(1, 6)}
    index["outputs"] = [item for item in index["outputs"] if item["file"] not in names]
    for number, (x, y, width, height) in enumerate(boxes, 1):
        name = f"artwork/paylines/line-{number}.png"
        path = args.output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        image = native.crop((x, y, x + width, y + height)).resize(
            (width * 6, height * 6), Image.Resampling.NEAREST)
        image.save(path, "PNG", optimize=True)
        image.close()
        item = {**source, "file": name, "kind": "symbol-crop", "still_name": f"payline-example-{number}",
                "crop": [x, y, width, height], "scale": 6, "dimensions": [width * 6, height * 6],
                "sha256": sha(path), "bytes": path.stat().st_size,
                "context_file": source["file"], "line_number": number, "line_layout": 5,
                "processing_tool": "tools/crop_payline_examples.py", "processing_tool_sha256": sha(Path(__file__))}
        index["outputs"].append(item)
    native.close()
    args.index.write_text(json.dumps(index, indent=2) + "\n")
    print("Enlarged all five actual recorded line diagrams.")


if __name__ == "__main__":
    main()
