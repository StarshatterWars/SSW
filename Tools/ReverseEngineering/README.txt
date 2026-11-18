Legacy Python bytecode reader

Uses the installed xdis cross-version reader. It reads Python 2.7 bytecode using
modern Python without importing or executing the inspected program.

Example (PowerShell, from D:\SSW):
python Tools/ReverseEngineering/read_legacy_bytecode.py Saved/Analysis/EveMissionFlow/0.pyc --output-dir Saved/Analysis/LegacyBytecodeRead

Outputs:
- .json: Python version, file SHA-256, original source path and function index.
- .dis.txt: complete opcode disassembly, including nested functions.

--match NAME filters the JSON function index. It does not truncate disassembly.
This is static bytecode analysis, not recovered original source or a runtime test.
Requires xdis 6.3.0 (already installed in C:\python in this workspace).
REA's current inspect_python_pyc provider supports newer bytecode; this reader is
an explicit local fallback for older Python 2.x artifacts, not a modification to REA.

Reference: https://github.com/rocky/python-xdis
