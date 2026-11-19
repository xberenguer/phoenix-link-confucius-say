#!/usr/bin/env python3
"""Publish every implemented payline using crops of recorded menu pages.

No route is drawn or reconstructed here. Each diagram is cut from an actual
captured chart; its five coloured dot positions are checked against the game's
exported row path before publication. Only whole-pixel enlargement is used.
"""
import argparse
import csv
import json
import math
from pathlib import Path

from PIL import Image

from build_gameplay_gallery import sha

BEGIN = "<!-- BEGIN COMPLETE PAYLINE REFERENCE -->"
END = "<!-- END COMPLETE PAYLINE REFERENCE -->"
LABELS = ("Top", "Middle", "Bottom")


def build(root, capture, index_path):
    csv_path = root / "docs/data/implemented-paylines.csv"
    with csv_path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    routes = {(int(row["line_selection"]), int(row["line_number"])):
              [int(row[f"reel_{col}_row"]) for col in range(1, 6)] for row in rows}
    layouts = sorted({count for count, _ in routes})
    expected = {(count, number) for count in layouts for number in range(1, count + 1)}
    assert len(rows) == len(routes) and set(routes) == expected, "Route data is incomplete or duplicated"
    index = json.loads(index_path.read_text())
    screenshots = {item["still_name"]: item for item in index["outputs"] if item["kind"] == "screenshot"}
    index["outputs"] = [item for item in index["outputs"] if item.get("role") != "payline-reference"]
    originals, images = {}, {}
    try:
        for count, number in sorted(routes):
            page, slot = (number - 1) // 6 + 1, (number - 1) % 6
            still_name = f"paylines-{count}-{page}"
            source = screenshots[still_name]
            if still_name not in originals:
                raw = capture / f"{still_name}.rgb565"
                assert sha(raw) == source["source_raw_sha256"], "Captured chart identity differs"
                originals[still_name] = Image.frombytes("RGB", (240, 135), raw.read_bytes(), "raw", "BGR;16")
            native = originals[still_name]
            x, y, width, height = 12 + slot % 2 * 110, 50 + slot // 2 * 22, 100, 18
            path = routes[count, number]
            colour = native.getpixel((x + 32, y + 3 + path[0] * 6))
            assert max(colour) >= 128, f"Route dot is not visible: {count}/{number}"
            for col, row in enumerate(path):
                px = x + 32 + col * 15
                matches = [r for r in range(3) if native.getpixel((px, y + 3 + r * 6)) == colour]
                assert matches == [row], f"Captured path differs from current data: {count}/{number}, reel {col + 1}"
            file = f"artwork/paylines/{count}-lines/line-{number:03d}.png"
            target = root / "docs/media/gameplay" / file
            target.parent.mkdir(parents=True, exist_ok=True)
            enlarged = native.crop((x, y, x + width, y + height)).resize((600, 108), Image.Resampling.NEAREST)
            enlarged.save(target, "PNG", optimize=True)
            enlarged.close()
            index["outputs"].append({**source, "file": file, "kind": "symbol-crop",
                "still_name": f"payline-{count}-{number}", "role": "payline-reference",
                "crop": [x, y, width, height], "scale": 6, "dimensions": [600, 108],
                "sha256": sha(target), "bytes": target.stat().st_size, "context_file": source["file"],
                "line_layout": count, "line_number": number, "row_path": path,
                "source_route_csv_sha256": sha(csv_path), "captured_route_dots_verified": True,
                "processing_tool": "tools/build_payline_reference.py", "processing_tool_sha256": sha(Path(__file__))})
            images[count, number] = "media/gameplay/" + file
    finally:
        for image in originals.values():
            image.close()
    index.setdefault("coverage", {})["complete_payline_reference"] = {
        "routes": len(routes), "layouts": layouts, "captured_chart_pages": len(originals),
        "source_route_csv_sha256": sha(csv_path), "verified_dot_positions": len(routes) * 5}
    index_path.write_text(json.dumps(index, indent=2) + "\n")

    navigation = " · ".join(f"[{count} lines](#layout-{count})" for count in layouts)
    body = [BEGIN, '<a id="every-line"></a>', "## Every line, individually",
            f"**All {len(routes)} numbered entries across the six implemented layouts are shown below.** "
            "Each enlarged diagram is cropped from the actual in-game chart, with its route written out from left to right. "
            "Some routes repeat between layouts; use the numbers for the layout you are playing.", navigation,
            "[Download the complete row-by-row route list](data/implemented-paylines.csv). "
            "The original full-screen chart photos follow this reference."]
    for count in layouts:
        body += [f'<a id="layout-{count}"></a>', f"### {count}-line layout — every line from 1 to {count}",
                 f"{count} routes, shown on {math.ceil(count / 6)} original chart pages. Every diagram below is visible directly on this page."]
        jumps = sorted({1, count, *range(10 if count > 25 else 5, count, 10 if count > 25 else 5)})
        body.append("Jump to line: " + " · ".join(f"[{n}](#layout-{count}-line-{n})" for n in jumps))
        for number in range(1, count + 1):
            page = (number - 1) // 6 + 1
            route = " → ".join(LABELS[r] for r in routes[count, number])
            file = images[count, number]
            body += [f'<a id="layout-{count}-line-{number}"></a>', f"#### Line {number}",
                     f"**{route}**",
                     f'<a href="{file}"><img src="{file}" alt="{count}-line layout, line {number}: {route}" width="600" height="108" loading="lazy"></a>',
                     f'[View the original chart, page {page} of {math.ceil(count / 6)}](media/gameplay/screens/menus/paylines-{count}-{page}.png).']
        body.append("[Back to the layout list](#every-line)")
    body.append(END)
    document = root / "docs/paylines.md"
    text = document.read_text()
    block = "\n\n".join(body)
    if BEGIN in text:
        start = text.index(BEGIN)
        end = text.index(END, start) + len(END)
        text = text[:start] + block + text[end:]
    else:
        before = "## Open the charts while playing"
        text = text.replace(before, block + "\n\n" + before, 1)
    marker = "<!-- COMPLETE PAYLINE NAVIGATION -->"
    if marker not in text:
        lead = "You choose how many routes to play; the game checks them for you."
        text = text.replace(lead, lead + "\n\n" + marker + "\n\n"
                            f"**[See every line](#every-line):** {navigation} — {len(routes)} numbered entries, all enlarged below.", 1)
    document.write_text(text)
    return {"routes": len(routes), "layouts": layouts, "captured_pages": len(originals),
            "verified_dot_positions": len(routes) * 5}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--index", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.root, args.capture, args.index), indent=2))


if __name__ == "__main__":
    main()
