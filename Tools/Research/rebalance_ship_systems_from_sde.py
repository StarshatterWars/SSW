"""Reversible SSW credit/quality calibration from a pinned official EVE SDE.

This is an SSW design policy, not an ISK exchange rate or EVE server formula.
Default is a dry run. --apply backs up exact source bytes and records every row.
--restore MANIFEST restores only when current bytes still match this run.
"""
from __future__ import annotations
import argparse
import copy
import datetime as dt
import hashlib
import json
import math
import re
import shutil
import statistics
import uuid
import zipfile
from pathlib import Path

EXPECTED_SDE_SHA256 = "4eabd0c60c4c92e1b82b0302b981c7010191cac3aa68ae2c374f471c16109c20"
POLICY = "SSW-EVE-role-calibration-1"
ROLE_EXPONENT = .75
ROW_FACTOR_BOUNDS = (.5, 2.0)

# Selected ordinary hulls avoid faction, event, rookie and zero-base-price outliers.
# SSW ship classes have different physical roles/sizes from identically named EVE
# classes: these equivalences are explicit design choices, not asset identities.
ROLES = {
    "FIGHTER": ("Frigate", 25, [587, 597, 602, 603], 1.0),
    "ATTACK": ("Destroyer", 420, [16236, 16238, 16240, 16242], 1.0),
    "LCA": ("Utility frigate", 25, [587, 597, 602, 603], .8),
    "COURIER": ("Light hauler", 28, [648, 650, 652, 1944], 1.0),
    "CARGO": ("Hauler", 28, [648, 650, 652, 1944], 1.0),
    "CORVETTE": ("Heavy destroyer role", 420, [16236, 16238, 16240, 16242], 1.5),
    "FREIGHTER": ("Freighter", 513, [20183, 20185, 20187, 20189], 1.0),
    "FRIGATE": ("Cruiser", 26, [621, 623, 624, 627], 1.0),
    "DESTROYER": ("Combat battlecruiser", 419, [16227, 16229, 16231, 16233], 1.0),
    "CRUISER": ("Battleship", 27, [638, 639, 641, 642], 1.0),
    "BATTLESHIP": ("Dreadnought", 485, [19720, 19722, 19724, 19726], 1.0),
    "CARRIER": ("Carrier", 547, [23757, 23911, 23915, 24483], 1.0),
    "DREADNAUGHT": ("Titan", 30, [671, 3764, 11567, 23773], 1.0),
}
ROW_ROLES = {"Prospector": ("Mining frigate", 25, [32880], 1.0)}
FILES = ("Content/GameData/Ships/ships.json", "Content/GameData/Systems/sys.json")


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def json_bytes(value, original: bytes) -> bytes:
    text = original.decode("utf-8-sig")
    indentation = "\t" if re.search(r"\n\t+\S", text) else 2
    result = json.dumps(value, ensure_ascii=False, indent=indentation, allow_nan=False) + "\n"
    if b"\r\n" in original:
        result = result.replace("\n", "\r\n")
    return (b"\xef\xbb\xbf" if original.startswith(b"\xef\xbb\xbf") else b"") + result.encode("utf-8")


def hull_price_bytes(rows, original: bytes) -> bytes:
    # Native exports contain unusual legacy line endings. Replace only the
    # numeric price tokens rather than reformatting hundreds of geometry rows.
    pattern = re.compile(rb'("BaseCost"\s*:\s*)([0-9]+)')
    matches = list(pattern.finditer(original))
    baseline = json.loads(original.decode("utf-8-sig"))
    if len(matches) != len(rows) or len(baseline) != len(rows):
        raise ValueError("Hull price tokens no longer match one-per-row schema")
    index = 0
    def replace(match):
        nonlocal index
        before, after = baseline[index], rows[index]; index += 1
        if before["Row"] != after["Row"] or int(match[2]) != before["BaseCost"]:
            raise ValueError("Hull catalog order/price token identity changed")
        return match[1] + str(after["BaseCost"]).encode("ascii")
    return pattern.sub(replace, original)


