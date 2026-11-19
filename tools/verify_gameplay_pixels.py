#!/usr/bin/env python3
"""Compare every animation frame with retained, original gameplay recordings.

This optional review requires Pillow and the original capture directories.
The ordinary gallery validator can run without those large private archives.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

from build_gameplay_gallery import Event, load_lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--session", action="append", required=True, help="ID:CAPTURE_DIRECTORY")
    args = parser.parse_args()
    roots = {name: Path(path) for name, path in (value.split(":", 1) for value in args.session)}
    specs = {name: {event["id"]: event for event in load_lines(folder / "events.ndjson")}
             for name, folder in roots.items()}
    catalog = json.loads((args.root / "docs/gameplay-media.json").read_text())
    events = {}
    checked = frames_checked = 0
    errors = []
    for item in catalog["images"]:
        if item["kind"] not in {"animation", "symbol-animation"}:
            continue
        session = item["session_id"]
        event_id = item["event_id"].split(":", 1)[1]
        key = session, event_id
        if key not in events:
            events[key] = Event(roots[session], specs[session][event_id])
        event = events[key]
        try:
            assert item["source_raw_sha256"] == event.raw_sha, "Raw recording identity differs"
            start, end = item["source_frame"], item["source_frame_end_exclusive"]
            assert 0 <= start < end <= len(event.rows), "Recorded frame range is out of bounds"
            digest = hashlib.sha256(event.buffer[start * 64800:end * 64800]).hexdigest()
            assert digest == item["source_frame_bytes_sha256"], "Source interval bytes differ"
            boundary = item.get("loop_boundary", {})
            if boundary.get("exact_source_continuation_match"):
                assert end < len(event.rows), "No captured continuation exists at the loop boundary"
                x, y, width, height = item["crop"]
                def native_crop(frame):
                    raw = event.frame_bytes(frame)
                    return b"".join(raw[(row * 240 + x) * 2:(row * 240 + x + width) * 2]
                                    for row in range(y, y + height))
                first_hash = hashlib.sha256(native_crop(start)).hexdigest()
                continuation_hash = hashlib.sha256(native_crop(end)).hexdigest()
                assert first_hash == continuation_hash, "Actual continuation does not match the opening crop"
                assert first_hash == boundary["first_crop_sha256"], "Opening crop proof differs"
                assert continuation_hash == boundary["actual_continuation_crop_sha256"], "Continuation proof differs"
            intro = item.get("intro_hold_ms", 0)
            with Image.open(args.root / "docs" / item["file"]) as animation:
                encoded_index = 0
                animation.seek(0)
                pixels = animation.convert("RGB")
                until = animation.info["timestamp"] + animation.info["duration"]
                for number, frame in enumerate(range(start, end)):
                    time = 0 if number == 0 else intro + number * 40
                    while time >= until:
                        encoded_index += 1
                        animation.seek(encoded_index)
                        pixels.close()
                        pixels = animation.convert("RGB")
                        until = animation.info["timestamp"] + animation.info["duration"]
                    raw = event.image(frame)
                    if item.get("crop"):
                        x, y, width, height = item["crop"]
                        cropped = raw.crop((x, y, x + width, y + height))
                        raw.close()
                        raw = cropped
                    enlarged = raw.resize(tuple(item["dimensions"]), Image.Resampling.NEAREST)
                    raw.close()
                    assert enlarged.tobytes() == pixels.tobytes(), f"Pixels differ at source frame {frame}"
                    enlarged.close()
                    frames_checked += 1
                pixels.close()
            checked += 1
            print(f"PASS {item['file']}", flush=True)
        except (AssertionError, EOFError, KeyError, ValueError, OSError) as exc:
            errors.append(f"{item['file']}: {exc}")
            print("FAIL " + errors[-1], flush=True)
    for event in events.values():
        event.close()
    result = {"status": "FAIL" if errors else "PASS", "animations": checked,
              "original_frames_compared": frames_checked, "errors": errors}
    print(json.dumps(result, indent=2))
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
