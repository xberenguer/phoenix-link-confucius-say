#!/usr/bin/env python3
"""Check that release docs, media and browser describe the included game.

This checks provenance and presentation consistency, not visual fidelity to
the original machine. A reviewer must still inspect new renders and wording.
Run from any directory; no third-party Python packages are required.
"""
import argparse
import csv
import hashlib
import html
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from urllib.parse import unquote, urlsplit


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dimensions(path):
    data = path.read_bytes()
    if data.startswith(b"\x89PNG\r\n\x1a\n"):
        return struct.unpack(">II", data[16:24])
    if data[:6] in (b"GIF87a", b"GIF89a"):
        return struct.unpack("<HH", data[6:10])
    if data[:4] == b"RIFF" and data[8:12] == b"WEBP":
        offset = 12
        while offset + 8 <= len(data):
            kind = data[offset:offset + 4]
            size = int.from_bytes(data[offset + 4:offset + 8], "little")
            payload = data[offset + 8:offset + 8 + size]
            if kind == b"VP8X":
                return (1 + int.from_bytes(payload[4:7], "little"),
                        1 + int.from_bytes(payload[7:10], "little"))
            if kind == b"VP8L" and len(payload) >= 5 and payload[0] == 47:
                packed = int.from_bytes(payload[1:5], "little")
                return 1 + (packed & 16383), 1 + ((packed >> 14) & 16383)
            offset += 8 + size + (size & 1)
    raise ValueError("Unsupported or malformed image: " + str(path))


