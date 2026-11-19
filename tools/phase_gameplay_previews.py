#!/usr/bin/env python3
"""Pair recorded full-screen stills with their actual moving presentation.

Only contiguous native frames are selected. One-shot presentations finish
before a declared pause holds their last real image for review and replay.
This tool writes previews and supplemental metadata, never source indices.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import json
from pathlib import Path
import shutil

from PIL import Image

from build_gameplay_gallery import Event, FRAME_MS, Gallery, sha
from complete_gameplay_replays import encode, visible_score


REVEAL_PHASES = {
    "spinning", "landing", "sage-expansion", "hold-fire", "return-fire",
    "phoenix-revival", "feature-end",
}


def presentation_key(row):
    snapshot = row["snapshot"]
    return (snapshot.get("view"), snapshot.get("mode"),
            snapshot.get("animation"), snapshot.get("actions"))


def select_range(event, frame):
    """Retain the entire selected phase; reveal transitions through ready."""
    rows = event.rows
    key = presentation_key(rows[frame])
    phase = rows[frame]["snapshot"]["animation"]
    phase_start, phase_end = frame, frame + 1
    while phase_start and presentation_key(rows[phase_start - 1]) == key:
        phase_start -= 1
    while phase_end < len(rows) and presentation_key(rows[phase_end]) == key:
        phase_end += 1
    start = phase_start if phase == "ready" else max(0, phase_start - 10)
    end = phase_end if phase == "ready" else min(len(rows), phase_end + 10)
    next_ready = None
    if phase in REVEAL_PHASES:
        for index in range(phase_end, len(rows)):
            snapshot = rows[index]["snapshot"]
            if snapshot.get("animation") == "ready":
                next_ready = index
                ready_key = presentation_key(rows[index])
                end = index + 1
                while end < min(len(rows), index + 10) and presentation_key(rows[end]) == ready_key:
                    end += 1
                break
    # A finished ready board may intentionally blink a numeric glyph. Choose
    # a real, fully visible end pose from its final available presentation.
    if end > frame + 1 and rows[end - 1]["snapshot"].get("animation") == "ready":
        tail = max(frame, start, end - 12, next_ready or phase_start)
        selected = max(range(tail, end), key=lambda i: (*visible_score(event, i), i))
        end = selected + 1
    return start, end, phase_start, phase_end, next_ready


def moving(event, start, end):
    first = event.frame_bytes(start)
    return any(event.frame_bytes(index) != first for index in range(start + 1, end))


def render_job(job):
    capture, output = Path(job["capture"]), Path(job["output"])
    session, spec = job["session"], job["spec"]
    event = Event(capture, spec)
    outputs, overrides = [], []
    try:
        start, end = job["start"], job["end"]
        pause_ms = job["pause_ms"]
        names = job["posters"]
        first_path = None
        gallery = Gallery(capture, output, session)
        for poster in names:
            name = f"previews/{session}/{Path(poster['file']).stem}.webp"
            path = output / name
            if first_path is None:
                encode(event, path, pause_ms=pause_ms, start=start, end=end)
                first_path = path
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(first_path, path)
            with Image.open(path) as image:
                record = gallery.register(
                    name, "animation", image, event.evidence(start, end),
                    source_frames=end - start, scale=3,
                    phase=poster["phase"], role="presentation-preview",
                    poster_file=poster["file"], preview_for=poster["file"],
                    poster_source_frame=poster["source_frame"],
                    poster_frame_offset=poster["source_frame"] - start,
                    presentation_source_frame=job["phase_start"],
                    presentation_end_exclusive=job["phase_end"],
                    next_ready_source_frame=job["next_ready"],
                    replay_pause_ms=pause_ms,
                    loop_policy="complete-recorded-presentation-with-replay-pause",
                    processing_tool="tools/phase_gameplay_previews.py",
                    processing_tool_sha256=sha(Path(__file__)),
                    encoder_tool="tools/complete_gameplay_replays.py",
                    encoder_tool_sha256=sha(Path(__file__).with_name("complete_gameplay_replays.py")),
                    context_file=f"sessions/{session}/{event.id}.webp",
                    context_parts=[f"sessions/{session}/{event.id}.webp"],
                    context_frame_offset=start, context_time_offset_ms=start * FRAME_MS,
                )
            outputs.append(record)
            overrides.append({"file": poster["file"], "session_id": session,
                              "preview_file": name, "preview_source_frame_offset":
                              poster["source_frame"] - start})
        return {"outputs": outputs, "still_overrides": overrides}
    finally:
        event.close()


def parse_session(value):
    session, capture, index = value.split(":", 2)
    return session, Path(capture).resolve(), Path(index).resolve()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", action="append", type=parse_session, required=True,
                        help="ID:CAPTURE_DIRECTORY:CONVERSION_INDEX")
    parser.add_argument("--extra-images", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--output-index", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=2)
    parser.add_argument("--pause-ms", type=int, default=1600)
    args = parser.parse_args()
    extra_data = json.loads(args.extra_images.read_text()) if args.extra_images else []
    extras = extra_data if isinstance(extra_data, list) else extra_data["outputs"]
    jobs, skipped, reused = {}, [], []
    for session, capture, index_path in args.session:
        index = json.loads(index_path.read_text())
        specs = {item["id"]: item for item in
                 (json.loads(line) for line in (capture / "events.ndjson").read_text().splitlines())}
        items = list(index["outputs"]) + [item for item in extras if item.get("session_id") == session]
        seen = set()
        for item in items:
            if item.get("kind") != "screenshot" or item.get("crop") or item.get("source_crop"):
                continue
            if item["file"] in seen:
                continue
            seen.add(item["file"])
            event_id = item["event_id"].removeprefix(session + ":")
            if event_id not in specs:
                skipped.append({"file": item["file"], "reason": "Actual UI still has no recorded moving interval"})
                continue
            event = Event(capture, specs[event_id])
            try:
                frame = item["source_frame"]
                if not 0 <= frame < len(event.rows):
                    raise ValueError("Screenshot source frame is outside event: " + item["file"])
                snapshot = event.rows[frame]["snapshot"]
                start, end, phase_start, phase_end, next_ready = select_range(event, frame)
                if not moving(event, start, end):
                    skipped.append({"file": item["file"], "reason": "Recorded presentation is static; no animation invented"})
                    continue
                key = session, event_id, start, end
                poster = {"file": item["file"], "source_frame": frame,
                          "phase": snapshot["animation"]}
                if key not in jobs:
                    jobs[key] = {"session": session, "capture": str(capture),
                        "output": str(args.output.resolve()), "spec": specs[event_id],
                        "start": start, "end": end, "phase_start": phase_start,
                        "phase_end": phase_end, "next_ready": next_ready,
                        "pause_ms": args.pause_ms, "posters": []}
                jobs[key]["posters"].append(poster)
            finally:
                event.close()
    outputs, overrides = [], []
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        pending = {pool.submit(render_job, job): job for job in jobs.values()}
        for future in as_completed(pending):
            result = future.result()
            outputs.extend(result["outputs"])
            overrides.extend(result["still_overrides"])
            job = pending[future]
            print(json.dumps({"session": job["session"], "event": job["spec"]["id"],
                              "start": job["start"], "end": job["end"],
                              "posters": [p["file"] for p in job["posters"]]}), flush=True)
    result = {"kind": "recorded-presentation-previews", "outputs": sorted(outputs, key=lambda i: i["file"]),
              "still_overrides": sorted(overrides, key=lambda i: i["file"]),
              "skipped": skipped, "reused": reused,
              "processing": "Contiguous actual frames at 40ms; whole presentation phases and available real reveal; declared final replay pause"}
    args.output_index.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
