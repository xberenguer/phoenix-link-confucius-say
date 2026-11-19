#!/usr/bin/env python3
"""Validate the recorded-play gallery, its source binding and capture journal.

This checks the published evidence chain and media files. It does not establish
original-casino frequencies or prove that every possible event was captured.
The recorder itself and the selected footage still need human review.
"""
import argparse
import hashlib
import html
import json
from pathlib import Path
import re
import struct
import sys
from urllib.parse import unquote, urlsplit


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def image_info(path):
    data = path.read_bytes()
    if data.startswith(b"\x89PNG\r\n\x1a\n"):
        return {"dimensions": list(struct.unpack(">II", data[16:24])),
                "frames": 1, "duration_ms": 0}
    if data[:4] == b"RIFF" and data[8:12] == b"WEBP":
        offset = 12
        dimensions = None
        frames = duration = 0
        loop_count = None
        while offset + 8 <= len(data):
            kind = data[offset:offset + 4]
            size = int.from_bytes(data[offset + 4:offset + 8], "little")
            payload = data[offset + 8:offset + 8 + size]
            if len(payload) != size:
                raise ValueError("Truncated WebP chunk")
            if kind == b"VP8X" and len(payload) >= 10:
                dimensions = [1 + int.from_bytes(payload[4:7], "little"),
                              1 + int.from_bytes(payload[7:10], "little")]
            elif kind == b"VP8L" and len(payload) >= 5 and payload[0] == 47:
                packed = int.from_bytes(payload[1:5], "little")
                dimensions = [1 + (packed & 16383),
                              1 + ((packed >> 14) & 16383)]
            elif kind == b"ANIM" and len(payload) >= 6:
                loop_count = int.from_bytes(payload[4:6], "little")
            elif kind == b"ANMF" and len(payload) >= 16:
                frames += 1
                duration += int.from_bytes(payload[12:15], "little")
            offset += 8 + size + (size & 1)
        if dimensions:
            return {"dimensions": dimensions, "frames": frames or 1,
                    "duration_ms": duration, "loop_count": loop_count}
    raise ValueError("Expected a PNG or lossless/extended WebP")


def public_markdown(root):
    for path in sorted(root.rglob("*.md")):
        parts = path.relative_to(root).parts
        if any(part in {".git", ".pio", "__pycache__", "node_modules"}
               for part in parts):
            continue
        if parts[:2] in {("browser", "build"), ("browser", "dist"),
                         ("firmware", "release")}:
            continue
        yield path


def image_targets(text):
    text = re.sub(r"```.*?```", "", text, flags=re.S)
    for target in re.findall(
            r'!\[[^\]]*\]\(([^)]+)\)|<img\b[^>]*src="([^"]+)"', text):
        yield html.unescape(next(part for part in target if part)).strip("<>")


