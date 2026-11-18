"""Read cross-version Python bytecode without executing it (including EVE Python 2.7).

Requires xdis. Writes a function index and disassembly with source hashes.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import fnmatch
import re
import tempfile
import zipfile
import zlib
from pathlib import Path
from xdis.load import load_module
from xdis.disasm import disco
from xdis.version_info import PythonImplementation

MAX_BYTES = 16 * 1024 * 1024


def decompress_member(raw: bytes) -> bytes:
    """Bound compressed PYJ input before asking the static marshal reader to decode it."""
    decoder = zlib.decompressobj()
    data = decoder.decompress(raw, MAX_BYTES + 1)
    if len(data) > MAX_BYTES or decoder.unconsumed_tail:
        raise ValueError("Decompressed bytecode exceeds 16 MiB")
    if not decoder.eof or decoder.unused_data:
        raise ValueError("Incomplete compressed bytecode or trailing data")
    return data


def read(path: Path, match: str = "") -> tuple[dict, str]:
    if path.stat().st_size > MAX_BYTES:
        raise ValueError("Bytecode file exceeds 16 MiB")
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
    parser.add_argument("files", nargs="*", type=Path, help="PYC or zlib-compressed PYJ files")
    parser.add_argument("--archive", type=Path, help="EVE code.ccp or ZIP archive")
    parser.add_argument("--member", action="append", default=[], help="Exact archive member; repeat for several")
    parser.add_argument("--list", dest="pattern", help="List archive members matching a shell-style pattern")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--match", default="", help="Filter the function index by name; disassembly remains complete")
    args = parser.parse_args()
    if (args.member or args.pattern) and not args.archive:
        parser.error("--member and --list require --archive")
    if args.pattern:
        with zipfile.ZipFile(args.archive) as archive:
            for name in archive.namelist():
                if fnmatch.fnmatchcase(name, args.pattern):
                    print(name)
        return
    if not args.files and not args.member:
        parser.error("Supply files or --archive with at least one --member")
    if not args.output_dir:
        parser.error("--output-dir is required for decoding")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    failures = 0
    inputs = [(path, None) for path in args.files] + [(args.archive, member) for member in args.member]
    manifest = []
    for index, (path, member) in enumerate(inputs):
        try:
            if member is not None:
                with zipfile.ZipFile(path) as archive:
                    info = archive.getinfo(member)
                    if info.is_dir() or info.file_size > MAX_BYTES:
                        raise ValueError("Archive member is a directory or exceeds 16 MiB")
                    raw = archive.read(info)
            else:
                if path.stat().st_size > MAX_BYTES:
                    raise ValueError("Selected file exceeds 16 MiB")
                raw = path.read_bytes()
            label = member or path.name
            bytecode = decompress_member(raw) if label.lower().endswith(".pyj") else raw
            with tempfile.TemporaryDirectory(prefix="ssw-bytecode-") as temporary:
                selected = Path(temporary) / "selected.pyc"
                selected.write_bytes(bytecode)
                metadata, assembly = read(selected, args.match)
            metadata["input"] = str(path.resolve())
            metadata["member"] = member
            metadata["stored_sha256"] = hashlib.sha256(raw).hexdigest()
            metadata["input_kind"] = "archive-member" if member else "file"
            stem = f"{index:03d}_" + re.sub(r"[^a-zA-Z0-9_-]", "_", Path(label).stem)
            (args.output_dir / (stem + ".json")).write_text(json.dumps(metadata, indent=2, ensure_ascii=False), encoding="utf-8")
            (args.output_dir / (stem + ".dis.txt")).write_text(assembly, encoding="utf-8")
            manifest.append({"input": str(path.resolve()), "member": member, "sha256": metadata["sha256"], "metadata": stem + ".json", "disassembly": stem + ".dis.txt"})
            print(f"OK {label}: Python {'.'.join(map(str, metadata['python_version']))}, {len(metadata['functions'])} indexed functions")
        except Exception as exc:
            failures += 1
            manifest.append({"input": str(path.resolve()), "member": member, "error": f"{type(exc).__name__}: {exc}"})
            print(f"FAILED {member or path}: {type(exc).__name__}: {exc}")
    (args.output_dir / "manifest.json").write_text(json.dumps({"executed": False, "results": manifest, "failures": failures}, indent=2), encoding="utf-8")
    raise SystemExit(1 if failures else 0)


if __name__ == "__main__":
    main()
