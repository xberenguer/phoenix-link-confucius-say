#!/usr/bin/env python3
"""Convert recorded, unchanged-game play to traceable gallery media.

Requires Pillow with WebP support. No game state, artwork, outcome or frame is
generated here. The only image operations are decoding native RGB565 pixels,
rectangular cropping, nearest-neighbour enlargement and lossless encoding.
Every animation is a contiguous excerpt of one recorded stream at its recorded
40 ms cadence. Repeated identical frames may be combined by the WebP encoder.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import mmap
import re
import shutil
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageStat, features


WIDTH, HEIGHT = 240, 135
FRAME_BYTES = WIDTH * HEIGHT * 2
FRAME_MS = 40
MAX_PART_FRAMES = 300
SYMBOL_NAMES = [
    "nine", "ten", "jack", "queen", "king", "bell", "abacus", "book",
    "candle", "confucius", "wisdom", "phoenix",
]
WIN_MOTION_SYMBOLS = {0, 1, 5, 6, 7, 9, 10}
MOTION_SYMBOLS = WIN_MOTION_SYMBOLS | {8, 11}
STABLE_PHASES = {
    "ready", "free-intro", "free-counter", "spin-award",
    "spin-award-settle", "feature-end",
}


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path):
    return json.loads(path.read_text())


def load_lines(path: Path):
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def slug(value: str) -> str:
    result = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")
    if not result:
        raise ValueError("Empty output identifier")
    return result


def safe_child(root: Path, name: str) -> Path:
    result = (root / name).resolve()
    if not result.is_relative_to(root.resolve()):
        raise ValueError(f"Unsafe recorder path: {name}")
    return result


def scrub_paths(value, repository: Path, capture: Path):
    """Retain receipt content while replacing only machine-local path prefixes."""
    if isinstance(value, dict):
        return {key: scrub_paths(item, repository, capture) for key, item in value.items()}
    if isinstance(value, list):
        return [scrub_paths(item, repository, capture) for item in value]
    if isinstance(value, str):
        for prefix, replacement in [(str(repository), "."), (str(capture), "capture")]:
            value = value.replace(prefix + "/", replacement + "/")
            if value == prefix:
                value = replacement
        if value.startswith("/"):
            value = "<local-path>/" + Path(value).name
    return value


def encoded_webp_info(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"WEBP":
        raise ValueError(f"Invalid WebP: {path.name}")
    count = duration = 0
    at = 12
    while at + 8 <= len(data):
        kind = data[at:at + 4]
        size = int.from_bytes(data[at + 4:at + 8], "little")
        payload = data[at + 8:at + 8 + size]
        if kind == b"ANMF":
            if len(payload) < 16:
                raise ValueError(f"Short animation frame: {path.name}")
            count += 1
            duration += int.from_bytes(payload[12:15], "little")
        at += 8 + size + (size & 1)
    return count or 1, duration


class Event:
    def __init__(self, capture: Path, data: dict):
        self.data = data
        self.id = data["id"]
        self.raw_path = safe_child(capture, data["raw_file"])
        self.journal_path = safe_child(capture, data["frame_journal"])
        self.rows = load_lines(self.journal_path)
        count = int(data["frame_count"])
        if (data.get("format") != "rgb565le" or int(data.get("width", 0)) != WIDTH
                or int(data.get("height", 0)) != HEIGHT or int(data.get("fps", 0)) != 25):
            raise ValueError(f"Unsupported native recording format: {self.id}")
        if count != len(self.rows) or self.raw_path.stat().st_size != count * FRAME_BYTES:
            raise ValueError(f"Frame/journal/raw length mismatch: {self.id}")
        for index, row in enumerate(self.rows):
            if row["index"] != index:
                raise ValueError(f"Non-sequential frame index: {self.id}")
            if index and row["time_ms"] - self.rows[index - 1]["time_ms"] != FRAME_MS:
                raise ValueError(f"Non-contiguous recorded timing: {self.id}")
        self.raw_sha = sha(self.raw_path)
        self.journal_sha = sha(self.journal_path)
        self.stream = self.raw_path.open("rb")
        self.buffer = mmap.mmap(self.stream.fileno(), 0, access=mmap.ACCESS_READ)

    def close(self):
        self.buffer.close()
        self.stream.close()

    def frame_bytes(self, index: int) -> bytes:
        start = index * FRAME_BYTES
        return self.buffer[start:start + FRAME_BYTES]

    def image(self, index: int) -> Image.Image:
        return Image.frombytes("RGB", (WIDTH, HEIGHT), self.frame_bytes(index), "raw", "BGR;16")

    def evidence(self, start: int, end: int | None = None) -> dict:
        end = start + 1 if end is None else end
        row = self.rows[start]
        raw_digest = hashlib.sha256(self.buffer[start * FRAME_BYTES:end * FRAME_BYTES]).hexdigest()
        return {
            "event_id": self.id,
            "action_index": self.data.get("action_index"),
            "engine_action_count": row["snapshot"].get("actions"),
            "source_raw_sha256": self.raw_sha,
            "source_frame_journal_sha256": self.journal_sha,
            "source_frame": start,
            "source_frame_end_exclusive": end,
            "source_time_ms": row["time_ms"],
            "source_end_time_ms": self.rows[end - 1]["time_ms"] + FRAME_MS,
            "source_frame_bytes_sha256": raw_digest,
            "source_snapshot": row["snapshot"],
            "source_sage_diagnostic": row.get("sage", {}),
            "source_award_diagnostic": row.get("award", {}),
        }


class Gallery:
    def __init__(self, capture: Path, output: Path, prefix: str | None = None,
                 only_tags: list[str] | None = None, skip_stills=False, no_hero=False):
        self.capture, self.output = capture, output
        self.session = slug(prefix or capture.name)
        self.skip_stills, self.no_hero = skip_stills, no_hero
        self.outputs: list[dict] = []
        self.events: list[dict] = []
        self.still_aliases: list[dict] = []
        self.phase_candidates: dict[str, tuple[int, str, int]] = {}
        self.static_candidates: dict[int, tuple[str, int, int]] = {}
        self.motion_candidates: dict[str, tuple[int, str, int, int, tuple[int, int, int, int]]] = {}
        self.event_specs = {item["id"]: item for item in load_lines(capture / "events.ndjson")
                            if not only_tags or any(tag.startswith(prefix) for tag in item.get("tags", [])
                                                   for prefix in only_tags)}
        self.event_parts: dict[str, list[str]] = {}
        self.hero: tuple[int, str] | None = None

    def path(self, relative: str) -> Path:
        result = safe_child(self.output, relative)
        result.parent.mkdir(parents=True, exist_ok=True)
        return result

    def register(self, relative: str, kind: str, image: Image.Image, evidence: dict,
                 source_frames=1, **extra) -> dict:
        path = self.path(relative)
        encoded_frames, duration = encoded_webp_info(path) if path.suffix == ".webp" else (1, 0)
        static_part = kind in {"animation", "symbol-animation"} and encoded_frames == 1
        if static_part:
            kind = "symbol-crop" if kind == "symbol-animation" else "screenshot"
        record = {
            "file": relative, "kind": kind, "session_id": self.session, "dimensions": list(image.size),
            "sha256": sha(path), "bytes": path.stat().st_size,
            "frames": encoded_frames, "duration_ms": duration,
            "source_frames": source_frames, "source_duration_ms": source_frames * FRAME_MS
            if source_frames > 1 else 0,
            **evidence, **extra,
        }
        if static_part:
            record["recorded_static_part"] = True
        self.outputs.append(record)
        return record

    def save_png(self, relative: str, image: Image.Image, kind: str, evidence: dict, **extra):
        image.save(self.path(relative), format="PNG", optimize=True)
        return self.register(relative, kind, image, evidence, **extra)

    def save_animation(self, relative: str, images: list[Image.Image], kind: str,
                       evidence: dict, **extra):
        images[0].save(self.path(relative), format="WEBP", save_all=True,
                       append_images=images[1:], duration=[FRAME_MS] * len(images),
                       loop=0, lossless=True, quality=75, method=0, exact=True)
        result = self.register(relative, kind, images[0], evidence,
                               source_frames=len(images), **extra)
        if result["duration_ms"] not in (0, len(images) * FRAME_MS):
            raise ValueError(f"Encoded timing differs from recorded frames: {relative}")
        for image in images:
            image.close()
        return result

    def export_events(self):
        for spec in self.event_specs.values():
            event = Event(self.capture, spec)
            try:
                parts = []
                for start in range(0, len(event.rows), MAX_PART_FRAMES):
                    end = min(start + MAX_PART_FRAMES, len(event.rows))
                    relative = f"sessions/{self.session}/{slug(event.id)}-part{len(parts) + 1:02d}.webp"
                    images = [event.image(index).resize((720, 405), Image.Resampling.NEAREST)
                              for index in range(start, end)]
                    self.save_animation(relative, images, "animation", event.evidence(start, end),
                                        scale=3, tags=spec.get("tags", []),
                                        part=len(parts) + 1, part_count=(len(event.rows) + 299) // 300)
                    parts.append(relative)
                self.event_parts[event.id] = parts
                self.events.append({key: value for key, value in spec.items()
                                    if key not in {"raw_file", "frame_journal"}} | {
                    "session_id": self.session, "outputs": parts, "raw_sha256": event.raw_sha,
                    "frame_journal_sha256": event.journal_sha,
                })
                tags = spec.get("tags", [])
                ordinary_win = any(tag.startswith("win-symbol-") for tag in tags)
                feature = any(tag.startswith(("mode-free", "mode-hold", "phase-free", "phase-hold"))
                              for tag in tags)
                score = 2 if ordinary_win and not feature else 1 if not feature else 0
                if parts and (self.hero is None or score > self.hero[0]):
                    self.hero = score, parts[0]
                self.find_phase_screens(event)
                self.find_symbols(event)
                print(f"Converted {event.id}: {len(event.rows)} recorded frames, {len(parts)} parts", flush=True)
            finally:
                event.close()

    def phase_key(self, row: dict) -> str:
        snapshot = row["snapshot"]
        mode, phase = snapshot.get("mode", "unknown"), snapshot.get("animation", "unknown")
        award = row.get("award", {})
        if award.get("valid") and not award.get("test") and award.get("action") == snapshot.get("actions"):
            if award.get("grand"):
                return "grand-" + phase
            if award.get("majorCount"):
                return "major-" + phase
        return str(mode) + "-" + str(phase)

    def find_phase_screens(self, event: Event):
        groups = []
        begin, key = 0, None
        for index, row in enumerate(event.rows):
            current = self.phase_key(row) if row["snapshot"].get("view") == "game" else None
            if current != key:
                if key is not None:
                    groups.append((key, begin, index))
                key, begin = current, index
        if key is not None:
            groups.append((key, begin, len(event.rows)))
        for key, begin, end in groups:
            candidate = (end - begin, event.id, (begin + end - 1) // 2)
            if key not in self.phase_candidates or candidate[0] > self.phase_candidates[key][0]:
                self.phase_candidates[key] = candidate

    def keep_motion(self, name: str, event: Event, start: int, end: int,
                    crop: tuple[int, int, int, int]):
        if end - start < 25:
            return
        score = min(end - start, 150)
        if name not in self.motion_candidates or score > self.motion_candidates[name][0]:
            self.motion_candidates[name] = (score, event.id, start, min(end, start + 150), crop)

    def find_symbols(self, event: Event):
        active: dict[tuple, tuple[int, tuple[int, int, int, int]]] = {}
        for index, row in enumerate(event.rows):
            snap = row["snapshot"]
            cells = snap.get("reelDisplay", {}).get("symbols", [])
            valid = (snap.get("view") == "game" and snap.get("animation") in STABLE_PHASES
                     and len(cells) == 15)
            sage = row.get("sage", {})
            win_mask = int(sage.get("winMask", 0))
            join_mask = int(sage.get("gameJoinGeometryMask", sage.get("joinMask", 0)))
            eligible = {}
            if valid:
                for cell, symbol in enumerate(cells):
                    if not 0 <= symbol < len(SYMBOL_NAMES) or (symbol == 9 and join_mask & (1 << cell)):
                        continue
                    crop = (2 + (cell // 3) * 47, 16 + (cell % 3) * 35, 47, 35)
                    if symbol not in self.static_candidates:
                        self.static_candidates[symbol] = event.id, index, cell
                    if symbol in MOTION_SYMBOLS and (symbol not in WIN_MOTION_SYMBOLS
                                                    or win_mask & (1 << cell)):
                        eligible[(SYMBOL_NAMES[symbol], cell, snap.get("actions"))] = crop
            merge_valid = (snap.get("view") == "game" and len(cells) == 15
                           and snap.get("animation") in STABLE_PHASES | {"sage-expansion", "landing"})
            if merge_valid:
                for col in range(5):
                    bits = (join_mask >> (col * 3)) & 7
                    if bits in {3, 6, 7}:
                        rows, first = (3, 0) if bits == 7 else (2, 0 if bits == 3 else 1)
                        header = 4 if first == 0 else 0
                        crop = (2 + col * 47, 16 + first * 35 - header, 47, rows * 35 + header)
                        eligible[(f"confucius-{rows}-row", col, snap.get("actions"))] = crop
            for key in list(active):
                if key not in eligible:
                    start, crop = active.pop(key)
                    self.keep_motion(key[0], event, start, index, crop)
            for key, crop in eligible.items():
                if key not in active:
                    active[key] = index, crop
        for key, (start, crop) in active.items():
            self.keep_motion(key[0], event, start, len(event.rows), crop)

    def context(self, event_id: str, frame: int, end: int | None = None) -> dict:
        end = frame + 1 if end is None else end
        return {
            "context_file": self.event_parts[event_id][frame // MAX_PART_FRAMES],
            "context_parts": self.event_parts[event_id][frame // MAX_PART_FRAMES:
                                                        (end - 1) // MAX_PART_FRAMES + 1],
            "context_frame_offset": frame % MAX_PART_FRAMES,
            "context_time_offset_ms": (frame % MAX_PART_FRAMES) * FRAME_MS,
        }

    def export_screens(self):
        for key, (_, event_id, frame) in sorted(self.phase_candidates.items()):
            event = Event(self.capture, self.event_specs[event_id])
            try:
                self.save_png(f"screens/{self.session}/{slug(key)}.png",
                              event.image(frame).resize((720, 405), Image.Resampling.NEAREST),
                              "screenshot", event.evidence(frame), scale=3,
                              phase=event.rows[frame]["snapshot"].get("animation"),
                              **self.context(event_id, frame))
            finally:
                event.close()

    def export_stills(self):
        if self.skip_stills:
            return
        seen = {}
        for raw in sorted(self.capture.glob("*.rgb565")):
            meta_path = raw.with_suffix(".json")
            if not meta_path.exists() or raw.stat().st_size != FRAME_BYTES:
                raise ValueError(f"Unjournalled or malformed native still: {raw.name}")
            meta = load_json(meta_path)
            data = raw.read_bytes()
            raw_sha = hashlib.sha256(data).hexdigest()
            if raw_sha in seen:
                self.still_aliases.append({
                    "still_name": raw.stem, "duplicate_of": seen[raw_sha],
                    "session_id": self.session, "source_raw_sha256": raw_sha,
                    "source_frame_journal_sha256": sha(meta_path),
                    "action_index": meta.get("action_index"),
                    "engine_action_count": meta.get("engine_action_count", meta["snapshot"].get("actions")),
                    "source_time_ms": meta["time_ms"], "source_snapshot": meta["snapshot"],
                })
                continue
            image = Image.frombytes("RGB", (WIDTH, HEIGHT), data, "raw", "BGR;16")
            relative = f"screens/{self.session}/{slug(raw.stem)}.png"
            seen[raw_sha] = relative
            self.save_png(relative, image.resize((720, 405), Image.Resampling.NEAREST),
                          "screenshot", {
                              "event_id": "still-" + raw.stem,
                              "action_index": meta.get("action_index"),
                              "engine_action_count": meta.get("engine_action_count", meta["snapshot"].get("actions")),
                              "source_raw_sha256": raw_sha,
                              "source_frame_journal_sha256": sha(meta_path),
                              "source_frame": 0, "source_frame_end_exclusive": 1,
                              "source_time_ms": meta["time_ms"],
                              "source_frame_bytes_sha256": raw_sha,
                              "source_snapshot": meta["snapshot"],
                          }, scale=3, still_name=raw.stem)

    def export_symbols(self, only_merges=False):
        static_items = [] if only_merges else sorted(self.static_candidates.items())
        for symbol, (event_id, frame, cell) in static_items:
            event = Event(self.capture, self.event_specs[event_id])
            try:
                crop = (2 + (cell // 3) * 47, 16 + (cell % 3) * 35, 47, 35)
                # Select a legible recorded pose, never synthesize or redraw it.
                candidates = []
                action = event.rows[frame]["snapshot"].get("actions")
                for candidate in range(frame, min(frame + 60, len(event.rows)), 10):
                    row = event.rows[candidate]
                    snap = row["snapshot"]
                    cells = snap.get("reelDisplay", {}).get("symbols", [])
                    join = int(row.get("sage", {}).get("gameJoinGeometryMask", 0))
                    if (snap.get("view") != "game" or snap.get("animation") not in STABLE_PHASES
                            or snap.get("actions") != action or len(cells) != 15
                            or cells[cell] != symbol or (symbol == 9 and join & (1 << cell))):
                        break
                    native = event.image(candidate).crop((crop[0], crop[1], crop[0] + crop[2], crop[1] + crop[3]))
                    candidates.append((sum(ImageStat.Stat(native).var), candidate, native))
                if not candidates:
                    continue
                _, selected, native = max(candidates, key=lambda item: item[0])
                for _, _, other in candidates:
                    if other is not native:
                        other.close()
                self.save_png(f"symbols/{self.session}/{SYMBOL_NAMES[symbol]}.png",
                              native.resize((282, 210), Image.Resampling.NEAREST),
                              "symbol-crop", event.evidence(selected), symbol=SYMBOL_NAMES[symbol],
                              source_crop=list(crop), scale=6, cell=cell,
                              **self.context(event_id, selected))
            finally:
                event.close()
        animated = []
        for name, (_, event_id, start, end, crop) in sorted(self.motion_candidates.items()):
            if only_merges and not name.startswith("confucius-"):
                continue
            event = Event(self.capture, self.event_specs[event_id])
            try:
                box = crop[0], crop[1], crop[0] + crop[2], crop[1] + crop[3]
                native = [event.image(frame).crop(box) for frame in range(start, end)]
                if len({hashlib.sha256(image.tobytes()).digest() for image in native}) < 2:
                    for image in native:
                        image.close()
                    continue
                images = [image.resize((crop[2] * 6, crop[3] * 6), Image.Resampling.NEAREST)
                          for image in native]
                for image in native:
                    image.close()
                self.save_animation(f"symbols/{self.session}/{name}-motion.webp", images,
                                    "symbol-animation", event.evidence(start, end), symbol=name,
                                    source_crop=list(crop), scale=6,
                                    **self.context(event_id, start, end))
                animated.append(name)
            finally:
                event.close()
        return animated

    def export_hero(self):
        if self.no_hero or self.hero is None:
            return
        original = next(item for item in self.outputs if item["file"] == self.hero[1])
        relative = "session.webp"
        shutil.copyfile(self.path(original["file"]), self.path(relative))
        self.outputs.append({**original, "file": relative, "role": "readme-preview",
                             "identical_to_output": original["file"]})

    def export_prizes(self):
        ledger_path = self.capture / "prize-observations.ndjson"
        if not ledger_path.exists():
            return []
        observed = []
        for observation in load_lines(ledger_path):
            name = observation["id"]
            raw = safe_child(self.capture, name + ".rgb565")
            meta_path = raw.with_suffix(".json")
            meta, data = load_json(meta_path), raw.read_bytes()
            if len(data) != FRAME_BYTES:
                raise ValueError(f"Malformed observed-prize native still: {name}")
            if (meta["time_ms"] != observation["time_ms"] or
                    meta["snapshot"]["actions"] != observation["engine_action_count"]):
                raise ValueError(f"Prize observation/still journal differs: {name}")
            cell = int(observation["cell_index"])
            if meta["snapshot"].get("reelDisplay", {}).get("symbols", [])[cell] != 11:
                raise ValueError(f"Observed prize is not on a rendered Phoenix: {name}")
            crop = tuple(int(value) for value in observation["crop"])
            if crop != (2 + (cell // 3) * 47, 16 + (cell % 3) * 35, 47, 35):
                raise ValueError(f"Observed prize crop differs from the recorded cell: {name}")
            image = Image.frombytes("RGB", (WIDTH, HEIGHT), data, "raw", "BGR;16")
            raw_sha = hashlib.sha256(data).hexdigest()
            evidence = {
                "event_id": "still-" + name, "action_index": observation["action_index"],
                "engine_action_count": observation["engine_action_count"],
                "source_raw_sha256": raw_sha, "source_frame_journal_sha256": sha(meta_path),
                "source_frame": 0, "source_frame_end_exclusive": 1,
                "source_time_ms": observation["time_ms"], "source_frame_bytes_sha256": raw_sha,
                "source_snapshot": observation["snapshot"],
                "source_prize_observations_sha256": sha(ledger_path),
            }
            subject = {key: observation[key] for key in
                       ["tier", "multiplier", "cell_index", "matched_opaque_pixels"]}
            context_file = f"screens/{self.session}/{slug(name)}.png"
            if not any(output["file"] == context_file for output in self.outputs):
                self.save_png(context_file, image.resize((720, 405), Image.Resampling.NEAREST),
                              "screenshot", evidence, scale=3, still_name=name, observed_prize=subject)
            box = crop[0], crop[1], crop[0] + crop[2], crop[1] + crop[3]
            self.save_png(f"symbols/{self.session}/{slug(name)}.png",
                          image.crop(box).resize((282, 210), Image.Resampling.NEAREST),
                          "symbol-crop", evidence, symbol=name, source_crop=list(crop), scale=6,
                          observed_prize=subject, context_file=context_file,
                          context_parts=[context_file], context_frame_offset=0)
            for spec in self.event_specs.values():
                if name not in spec.get("tags", []):
                    continue
                event = Event(self.capture, spec)
                try:
                    ranges, start = [], None
                    for index, row in enumerate(event.rows):
                        snap = row["snapshot"]
                        cells = snap.get("reelDisplay", {}).get("symbols", [])
                        valid = (snap.get("view") == "game" and snap.get("animation") in STABLE_PHASES
                                 and snap.get("actions") == observation["engine_action_count"]
                                 and len(cells) == 15 and cells[cell] == 11)
                        if valid and start is None:
                            start = index
                        elif not valid and start is not None:
                            ranges.append((start, index)); start = None
                    if start is not None:
                        ranges.append((start, len(event.rows)))
                    if not ranges:
                        continue
                    start, end = max(ranges, key=lambda pair: pair[1] - pair[0])
                    label_verification = {}
                    if end - start < 25:
                        # A locked prize may remain fully visible while other
                        # cells respin. Verify its actual opaque label pixels
                        # in EVERY frame before extending across an action.
                        # Templates classify pixels only; they are never drawn.
                        preferred_start = start
                        source = Path(__file__).resolve().parents[1] / "firmware/src/pdf_assets.cpp"
                        text = source.read_text()
                        asset = ["mini", "minor", "major"][int(observation["tier"]) - 1] + str(observation["multiplier"])
                        arrays = []
                        for suffix in ("Rgb", "Alpha"):
                            match = re.search(r"\b" + asset + suffix + r"\[\]=\{(.*?)\};", text, re.S)
                            if match is None:
                                raise ValueError(f"Cannot inspect prize label pixels: {name}")
                            arrays.append([int(value) for value in re.findall(r"\d+", match.group(1))])
                        rgb, alpha = arrays
                        # The final border row can be overdrawn by a real
                        # neighbouring wing. The interior lettering identifies
                        # the prize without treating that overlap as a change.
                        samples = [((crop[1] + y) * WIDTH + crop[0] + x,
                                    int(rgb[y * 47 + x]).to_bytes(2, "little"))
                                   for y in range(3, 31) for x in range(4, 43)
                                   if alpha[y * 47 + x] == 255 and rgb[y * 47 + x]]
                        if len(samples) < 100:
                            raise ValueError(f"Insufficient opaque prize lettering: {name}")
                        verified_ranges, begin = [], None
                        for frame, row in enumerate(event.rows):
                            snap = row["snapshot"]
                            cells = snap.get("reelDisplay", {}).get("symbols", [])
                            valid = (snap.get("view") == "game" and snap.get("mode") == "hold"
                                     and len(cells) == 15 and cells[cell] == 11)
                            base = frame * FRAME_BYTES
                            valid = valid and all(event.buffer[base + at * 2:base + at * 2 + 2] == color
                                                  for at, color in samples)
                            if valid and begin is None:
                                begin = frame
                            elif not valid and begin is not None:
                                verified_ranges.append((begin, frame)); begin = None
                        if begin is not None:
                            verified_ranges.append((begin, len(event.rows)))
                        for begin, finish in verified_ranges:
                            if begin <= preferred_start < finish and finish - preferred_start > end - start:
                                start, end = preferred_start, finish
                                label_verification = {
                                    "source": "firmware/src/pdf_assets.cpp",
                                    "source_sha256": sha(source),
                                    "opaque_interior_pixels_per_frame": len(samples),
                                    "all_source_frames_match": True,
                                    "excluded_border_row": 31,
                                    "purpose": "Observe unchanged label pixels through real held-cell respins; no template rendered",
                                }
                                break
                    end = min(end, start + 150)
                    if end - start < 2:
                        continue
                    frames = [event.image(frame).crop(box).resize((282, 210), Image.Resampling.NEAREST)
                              for frame in range(start, end)]
                    if len({hashlib.sha256(frame.tobytes()).digest() for frame in frames}) < 2:
                        for frame in frames:
                            frame.close()
                        continue
                    self.save_animation(f"symbols/{self.session}/{slug(name)}-motion.webp", frames,
                                        "symbol-animation", event.evidence(start, end), symbol=name,
                                        source_crop=list(crop), scale=6, observed_prize=subject,
                                        short_recorded_excerpt=end - start < 25,
                                        source_prize_label_verification=label_verification,
                                        source_engine_actions=list(dict.fromkeys(
                                            row["snapshot"].get("actions") for row in event.rows[start:end])),
                                        **self.context(event.id, start, end))
                finally:
                    event.close()
                break
            observed.append(name)
        return observed

    def refresh_merges(self, index_path: Path):
        index = load_json(index_path)
        if index["capture_id"] != self.session:
            raise ValueError("Existing index belongs to another capture session")
        self.outputs = [item for item in index["outputs"]
                        if not item.get("symbol", "").startswith("confucius-")]
        self.events = index["events"]
        self.event_parts = {item["id"]: item["outputs"] for item in self.events}
        for spec in self.event_specs.values():
            event = Event(self.capture, spec)
            try:
                self.find_symbols(event)
            finally:
                event.close()
        mergers = self.export_symbols(only_merges=True)
        for item in self.outputs:
            if item["kind"] in {"animation", "symbol-animation"} and item["frames"] == 1:
                item["kind"] = "symbol-crop" if item["kind"] == "symbol-animation" else "screenshot"
                item["recorded_static_part"] = True
        index["outputs"] = self.outputs
        index["coverage"]["symbol_animations"] = sorted(
            {name for name in index["coverage"]["symbol_animations"] if not name.startswith("confucius-")}
            | set(mergers))
        index["coverage"]["merge_animations"] = mergers
        index["converter_sha256"] = sha(Path(__file__))
        index_path.write_text(json.dumps(index, indent=2) + "\n")
        print(json.dumps({"refreshed_merge_animations": mergers,
                          "single_frame_parts": [item["file"] for item in self.outputs
                                                 if item.get("recorded_static_part")]}, indent=2))

    def export_full_phoenix(self):
        """Keep the visible wings around a cell, including real neighbouring pixels.

        The production renderer centres its 57x43 Phoenix artwork over the 47x35
        reel cell. A screen-edge crop is clipped to recorded pixels; missing
        off-screen artwork is never reconstructed.
        """
        originals = [item for item in self.outputs
                     if item.get("symbol") == "phoenix" or item.get("symbol", "").startswith("prize-")]
        expanded = []
        for original in originals:
            if original.get("crop_variant") or "source_crop" not in original:
                continue
            x, y, width, height = original["source_crop"]
            if (width, height) != (47, 35):
                continue
            nominal = [x - 5, y - 4, 57, 43]
            left, top = max(0, nominal[0]), max(0, nominal[1])
            right, bottom = min(WIDTH, nominal[0] + 57), min(HEIGHT, nominal[1] + 43)
            crop = [left, top, right - left, bottom - top]
            box = left, top, right, bottom
            relative = str(Path(original["file"]).with_stem(Path(original["file"]).stem + "-full"))
            # Refreshing is idempotent and leaves the original exact-cell view.
            self.outputs = [item for item in self.outputs if item["file"] != relative]
            excluded = {"file", "kind", "session_id", "dimensions", "sha256", "bytes",
                        "frames", "duration_ms", "source_frames", "source_duration_ms",
                        "scale", "source_crop", "recorded_static_part"}
            evidence = {key: value for key, value in original.items() if key not in excluded}
            extra = {
                "scale": 6, "source_crop": crop,
                "nominal_source_crop": nominal, "crop_variant": "full-visible-art",
                "screen_edge_clipped": crop != nominal,
                "cell_crop_file": original["file"],
            }
            start, end = original["source_frame"], original["source_frame_end_exclusive"]
            if original["event_id"] in self.event_specs:
                event = Event(self.capture, self.event_specs[original["event_id"]])
                try:
                    images = [event.image(frame).crop(box).resize((crop[2] * 6, crop[3] * 6),
                                                                 Image.Resampling.NEAREST)
                              for frame in range(start, end)]
                finally:
                    event.close()
            else:
                still_name = original["event_id"].removeprefix("still-")
                data = safe_child(self.capture, still_name + ".rgb565").read_bytes()
                if len(data) != FRAME_BYTES:
                    raise ValueError(f"Malformed native Phoenix still: {still_name}")
                image = Image.frombytes("RGB", (WIDTH, HEIGHT), data, "raw", "BGR;16")
                images = [image.crop(box).resize((crop[2] * 6, crop[3] * 6), Image.Resampling.NEAREST)]
            if len(images) > 1:
                self.save_animation(relative, images, "symbol-animation", evidence, **extra)
            else:
                self.save_png(relative, images[0], "symbol-crop", evidence, **extra)
                images[0].close()
            expanded.append(relative)
        return expanded

    def refresh_phoenix(self, index_path: Path):
        index = load_json(index_path)
        if index["capture_id"] != self.session:
            raise ValueError("Existing index belongs to another capture session")
        self.outputs = index["outputs"]
        expanded = self.export_full_phoenix()
        index["outputs"] = self.outputs
        index["coverage"]["full_visible_phoenix_crops"] = expanded
        index["converter_sha256"] = sha(Path(__file__))
        index_path.write_text(json.dumps(index, indent=2) + "\n")
        print(json.dumps({"refreshed_full_phoenix_crops": expanded}, indent=2))

    def refresh_prizes(self, index_path: Path):
        index = load_json(index_path)
        if index["capture_id"] != self.session:
            raise ValueError("Existing index belongs to another capture session")
        self.outputs = [item for item in index["outputs"]
                        if not item.get("symbol", "").startswith("prize-")]
        self.event_parts = {item["id"]: item["outputs"] for item in index["events"]}
        prizes = self.export_prizes()
        expanded = self.export_full_phoenix()
        index["outputs"] = self.outputs
        index["coverage"]["observed_prize_crops"] = prizes
        index["coverage"]["full_visible_phoenix_crops"] = expanded
        index["converter_sha256"] = sha(Path(__file__))
        index_path.write_text(json.dumps(index, indent=2) + "\n")
        print(json.dumps({"refreshed_prizes": prizes,
                          "full_visible_phoenix_crops": expanded}, indent=2))

    def run(self, index_path: Path):
        self.export_events()
        self.export_screens()
        self.export_stills()
        motions = self.export_symbols()
        prizes = self.export_prizes()
        full_phoenix = self.export_full_phoenix()
        self.export_hero()
        receipt_path = self.capture / "capture-provenance.json"
        summary_path = self.capture / "summary.json"
        index = {
            "schema": 1, "kind": "actual-gameplay-gallery-conversion",
            "capture_id": self.session,
            "converter_sha256": sha(Path(__file__)),
            "processing": "Recorded RGB565 pixels only; rectangular crops; exact nearest-neighbour enlargement; lossless PNG/WebP; source order and 40 ms frame durations preserved.",
            "source_provenance_sha256": sha(receipt_path),
            "source_provenance": scrub_paths(load_json(receipt_path), Path(__file__).resolve().parents[1], self.capture),
            "source_actions_journal_sha256": sha(self.capture / "actions.ndjson"),
            "source_events_journal_sha256": sha(self.capture / "events.ndjson"),
            "summary": load_json(summary_path) if summary_path.exists() else {},
            "events": self.events, "outputs": self.outputs, "still_aliases": self.still_aliases,
            "coverage": {
                "symbol_stills": [SYMBOL_NAMES[index] for index in sorted(self.static_candidates)],
                "symbol_animations": motions,
                "symbol_animations_missing": [SYMBOL_NAMES[index] for index in sorted(MOTION_SYMBOLS)
                                              if SYMBOL_NAMES[index] not in motions],
                "static_symbols_without_invented_motion": ["jack", "queen", "king"],
                "merge_animations": [name for name in motions if name.startswith("confucius-")],
                "observed_prize_crops": prizes,
                "full_visible_phoenix_crops": full_phoenix,
                "captured_phase_screens": sorted(self.phase_candidates),
                "event_tags": sorted({tag for event in self.events for tag in event.get("tags", [])}),
            },
        }
        index_path.parent.mkdir(parents=True, exist_ok=True)
        index_path.write_text(json.dumps(index, indent=2) + "\n")
        print(json.dumps({"outputs": len(self.outputs), "coverage": index["coverage"]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--index", type=Path)
    parser.add_argument("--prefix", help="Unique session directory/metadata name (defaults to capture directory name)")
    parser.add_argument("--only-tag-prefix", action="append", default=[],
                        help="Convert only events with a matching tag prefix; repeat for multiple prefixes")
    parser.add_argument("--skip-stills", action="store_true", help="Skip root-level menu/startup stills")
    parser.add_argument("--no-hero", action="store_true", help="Do not replace session.webp when adding another session")
    parser.add_argument("--refresh-merges", action="store_true",
                        help="Refresh only merge crops and metadata in an existing index; retain full-scene files")
    parser.add_argument("--refresh-phoenix", action="store_true",
                        help="Add/refresh faithful expanded Phoenix crops in an existing index; retain full-scene files")
    parser.add_argument("--refresh-prizes", action="store_true",
                        help="Refresh observed prize crops and animations in an existing index; retain full-scene files")
    args = parser.parse_args()
    if not features.check("webp"):
        parser.error("Pillow requires animated WebP support")
    capture, output = args.capture.resolve(), args.output.resolve()
    if output == capture or output.is_relative_to(capture):
        parser.error("--output must be separate from the native recording")
    if not (capture / "summary.json").exists():
        parser.error("The recorder must finish before conversion (summary.json missing)")
    output.mkdir(parents=True, exist_ok=True)
    # A known red RGB565 word guards against accidental byte/channel reversal.
    if Image.frombytes("RGB", (1, 1), b"\x00\xf8", "raw", "BGR;16").getpixel((0, 0)) != (255, 0, 0):
        raise RuntimeError("Unexpected Pillow native RGB565 decoder")
    gallery = Gallery(capture, output, args.prefix, args.only_tag_prefix, args.skip_stills, args.no_hero)
    index_path = (args.index or output / "conversion-index.json").resolve()
    if sum((args.refresh_merges, args.refresh_phoenix, args.refresh_prizes)) > 1:
        parser.error("Refresh one crop group at a time")
    if args.refresh_merges:
        gallery.refresh_merges(index_path)
    elif args.refresh_phoenix:
        gallery.refresh_phoenix(index_path)
    elif args.refresh_prizes:
        gallery.refresh_prizes(index_path)
    else:
        gallery.run(index_path)


if __name__ == "__main__":
    main()
