#!/usr/bin/env python3
"""Close symbol previews using contiguous recorded pixels, never generated art.

Periodic previews end just before a real recorded frame matching their opening
frame. One-shot joining sequences include a complete settled pulse and a
declared review pause made by holding their actual final image. This tool does
not alter the game, recordings, capture indices, or public catalog.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import shutil
from pathlib import Path

from PIL import Image

from build_gameplay_gallery import Event, FRAME_MS, Gallery, SYMBOL_NAMES, sha, scrub_paths


PERIODS = {
    "nine": (1250, "generated_assets.cpp: nineWinFrames[(ms/125)%10]; real payline clock can reset at1300ms"),
    "ten": (1000, "generated_assets.cpp: tenWinFrames[(ms/125)%8]; real payline clock can reset at1300ms"),
    "bell": (4000, "bell_art.h: frameCount96, periodMs4000"),
    "abacus": (4000, "generated_assets.cpp: (ms/125)%32"),
    "book": (3000, "generated_assets.cpp: 1250ms opening, then ms%3000"),
    "candle": (2000, "generated_assets.cpp: candleFrames[(ms/125)%16]"),
    "confucius": (6000, "generated_assets.cpp: sageBlueAmount (ms-600)%6000 after settled introduction"),
    "wisdom": (4000, "generated_assets.cpp: 2000ms delay then ((ms-2000)/40)%100"),
    "phoenix": (2500, "uiSnapshot phoenixPeriodMs2500; exact40ms sampled recurrence at5000ms"),
}
CAPTURES = {
    "primary": "capture-real-2026-11-18",
    "major": "capture-extra-real",
    "prizes": "capture-prizes-real",
}
PHASES = {"ready", "landing", "sage-expansion", "free-intro", "free-counter",
          "spin-award", "spin-award-settle", "feature-end", "hold-intro"}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def crop_bytes(event, frame, crop):
    x, y, width, height = crop
    base = frame * 64800
    return b"".join(event.buffer[base + ((y + row) * 240 + x) * 2:
                                 base + ((y + row) * 240 + x + width) * 2]
                    for row in range(height))


def original_cell(item):
    if item.get("observed_prize"):
        return item["observed_prize"]["cell_index"]
    x, y, width, height = item["source_crop"]
    if item.get("crop_variant"):
        x, y = item["nominal_source_crop"][:2]
        x, y = x + 5, y + 4
    return ((x - 2) // 47) * 3 + (y - 16) // 35


def cell_crop(cell, expanded):
    x, y = 2 + cell // 3 * 47, 16 + cell % 3 * 35
    if not expanded:
        return [x, y, 47, 35]
    left, top, right, bottom = max(0, x - 5), max(0, y - 4), min(240, x + 52), min(135, y + 39)
    return [left, top, right - left, bottom - top]


def visible_digit(data, size, symbol):
    image = Image.frombytes("RGB", size, data, "raw", "BGR;16")
    if symbol == "nine":
        return sum(g > 85 and g > r * 1.5 and g > b * 1.3 for r, g, b in image.getdata()) > 30
    if symbol == "ten":
        return sum(r > 85 and r > g * 1.5 and r > b * 1.3 for r, g, b in image.getdata()) > 30
    return True


def frame_valid(event, frame, item, cell, base):
    row = event.rows[frame]
    snap = row["snapshot"]
    cells = snap.get("reelDisplay", {}).get("symbols", [])
    if snap.get("view") != "game" or len(cells) != 15 or cells[cell] != SYMBOL_NAMES.index(base):
        return False
    # Prize observation already proves the locked label in these source clips.
    # Other previews stay within the same genuinely recorded engine action.
    if not item.get("observed_prize") and snap.get("actions") != item["engine_action_count"]:
        return False
    if snap.get("animation") not in PHASES:
        if not (item.get("observed_prize") and snap.get("mode") == "hold"):
            return False
    sage = row.get("sage", {})
    if base == "confucius" and int(sage.get("gameJoinGeometryMask", 0)) & (1 << cell):
        return False
    if base in {"nine", "ten", "bell", "abacus", "book", "confucius", "wisdom"}:
        if not int(sage.get("winMask", 0)) & (1 << cell):
            return False
    return True


def select_cycle(event, item):
    symbol = item["symbol"]
    base = "phoenix" if symbol.startswith("prize-") else symbol
    period, evidence = PERIODS[base]
    current_cell = original_cell(item)
    cells = [current_cell] if item.get("observed_prize") else [current_cell] + [n for n in range(15) if n != current_cell]
    expanded = bool(item.get("crop_variant"))
    minimum = max(2, math.floor(period / FRAME_MS))
    maximum = max(150, minimum)
    best = None
    near = None
    settled = next((n for n, row in enumerate(event.rows)
                    if row["snapshot"].get("animation") != "spinning"), 0)
    for cell in cells:
        crop = cell_crop(cell, expanded)
        blobs = [crop_bytes(event, n, crop) for n in range(len(event.rows))]
        hashes = [digest(blob) for blob in blobs]
        valid = [frame_valid(event, n, item, cell, base) for n in range(len(event.rows))]
        if base in {"book", "wisdom", "confucius"}:
            lead = {"book": 1250, "wisdom": 2000, "confucius": 700}[base]
            valid = [ok and (n - settled) * FRAME_MS >= lead for n, ok in enumerate(valid)]
        stop = [len(valid)] * len(valid)
        end = len(valid)
        for n in range(len(valid) - 1, -1, -1):
            if not valid[n]:
                end = n
            stop[n] = end
        for start in range(len(valid)):
            if not valid[start] or not visible_digit(blobs[start], tuple(crop[2:]), base):
                continue
            for end in range(start + minimum, min(start + maximum, stop[start], len(valid) - 1) + 1):
                if len(set(hashes[start:end])) < 2:
                    continue
                duration = (end - start) * FRAME_MS
                period_error = abs(duration - max(1, round(duration / period)) * period)
                next_equal = sum(hashes[start + n] == hashes[end + n]
                                 for n in range(min(4, len(hashes) - end, end - start)))
                score = (period_error, cell != current_cell, -next_equal,
                         abs(start - item["source_frame"]), end - start)
                if hashes[start] == hashes[end]:
                    if best is None or score < best[0]:
                        best = score, start, end, crop, cell, hashes[start], hashes[end]
                elif period_error <= 20:
                    differences = sum(blobs[start][n:n + 2] != blobs[end][n:n + 2]
                                      for n in range(0, len(blobs[start]), 2))
                    near_score = (differences / (crop[2] * crop[3]), period_error,
                                  cell != current_cell, abs(start - item["source_frame"]))
                    if near is None or near_score < near[0]:
                        near = near_score, start, end, crop, cell, hashes[start], hashes[end]
    selected = best or near
    if selected is None:
        raise ValueError(f"No complete recorded cycle available: {item['file']}")
    _, start, end, crop, cell, first, continuation = selected
    return start, end, crop, cell, {
        "policy": "closed-recorded-cycle" if best else "recorded-cycle-replay",
        "nominal_animation_period_ms": period,
        "period_evidence": evidence,
        "first_crop_sha256": first,
        "actual_continuation_crop_sha256": continuation,
        "exact_source_continuation_match": best is not None,
        "boundary_hash_format": "native-rgb565le-crop-row-order",
        "boundary_pixel_difference_fraction": 0 if best else near[0][0],
        "first_frame_visible": True,
        "start_review_hold_ms": 0 if best else 200,
        "end_review_hold_ms": 0 if best else 800,
    }


def select_merge(event, item):
    start = item["source_frame"]
    # Both real merge recordings start with Expand. At6600ms the measured
    # first settled gold→blue→gold pulse has completed; keep that genuine gold
    # image as the terminal review image instead of truncating its return.
    end = min(len(event.rows), start + math.ceil(6600 / FRAME_MS) + 1)
    if event.rows[end - 1]["snapshot"].get("animation") != "ready":
        raise ValueError("Merge recording does not reach its complete settled state")
    crop = item["source_crop"]
    return start, end, crop, None, {
        "policy": "complete-recorded-merge-replay",
        "period_evidence": "generated_assets.cpp sageBlueAmount returns to gold at6600ms after complete join",
        "exact_source_continuation_match": False,
        "boundary_hash_format": "native-rgb565le-crop-row-order",
        "first_crop_sha256": digest(crop_bytes(event, start, crop)),
        "terminal_crop_sha256": digest(crop_bytes(event, end - 1, crop)),
        "start_review_hold_ms": 200,
        "end_review_hold_ms": 1400,
        "terminal_phase": event.rows[end - 1]["snapshot"]["animation"],
        "terminal_state": "completed join and complete first gold-blue-gold pulse",
    }


def artwork_loops(capture, output, index_path):
    """Export the actual header's combined4s rim/1.24s bird124s cycle."""
    root = Path(__file__).resolve().parents[1]
    gallery = Gallery(capture, output, "hud", no_hero=True)
    specs = list(map(json.loads, (capture / "events.ndjson").read_text().splitlines()))
    spec = next(row for row in specs if row["frame_count"] >= 3101)
    event = Event(capture, spec)
    try:
        start, end = 0, 3100
        if any(row["snapshot"].get("view") != "game" or
               row["snapshot"].get("mode") != "base" or
               row["snapshot"].get("animation") != "ready" for row in event.rows[start:end + 1]):
            raise ValueError("HUD source must be a continuously recorded ordinary ready board")
        context_file = "screens/hud/ready-context.png"
        gallery.save_png(context_file, event.image(start).resize((720, 405), Image.Resampling.NEAREST),
                         "screenshot", event.evidence(start), scale=3, role="hud-source-full-board")
        context = {"context_file": context_file, "context_parts": [context_file],
                   "context_frame_offset": 0, "context_time_offset_ms": 0,
                   "context_kind": "full-board-still-from-same-source-start"}
        for name, crop, scale in [("collection-meter", [188, 0, 52, 15], 6),
                                  ("header", [0, 0, 240, 14], 3)]:
            first, continuation = digest(crop_bytes(event, start, crop)), digest(crop_bytes(event, end, crop))
            if first != continuation:
                raise ValueError(f"HUD source does not exactly close: {name}")
            x, y, width, height = crop
            images = [event.image(frame).crop((x, y, x + width, y + height)).resize(
                (width * scale, height * scale), Image.Resampling.NEAREST) for frame in range(start, end)]
            relative = f"artwork/primary/{name}-motion.webp"
            images[0].save(gallery.path(relative), format="WEBP", save_all=True,
                           append_images=images[1:], duration=[FRAME_MS] * len(images),
                           loop=0, lossless=True, exact=True, method=4, quality=75)
            boundary = {"policy": "closed-recorded-cycle", "nominal_animation_period_ms": 124000,
                        "period_evidence": "LCM of counter_calm90.h4000ms rim and counter_hud.h1240ms bird; real124000ms pixel recurrence",
                        "first_crop_sha256": first, "actual_continuation_crop_sha256": continuation,
                        "exact_source_continuation_match": True,
                        "boundary_hash_format": "native-rgb565le-crop-row-order",
                        "boundary_pixel_difference_fraction": 0}
            gallery.register(relative, "symbol-animation", images[0], event.evidence(start, end),
                source_frames=end - start, source_crop=crop, scale=scale, loop_count=0, loop=0,
                replay_pause_ms=0, intro_hold_ms=0, declared_review_hold_ms=0,
                loop_policy="closed-recorded-cycle", loop_boundary=boundary,
                preview_for=f"artwork/primary/{name}.png", role="visible-artwork-motion",
                loop_fixer_sha256=sha(Path(__file__)), **context)
            gallery.save_png(f"artwork/primary/{name}.png", images[0], "symbol-crop", event.evidence(start),
                source_crop=crop, scale=scale, role="visible-artwork",
                loop_fixer_sha256=sha(Path(__file__)), **context)
            for image in images:
                image.close()
            print(json.dumps({"file": relative, "source_frames": [start, end],
                              "event": event.id, "policy": "closed-recorded-cycle"}), flush=True)
        receipt = capture / "capture-provenance.json"
        index = {"schema": 1, "kind": "actual-gameplay-gallery-conversion", "capture_id": "hud",
                 "converter_sha256": sha(Path(__file__)),
                 "processing": "Actual contiguous RGB565 crops only; nearest-neighbour enlargement; lossless encoding;40ms frames; exact124000ms combined cycle",
                 "source_provenance_sha256": sha(receipt),
                 "source_provenance": scrub_paths(json.loads(receipt.read_text()), root, capture),
                 "source_actions_journal_sha256": sha(capture / "actions.ndjson"),
                 "source_events_journal_sha256": sha(capture / "events.ndjson"),
                 "summary": json.loads((capture / "summary.json").read_text()),
                 "events": [{key: value for key, value in spec.items() if key not in {"raw_file", "frame_journal"}} | {
                     "session_id": "hud", "outputs": [context_file], "raw_sha256": event.raw_sha,
                     "frame_journal_sha256": event.journal_sha}],
                 "outputs": gallery.outputs, "still_aliases": [],
                 "coverage": {"artwork_motion": ["header", "collection-meter"]}}
        index_path.write_text(json.dumps(index, indent=2) + "\n")
        print(f"Wrote HUD index to {index_path}")
    finally:
        event.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--overrides", type=Path)
    parser.add_argument("--hud-capture", type=Path, help="Export only the separately captured124s actual HUD cycle")
    parser.add_argument("--hud-index", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    work = args.work.resolve()
    output = root / "docs/media/gameplay"
    if args.hud_capture:
        artwork_loops(args.hud_capture.resolve(), output,
                      args.hud_index or work / "gameplay-hud-index.json")
        return
    records, extras, summaries = [], [], []
    for session, folder in CAPTURES.items():
        index = json.loads((work / f"gameplay-{session}-index.json").read_text())
        capture = work / folder
        specs = {row["id"]: row for row in map(json.loads, (capture / "events.ndjson").read_text().splitlines())}
        gallery = Gallery(capture, output, session, no_hero=True)
        gallery.event_parts = {row["id"]: row["outputs"] for row in index["events"]}
        for item in index["outputs"]:
            if item["kind"] != "symbol-animation":
                continue
            if session == "prizes" and item["symbol"] == "candle":
                # This duplicate artwork's short prize-session crop contains an
                # unrelated neighbouring Phoenix. Use the already closed,
                # genuine Candle recording and retain its true source identity.
                original = next(record for record in records
                                if record.get("symbol") == "candle"
                                and record["loop_boundary"]["exact_source_continuation_match"])
                shutil.copyfile(output / original["file"], output / item["file"])
                record = {**original, "file": item["file"], "identical_to_output": original["file"]}
                records.append(record)
                summary = {"file": item["file"], "source_frames": [record["source_frame"],
                                                                    record["source_frame_end_exclusive"]],
                           "crop": record["source_crop"], "policy": record["loop_policy"],
                           "duration_ms": record["duration_ms"], "boundary_difference": 0,
                           "identical_to_output": original["file"]}
                summaries.append(summary)
                print(json.dumps(summary), flush=True)
                continue
            source_gallery = gallery
            source_capture = capture
            source_spec = specs[item["event_id"]]
            if item["file"] == "symbols/prizes/prize-major-1x-motion-full.webp":
                # The first occurrence has unrelated changing neighbours. This
                # other already-recorded Major1 cell has a clean full-wing loop.
                source_capture = work / CAPTURES["major"]
                major_index = json.loads((work / "gameplay-major-index.json").read_text())
                source_gallery = Gallery(source_capture, output, "major", no_hero=True)
                source_gallery.event_parts = {row["id"]: row["outputs"] for row in major_index["events"]}
                source_spec = next(row for row in map(json.loads, (source_capture / "events.ndjson").read_text().splitlines())
                                   if row["id"] == "action-061121-clip-027")
                item = {**item, "session_id": "major", "event_id": source_spec["id"],
                        "action_index": source_spec["action_index"], "engine_action_count": 61121,
                        "source_frame": 1220, "source_crop": [91, 47, 57, 43],
                        "nominal_source_crop": [91, 47, 57, 43], "screen_edge_clipped": False,
                        "observed_prize": {**item["observed_prize"], "cell_index": 7}}
                item.pop("source_prize_observations_sha256", None)
                item.pop("cell_crop_file", None)
            event = Event(source_capture, source_spec)
            try:
                if item["symbol"].startswith("confucius-"):
                    start, end, crop, cell, policy = select_merge(event, item)
                else:
                    start, end, crop, cell, policy = select_cycle(event, item)
                x, y, width, height = crop
                images = [event.image(n).crop((x, y, x + width, y + height)).resize(
                    (width * 6, height * 6), Image.Resampling.NEAREST) for n in range(start, end)]
                durations = [FRAME_MS] * len(images)
                durations[0] += policy["start_review_hold_ms"]
                durations[-1] += policy["end_review_hold_ms"]
                images[0].save(output / item["file"], format="WEBP", save_all=True,
                               append_images=images[1:], duration=durations, loop=0,
                               lossless=True, exact=True, method=4, quality=75)
                excluded = {"file", "kind", "dimensions", "sha256", "bytes", "frames", "duration_ms",
                            "source_frames", "source_duration_ms", "source_crop", "source_snapshot",
                            "source_sage_diagnostic", "source_award_diagnostic", "source_raw_sha256",
                            "source_frame_journal_sha256", "source_frame_bytes_sha256", "source_frame",
                            "source_frame_end_exclusive", "source_time_ms", "source_end_time_ms",
                            "context_file", "context_parts", "context_frame_offset", "context_time_offset_ms"}
                extra = {key: value for key, value in item.items() if key not in excluded}
                extra.update({"source_crop": crop, "loop": 0, "loop_count": 0, "loop_boundary": policy,
                              "loop_policy": policy["policy"],
                              "replay_pause_ms": policy["end_review_hold_ms"],
                              "intro_hold_ms": policy["start_review_hold_ms"],
                              "loop_fixer_sha256": sha(Path(__file__)),
                              "source_engine_actions": list(dict.fromkeys(
                                  row["snapshot"].get("actions") for row in event.rows[start:end])),
                              "declared_review_hold_ms": policy["start_review_hold_ms"] + policy["end_review_hold_ms"]})
                if cell is not None:
                    extra["cell"] = cell
                    if item.get("crop_variant"):
                        native_x, native_y = 2 + cell // 3 * 47, 16 + cell % 3 * 35
                        extra["nominal_source_crop"] = [native_x - 5, native_y - 4, 57, 43]
                        extra["screen_edge_clipped"] = crop != extra["nominal_source_crop"]
                        if cell != original_cell(item):
                            extra.pop("cell_crop_file", None)
                if item["file"] == "symbols/prizes/prize-major-1x-motion-full.webp":
                    source = root / "firmware/src/pdf_assets.cpp"
                    text = source.read_text()
                    rgb, alpha = [[int(n) for n in re.findall(r"\d+", re.search(
                        r"\bmajor1" + suffix + r"\[\]=\{(.*?)\};", text, re.S).group(1))]
                                  for suffix in ("Rgb", "Alpha")]
                    samples = [((51 + y) * 240 + 96 + x, rgb[y * 47 + x].to_bytes(2, "little"))
                               for y in range(3, 32) for x in range(4, 43)
                               if alpha[y * 47 + x] == 255 and rgb[y * 47 + x]]
                    if not all(event.buffer[frame * 64800 + at * 2:frame * 64800 + at * 2 + 2] == color
                               for frame in range(start, end) for at, color in samples):
                        raise ValueError("Alternative Major1 source does not retain its actual label")
                    extra["source_prize_label_verification"] = {
                        "source": "firmware/src/pdf_assets.cpp", "source_sha256": sha(source),
                        "opaque_interior_pixels_per_frame": len(samples), "all_source_frames_match": True,
                        "purpose": "Read-only exact opaque label-pixel classification; no template rendered"}
                record = source_gallery.register(item["file"], "symbol-animation", images[0],
                                          event.evidence(start, end), source_frames=end - start,
                                          **source_gallery.context(event.id, start, end), **extra)
                if record["duration_ms"] != sum(durations):
                    raise ValueError("Encoder changed the declared recorded-frame timing")
                records.append(record)
                if item["symbol"].startswith("confucius-"):
                    static_file = f"symbols/{session}/{item['symbol']}.png"
                    static_extra = {"symbol": item["symbol"], "source_crop": crop,
                                    "scale": 6, "role": "settled-merge-still",
                                    "loop_fixer_sha256": sha(Path(__file__))}
                    static = gallery.save_png(static_file, images[-1], "symbol-crop",
                                              event.evidence(end - 1),
                                              **gallery.context(event.id, end - 1), **static_extra)
                    extras.append(static)
                summary = {"file": item["file"], "source_frames": [start, end], "crop": crop,
                           "policy": policy["policy"], "duration_ms": record["duration_ms"],
                           "boundary_difference": policy.get("boundary_pixel_difference_fraction", "one-shot")}
                summaries.append(summary)
                print(json.dumps(summary), flush=True)
                for image in images:
                    image.close()
            finally:
                event.close()
    target = args.overrides or work / "gameplay-loop-symbols.json"
    target.write_text(json.dumps({"kind": "recorded-symbol-loop-overrides", "outputs": records,
                                  "extras": extras, "summary": summaries}, indent=2) + "\n")
    print(f"Wrote {len(records)} overrides to {target}")


if __name__ == "__main__":
    main()
