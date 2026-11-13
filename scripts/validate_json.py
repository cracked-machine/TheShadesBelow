#!/usr/bin/env python3
"""Validate every <name>.json in a directory against its <name>_schema.json,
then check cross-file name references (see REF_CHECKS) that JSON Schema can't express.

Usage: validate_json.py <json_dir>
Exits 1 if any file fails validation, is missing its schema/data partner,
or references a name that doesn't exist in its target file.
"""
import json
import sys
from pathlib import Path

import jsonschema

SCHEMA_SUFFIX = "_schema.json"

# Cross-file references: wherever a dict has key <key> whose value is an object, each of its
# <fields> (if a string) must be a name from <target>.json, or one of the allowed sentinels.
# (key, fields, target, label, names_from_target, sentinels)
REF_CHECKS = [
    # sfx names are looked up via SoundBank::get_effect(), so only "sounds" (not "music") counts.
    # Empty string means "play nothing".
    ("sfx", ("loop", "final"), "audio", "sound",
     lambda d: {e["name"] for e in d.get("sounds", []) if isinstance(e, dict) and "name" in e}, {""}),
    # ERROR_SPRITE is an intentional placeholder with no spritesheet entry.
    ("sprite", ("name",), "spritesheets", "sprite",
     lambda d: set(d.get("sprites", {})), {"ERROR_SPRITE"}),
]


def load(path, errors):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        errors.append(f"{path}: invalid JSON -> {e}")
    return None


def walk(node, path, key, fields, on_ref):
    """Call on_ref(path, value) for every string field of every <key> object in node."""
    if isinstance(node, dict):
        ref = node.get(key)
        if isinstance(ref, dict):
            for field in fields:
                value = ref.get(field)
                if isinstance(value, str):
                    on_ref(path + [key, field], value)
        for k, v in node.items():
            walk(v, path + [k], key, fields, on_ref)
    elif isinstance(node, list):
        for i, v in enumerate(node):
            walk(v, path + [i], key, fields, on_ref)


def check_refs(datas, loaded, errors):
    """Check REF_CHECKS against already-loaded data files. Returns number of refs checked."""
    count = 0
    for key, fields, target, label, names_of, sentinels in REF_CHECKS:
        target_data = loaded.get(target)
        if not isinstance(target_data, dict):
            continue  # missing/invalid target already reported
        valid = names_of(target_data) | sentinels
        for name in sorted(loaded):
            if name == target or loaded[name] is None:
                continue

            def on_ref(path, value, name=name):
                nonlocal count
                count += 1
                if value not in valid:
                    loc = "/".join(map(str, path))
                    errors.append(f"{datas[name]}: {loc} -> unknown {label} '{value}' (not in {datas[target].name})")

            walk(loaded[name], [], key, fields, on_ref)
    return count


def main():
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2

    json_dir = Path(sys.argv[1])
    errors = []
    schemas = {p.name[: -len(SCHEMA_SUFFIX)]: p for p in json_dir.glob("*" + SCHEMA_SUFFIX)}
    datas = {p.stem: p for p in json_dir.glob("*.json") if not p.name.endswith(SCHEMA_SUFFIX)}

    for name in sorted(set(datas) - set(schemas)):
        errors.append(f"{datas[name]}: no matching {name}{SCHEMA_SUFFIX}")
    for name in sorted(set(schemas) - set(datas)):
        errors.append(f"{schemas[name]}: no matching {name}.json")

    checked = 0
    loaded = {}
    for name in sorted(set(schemas) & set(datas)):
        schema = load(schemas[name], errors)
        data = load(datas[name], errors)
        loaded[name] = data
        if schema is None or data is None:
            continue
        cls = jsonschema.validators.validator_for(schema)
        try:
            cls.check_schema(schema)
        except jsonschema.exceptions.SchemaError as e:
            errors.append(f"{schemas[name]}: invalid schema -> {e.message}")
            continue
        for e in sorted(cls(schema).iter_errors(data), key=lambda e: list(map(str, e.path))):
            loc = "/".join(map(str, e.path)) or "<root>"
            errors.append(f"{datas[name]}: {loc} -> {e.message}")
        checked += 1

    refs = check_refs(datas, loaded, errors)

    if errors:
        for msg in errors:
            print(msg, file=sys.stderr)
        print(f"JSON schema validation: {len(errors)} error(s)", file=sys.stderr)
        return 1

    print(f"JSON schema validation: {checked} files OK, {refs} references OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
