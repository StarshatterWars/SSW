"""Local regression checks for an applied SSW calibration; never alters live data."""
import copy
import json
import math
import tempfile
import unittest
from pathlib import Path

import rebalance_ship_systems_from_sde as calibration

ROOT = Path(__file__).resolve().parents[2]
CURRENT = ROOT / "Saved/Research/EVE/SDE/ship-system-calibration-current.json"


@unittest.skipUnless(CURRENT.exists(), "No local applied calibration evidence")
class CalibrationTests(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads(CURRENT.read_text(encoding="utf-8"))
        self.files = self.manifest["files"]
        profiles = ROOT / "Saved/Research/EVE/SDE/weapon-component-profiles-current.json"
        self.profiles = json.loads(profiles.read_text(encoding="utf-8")) if profiles.exists() else None

    def original_calibrated_bytes(self, item):
        current = (ROOT / item["path"]).read_bytes()
        if calibration.sha(current) == item["afterSha256"]:
            return current
        # Weapon overrides are a later independent approved layer. Test the
        # first layer's rollback with its exact after-image, not mixed versions.
        if self.profiles and item["path"] == self.profiles["target"]:
            previous = Path(self.profiles["backup"]).read_bytes()
            self.assertEqual(calibration.sha(previous), item["afterSha256"])
            return previous
        self.fail("Unrecognized edits after the calibrated catalog")

    def test_hulls_change_only_price_and_keep_missing_models(self):
        item = self.files[0]
        old = json.loads(Path(item["backup"]).read_text(encoding="utf-8-sig"))
        new = json.loads((ROOT / item["path"]).read_text(encoding="utf-8-sig"))
        self.assertEqual(len(old), len(new))
        missing = []
        for before, after in zip(old, new):
            a, b = copy.deepcopy(before), copy.deepcopy(after)
            price_a, price_b = a.pop("BaseCost"), b.pop("BaseCost")
            self.assertEqual(a, b, before["Row"])
            self.assertGreaterEqual(price_b, math.ceil(price_a * .5))
            self.assertLessEqual(price_b, math.floor(price_a * 2))
            if not before.get("Model"):
                missing.append(before["Row"])
                self.assertFalse(after.get("Model"))
        self.assertIn("Berents", missing)
        self.assertIn("ZZDD", missing)
        self.assertEqual(next(x["BaseCost"] for x in new if x["Row"] == "Falcon"), self.manifest["formula"]["anchor"]["credits"])

    def test_component_physical_baselines_unchanged_and_quality_has_power_tradeoff(self):
        item = self.files[1]
        old = json.loads(Path(item["backup"]).read_text(encoding="utf-8-sig"))
        new = json.loads((ROOT / item["path"]).read_text(encoding="utf-8-sig"))
        old_grades, new_grades = old.pop("ComponentGrades"), new.pop("ComponentGrades")
        if self.profiles:
            approved = {p["system"]: p["newOverride"] for p in self.profiles["profiles"]}
            for system in new["Systems"]:
                if system["Name"] in approved:
                    self.assertEqual(system.pop("ComponentGrades"), approved[system["Name"]])
        self.assertEqual(old, new)
        self.assertEqual([g["Level"] for g in new_grades], [1, 2, 3, 4, 5])
        self.assertEqual(new_grades[2]["PerformanceMultiplier"], 1)
        self.assertEqual(new_grades[2]["PowerDemandMultiplier"], 1)
        for field in ("PerformanceMultiplier", "PowerDemandMultiplier", "PriceMultiplier"):
            self.assertEqual([g[field] for g in new_grades], sorted(g[field] for g in new_grades))
        for before, after in zip(old_grades, new_grades):
            self.assertEqual(before["RepairScale"], after["RepairScale"])
            self.assertEqual(before["ReplacementScale"], after["ReplacementScale"])

    def test_restore_is_exact_and_refuses_to_overwrite_subsequent_edits(self):
        with tempfile.TemporaryDirectory(prefix="ssw-calibration-check-") as folder:
            root = Path(folder)
            for item in self.files:
                path = root / item["path"]; path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(self.original_calibrated_bytes(item))
            changed = root / self.files[0]["path"]
            changed.write_bytes(changed.read_bytes() + b"\n")
            other_before = (root / self.files[1]["path"]).read_bytes()
            with self.assertRaises(ValueError):
                calibration.restore(root, Path(self.manifest["manifestPath"]))
            self.assertEqual((root / self.files[1]["path"]).read_bytes(), other_before)
            changed.write_bytes(self.original_calibrated_bytes(self.files[0]))
            calibration.restore(root, Path(self.manifest["manifestPath"]))
            for item in self.files:
                self.assertEqual((root / item["path"]).read_bytes(), Path(item["backup"]).read_bytes())


if __name__ == "__main__":
    unittest.main()
