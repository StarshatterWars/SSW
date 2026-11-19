"""Calibrate selected SSW commodity relationships from the official EVE SDE.

This is an offline authoring tool, never a game dependency. Static base prices
are reference metadata, not market quotations. EVE volumes are not SSW tonnes.
Unmapped minerals, fictional goods, combat values and ship costs stay untouched.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import zipfile


MAPPINGS = {
    "water": (3645, "Water: direct commodity role"),
    "construction materials": (3828, "Construction Blocks: structural construction commodity"),
    "machine parts": (3689, "Mechanical Parts: manufactured mechanical components"),
    "finished electronics": (9836, "Consumer Electronics: finished electronic assemblies; SSW crate unit retained"),
    "optical glass": (3697, "Silicate Glass: industrial glass commodity; SSW grade retained"),
    "thruster propellant": (9830, "Rocket Fuel: chemical propulsion consumable; no ship fuel-rate change"),
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def calibrated_price(old: int, reference_price: float, anchor: float) -> int:
    if old <= 0 or reference_price <= 1 or not math.isfinite(reference_price):
        raise ValueError("Positive native price and non-placeholder reference price required")
    # A fourth-root blend preserves native quality/unit premiums while moving
    # relative values toward the published commodity relationship. Never ISK=CR.
    factor = max(.8, min(1.2, ((reference_price * anchor) / old) ** .25))
    return max(1, round(old * factor))


def calibrated_time(old: float, source_cycle: float, water_cycle: float, native_water_time: float) -> float:
    # Compress the published basic:advanced cycle ratio into the short SSW test
    # loop; this policy is original SSW tuning, not an EVE server equation.
    target = native_water_time * math.sqrt(source_cycle / water_cycle)
    return round(max(old * .8, min(old * 1.2, target)), 3)


def patch_fields(text: str, changes: dict[str, dict]) -> str:
    """Replace only selected numeric row fields, preserving unrelated bytes."""
    decoder = json.JSONDecoder()
    pos = text.index("[") + 1
    patches = []
    while True:
        while pos < len(text) and text[pos] in " \r\n\t,":
            pos += 1
        if text[pos] == "]":
            break
        row, end = decoder.raw_decode(text, pos)
        if row["Name"] in changes:
            fragment = text[pos:end]
            for field, value in changes[row["Name"]].items():
                pattern = re.compile(r'("' + re.escape(field) + r'"\s*:\s*)(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)')
                fragment, count = pattern.subn(lambda m: m[1] + json.dumps(value), fragment)
                if count != 1:
                    raise ValueError(f"Expected one {field} in {row['Name']}")
            patches.append((pos, end, fragment))
        pos = end
    for start, end, fragment in reversed(patches):
        text = text[:start] + fragment + text[end:]
    return text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--apply", action="store_true", help="Back up sources and apply the reported numeric changes")
    args = parser.parse_args()
    root = args.project.resolve()
    research = root / "Saved/Research/EVE/SDE"
    archive = research / "eve-online-static-data-jsonl.zip"
    cargo_path = root / "Content/GameData/Cargo/cargo.json"
    recipe_path = root / "Content/GameData/Cargo/recipes.json"
    report_dir = root / "Saved/Analysis/SDE_CommodityRebalance_20261119"
    profile_path = report_dir / "profile.json"
    previous = json.loads(profile_path.read_text(encoding="utf8")) if profile_path.exists() else None
    # Reproducible from a preserved baseline; running the authoring tool twice
    # never repeatedly discounts the same catalog.
    baseline_dir = Path(previous["baselineDirectory"]) if previous and previous.get("applied") else None
    base_cargo_path = baseline_dir / "cargo.json" if baseline_dir else cargo_path
    base_recipe_path = baseline_dir / "recipes.json" if baseline_dir else recipe_path
    cargo = json.loads(base_cargo_path.read_text(encoding="utf-8-sig"))
    recipes = json.loads(base_recipe_path.read_text(encoding="utf-8-sig"))
    cargo_by_name = {r["Name"]: r for r in cargo}
    recipes_by_name = {r["Name"]: r for r in recipes}
    with zipfile.ZipFile(archive) as export:
        with export.open("types.jsonl") as stream:
            types = {r["_key"]: r for r in map(json.loads, stream)}
        with export.open("planetSchematics.jsonl") as stream:
            schematics = list(map(json.loads, stream))
    anchor = cargo_by_name["construction materials"]["Value"] / types[3828]["basePrice"]
    water_schematic = next(r for r in schematics if r["_key"] == 121)
    native_water_time = recipes_by_name["WaterRecovery"]["SecondsPerBatch"]
    price_changes, time_changes, evidence = {}, {}, []
    for name, (type_id, reason) in MAPPINGS.items():
        native = cargo_by_name[name]
        reference = types[type_id]
        new = calibrated_price(native["Value"], reference["basePrice"], anchor)
        price_changes[name] = {"Value": new}
        proof = {"item": name, "eveTypeID": type_id, "eveName": reference["name"]["en"],
                 "eveStaticBasePrice": reference["basePrice"], "oldCredits": native["Value"], "newCredits": new,
                 "nativeUnit": native["Unit"], "nativeUnitTonnes": native["Size"], "rationale": reason}
        matching = [r for r in schematics if any(t["_key"] == type_id and not t["isInput"] for t in r["types"])]
        for recipe in recipes:
            if recipe["CargoItem"] != name or not matching:
                continue
            schematic = matching[0]
            new_time = calibrated_time(recipe["SecondsPerBatch"], schematic["cycleTime"], water_schematic["cycleTime"], native_water_time)
            time_changes[recipe["Name"]] = {"SecondsPerBatch": new_time}
            proof.setdefault("production", []).append({"recipe": recipe["Name"], "eveSchematicID": schematic["_key"],
                "eveCycleSeconds": schematic["cycleTime"], "oldSeconds": recipe["SecondsPerBatch"], "newSeconds": new_time,
                "inputsUnchanged": True, "outputsUnchanged": True})
        evidence.append(proof)
    cargo_text = cargo_path.read_bytes().decode("utf-8-sig")
    recipe_text = recipe_path.read_bytes().decode("utf-8-sig")
    new_cargo_text = patch_fields(cargo_text, price_changes)
    new_recipe_text = patch_fields(recipe_text, time_changes)
    actual_cargo, actual_recipes = json.loads(cargo_text), json.loads(recipe_text)
    proposed_cargo, proposed_recipes = json.loads(new_cargo_text), json.loads(new_recipe_text)
    for before, after in zip(actual_cargo, proposed_cargo):
        assert {k: v for k, v in before.items() if k != "Value"} == {k: v for k, v in after.items() if k != "Value"}
    for before, after in zip(actual_recipes, proposed_recipes):
        assert {k: v for k, v in before.items() if k != "SecondsPerBatch"} == {k: v for k, v in after.items() if k != "SecondsPerBatch"}
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%d-%H%M%SZ")
    if args.apply:
        if baseline_dir is None:
            baseline_dir = root / "Saved/Backups" / ("SDE_CommodityRebalance_" + stamp)
            baseline_dir.mkdir(parents=True, exist_ok=False)
            shutil.copy2(cargo_path, baseline_dir / cargo_path.name)
            shutil.copy2(recipe_path, baseline_dir / recipe_path.name)
        cargo_path.write_bytes(new_cargo_text.encode("utf8"))
        recipe_path.write_bytes(new_recipe_text.encode("utf8"))
    report = {"createdUtc": dt.datetime.now(dt.timezone.utc).isoformat(), "applied": args.apply,
        "baselineDirectory": str(baseline_dir) if baseline_dir else None,
        "archiveSha256": digest(archive), "officialDownload": json.loads((research / "download.json").read_text(encoding="utf8"))["resolvedUrl"],
        "policy": {"anchor": "Existing SSW construction-material credit price / EVE Construction Blocks static basePrice",
                   "creditsPerReferenceBaseUnit": anchor, "priceBlendExponent": .25, "maximumRelativeChange": .2,
                   "timeReference": "WaterRecovery35s * sqrt(EVE schematic cycle / Water schematic1800s), bounded20%",
                   "units": "Native catalog units, unit masses, quantities and yields retained; no EVE volume-to-tonne conversion"},
        "evidence": evidence, "unmappedUnchanged": sorted(set(cargo_by_name) - set(MAPPINGS)),
        "notImported": ["EVE live prices or stocks", "placeholder basePrice1 industrial inputs", "EVE art or descriptions",
                        "EVE mineral yields in units as SSW tonnes", "fictitious SSW minerals without direct counterparts"],
        "outputHashes": {str(cargo_path): hashlib.sha256(new_cargo_text.encode("utf8")).hexdigest(),
                         str(recipe_path): hashlib.sha256(new_recipe_text.encode("utf8")).hexdigest()}}
    report_dir.mkdir(parents=True, exist_ok=True)
    # A dry-run never replaces the applied profile/baseline identity.
    destination = profile_path if args.apply else report_dir / "preview.json"
    destination.write_text(json.dumps(report, indent=2), encoding="utf8")
    print(json.dumps({"applied": args.apply, "report": str(destination), "mappedItems": len(evidence),
                      "pricesChanged": sum(r["oldCredits"] != r["newCredits"] for r in evidence),
                      "recipesChanged": sum(x["oldSeconds"] != x["newSeconds"] for r in evidence for x in r.get("production", []))}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
