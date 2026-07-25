#!/usr/bin/env python3

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


LOCALE_DIR = Path("locale")
PLACEHOLDER_RE = re.compile(r"\{\{([^{}]+)\}\}|\{([^{}]*)\}")


def flatten_messages(node: Any, prefix: str = "") -> dict[str, Any]:
    if not isinstance(node, dict):
        return {prefix: node} if prefix else {}

    messages: dict[str, Any] = {}
    for key, value in node.items():
        path = f"{prefix}.{key}" if prefix else key
        if isinstance(value, dict):
            messages.update(flatten_messages(value, path))
        else:
            messages[path] = value
    return messages


def extract_placeholders(value: str) -> list[str]:
    placeholders: list[str] = []
    for match in PLACEHOLDER_RE.finditer(value):
        escaped_name = match.group(1)
        plain_name = match.group(2)

        if escaped_name is not None:
            placeholders.append(f"{{{escaped_name}}}")
        else:
            placeholders.append("{" + (plain_name or "") + "}")

    return placeholders


def load_locale_messages(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"{path}: invalid JSON ({exc})") from exc

    if not isinstance(data, dict):
        raise ValueError(f"{path}: top-level JSON value must be an object")

    return flatten_messages(data)


def main() -> int:
    locale_files = sorted(LOCALE_DIR.glob("*.json"))
    if not locale_files:
        print(f"No locale JSON files found in {LOCALE_DIR}/", file=sys.stderr)
        return 1

    errors: list[str] = []
    file_messages: dict[Path, dict[str, Any]] = {}

    for path in locale_files:
        try:
            file_messages[path] = load_locale_messages(path)
        except ValueError as exc:
            errors.append(str(exc))

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    reference_path = locale_files[0]
    reference_messages = file_messages[reference_path]
    reference_keys = set(reference_messages)
    mismatch_found = False

    for path in locale_files[1:]:
        current_messages = file_messages[path]
        current_keys = set(current_messages)
        missing = sorted(reference_keys - current_keys)
        extra = sorted(current_keys - reference_keys)

        type_mismatches: list[str] = []
        placeholder_mismatches: list[tuple[str, list[str], list[str]]] = []

        for key in sorted(reference_keys & current_keys):
            reference_value = reference_messages[key]
            current_value = current_messages[key]

            if type(reference_value) is not type(current_value):
                type_mismatches.append(
                    f"{key}: expected {type(reference_value).__name__}, got {type(current_value).__name__}"
                )
                continue

            if isinstance(reference_value, str):
                reference_placeholders = extract_placeholders(reference_value)
                current_placeholders = extract_placeholders(current_value)
                if reference_placeholders != current_placeholders:
                    placeholder_mismatches.append(
                        (key, reference_placeholders, current_placeholders)
                    )

        if missing or extra or type_mismatches or placeholder_mismatches:
            mismatch_found = True
            print(f"{path} does not match {reference_path}:", file=sys.stderr)
            if missing:
                print("  Missing keys:", file=sys.stderr)
                for key in missing:
                    print(f"    - {key}", file=sys.stderr)
            if extra:
                print("  Extra keys:", file=sys.stderr)
                for key in extra:
                    print(f"    - {key}", file=sys.stderr)
            if type_mismatches:
                print("  Type mismatches:", file=sys.stderr)
                for entry in type_mismatches:
                    print(f"    - {entry}", file=sys.stderr)
            if placeholder_mismatches:
                print("  Placeholder mismatches:", file=sys.stderr)
                for key, expected, got in placeholder_mismatches:
                    print(
                        f"    - {key}: expected {expected}, got {got}",
                        file=sys.stderr,
                    )

    if mismatch_found:
        return 1

    print(
        f"Locale keys and placeholders are consistent across {len(locale_files)} files."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
