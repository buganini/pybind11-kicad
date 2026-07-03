from pathlib import Path
import unittest

import pybind11_kicad as kk


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = Path(__file__).parent / "golden" / "simple_board.kicad_pcb"
TMP_DIR = ROOT / "tmp"


class CleanApiTests(unittest.TestCase):
    def test_initialize_and_backend_version(self):
        kicad_dir = str(TMP_DIR / "kicad")
        resource_dir = str(TMP_DIR / "kicad-resources")
        config_dir = str(TMP_DIR / "kikakuka-config")

        config = kk.initialize(
            kicad_dir=kicad_dir,
            resource_dir=resource_dir,
            config_dir=config_dir,
        )

        self.assertEqual(config.kicad_dir, kicad_dir)
        self.assertEqual(config.resource_dir, resource_dir)
        self.assertEqual(kk.runtime_config().config_dir, config_dir)
        self.assertEqual(kk.target_kicad_major(), 10)
        self.assertEqual(kk.target_kicad_version(), "10.0.4")
        self.assertIn(
            kk.backend_version(),
            {
                "kicad-10.0.4-native-extension-unavailable",
                "kicad-10.0.4-native-scaffold-pybind11-kicad-0.1",
                "kicad-10.0.4-native-pybind11-kicad-0.1",
            },
        )

    def test_board_open_behavior_matches_backend_mode(self):
        if kk.backend_version() == "kicad-10.0.4-native-pybind11-kicad-0.1":
            board = kk.Board.open(FIXTURE)
            self.assertEqual(len(board.footprints()), 1)

            TMP_DIR.mkdir(parents=True, exist_ok=True)
            output = TMP_DIR / "pybind11-kicad-test-roundtrip.kicad_pcb"
            board.save(output)
            self.assertEqual(len(kk.Board.open(output).footprints()), 1)
            return

        with self.assertRaises(Exception) as raised:
            kk.Board.open(FIXTURE)

        message = str(raised.exception).lower()
        self.assertIn("native", message)
        self.assertTrue(
            "alternate board-file implementation" in message
            or "board io" in message
        )


if __name__ == "__main__":
    unittest.main()