def load_sde(archive: Path):
    digest = sha(archive.read_bytes())
    if digest != EXPECTED_SDE_SHA256:
        raise ValueError("SDE differs from the reviewed build3714173; review and repin before recalibrating")
    with zipfile.ZipFile(archive) as z:
        def rows(name):
            return [json.loads(line) for line in z.open(name + ".jsonl") if line.strip()]
        types = {row["_key"]: row for row in rows("types")}
        dogma = {row["_key"]: {a["attributeID"]: a["value"] for a in row.get("dogmaAttributes", [])}
                 for row in rows("typeDogma") if row["_key"] in (438, 439)}
    return types, dogma, digest


def role_evidence(role, types):
    label, group, ids, factor = role
    records = []
    for type_id in ids:
        row = types[type_id]
        if not row.get("published") or row.get("groupID") != group or not row.get("basePrice", 0) > 0:
            raise ValueError(f"Invalid mapped reference hull {type_id}")
        records.append({"typeID": type_id, "name": row["name"]["en"], "basePrice": row["basePrice"], "groupID": group})
    return {"equivalentRole": label, "groupID": group, "types": records,
            "medianBasePrice": statistics.median(r["basePrice"] for r in records), "sswRoleFactor": factor}


def plan(root: Path, archive: Path):
    types, dogma, digest = load_sde(archive)
    originals = {relative: (root / relative).read_bytes() for relative in FILES}
    ships, systems = [json.loads(originals[p].decode("utf-8-sig")) for p in FILES]
    old_ships, old_systems = copy.deepcopy(ships), copy.deepcopy(systems)
    falcon = next(row for row in ships if row["Row"] == "Falcon")
    anchor = falcon["BaseCost"]
    if anchor <= 0:
        raise ValueError("Falcon credit anchor must be positive")
    reference = role_evidence(ROLES["FIGHTER"], types)["medianBasePrice"]
    classes = {}
    for row in ships:
        classes.setdefault(row["ShipClass"].upper(), []).append(row["BaseCost"])
    class_medians = {key: statistics.median(values) for key, values in classes.items()}
    enum_text = (root / "Source/StarshatterWars/Util/GameStructs.h").read_bytes().decode("cp1252")
    enum_values = {key: int(value, 16) for key, value in re.findall(r"\b([A-Z_]+)\s*=\s*(0x[0-9a-fA-F]+)", enum_text)}
    mappings = {key: dict(role_evidence(role, types), classificationValue=enum_values[key]) for key, role in ROLES.items()}
    ship_changes = []
    for row in ships:
        key, classification = row["Row"], row["ShipClass"].upper()
        evidence = role_evidence(ROW_ROLES[key], types) if key in ROW_ROLES else mappings.get(classification)
        before = row["BaseCost"]
        item = {"row": key, "shipClass": row["ShipClass"], "oldBaseCost": before,
                "modelAuthored": bool(row.get("Model")), "modelProvisioningChanged": False}
        if evidence and before > 0:
            variation = before / class_medians[classification]
            desired = anchor * (evidence["medianBasePrice"] / reference) ** ROLE_EXPONENT * evidence["sswRoleFactor"] * variation
            bounded = max(before * ROW_FACTOR_BOUNDS[0], min(before * ROW_FACTOR_BOUNDS[1], desired))
            after = anchor if key == "Falcon" else int(math.floor(bounded / 1000 + .5) * 1000)
            # Rounding cannot exceed the bounds (important for small future hulls).
            after = max(math.ceil(before * ROW_FACTOR_BOUNDS[0]), min(math.floor(before * ROW_FACTOR_BOUNDS[1]), after))
            row["BaseCost"] = after
            item.update(newBaseCost=after, targetBeforeBounds=desired, withinClassVariation=variation,
                        equivalentRole=evidence["equivalentRole"], sourceTypeIDs=[r["typeID"] for r in evidence["types"]],
                        sourceMedianBasePrice=evidence["medianBasePrice"], status="changed" if before != after else "anchor/unchanged")
        else:
            item.update(newBaseCost=before, status="unmapped unchanged", reason="No defensible published hull-role/basePrice equivalent")
        ship_changes.append(item)
    # A tier curve uses an actual I/II module pair as a ratio reference, then
    # centers SSW grade III on its existing physical baseline. Higher output
    # requires more rated power. This does not change hull reactor output.
    performance_ratio = dogma[438][20] / dogma[439][20]
    demand_ratio = dogma[438][30] / dogma[439][30]
    price_ratio = types[438]["basePrice"] / types[439]["basePrice"]
    grades = []
    for row in systems["ComponentGrades"]:
        old = copy.deepcopy(row)
        level = row["Level"]
        row["PerformanceMultiplier"] = round(performance_ratio ** ((level - 3) / 4), 9)
        row["PowerDemandMultiplier"] = round(demand_ratio ** ((level - 3) / 4), 9)
        row["PriceMultiplier"] = round(price_ratio ** ((level - 1) / 4), 9)
        grades.append({"level": level, "old": old, "new": copy.deepcopy(row)})
    components = []
    for system in systems["Systems"]:
        for component in system["Components"]:
            for grade in grades:
                def effective(values):
                    return {"credits": math.ceil(component["BaseCost"] * values["PriceMultiplier"]),
                            "performanceMultiplier": component.get("PerformanceMultiplier", 1) * values["PerformanceMultiplier"],
                            "powerDemandMultiplier": component.get("PowerDemandMultiplier", 1) * values["PowerDemandMultiplier"]}
                components.append({"system": system["Name"], "component": component["Name"], "level": grade["level"],
                                   "baseCostUnchanged": component["BaseCost"], "old": effective(grade["old"]), "new": effective(grade["new"])})
    # Verify this tool has no authority to alter geometry, physics, reactor
    # outputs, component base costs, damage, rates, repair times or unlocks.
    for before, after in zip(old_ships, ships):
        before.pop("BaseCost"); after_copy = copy.deepcopy(after); after_copy.pop("BaseCost")
        if before != after_copy:
            raise AssertionError("Non-price hull data changed")
    old_systems.pop("ComponentGrades")
    if old_systems != {k: v for k, v in systems.items() if k != "ComponentGrades"}:
        raise AssertionError("Base component design changed")
    results = {FILES[0]: hull_price_bytes(ships, originals[FILES[0]]), FILES[1]: json_bytes(systems, originals[FILES[1]])}
    manifest = {"policy": POLICY, "createdUtc": dt.datetime.now(dt.timezone.utc).isoformat(),
                "authority": "SSW-authorized balance calibration using EVE reference ratios",
                "provenance": {"url": "https://developers.eveonline.com/static-data/tranquility/eve-online-static-data-3714173-jsonl.zip",
                               "build": 3714173, "archive": str(archive.resolve()), "sha256": digest,
                               "tables": ["types.jsonl", "typeDogma.jsonl"],
                               "clientEvidence": "Saved/Analysis/SDE_FittingServices_20261119 (metadata/effect/validation structure; no recovered universal price formula)"},
                "formula": {"anchor": {"row": "Falcon", "credits": anchor, "eveFrigateMedian": reference},
                            "hull": "FalconCredits*(roleMedian/EVEFrigateMedian)^0.75*roleFactor*(oldPrice/oldSSWClassMedian), bounded to[0.5,2]*oldPrice, rounded1000cr; Falcon fixed",
                            "hullExponent": ROLE_EXPONENT, "perHullBounds": ROW_FACTOR_BOUNDS,
                            "componentPerformance": "(135/115)^((Level-3)/4)", "componentPowerDemand": "(11/10)^((Level-3)/4)",
                            "componentPrice": "(40566/6450)^((Level-1)/4)",
                            "qualityReference": {"typeI": 439, "typeII": 438, "attributePerformance": 20, "attributePower": 30,
                                                 "performanceRatio": performance_ratio, "powerRatio": demand_ratio, "priceRatio": price_ratio},
                            "notChanged": ["hull mass/integrity/thrust/reactor/geometry", "weapons", "component base costs", "repair/replacement times", "rank/empire gates", "spawn/model provisioning"]},
                "classMappings": mappings, "emptyMappedClasses": sorted(set(ROLES) - set(classes)),
                "ships": ship_changes, "componentGrades": grades, "components": components,
                "files": [{"path": relative, "beforeSha256": sha(originals[relative]), "afterSha256": sha(results[relative])} for relative in FILES]}
    return originals, results, manifest