def verify(root):
    root = root.resolve()
    docs = root / "docs"
    errors = []
    checks = 0

    def require(condition, message):
        nonlocal checks
        checks += 1
        if not condition:
            errors.append(message)
        return bool(condition)

    def local_file(base, name, folder, label):
        if not isinstance(name, str):
            require(False, label + " path is missing")
            return None
        path = (base / name).resolve()
        if not require(path.is_relative_to(folder.resolve()) and path.is_file(),
                       "Missing/unsafe " + label + ": " + name):
            return None
        return path

    manifest_path = docs / "gameplay-media.json"
    if not require(manifest_path.is_file(), "Missing docs/gameplay-media.json"):
        return {"status": "FAIL", "checks": checks, "errors": errors}
    try:
        manifest = json.loads(manifest_path.read_text())
        release = json.loads((root / "RELEASE.json").read_text())
    except (OSError, ValueError) as exc:
        require(False, "Cannot read gallery/release metadata: " + str(exc))
        return {"status": "FAIL", "checks": checks, "errors": errors}
    require(manifest.get("kind") == "recorded-gameplay", "Gallery is not recorded-play evidence")
    require(manifest.get("version") == release["version"], "Recorded gallery version is stale")
    require(manifest.get("runtime") == release["runtime_fingerprint"], "Recorded gallery runtime is stale")
    source = root / "firmware/src"
    pins = {p.relative_to(source).as_posix(): sha(p)
            for p in sorted(source.rglob("*")) if p.is_file()}
    require(manifest.get("source_files_sha256") == pins,
            "Recorded gallery does not bind the unchanged current source")
    capture = manifest.get("capture", {})
    require(capture.get("method") == "public-ui-buttons", "Capture did not use public UI buttons")
    require(capture.get("rng") == "production-os-entropy", "Capture random source is not production OS entropy")
    require(capture.get("source_unchanged") is True, "Capture does not attest unchanged game source")
    require(isinstance(capture.get("virtual_clock"), bool), "Capture clock scope is missing")
    require(isinstance(capture.get("frame_rate"), (int, float)) and
            0 < capture["frame_rate"] <= 120, "Invalid capture frame rate")
    journal_scope = capture.get("journal_scope", "full")
    require(journal_scope in {"full", "capture-anchors"}, "Unknown capture journal scope")
    recorder = local_file(root, capture.get("recorder_file"), root / "tools", "recorder")
    if recorder:
        require(sha(recorder) == capture.get("recorder_sha256"), "Recorder hash differs from capture evidence")
    sessions = {}
    runtime_assets = root / "firmware/data"
    asset_pins = {p.relative_to(runtime_assets).as_posix(): sha(p)
                  for p in sorted(runtime_assets.rglob("*")) if p.is_file()}
    session_entries = capture.get("sessions", [])
    if not isinstance(session_entries, list):
        require(False, "Capture sessions must be a list")
        session_entries = []
    for session in session_entries:
        session_id = session.get("id")
        if not require(isinstance(session_id, str) and bool(session_id) and session_id not in sessions,
                       "Missing/duplicate capture session id: " + str(session_id)):
            continue
        sessions[session_id] = session
        session_recorder = local_file(root, session.get("recorder_file"), root / "tools",
                                      "session recorder " + session_id)
        if session_recorder:
            require(sha(session_recorder) == session.get("recorder_sha256"),
                    "Session recorder hash differs: " + session_id)
        require(session.get("source_files_sha256") == pins,
                "Session does not bind the unchanged current source: " + session_id)
        require(session.get("runtime_assets_sha256") == asset_pins,
                "Session runtime artwork/audio asset binding differs: " + session_id)
        if journal_scope == "capture-anchors":
            require(isinstance(session.get("full_journal_sha256"), str) and
                    re.fullmatch(r"[0-9a-f]{64}", session["full_journal_sha256"]),
                    "Missing retained full-journal hash: " + session_id)
            require(type(session.get("full_journal_actions")) is int and
                    session["full_journal_actions"] > 0,
                    "Missing retained full-journal row count: " + session_id)
        for key in ("method", "rng", "source_unchanged", "virtual_clock", "frame_rate"):
            if key in session:
                require(session[key] == capture.get(key),
                        "Session capture policy differs for " + key + ": " + session_id)
        # A thin observational wrapper can include the primary recorder. Bind
        # any additional recorder/adapter files explicitly when supplied.
        for name, expected_hash in session.get("recorder_files_sha256", {}).items():
            dependency = local_file(root, name, root / "tools",
                                    "session recorder dependency " + session_id)
            if dependency:
                require(sha(dependency) == expected_hash,
                        "Session recorder dependency hash differs: " + name)
    journal = local_file(docs, capture.get("journal_file"), docs / "data", "capture journal")
    action_records = {}
    session_previous_index = {}
    session_anchor_counts = {}
    if journal_scope == "capture-anchors":
        require(bool(sessions), "Capture-anchor journal requires session evidence")
    if journal:
        require(sha(journal) == capture.get("journal_sha256"), "Capture journal hash mismatch")
        previous_index = -1
        for number, line in enumerate(journal.read_text().splitlines(), 1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
            except ValueError:
                require(False, "Malformed action journal row " + str(number))
                continue
            index = record.get("action_index")
            if not require(type(index) is int and index >= 0 and index > previous_index,
                           "Action indices must increase: journal row " + str(number)):
                continue
            require(isinstance(record.get("input"), str) and bool(record["input"]),
                    "Missing public UI input: journal row " + str(number))
            if sessions:
                session_id = record.get("session_id")
                require(session_id in sessions,
                        "Journal action has no capture session: row " + str(number))
                original_index = record.get("original_action_index")
                if require(type(original_index) is int and original_index >= 0 and
                           original_index > session_previous_index.get(session_id, -1),
                           "Original input indices must increase within each session: row " + str(number)):
                    session_previous_index[session_id] = original_index
                session_anchor_counts[session_id] = session_anchor_counts.get(session_id, 0) + 1
                session = sessions.get(session_id, {})
                if "global_action_offset" in session and type(original_index) is int:
                    require(index - original_index == session["global_action_offset"],
                            "Global input offset differs from session binding: row " + str(number))
            previous_index = index
            action_records[index] = record
        require(bool(action_records), "Capture journal contains no UI actions")
        if sessions:
            require(set(session_previous_index) == set(sessions),
                    "Capture session has no journal actions")
            for session_id, session in sessions.items():
                anchors = session_anchor_counts.get(session_id, 0)
                if "published_anchor_actions" in session:
                    require(session["published_anchor_actions"] == anchors,
                            "Published session anchor count differs: " + session_id)
                if journal_scope == "capture-anchors" and type(session.get("full_journal_actions")) is int:
                    require(session["full_journal_actions"] >= anchors,
                            "Full journal is smaller than its published anchors: " + session_id)
                if "full_journal_max_action_index" in session:
                    require(session["full_journal_max_action_index"] >= session_previous_index.get(session_id, 0),
                            "Published anchor exceeds original journal range: " + session_id)
    assembly = manifest.get("assembly")
    if assembly is not None:
        assembly_tool = local_file(root, assembly.get("tool"), root / "tools", "catalog assembly tool")
        if assembly_tool:
            require(sha(assembly_tool) == assembly.get("sha256"), "Catalog assembly tool hash mismatch")
    events = {}
    for event in manifest.get("events", []):
        event_id = event.get("id")
        if not require(isinstance(event_id, str) and bool(event_id) and event_id not in events,
                       "Missing/duplicate captured event id: " + str(event_id)):
            continue
        require(event.get("action_index") in action_records,
                "Event has no journal action: " + event_id)
        require(isinstance(event.get("kind"), str) and bool(event["kind"]),
                "Event has no kind: " + event_id)
        snapshot = event.get("source_snapshot")
        if isinstance(snapshot, str):
            try:
                snapshot = json.loads(snapshot)
            except ValueError:
                snapshot = None
        valid_snapshot = (isinstance(snapshot, dict) and
                          isinstance(snapshot.get("grid"), list) and
                          len(snapshot["grid"]) == 15 and
                          all(type(symbol) is int and 0 <= symbol < 12
                              for symbol in snapshot["grid"]) and
                          type(snapshot.get("actions")) is int and
                          snapshot["actions"] >= 0 and
                          snapshot.get("mode") in {"base", "free", "hold"} and
                          isinstance(snapshot.get("view"), str) and
                          isinstance(snapshot.get("animation"), str))
        require(valid_snapshot, "Event has no valid runtime UI snapshot: " + event_id)
        action_record = action_records.get(event.get("action_index"), {})
        if sessions:
            session_id = event.get("session_id")
            require(session_id in sessions and session_id == action_record.get("session_id"),
                    "Event capture session differs from journal: " + event_id)
            if "original_action_index" in event:
                require(event["original_action_index"] == action_record.get("original_action_index"),
                        "Event original input index differs from journal: " + event_id)
        if valid_snapshot and "engine_action_count" in action_record:
            require(snapshot["actions"] == action_record["engine_action_count"],
                    "Event Engine action differs from journal: " + event_id)
        events[event_id] = event
    require(bool(events), "Gallery has no recorded events")
    catalog = {}
    image_count = animation_count = 0
    for item in manifest.get("images", []):
        name = item.get("file")
        path = local_file(docs, name, docs / "media/gameplay", "recorded image")
        if path is None:
            continue
        require(path not in catalog, "Duplicate recorded image: " + name)
        catalog[path] = item
        require(sha(path) == item.get("sha256"), "Recorded image hash mismatch: " + name)
        require(item.get("source_version") == manifest.get("version"), "Image version differs: " + name)
        event = events.get(item.get("event_id"))
        require(event is not None and event.get("action_index") == item.get("action_index"),
                "Image has no matching recorded event/action: " + name)
        kind = item.get("kind")
        require(kind in {"screenshot", "animation", "symbol-crop", "symbol-animation"},
                "Unknown recorded image kind: " + name)
        scale = item.get("scale")
        if require(type(scale) is int and scale > 0, "Invalid integer scaling: " + name):
            crop = item.get("crop")
            if kind in {"symbol-crop", "symbol-animation"}:
                good_crop = (isinstance(crop, list) and len(crop) == 4 and
                             all(type(v) is int for v in crop) and
                             crop[0] >= 0 and crop[1] >= 0 and crop[2] > 0 and crop[3] > 0 and
                             crop[0] + crop[2] <= 240 and crop[1] + crop[3] <= 135)
                if require(good_crop, "Invalid native-frame crop: " + name):
                    require(item.get("dimensions") == [crop[2] * scale, crop[3] * scale],
                            "Crop dimensions are not exact integer scaling: " + name)
            else:
                require(crop is None, "Full gameplay frame must not hide a crop: " + name)
                require(item.get("dimensions") == [240 * scale, 135 * scale],
                        "Gameplay dimensions are not exact integer scaling: " + name)
        frame_hash = item.get("source_frame_sha256")
        if frame_hash is not None:
            require(isinstance(frame_hash, str) and re.fullmatch(r"[0-9a-f]{64}", frame_hash),
                    "Invalid source-frame hash: " + name)
        try:
            info = image_info(path)
            require(info["dimensions"] == item.get("dimensions"), "Encoded image dimensions differ: " + name)
            require(info["frames"] == item.get("frames"), "Encoded image frame count differs: " + name)
            require(info["duration_ms"] == item.get("duration_ms"), "Encoded image duration differs: " + name)
            is_animation = kind in {"animation", "symbol-animation"}
            require((info["frames"] > 1 and info["duration_ms"] > 0) if is_animation else
                    (info["frames"] == 1 and info["duration_ms"] == 0),
                    "Image/animation evidence kind differs from content: " + name)
            if is_animation:
                require(info["loop_count"] == 0, "Animation must repeat indefinitely: " + name)
                require(bool(item.get("loop_policy")), "Animation has no reviewed replay boundary: " + name)
                source_frames = item.get("source_frames")
                require(type(source_frames) is int and source_frames > 1,
                        "Animation has no recorded frame span: " + name)
                if type(source_frames) is int:
                    require(item.get("source_frame_end_exclusive", 0) - item.get("source_frame", 0) == source_frames,
                            "Recorded animation interval differs from its frame count: " + name)
                    require(item.get("source_duration_ms") == source_frames * 40,
                            "Recorded animation timing differs from 25 fps: " + name)
                    holds = [item.get("intro_hold_ms", 0), item.get("replay_pause_ms", 0)]
                    valid_holds = all(type(value) is int and 0 <= value <= 10000 for value in holds)
                    require(valid_holds, "Invalid declared review pause: " + name)
                    if valid_holds:
                        require(info["duration_ms"] == source_frames * 40 + sum(holds),
                                "Animation playback timing differs from recorded frames and declared pauses: " + name)
                boundary = item.get("loop_boundary", {})
                if boundary.get("exact_source_continuation_match"):
                    require(boundary.get("first_crop_sha256") == boundary.get("actual_continuation_crop_sha256"),
                            "Seamless-loop boundary does not match the actual continuation: " + name)
                    require(boundary.get("boundary_pixel_difference_fraction") == 0,
                            "Seamless loop has a changed boundary: " + name)
            tool_name = item.get("processing_tool")
            if tool_name:
                tool_path = local_file(root, tool_name, root / "tools", "media processing tool")
                if tool_path:
                    require(sha(tool_path) == item.get("processing_tool_sha256"),
                            "Media processing tool hash differs: " + name)
            animation_count += int(is_animation)
        except (OSError, ValueError, struct.error) as exc:
            require(False, "Cannot inspect recorded image " + name + ": " + str(exc))
        image_count += 1
    require(bool(catalog), "Gallery has no captured images")
    for path in (docs / "media/gameplay").rglob("*"):
        if path.is_file():
            require(path.resolve() in catalog, "Uncatalogued recorded media: " + path.relative_to(docs).as_posix())
    references = 0
    for document in public_markdown(root):
        # The changelog is historical release documentation, not the current
        # player gallery. Current README/guide pages must show recorded media.
        if document.relative_to(root).as_posix() == "CHANGELOG.md":
            continue
        for target in image_targets(document.read_text()):
            parsed = urlsplit(target)
            label = document.relative_to(root).as_posix() + ": " + target
            if parsed.scheme or parsed.netloc:
                require(False, "Uncatalogued remote documentation image: " + label)
                continue
            path = (document.parent / unquote(parsed.path)).resolve()
            require(path in catalog, "Current documentation embeds unrecorded/fixture media: " + label)
            references += 1
    return {"status": "FAIL" if errors else "PASS", "version": manifest.get("version"),
            "checks": checks, "sessions": len(sessions),
            "journal_actions": len(action_records), "events": len(events),
            "images": image_count, "animations": animation_count,
            "image_references": references, "errors": errors}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    result = verify(args.root)
    print(json.dumps(result, indent=2))
    sys.exit(1 if result["errors"] else 0)
