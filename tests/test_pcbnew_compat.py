from pathlib import Path
import unittest

import pybind11_kicad as kk
import pcbnew


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = Path(__file__).parent / "golden" / "simple_board.kicad_pcb"
TMP_DIR = ROOT / "tmp"


class PcbnewCompatTests(unittest.TestCase):
    def test_pcbnew_load_behavior_matches_backend_mode(self):
        if kk.backend_version() == "kicad-10.0.4-native-pybind11-kicad-0.1":
            board = pcbnew.LoadBoard(FIXTURE)
            self.assertEqual(len(board.GetFootprints()), 1)

            TMP_DIR.mkdir(parents=True, exist_ok=True)
            output = TMP_DIR / "pybind11-kicad-pcbnew-test-roundtrip.kicad_pcb"
            board.Save(output)
            self.assertEqual(len(pcbnew.LoadBoard(output).GetFootprints()), 1)
            return

        with self.assertRaises(Exception) as raised:
            pcbnew.LoadBoard(FIXTURE)

        message = str(raised.exception).lower()
        self.assertIn("native", message)
        self.assertTrue(
            "alternate board-file implementation" in message
            or "board io" in message
        )

    def test_pcbnew_units_and_metadata(self):
        self.assertEqual(pcbnew.ToMM(pcbnew.FromMM(1.0)), 1.0)
        self.assertEqual(pcbnew.FromMils(1), 25_400)
        self.assertEqual(pcbnew.ToMils(25_400), 1.0)
        self.assertEqual(pcbnew.GetMajorMinorVersion(), "10.0")
        self.assertEqual(pcbnew.CompatibilityLevel(), "partial-pcbnew-v10")
        self.assertIn("pybind11-kicad pcbnew compatibility layer", pcbnew.GetBuildVersion())

    def test_pcbnew_value_types_for_kikit_unit_tests(self):
        angle = pcbnew.EDA_ANGLE(180, pcbnew.DEGREES_T)
        self.assertEqual(angle.AsDegrees(), 180)
        self.assertAlmostEqual(pcbnew.EDA_ANGLE(3.141592653589793, pcbnew.RADIANS_T).AsDegrees(), 180)
        self.assertEqual(pcbnew.EDA_ANGLE(900, pcbnew.TENTHS_OF_A_DEGREE_T).AsDegrees(), 90)
        self.assertEqual((2 * pcbnew.EDA_ANGLE(15, pcbnew.DEGREES_T)).AsDegrees(), 30)

        origin = pcbnew.VECTOR2I(10, 20)
        size = pcbnew.VECTOR2I(30, 40)
        box = pcbnew.BOX2I(origin, size)
        self.assertEqual(origin + size, pcbnew.VECTOR2I(40, 60))
        self.assertEqual(size - origin, pcbnew.VECTOR2I(20, 20))
        self.assertEqual(origin[0], 10)
        self.assertEqual(tuple(origin), (10, 20))
        self.assertEqual(box.GetX(), 10)
        self.assertEqual(box.GetY(), 20)
        self.assertEqual(box.GetWidth(), 30)
        self.assertEqual(box.GetHeight(), 40)
        self.assertEqual(box.GetEnd(), pcbnew.VECTOR2I(40, 60))

    def test_unsupported_gui_calls_fail_clearly(self):
        with self.assertRaisesRegex(NotImplementedError, "GUI/editor state"):
            pcbnew.GetPcbFrame()


if __name__ == "__main__":
    unittest.main()
