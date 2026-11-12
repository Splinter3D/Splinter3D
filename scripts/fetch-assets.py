#!/usr/bin/env python3
"""Downloads the optional large STL samples listed in assets/stl/manifest.json.

Files land in the manifest's `path` (assets/stl/large, gitignored) and are verified against
their sha256. Already-valid files are kept. They are never copied into builds or packages.

Usage:
    scripts/fetch-assets.py              # fetch every model
    scripts/fetch-assets.py lucy         # fetch only the named models
    scripts/fetch-assets.py --list       # show models and their local state
    scripts/fetch-assets.py --force      # re-download even if the file is valid
"""

import argparse
import hashlib
import json
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "assets" / "stl" / "manifest.json"


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def is_valid(path: Path, model: dict) -> bool:
    return path.is_file() and path.stat().st_size == model["size"] and sha256_of(path) == model["sha256"]

def getBestUnit(size: int) -> tuple[str, float]:
    # Hopefully nobody will ever upload a PB+ file
    units = {"B": 1, "KB": 1024, "MB": 1024 ** 2, "GB": 1024 ** 3, "TB": 1024 ** 4}
    best_unit = ("B", 1)
    for unit, factor in units.items():
        if size < factor:
            break
        best_unit = (unit, factor)
    return best_unit[0], float(size) / float(best_unit[1])

def download(url: str, target: Path, expected: int) -> None:
    expected_unit, expected_value = getBestUnit(expected)
    partial = target.with_name(target.name + ".part")
    with urllib.request.urlopen(url) as response, partial.open("wb") as out:
        done = 0
        while chunk := response.read(1 << 16):
            out.write(chunk)
            done += len(chunk)
            done_unit, done_value = getBestUnit(done)
            print("\r" + f"{done_value:7.1f} {done_unit:<2} / {expected_value:.1f} {expected_unit}   ".lstrip(), end="", flush=True)
    print()
    partial.replace(target)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("names", nargs="*", help="models to fetch (default: all)")
    parser.add_argument("--list", action="store_true", help="list models and local state, download nothing")
    parser.add_argument("--force", action="store_true", help="re-download even if the file is valid")
    args = parser.parse_args()

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    entries = [
        (ROOT / group["path"] / model["file"], model)
        for group in manifest["assets"]
        for model in group["models"]
    ]
    unknown = set(args.names) - {model["name"] for _, model in entries}
    if unknown:
        print(f"Unknown model(s): {', '.join(sorted(unknown))}", file=sys.stderr)
        return 2

    failed = False
    for target, model in entries:
        if args.names and model["name"] not in args.names:
            continue
        mb = model["size"] / 1e6
        if args.list:
            state = "present" if is_valid(target, model) else "missing/invalid"
            print(f"{model['name']:<16} {mb:8.1f} MB  {state}  {target.relative_to(ROOT)}")
            continue
        if not args.force and is_valid(target, model):
            print(f"{model['name']}: up to date")
            continue
        print(f"{model['name']}: downloading {mb:.1f} MB -> {target.relative_to(ROOT)}")
        target.parent.mkdir(parents=True, exist_ok=True)
        try:
            download(model["url"], target, model["size"])
        except OSError as error:
            print(f"download failed: {error}", file=sys.stderr)
            failed = True
            continue
        if not is_valid(target, model):
            print("checksum mismatch, removing file", file=sys.stderr)
            target.unlink()
            failed = True
        else:
            print(f"model {model['name']} downloaded and verified successfully")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
