from pathlib import Path
import unittest

import pcbnew


FIXTURE = Path(__file__).parent / "golden" / "simple_board.kicad_pcb"


class PcbnewCompatTests(unittest.TestCase):
    def test_pcbnew_load_requires_native_backend(self):
        with self.assertRaises(Exception) as raised:
            pcbnew.LoadBoard(FIXTURE)

        message = str(raised.exception).lower()
        self.assertIn("native", message)
        self.assertTrue(
            "alternate board-file implementation" in message
            or "target kicad" in message
        )

    def test_pcbnew_units_and_metadata(self):
        self.assertEqual(pcbnew.ToMM(pcbnew.FromMM(1.0)), 1.0)
        self.assertEqual(pcbnew.CompatibilityLevel(), "partial-pcbnew-v10")
        self.assertIn("pybind11-kicad pcbnew compatibility layer", pcbnew.GetBuildVersion())

    def test_unsupported_gui_calls_fail_clearly(self):
        with self.assertRaisesRegex(NotImplementedError, "GUI/editor state"):
            pcbnew.GetPcbFrame()


if __name__ == "__main__":
    unittest.main()
