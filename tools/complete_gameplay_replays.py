#!/usr/bin/env python3
"""Export whole, genuinely recorded actions without arbitrary replay cut points.

Pixels stay in their original order at 40 ms per frame. A declared pause holds
the last recorded image before replay; no transition or intermediate frame is
invented. Pillow's streaming WebP encoder keeps long recordings memory bounded.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from PIL import Image, _webp

from build_gameplay_gallery import Event, FRAME_MS, Gallery, sha


def encode(event, path, pause_ms=1600, start=0, end=None, crop=None, scale=3):
    end = len(event.rows) if end is None else end
    first = event.image(start)
    if crop:
        x, y, w, h = crop
        first = first.crop((x, y, x + w, y + h))
    size = (first.width * scale, first.height * scale)
    first.close()
    encoder = _webp.WebPAnimEncoder(size, 0xFF000000, 0, False, 9, 17, False, False)
    timestamp = 0
    for index in range(start, end):
        native = event.image(index)
        if crop:
            x, y, w, h = crop
            selected = native.crop((x, y, x + w, y + h))
            native.close()
        else:
            selected = native
        frame = selected.resize(size, Image.Resampling.NEAREST)
        selected.close()
        encoder.add(frame.getim(), timestamp, True, 80, 100, 4)
        frame.close()
        timestamp += FRAME_MS + (pause_ms if index == end - 1 else 0)
    encoder.add(None, timestamp, True, 80, 100, 0)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(encoder.assemble("", "", ""))
    return size


def visible_score(event, frame):
    """Prefer an actually visible 9/10 glyph over its intentional blank blink."""
    row = event.rows[frame]
    symbols = row["snapshot"].get("reelDisplay", {}).get("symbols", [])
    cells = [cell for cell, symbol in enumerate(symbols) if symbol in (0, 1)]
    if not cells:
        return (0, 0)
    image = event.image(frame)
    values = []
    for cell in cells:
        x, y = 2 + (cell // 3) * 47, 16 + (cell % 3) * 35
        pixels = image.crop((x + 3, y + 2, x + 44, y + 33))
        values.append(sum(max(rgb) > 112 for rgb in pixels.getdata()))
        pixels.close()
    image.close()
    return (min(values), sum(values))


def refresh_still(event, item, output):
    frame = item.get("source_frame")
    if not isinstance(frame, int) or item.get("crop") or item.get("source_crop"):
        return None
    phase = event.rows[frame]["snapshot"]["animation"]
    if phase not in {"ready", "free-intro", "free-counter", "spin-award", "spin-award-settle"}:
        return None
    start = frame
    while start > 0 and event.rows[start - 1]["snapshot"]["animation"] == phase:
        start -= 1
    end = frame + 1
    while end < len(event.rows) and event.rows[end]["snapshot"]["animation"] == phase:
        end += 1
    selected = max(range(start, end), key=lambda i: (*visible_score(event, i), -abs(i - frame)))
    if selected == frame:
        return None
    native = event.image(selected)
    image = native.resize(tuple(item["dimensions"]), Image.Resampling.NEAREST)
    native.close()
    path = output / item["file"]
    image.save(path, "PNG", optimize=True)
    image.close()
    return {**item, **event.evidence(selected), "sha256": sha(path), "bytes": path.stat().st_size,
            "selection": "Fully visible recorded numeric glyphs within the original presentation phase"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--index", type=Path, required=True)
    parser.add_argument("--output-index", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--session", required=True)
    parser.add_argument("--hero-event", help="Export just this fresh event as the README preview")
    parser.add_argument("--include-event", action="append", default=[], help="Also export this complete fresh event")
    args = parser.parse_args()
    gallery = Gallery(args.capture, args.output, args.session)
    if args.hero_event:
        index = {
            "kind": "recorded-gameplay-conversion", "schema": 1, "capture_id": args.session,
            "source_provenance": json.loads((args.capture / "capture-provenance.json").read_text()),
            "source_provenance_sha256": sha(args.capture / "capture-provenance.json"),
            "source_actions_journal_sha256": sha(args.capture / "actions.ndjson"),
            "source_events_journal_sha256": sha(args.capture / "events.ndjson"),
            "coverage": {"scope": "One whole natural spin with an actual pre-input rest"},
            "events": [], "outputs": [],
        }
        specs = [gallery.event_specs[event_id] for event_id in [args.hero_event, *args.include_event]]
    else:
        index = json.loads(args.index.read_text())
        specs = [gallery.event_specs[e["id"]] for e in index["events"]]
    outputs = [i for i in index["outputs"] if not (i["kind"] == "animation" or i.get("role") == "readme-preview")]
    mapping = {}
    for spec in specs:
        event = Event(args.capture, spec)
        try:
            is_hero = event.id == args.hero_event
            name = "session.webp" if is_hero else f"sessions/{args.session}/{event.id}.webp"
            path = args.output / name
            size = encode(event, path)
            with Image.open(path) as image:
                record = gallery.register(name, "animation", image, event.evidence(0, len(event.rows)),
                    source_frames=len(event.rows), scale=3, tags=spec.get("tags", []),
                    replay_pause_ms=1600, loop_policy="whole-recorded-event-with-replay-pause",
                    role="readme-preview" if is_hero else "complete-event",
                    processing_tool="tools/complete_gameplay_replays.py", processing_tool_sha256=sha(Path(__file__)))
            outputs.append(record)
            if is_hero:
                poster = event.image(len(event.rows) - 1).resize(size, Image.Resampling.NEAREST)
                poster_name = f"screens/{args.session}/spin-result.png"
                outputs.append(gallery.save_png(poster_name, poster, "screenshot",
                    event.evidence(len(event.rows) - 1), scale=3, still_name="first-spin-result",
                    context_file=name, poster_for=name))
                poster.close()
            for previous in index["outputs"]:
                if previous.get("event_id") == event.id and previous["kind"] == "animation":
                    mapping[previous["file"]] = name
            for n, item in enumerate(outputs):
                if item.get("event_id") == event.id and item["kind"] == "screenshot":
                    refreshed = refresh_still(event, item, args.output)
                    if refreshed:
                        outputs[n] = refreshed
            if args.hero_event:
                index["events"].append({k: v for k, v in spec.items() if k not in {"raw_file", "frame_journal"}})
            print(json.dumps({"event": event.id, "frames": len(event.rows), "bytes": path.stat().st_size}), flush=True)
        finally:
            event.close()
    index["outputs"] = outputs
    index["replay_path_mapping"] = mapping
    args.output_index.write_text(json.dumps(index, indent=2) + "\n")


if __name__ == "__main__":
    main()
