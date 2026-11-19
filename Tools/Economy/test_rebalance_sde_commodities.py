import unittest

from rebalance_sde_commodities import calibrated_price, calibrated_time, patch_fields


class CommodityCalibrationTests(unittest.TestCase):
    def test_native_credit_anchor_and_bounded_prices(self):
        self.assertEqual(calibrated_price(280, 500, .56), 280)
        self.assertEqual(calibrated_price(1000, 600, .56), 800)
        self.assertEqual(calibrated_price(220, 505, .56), 234)
        for reference in [2, 35, 185, 230, 500, 600, 32768]:
            value = calibrated_price(100, reference, .56)
            self.assertGreaterEqual(value, 80)
            self.assertLessEqual(value, 120)

    def test_placeholder_prices_cannot_drive_the_economy(self):
        for invalid in [0, 1, -1, float("nan")]:
            with self.assertRaises(ValueError):
                calibrated_price(100, invalid, .56)

    def test_relative_cycle_calibration_retains_short_native_loop(self):
        self.assertEqual(calibrated_time(35, 1800, 1800, 35), 35)
        self.assertEqual(calibrated_time(45, 3600, 1800, 35), 49.497)
        self.assertEqual(calibrated_time(200, 3600, 1800, 35), 160)

    def test_row_patching_preserves_other_data_and_is_idempotent(self):
        source = '[\r\n  {"Name":"water","Value":25,"Size":1,"Description":"A"},\r\n  {"Name":"other","Value":100,"Size":2}\r\n]\r\n'
        target = source.replace('"Value":25', '"Value":24')
        result = patch_fields(source, {"water": {"Value": 24}})
        self.assertEqual(result, target)
        self.assertEqual(patch_fields(result, {"water": {"Value": 24}}), target)


if __name__ == "__main__":
    unittest.main()
