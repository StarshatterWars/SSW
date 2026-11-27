"""Validate authored facility coverage and deck contracts without launching Unreal."""
import copy
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ships = json.loads((ROOT / "Content/GameData/Ships/ships.json").read_text(encoding="utf-8"))
catalog = json.loads((ROOT / "Content/GameData/Ships/flight_facilities.json").read_text(encoding="utf-8"))
assert catalog["schemaVersion"] == 1
policies = {r["Name"]: r for r in catalog["facilities"]}
assert len(policies) == len(catalog["facilities"])
facilities = {s["Row"]: s for s in ships if s["ShipClass"].lower() in ("carrier", "station", "starbase")}
assert set(policies) == set(facilities), "Missing or orphan facility policy"
docking = {n for n,p in policies.items() if p["Mode"] == "DockingOnly"}
assert docking == {"Colony", "Station7", "Mining Station"}
count = 0
for name, ship in facilities.items():
    p = policies[name]
    assert p["AzimuthUnit"] in ("Degrees", "Radians")
    assert math.isfinite(p["CoordinateScale"]) and p["CoordinateScale"] > 0
    overrides = {o["Index"]:o["Replacement"] for o in p["DeckOverrides"]}
    assert len(overrides) == len(p["DeckOverrides"])
    if name in docking:
        assert not ship["FlightDeck"] and not overrides
        continue
    assert p["Mode"] == "FlightOperations"
    assert all(0 <= i < len(ship["FlightDeck"]) for i in overrides)
    roles = set()
    for i, original in enumerate(ship["FlightDeck"]):
        d = copy.deepcopy(overrides.get(i,original))
        az = d["AzimuthRadians"]
        if i not in overrides and p["AzimuthUnit"] == "Degrees": az = math.radians(az)
        assert math.isfinite(az) and abs(az) <= math.tau + 1e-5, (name,i)
        assert d["bLaunchDeck"] != d["bRecoveryDeck"]
        roles.add("launch" if d["bLaunchDeck"] else "recovery")
        assert d["Size"] > 0 and d["CycleTime"] > 0 and d["MaxShips"] >= 0
        assert 0 < len(d["Slots"]) <= 10
        assert len(d["ApproachPoints"]) <= 4 and len(d["RunwayPoints"]) <= 2
        for slot in d["Slots"]:
            assert 0 < slot["FilterMask"] <= 15
            assert all(math.isfinite(x) for x in slot["Location"].values())
        for key in ("Location", "StartPoint", "EndPoint", "CamLocation", "BoundingBox"):
            assert all(math.isfinite(x) for x in d[key].values())
        if d["bRecoveryDeck"]:
            dx=d["EndPoint"]["X"]-d["StartPoint"]["X"]
            dz=d["EndPoint"]["Z"]-d["StartPoint"]["Z"]
            assert math.hypot(dx,dz)>0
            assert (dx*math.sin(az)+dz*math.cos(az))/math.hypot(dx,dz)>.9, (name,i,"opposed recovery heading")
        count += 1
    assert roles == {"launch","recovery"}, name
print(json.dumps({"facilities":len(facilities),"flightOperations":len(facilities)-len(docking),"dockingOnly":sorted(docking),"decks":count,"status":"PASS"},indent=2))
