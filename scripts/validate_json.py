#!/usr/bin/env python3
"""Validate every <name>.json in a directory against its <name>_schema.json.

Usage: validate_json.py <json_dir>
Exits 1 if any file fails validation or is missing its schema/data partner.
"""
import json
import sys
from pathlib import Path

import jsonschema

SCHEMA_SUFFIX = "_schema.json"


def load(path, errors):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        errors.append(f"{path}: invalid JSON -> {e}")
    return None


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
    for name in sorted(set(schemas) & set(datas)):
        schema = load(schemas[name], errors)
        data = load(datas[name], errors)
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

    if errors:
        for msg in errors:
            print(msg, file=sys.stderr)
        print(f"JSON schema validation: {len(errors)} error(s)", file=sys.stderr)
        return 1

    print(f"JSON schema validation: {checked} files OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
