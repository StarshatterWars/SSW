"""Normalize SSW drone throughput using dimensionless EVE mining-cycle ratios.

The native Mining Laser stays the tonnes/second anchor. EVE's mining amount
units and cubic metres are never converted to SSW tonnes. Equipment assignments
below are explicit SSW authoring decisions, not EVE ship-equivalence claims.
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


REFERENCE_MINER = 483
REFERENCE_TYPES = (483, 482, 10246, 10250)
DRONE_ROLES = {
    "Prospector": (10246, "Basic mining shuttle bay receives the basic drone throughput tier"),
    "Rockhopper": (10250, "Specialized mining freighter bay receives the advanced drone throughput tier"),
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cycle_rate(amount: float, duration_ms: float) -> float:
    if not all(math.isfinite(v) and v > 0 for v in (amount, duration_ms)):
        raise ValueError("Mining amount and cycle duration must be finite and positive")
    return amount / (duration_ms / 1000)


def relative_yield(amount: float, duration_ms: float, anchor_amount: float, anchor_ms: float) -> float:
    return round(cycle_rate(amount, duration_ms) / cycle_rate(anchor_amount, anchor_ms), 8)


def patch_rates(text: str, rates: dict[str, float]) -> str:
    decoder = json.JSONDecoder()
    pos = text.index("[") + 1
    patches = []
    while True:
        while text[pos] in " \r\n\t,":
            pos += 1
        if text[pos] == "]":
            break
        row, end = decoder.raw_decode(text, pos)
        if row["ShipDesign"] in rates:
            fragment = text[pos:end]
            fragment, count = re.subn(r'("MiningRateScale"\s*:\s*)(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)',
                lambda m: m[1] + json.dumps(rates[row["ShipDesign"]]), fragment)
            if count != 1:
                raise ValueError("Expected one native mining-rate field")
            patches.append((pos, end, fragment))
        pos = end
    for start, end, fragment in reversed(patches):
        text = text[:start] + fragment + text[end:]
    return text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    root = args.project.resolve()
    source = root / "Content/GameData/Cargo/mining_drones.json"
    weapon_path = root / "Content/GameData/Weapons/wep.json"
    research = root / "Saved/Research/EVE/SDE"
    archive = research / "eve-online-static-data-jsonl.zip"
    output = root / "Saved/Analysis/SDE_MiningYieldRebalance_20261119"
    profile = output / "profile.json"
    previous = json.loads(profile.read_text(encoding="utf8")) if profile.exists() else None
    baseline_dir = Path(previous["baselineDirectory"]) if previous and previous.get("applied") else None
    baseline = baseline_dir / source.name if baseline_dir else source
    old_rows = {r["ShipDesign"]: r for r in json.loads(baseline.read_text(encoding="utf-8-sig"))}
    current_rows = {r["ShipDesign"]: r for r in json.loads(source.read_text(encoding="utf-8-sig"))}
    weapons = json.loads(weapon_path.read_text(encoding="utf-8-sig"))
    laser = next(r for r in weapons if r["Name"] == "Mining Laser")
    native_anchor = laser["MiningTonnesPerSecond"]
    if not math.isfinite(native_anchor) or native_anchor <= 0:
        raise ValueError("Native Mining Laser throughput is unavailable")
    with zipfile.ZipFile(archive) as export:
        with export.open("types.jsonl") as stream:
            types = {r["_key"]: r for r in map(json.loads, stream) if r["_key"] in REFERENCE_TYPES}
        with export.open("typeDogma.jsonl") as stream:
            dogma = {r["_key"]: {a["attributeID"]: a["value"] for a in r["dogmaAttributes"]}
                     for r in map(json.loads, stream) if r["_key"] in REFERENCE_TYPES}
    anchor = dogma[REFERENCE_MINER]
    rates = {}
    evidence = []
    for design, (type_id, rationale) in DRONE_ROLES.items():
        attributes = dogma[type_id]
        scale = relative_yield(attributes[77], attributes[73], anchor[77], anchor[73])
        if not 0 < scale <= 1:
            raise ValueError("Reference ratio exceeds the current native drone catalog range; review instead of silently clipping")
        rates[design] = scale
        old = old_rows[design]
        evidence.append({"shipDesign": design, "rationale": rationale, "eveDroneTypeID": type_id,
            "eveDroneName": types[type_id]["name"]["en"], "eveMiningAmountAttribute77": attributes[77],
            "eveCycleMillisecondsAttribute73": attributes[73], "eveReferenceUnitsPerSecond": cycle_rate(attributes[77], attributes[73]),
            "oldMiningRateScale": old["MiningRateScale"], "newMiningRateScale": scale,
            "oldNativeTonnesPerSecond": native_anchor * old["MiningRateScale"], "newNativeTonnesPerSecond": native_anchor * scale,
            "configuredMaximumActiveDrones": min(current_rows[design]["BayCount"], current_rows[design]["Bandwidth"] // current_rows[design]["DroneBandwidth"]),
            "cargoTonnesUnchanged": old["CapacityTonnes"]})
    current = source.read_bytes().decode("utf-8-sig")
    replacement = patch_rates(current, rates)
    before, after = json.loads(current), json.loads(replacement)
    assert len(before) == len(after)
    for a, b in zip(before, after):
        assert {k: v for k, v in a.items() if k != "MiningRateScale"} == {k: v for k, v in b.items() if k != "MiningRateScale"}
    if args.apply:
        if baseline_dir is None:
            stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%d-%H%M%SZ")
            baseline_dir = root / "Saved/Backups" / ("SDE_MiningYieldRebalance_" + stamp)
            baseline_dir.mkdir(parents=True, exist_ok=False)
            shutil.copy2(source, baseline_dir / source.name)
        source.write_bytes(replacement.encode("utf8"))
    report = {"createdUtc": dt.datetime.now(dt.timezone.utc).isoformat(), "applied": args.apply,
        "baselineDirectory": str(baseline_dir) if baseline_dir else None,
        "officialDownload": json.loads((research / "download.json").read_text(encoding="utf8"))["resolvedUrl"],
        "archiveSha256": sha(archive), "weaponJsonSha256Unchanged": sha(weapon_path),
        "outputSha256": hashlib.sha256(replacement.encode("utf8")).hexdigest(),
        "anchor": {"sswWeapon": "Mining Laser", "nativeTonnesPerSecondUnchanged": native_anchor,
                   "eveTypeID": REFERENCE_MINER, "eveName": types[REFERENCE_MINER]["name"]["en"],
                   "eveMiningAmountAttribute77": anchor[77], "eveCycleMillisecondsAttribute73": anchor[73]},
        "referenceTiers": [{"typeID": i, "name": types[i]["name"]["en"],
                            "rateRelativeToMinerI": relative_yield(dogma[i][77], dogma[i][73], anchor[77], anchor[73])} for i in REFERENCE_TYPES],
        "evidence": evidence,
        "unchanged": ["Native laser0.25 tonnes/second anchor", "Ore and product identity/tonne units", "Drone counts separately authored as Prospector3/Rockhopper10 by user request",
                      "Drone hold capacity/speed/control range/mining range", "Reserve/extract/return/unload ownership rules", "Crystal wear/yield modifiers"],
        "limits": ["Dimensionless reference output relationships only; no EVE volume-to-mass conversion.",
                   "Bay-tier assignment is authored SSW policy, not an equivalent EVE ship or player skill claim.",
                   "EVE Miner II ratio1.5 is recorded for research but no second SSW laser is invented.",
                   "Travel, station keeping, hold capacity and return-only credit continue to bound delivered ore per minute.",
                   "No SDE residue/probability is silently introduced, and no asteroid ore is credited before extraction/return."]}
    output.mkdir(parents=True, exist_ok=True)
    destination = profile if args.apply else output / "preview.json"
    destination.write_text(json.dumps(report, indent=2), encoding="utf8")
    if args.apply:
        lines = ["SSW mining yield normalization from official EVE SDE3714173", "",
                 f"Native Mining Laser remains {native_anchor}t/s. EVE MinerI483={anchor[77]} per{anchor[73]}ms.",
                 "MiningRateScale=(EVEdrone miningAmount/cycle)/(EVEMinerI miningAmount/cycle). No EVE unit becomes a tonne.", ""]
        for row in evidence:
            lines.append(f"{row['shipDesign']}: scale{row['oldMiningRateScale']} -> {row['newMiningRateScale']}; native per-drone extraction{row['oldNativeTonnesPerSecond']} -> {row['newNativeTonnesPerSecond']}t/s. EVE{row['eveDroneTypeID']} {row['eveDroneName']}.")
        lines += ["", "Backups: " + str(baseline_dir), "Capacities, movement, crystals, weapon data and cargo credit rules unchanged. Drone counts separately authored as Prospector3/Rockhopper10 by user request."] + report["limits"]
        (output / "report.txt").write_text("\n".join(lines), encoding="utf8")
    print(json.dumps({"applied": args.apply, "report": str(destination), "nativeLaserUnchanged": native_anchor, "droneScales": rates}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