def restore(root: Path, path: Path):
    manifest = json.loads(path.read_text(encoding="utf-8"))
    for item in manifest["files"]:
        if item["path"] not in FILES or sha((root / item["path"]).read_bytes()) != item["afterSha256"]:
            raise ValueError("Restore refused: target changed since calibration; preserve those edits first")
        if sha(Path(item["backup"]).read_bytes()) != item["beforeSha256"]:
            raise ValueError("Backup digest mismatch")
    for item in manifest["files"]:
        shutil.copy2(item["backup"], root / item["path"])
    (root / "Saved/Research/EVE/SDE/ship-system-calibration-current.json").unlink(missing_ok=True)
    print("Restored exact source bytes for ships.json and sys.json")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--restore", type=Path)
    args = parser.parse_args(); root = args.root.resolve()
    if args.restore:
        restore(root, args.restore.resolve()); return
    current = root / "Saved/Research/EVE/SDE/ship-system-calibration-current.json"
    if current.exists():
        prior = json.loads(current.read_text(encoding="utf-8"))
        if all(sha((root / item["path"]).read_bytes()) == item["afterSha256"] for item in prior["files"]):
            print("Already applied; no compounding or source changes"); return
        raise ValueError("A prior calibration exists and source changed; restore/review rather than compound its ratios")
    archive = args.archive or root / "Saved/Research/EVE/SDE/eve-online-static-data-jsonl.zip"
    originals, results, manifest = plan(root, archive)
    if args.apply:
        stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%d-%H%M%SZ") + "-" + uuid.uuid4().hex[:6]
        backup = root / "Saved/Backups" / ("ShipSystemSDECalibration-" + stamp); backup.mkdir(parents=True)
        for item in manifest["files"]:
            saved = backup / Path(item["path"]).name; saved.write_bytes(originals[item["path"]]); item["backup"] = str(saved)
        manifest["manifestPath"] = str(backup / "manifest.json")
        # Save provenance/backups before changing either live catalog.
        text = json.dumps(manifest, indent=2, ensure_ascii=False)
        (backup / "manifest.json").write_text(text, encoding="utf-8")
        try:
            for relative, data in results.items():
                (root / relative).write_bytes(data)
            current.write_text(text, encoding="utf-8")
        except Exception:
            for relative, data in originals.items():
                (root / relative).write_bytes(data)
            current.unlink(missing_ok=True)
            raise
        print("Applied; reversible manifest:", backup / "manifest.json")
    else:
        output = root / "Saved/Research/EVE/SDE/ship-system-calibration-preview.json"
        output.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
        print("Dry run:", output)
    print("Changed hull prices:", sum(row["oldBaseCost"] != row["newBaseCost"] for row in manifest["ships"]), "of", len(manifest["ships"]))
    print("Unmapped preserved:", sum(row["status"] == "unmapped unchanged" for row in manifest["ships"]))
    print("Grade III:", manifest["componentGrades"][2]["new"])


if __name__ == "__main__":
    main()
