import math
import unittest

from rebalance_sde_mining import cycle_rate, patch_rates, relative_yield


class MiningYieldCalibrationTests(unittest.TestCase):
    def test_published_cycle_ratios_are_dimensionless(self):
        self.assertEqual(relative_yield(25, 60000, 10, 15000), .625)
        self.assertEqual(relative_yield(33, 60000, 10, 15000), .825)
        self.assertEqual(relative_yield(15, 15000, 10, 15000), 1.5)
        self.assertEqual(.25 * relative_yield(25, 60000, 10, 15000), .15625)
        self.assertAlmostEqual(.25 * relative_yield(33, 60000, 10, 15000), .20625)

    def test_invalid_cycle_data_cannot_produce_yield(self):
        for amount, duration in [(0, 1), (1, 0), (-1, 1), (math.nan, 1), (1, math.inf)]:
            with self.assertRaises(ValueError):
                cycle_rate(amount, duration)

    def test_only_native_scale_changes_and_reapply_does_not_compound(self):
        original = '[\r\n {"ShipDesign":"Prospector","CapacityTonnes":0.25,"MiningRateScale":0.25},\r\n {"ShipDesign":"Rockhopper","CapacityTonnes":0.5,"MiningRateScale":0.25}\r\n]'
        target = original.replace('"MiningRateScale":0.25', '"MiningRateScale":0.625', 1)
        patched = patch_rates(original, {"Prospector": .625})
        self.assertEqual(patched, target)
        self.assertEqual(patch_rates(patched, {"Prospector": .625}), target)


if __name__ == "__main__":
    unittest.main()