def verify(root):
    errors = []
    checks = 0

    def require(value, message):
        nonlocal checks
        checks += 1
        if not value:
            errors.append(message)

    def read(name):
        return json.loads((root / name).read_text())

    def public_files():
        for path in root.rglob("*"):
            parts = path.relative_to(root).parts
            if any(p in {".git", ".pio", "__pycache__"} for p in parts):
                continue
            if parts[:2] in {("browser", "build"), ("browser", "dist"),
                              ("firmware", "release")}:
                continue
            if path.is_file():
                yield path

    source = root / "firmware/src"
    pins = {p.relative_to(source).as_posix(): sha(p)
            for p in sorted(source.rglob("*")) if p.is_file()}
    version = re.search(r'PHOENIX_BUILD_VERSION "([^"]+)"',
                        (source / "build_version.h").read_text())[1]
    identity = (source / "math_runtime_identity.h").read_text()
    runtime = re.search(r'fingerprint="([^"]+)"', identity)[1]
    family = re.search(r'mathematicalFamily="([^"]+)"', identity)[1]
    release = read("RELEASE.json")
    require(release["version"] == version, "RELEASE version is stale")
    require(release["runtime_fingerprint"] == runtime, "RELEASE runtime is stale")
    require(release["mathematical_family"] == family, "RELEASE math family is stale")
    require(release["browser_version"] == version, "Browser release label is stale")
    bodies = {name: value for name, value in pins.items() if name not in {
        "math_runtime_identity.h", "static_grand_table.h", "phoenix_crypto124/LICENSE"}}
    canonical = json.dumps({"family": family, "sources": bodies},
                           sort_keys=True, separators=(",", ":")).encode()
    require(hashlib.sha256(canonical).hexdigest() == runtime,
            "Runtime fingerprint does not match the actual source bodies")
    require(len(bodies) == release["source_body_files"] and
            len(pins) == release["source_total_files"], "Release source counts are stale")
    gallery = read("docs/gallery-manifest.json")
    require(gallery["describes_version"] == version, "Gallery version is stale")
    require(gallery["runtime"] == runtime, "Gallery runtime is stale")
    require(gallery["source_files_sha256"] == pins,
            "Gallery was not regenerated/bound to the current source")
    require(gallery["renderer_sha256"] == pins["ui.cpp"], "Gallery renderer is stale")
    media = read("docs/changelog-media.json")
    require(media["version"] == version, "Media version is stale")
    require(media["runtime_fingerprint"] == runtime, "Media runtime is stale")
    catalog = {}
    for item in [*gallery["images"], *media["images"]]:
        path = (root / "docs" / item["file"]).resolve()
        require(path.is_relative_to(root) and path.is_file(),
                "Missing/unsafe catalog image: " + item["file"])
        if not path.is_file():
            continue
        require(item["source_version"] == version, "Stale image source: " + item["file"])
        require(sha(path) == item["sha256"], "Image hash mismatch: " + item["file"])
        require(list(dimensions(path)) == item["dimensions"],
                "Image size mismatch: " + item["file"])
        catalog[path] = item
    for name, key in [("docs/data/reference-metadata.json", "source_version"),
                      ("docs/data/rng-validation.json", "version")]:
        record = read(name)
        require(record[key] == version, name + " version is stale")
        require(record["runtime_fingerprint"] == runtime, name + " runtime is stale")
    reference = read("docs/data/reference-metadata.json")
    require(reference["source_engine_sha256"] == pins["phoenix_engine.h"],
            "Payout reference Engine is stale")
    require(reference["source_cabinet_profile_sha256"] == pins["cabinet_profile.h"],
            "Wager reference menu source is stale")
    require(reference["source_static_grand_table_sha256"] == pins["static_grand_table.h"],
            "Return reference table is stale")
    model = re.search(r"modelId\s*=\s*(0x[0-9a-fA-F]+)",
                      (source / "phoenix_engine.h").read_text())[1]
    require(int(reference["engine_model"], 16) == int(model, 16) ==
            int(release["local_model"], 16) == int(gallery["model"], 16),
            "Published model labels differ from the current Engine")
    with (root / "docs/data/offered-wagers.csv").open(newline="") as stream:
        offers = list(csv.DictReader(stream))
    require(len(offers) == reference["rows"]["offered_wagers"],
            "Offered wager count is stale")
    for mode in ["standard", "high_limit"]:
        count = sum(row["menu_mode"] == mode for row in offers)
        require(count == reference["rows"][mode + "_offered_wagers"],
                "Menu-mode wager count is stale: " + mode)
    require(all(row["menu_mode"] in {"standard", "high_limit"} for row in offers),
            "Unknown wager menu mode")
    keys = {(row["denomination_minor"], row["selected_lines"], row["bet_multiplier"])
            for row in offers}
    require(len(keys) == len(offers), "Duplicate published wager")
    require(all(int(row["stake_credits"]) == int(row["selected_lines"]) *
                int(row["bet_multiplier"]) and int(row["stake_money_minor"]) ==
                int(row["stake_credits"]) * int(row["denomination_minor"])
                for row in offers), "Published wager arithmetic is inconsistent")
    for name, item in reference["datasets"].items():
        path = root / "docs/data" / name
        require(path.stat().st_size == item["bytes"] and sha(path) == item["sha256"],
                "Payout dataset mismatch: " + name)
    links = images = 0
    for document in sorted(p for p in public_files() if p.suffix == ".md"):
        text = re.sub(r"```.*?```", "", document.read_text(), flags=re.S)
        targets = re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", text)
        targets += re.findall(r'(?:src|href)="([^"]+)"', text)
        for target in targets:
            target = html.unescape(target).strip("<>")
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc or not parsed.path:
                continue
            path = (document.parent / unquote(parsed.path)).resolve()
            require(path.is_relative_to(root) and path.exists(),
                    "Broken/unsafe link in " + str(document.relative_to(root)) + ": " + target)
            links += 1
        for target in re.findall(r'!\[[^\]]*\]\(([^)]+)\)|<img\b[^>]*src="([^"]+)"', text):
            target = html.unescape(next(part for part in target if part))
            if urlsplit(target).scheme:
                continue
            path = (document.parent / unquote(urlsplit(target).path)).resolve()
            require(path in catalog, "Untracked/stale documentation image: " + target)
            if path.is_file():
                require(dimensions(path)[0] >= 720, "Small documentation image: " + target)
            images += 1
    for path in sorted((root / "docs/media").iterdir()):
        if path.is_file():
            require(path.resolve() in catalog, "Uncatalogued media: " + path.name)
    # Build these markers from pieces so the audit source is not itself a
    # false positive. No developer username or device identifier is embedded.
    private = [b"/" + b"Users" + b"/", b"/private/" + b"var/folders/",
               b"codex-" + b"clipboard-", b".codex/" + b"attachments/"]
    for path in public_files():
        data = path.read_bytes()
        require(not any(marker in data for marker in private),
                "Private development path/input in " + str(path.relative_to(root)))
    command = [sys.executable, str(root / "browser/verify_bundle.py"),
               "--source", str(source), "--assets", str(root / "firmware/data"),
               "--bundle", str(root / "demo")]
    result = subprocess.run(command, capture_output=True, text=True)
    require(result.returncode == 0, "Browser binding failed: " + result.stdout + result.stderr)
    return {"status": "FAIL" if errors else "PASS", "version": version,
            "runtime": runtime, "checks": checks, "local_links": links,
            "image_references": images, "catalog_images": len(catalog), "errors": errors}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    result = verify(args.root.resolve())
    print(json.dumps(result, indent=2))
    sys.exit(1 if result["errors"] else 0)
