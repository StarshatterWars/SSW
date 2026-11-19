"""Apply/restore reviewed per-system weapon quality profiles; no physical base edits."""
from __future__ import annotations
import argparse
import copy
import datetime as dt
import hashlib
import json
import math
import uuid
import zipfile
from pathlib import Path

SHA = "4eabd0c60c4c92e1b82b0302b981c7010191cac3aa68ae2c374f471c16109c20"
PROFILES = {"Bolt Weapon": (10678, 10680), "Plasma Weapon": (10678, 10680), "Beam Weapon": (453, 3041)}
TARGET = "Content/GameData/Systems/sys.json"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--restore", type=Path)
    args = parser.parse_args(); root = args.root.resolve(); target = root / TARGET
    pointer = root / "Saved/Research/EVE/SDE/weapon-component-profiles-current.json"
    if args.restore:
        record = json.loads(args.restore.read_text(encoding="utf-8"))
        before = Path(record["backup"]).read_bytes()
        if record["target"] != TARGET or digest(target.read_bytes()) != record["afterSha256"] or digest(before) != record["beforeSha256"]:
            raise ValueError("Refusing to overwrite later source edits or a changed backup")
        target.write_bytes(before); pointer.unlink(missing_ok=True); print("Exact previous sys.json restored"); return
    if pointer.exists():
        record = json.loads(pointer.read_text(encoding="utf-8"))
        if digest(target.read_bytes()) != record["afterSha256"]:
            raise ValueError("Catalog changed since this calibration; review instead of compounding")
        print("Weapon profiles already applied; no changes"); return
    archive = root / "Saved/Research/EVE/SDE/eve-online-static-data-jsonl.zip"
    if digest(archive.read_bytes()) != SHA:
        raise ValueError("Only reviewed SDE build3714173 is accepted")
    ids = {n for pair in PROFILES.values() for n in pair}
    with zipfile.ZipFile(archive) as z:
        types = {}; dogma = {}
        for line in z.open("types.jsonl"):
            row = json.loads(line)
            if row["_key"] in ids: types[row["_key"]] = row
        for line in z.open("typeDogma.jsonl"):
            row = json.loads(line)
            if row["_key"] in ids: dogma[row["_key"]] = {a["attributeID"]: a["value"] for a in row["dogmaAttributes"]}
    original = target.read_bytes(); data = json.loads(original.decode("utf-8-sig")); before = copy.deepcopy(data); changes = []
    for system in data["Systems"]:
        if system["Name"] not in PROFILES: continue
        basic, advanced = PROFILES[system["Name"]]
        performance = dogma[advanced][64] / dogma[basic][64]
        power = dogma[advanced][30] / dogma[basic][30]
        has_price = types[basic].get("basePrice", 0) > 0 and types[advanced].get("basePrice", 0) > 0
        price = types[advanced]["basePrice"] / types[basic]["basePrice"] if has_price else None
        old_grades = copy.deepcopy(system.get("ComponentGrades", []))
        grades = copy.deepcopy(data["ComponentGrades"])
        for grade in grades:
            grade["PerformanceMultiplier"] = round(performance ** ((grade["Level"] - 3) / 4), 9)
            grade["PowerDemandMultiplier"] = round(power ** ((grade["Level"] - 3) / 4), 9)
            if price is not None: grade["PriceMultiplier"] = round(price ** ((grade["Level"] - 1) / 4), 9)
        system["ComponentGrades"] = grades
        changes.append({"system": system["Name"], "sourceTypes": [{"typeID": n, "name": types[n]["name"]["en"],
                         "basePrice": types[n].get("basePrice"), "damageMultiplier64": dogma[n][64], "fittingPower30": dogma[n][30]} for n in (basic, advanced)],
                        "sswMapping": "Railgun projectile-role quality analogue" if basic == 10678 else "Pulse laser beam-role quality analogue",
                        "priceFallback": None if has_price else "No pulse basePrice in SDE: retain generic SSW price tier",
                        "oldOverride": old_grades, "newOverride": grades,
                        "oldEffective": copy.deepcopy(data["ComponentGrades"]) if not old_grades else old_grades})
    if len(changes) != len(PROFILES): raise ValueError("Expected three existing weapon system profiles")
    stripped = copy.deepcopy(data)
    for system in stripped["Systems"]:
        if system["Name"] in PROFILES: system.pop("ComponentGrades", None)
    before_stripped = copy.deepcopy(before)
    for system in before_stripped["Systems"]:
        if system["Name"] in PROFILES: system.pop("ComponentGrades", None)
    if stripped != before_stripped: raise AssertionError("Non-profile design data changed")
    result = (json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + "\n").encode("utf-8")
    if b"\r\n" in original: result = result.replace(b"\n", b"\r\n")
    record = {"target": TARGET, "createdUtc": dt.datetime.now(dt.timezone.utc).isoformat(),
              "source": {"build": 3714173, "archiveSha256": SHA, "tables": ["types.jsonl", "typeDogma.jsonl"]},
              "formula": "SSW normalization: performance/power=(II/I)^((level-3)/4), price=(II/I)^((level-1)/4); gradeIII physical1",
              "limits": "Design calibration, not EVE server formula; all base physical values, missile profile and repair times retained",
              "beforeSha256": digest(original), "afterSha256": digest(result), "profiles": changes}
    if args.apply:
        stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%d-%H%M%SZ") + "-" + uuid.uuid4().hex[:6]
        folder = root / "Saved/Backups" / ("WeaponComponentProfiles-" + stamp); folder.mkdir(parents=True)
        backup = folder / "sys.json"; backup.write_bytes(original); record["backup"] = str(backup)
        record["manifestPath"] = str(folder / "manifest.json")
        text = json.dumps(record, indent=2, ensure_ascii=False)
        (folder / "manifest.json").write_text(text, encoding="utf-8")
        try:
            target.write_bytes(result); pointer.write_text(text, encoding="utf-8")
        except Exception:
            target.write_bytes(original); pointer.unlink(missing_ok=True); raise
        print("Applied", folder / "manifest.json")
    else:
        (root / "Saved/Research/EVE/SDE/weapon-component-profiles-preview.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
        print("Dry run; no source changes")


if __name__ == "__main__": main()
