"""Read cross-version Python bytecode without executing it (including EVE Python 2.7).

Requires xdis. Writes a function index and disassembly with source hashes.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path
from xdis.load import load_module
from xdis.disasm import disco
from xdis.version_info import PythonImplementation


def read(path: Path, match: str = "") -> tuple[dict, str]:
    loaded = load_module(str(path))
    version, timestamp, magic, code, is_pypy, source_size, sip_hash = loaded[:7]
    functions = []
    def visit(obj, prefix=""):
        name = prefix + obj.co_name
        if not match or match.casefold() in name.casefold():
            functions.append({"name": name, "line": obj.co_firstlineno,
                              "arguments": list(obj.co_varnames[:obj.co_argcount]),
                              "names": list(obj.co_names),
                              "strings": [v for v in obj.co_consts if isinstance(v, str)]})
        for item in obj.co_consts:
            if hasattr(item, "co_code") and hasattr(item, "co_consts"):
                visit(item, name + ".")
    visit(code)
    out = io.StringIO()
    disco(version, code, timestamp, out=out, magic_int=magic, source_size=source_size,
          sip_hash=sip_hash, asm_format="extended", show_source=False,
          python_implementation=PythonImplementation.PyPy if is_pypy else PythonImplementation.CPython)
    return {"input": str(path.resolve()), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "python_version": list(version), "magic": magic, "source_filename": code.co_filename,
            "reader": "xdis", "executed": False, "functions": functions}, out.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--match", default="", help="Filter the function index by name; disassembly remains complete")
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    failures = 0
    for index, path in enumerate(args.files):
        try:
            metadata, assembly = read(path, args.match)
            stem = f"{index:03d}_{path.stem}"
            (args.output_dir / (stem + ".json")).write_text(json.dumps(metadata, indent=2, ensure_ascii=False), encoding="utf-8")
            (args.output_dir / (stem + ".dis.txt")).write_text(assembly, encoding="utf-8")
            print(f"OK {path.name}: Python {'.'.join(map(str, metadata['python_version']))}, {len(metadata['functions'])} indexed functions")
        except Exception as exc:
            failures += 1
            print(f"FAILED {path}: {type(exc).__name__}: {exc}")
    raise SystemExit(1 if failures else 0)


if __name__ == "__main__":
    main()
