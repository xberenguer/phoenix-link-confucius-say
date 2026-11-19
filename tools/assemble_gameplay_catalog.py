#!/usr/bin/env python3
"""Assemble recorded-play media catalogs from converted real capture sessions.

Run the recorder and converter first, retaining their complete capture folders.
Repeat --session ID:CAPTURE_DIRECTORY:CONVERSION_INDEX for each session.
--extra-images accepts a JSON list (or {"outputs": [...]}) of additional crops
using the same output metadata schema. This tool never renders or edits pixels.
It publishes only the full-log rows that anchor the selected captures, together
with the hash and row count of each retained full input journal.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys

from verify_gameplay_gallery import image_info, sha


def read_json(path):
    return json.loads(path.read_text())


def game_path(name):
    """Normalize a converter-relative output to a docs-relative media path."""
    path = Path(name)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError("Media path must be relative and contained")
    value = path.as_posix()
    if value.startswith("docs/"):
        value = value[5:]
    if not value.startswith("media/gameplay/"):
        value = "media/gameplay/" + value
    return value


def public_data(value):
    """Keep local capture paths and build commands out of public evidence."""
    if isinstance(value, str):
        private = ["/" + "Users" + "/", "/private/" + "var/folders/",
                   "codex-" + "clipboard-", ".codex/" + "attachments/"]
        if any(marker in value for marker in private):
            raise ValueError("Private local path in output metadata")
    elif isinstance(value, dict):
        for item in value.values():
            public_data(item)
    elif isinstance(value, list):
        for item in value:
            public_data(item)
    return value


def recorder_identity(root, provenance):
    expected = provenance["recorder_cpp_sha256"]
    candidates = []
    for item in provenance.get("build_command", []):
        if not isinstance(item, str) or not item.endswith(".cpp"):
            continue
        path = Path(item).resolve()
        if path.is_relative_to(root / "tools") and path.is_file():
            candidates.append(path)
    matches = [path for path in candidates if sha(path) == expected]
    if len(set(matches)) != 1:
        raise ValueError("Cannot bind capture receipt to one unchanged recorder")
    recorder = matches[0].relative_to(root).as_posix()
    dependencies = copy.deepcopy(provenance.get("recorder_dependencies_sha256", {}))
    dependencies[recorder] = expected
    for name, expected_hash in dependencies.items():
        path = (root / name).resolve()
        if not path.is_relative_to(root / "tools") or not path.is_file() or sha(path) != expected_hash:
            raise ValueError("Recorder dependency changed: " + name)
    return recorder, expected, dependencies


def event_snapshot(event):
    value = event.get("source_snapshot", event.get("snapshot_start"))
    if isinstance(value, str):
        value = json.loads(value)
    if not isinstance(value, dict) or "actions" not in value:
        raise ValueError("Event lacks its actual runtime snapshot")
    return value


def collect_actions(path, events):
    """Stream even very large journals; retain only real candidate anchor rows."""
    required = {event["action_index"] for event in events.values()}
    by_engine_action = {}
    for event_id, event in events.items():
        count = event_snapshot(event)["actions"]
        by_engine_action.setdefault(count, []).append((event_id, event.get("start_ms")))
    records = {}
    candidates = {}
    digest = hashlib.sha256()
    count = maximum = max_engine = 0
    last = -1
    index_pattern = re.compile(rb'"action_index"\s*:\s*(\d+)')
    engine_pattern = re.compile(rb'"engine_action_count"\s*:\s*(\d+)')
    time_pattern = re.compile(rb'"time_ms"\s*:\s*(\d+)')
    with path.open("rb") as stream:
        for line in stream:
            digest.update(line)
            if not line.strip():
                continue
            prefix = line[:320]
            found = index_pattern.search(prefix)
            if found is None:
                raise ValueError("Unrecognized source action journal row")
            index = int(found[1])
            if index <= last:
                raise ValueError("Source action journal is not strictly increasing")
            last = index
            count += 1
            maximum = max(maximum, index)
            engine_found = engine_pattern.search(prefix)
            engine = int(engine_found[1]) if engine_found else None
            if engine is not None:
                max_engine = max(max_engine, engine)
            time_found = time_pattern.search(prefix)
            time_ms = int(time_found[1]) if time_found else None
            parsed = None
            if index in required:
                parsed = json.loads(line)
                records[index] = parsed
            for event_id, start_ms in by_engine_action.get(engine, []):
                if start_ms is not None and time_ms is not None and time_ms <= start_ms:
                    # The latest actual log row before the capture, with the
                    # same committed Engine action, is a truthful fallback.
                    if parsed is None:
                        parsed = json.loads(line)
                    candidates[event_id] = parsed
    return records, candidates, {"sha256": digest.hexdigest(), "rows": count,
                                  "max_action_index": maximum, "max_engine_actions": max_engine}


def assemble(root, session_arguments, extras):
    docs = root / "docs"
    release = read_json(root / "RELEASE.json")
    source = root / "firmware/src"
    pins = {p.relative_to(source).as_posix(): sha(p)
            for p in sorted(source.rglob("*")) if p.is_file()}
    assets_root = root / "firmware/data"
    asset_pins = {p.relative_to(assets_root).as_posix(): sha(p)
                  for p in sorted(assets_root.rglob("*")) if p.is_file()}
    images, events, journal_rows, sessions = [], [], [], []
    file_names, session_ids = set(), set()
    offset = 0
    for session_id, capture, index_path in session_arguments:
        if not session_id or session_id in session_ids:
            raise ValueError("Duplicate/empty session id")
        session_ids.add(session_id)
        capture, index_path = capture.resolve(), index_path.resolve()
        index = read_json(index_path)
        provenance_path = capture / "capture-provenance.json"
        provenance = read_json(provenance_path)
        if index.get("capture_id") != session_id:
            raise ValueError("Conversion index session id differs: " + session_id)
        if index.get("source_provenance_sha256") != sha(provenance_path):
            raise ValueError("Conversion index capture receipt differs: " + session_id)
        if provenance["source_sha256"] != pins or provenance["runtime_asset_sha256"] != asset_pins:
            raise ValueError("Capture did not use the unchanged source/assets: " + session_id)
        recorder, recorder_hash, dependencies = recorder_identity(root, provenance)
        outputs = copy.deepcopy(index["outputs"])
        outputs += [copy.deepcopy(item) for item in extras if item.get("session_id") == session_id]
        local_events = {event["id"]: copy.deepcopy(event) for event in index.get("events", [])}
        # Menu stills and components are captured through the same real UI.
        # Their metadata supplies the runtime snapshot without inventing a
        # reel result or inserting a synthetic input into the source journal.
        for item in outputs:
            event_id = item["event_id"]
            if event_id not in local_events:
                local_events[event_id] = {"id": event_id, "kind": "actual-ui-capture",
                    "action_index": item["action_index"],
                    "engine_action_count": item.get("engine_action_count"),
                    "start_ms": item.get("source_time_ms"),
                    "source_snapshot": item["source_snapshot"],
                    "source_raw_sha256": item.get("source_raw_sha256")}
        # An index may retain unused clips for conversion coverage. Publish
        # event anchors only when an actual output references that event.
        used_events = {item["event_id"] for item in outputs}
        local_events = {key: event for key, event in local_events.items() if key in used_events}
        records, candidates, full_log = collect_actions(capture / "actions.ndjson", local_events)
        if full_log["sha256"] != index.get("source_actions_journal_sha256"):
            raise ValueError("Full source action journal differs from conversion: " + session_id)
        source_events_hash = sha(capture / "events.ndjson")
        if source_events_hash != index.get("source_events_journal_sha256"):
            raise ValueError("Source event journal differs from conversion: " + session_id)
        anchor_rows = {}
        normalized_events = {}
        for event_id, event in local_events.items():
            snapshot = event_snapshot(event)
            recorded_index = event["action_index"]
            row = records.get(recorded_index)
            if row is None or row.get("engine_action_count") != snapshot["actions"]:
                row = candidates.get(event_id)
                if row is None or row.get("engine_action_count") != snapshot["actions"]:
                    raise ValueError("No truthful input anchor for event " + session_id + ":" + event_id)
            original_index = row["action_index"]
            anchor_rows[original_index] = row
            event.pop("raw_file", None)
            event.pop("frame_journal", None)
            event.pop("outputs", None)
            event.pop("snapshot_start", None)
            event["source_snapshot"] = snapshot
            event["session_id"] = session_id
            event["id"] = session_id + ":" + event_id
            event["original_action_index"] = original_index
            event["action_index"] = offset + original_index
            if original_index != recorded_index:
                event["recorded_event_action_index"] = recorded_index
                event["anchor_resolution"] = "Latest actual source-journal row before capture with matching Engine action"
            events.append(event)
            normalized_events[event_id] = event
        for original_index, row in sorted(anchor_rows.items()):
            published = copy.deepcopy(row)
            published["session_id"] = session_id
            published["original_action_index"] = original_index
            published["action_index"] = offset + original_index
            journal_rows.append(published)
        for item in outputs:
            item["file"] = game_path(item["file"])
            if item["file"] in file_names:
                raise ValueError("Duplicate output media path: " + item["file"])
            file_names.add(item["file"])
            path = (docs / item["file"]).resolve()
            if not path.is_relative_to(docs / "media/gameplay") or not path.is_file():
                raise ValueError("Missing/unsafe converted media: " + item["file"])
            info = image_info(path)
            if item["sha256"] != sha(path) or item["dimensions"] != info["dimensions"] or \
                    item["frames"] != info["frames"] or item["duration_ms"] != info["duration_ms"]:
                raise ValueError("Converted media content changed: " + item["file"])
            event = normalized_events[item["event_id"]]
            item["source_version"] = release["version"]
            item["original_action_index"] = event["original_action_index"]
            item["action_index"] = event["action_index"]
            item["event_id"] = event["id"]
            item["session_id"] = session_id
            if "source_crop" in item:
                item["crop"] = item.pop("source_crop")
            for path_field in ("context_file", "preview_for", "preview_file", "poster_file", "poster_for"):
                if path_field in item:
                    item[path_field] = game_path(item[path_field])
            if "same_recorded_frames_as" in item:
                item["same_recorded_frames_as"] = game_path(item["same_recorded_frames_as"])
            if "context_parts" in item:
                item["context_parts"] = [game_path(name) for name in item["context_parts"]]
            images.append(item)
        sessions.append({"id": session_id, "recorder_file": recorder,
            "recorder_sha256": recorder_hash, "recorder_files_sha256": dependencies,
            "source_files_sha256": pins, "runtime_assets_sha256": asset_pins,
            "source_provenance_sha256": sha(provenance_path),
            "conversion_index_sha256": sha(index_path),
            "source_events_journal_sha256": source_events_hash,
            "full_journal_sha256": full_log["sha256"],
            "full_journal_actions": full_log["rows"],
            "full_journal_max_action_index": full_log["max_action_index"],
            "engine_actions": full_log["max_engine_actions"],
            "published_anchor_actions": len(anchor_rows), "global_action_offset": offset,
            "scope": provenance.get("scope"), "coverage": index.get("coverage", {})})
        offset += full_log["max_action_index"] + 1
    if any(item.get("session_id") not in session_ids for item in extras):
        raise ValueError("Extra image references an undeclared capture session")
    journal_path = docs / "data/gameplay-actions.jsonl"
    journal_text = "".join(json.dumps(public_data(row), separators=(",", ":")) + "\n"
                           for row in journal_rows)
    journal_path.write_text(journal_text)
    primary_recorder = "tools/capture_gameplay.cpp"
    manifest = {"kind": "recorded-gameplay", "version": release["version"],
        "runtime": release["runtime_fingerprint"], "source_files_sha256": pins,
        "capture": {"method": "public-ui-buttons", "rng": "production-os-entropy",
            "virtual_clock": True, "source_unchanged": True, "frame_rate": 25,
            "audio": "Silent native host adapter; animation clocks remain recorded",
            "journal_scope": "capture-anchors", "journal_file": "data/gameplay-actions.jsonl",
            "journal_sha256": sha(journal_path), "recorder_file": primary_recorder,
            "recorder_sha256": sha(root / primary_recorder), "sessions": sessions},
        "processing": "Recorded pixels only; rectangular crops; exact integer nearest-neighbour enlargement; lossless PNG/WebP. Frame order and recorded durations are preserved. Clips are selected excerpts, not probability samples.",
        "assembly": {"tool": "tools/assemble_gameplay_catalog.py", "sha256": sha(Path(__file__))},
        "images": images, "events": events}
    public_data(manifest)
    (docs / "gameplay-media.json").write_text(json.dumps(manifest, indent=2) + "\n")
    gallery_path = docs / "gallery-manifest.json"
    gallery = read_json(gallery_path)
    gallery.pop("renderer_fixture_sha256", None)
    gallery["kind"] = "recorded-gameplay-summary"
    gallery["capture_manifest"] = "gameplay-media.json"
    gallery["capture_manifest_sha256"] = sha(docs / "gameplay-media.json")
    summary_fields = {"file", "dimensions", "sha256", "source_version", "kind", "scale",
                      "crop", "frames", "duration_ms", "event_id", "action_index", "session_id"}
    gallery["images"] = [{key: value for key, value in item.items() if key in summary_fields}
                         for item in images]
    gallery_path.write_text(json.dumps(public_data(gallery), indent=2) + "\n")
    return {"sessions": len(sessions), "images": len(images), "events": len(events),
            "public_anchor_actions": len(journal_rows),
            "retained_full_journal_actions": sum(item["full_journal_actions"] for item in sessions)}


def session_argument(value):
    parts = value.split(":", 2)
    if len(parts) != 3 or not all(parts):
        raise argparse.ArgumentTypeError("Expected ID:CAPTURE_DIRECTORY:CONVERSION_INDEX")
    return parts[0], Path(parts[1]), Path(parts[2])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--session", action="append", type=session_argument, required=True)
    parser.add_argument("--extra-images", type=Path)
    args = parser.parse_args()
    extras = read_json(args.extra_images) if args.extra_images else []
    if isinstance(extras, dict):
        extras = extras["outputs"]
    if not isinstance(extras, list):
        parser.error("--extra-images must contain a list or an outputs list")
    try:
        print(json.dumps(assemble(args.root.resolve(), args.session, extras), indent=2))
    except (KeyError, OSError, ValueError) as exc:
        print("Catalog assembly failed: " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
