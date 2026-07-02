from pathlib import Path
import unittest

import pybind11_kicad as kk


FIXTURE = Path(__file__).parent / "golden" / "simple_board.kicad_pcb"


class CleanApiTests(unittest.TestCase):
    def test_initialize_and_backend_version(self):
        config = kk.initialize(
            kicad_dir="/tmp/kicad",
            resource_dir="/tmp/kicad-resources",
            config_dir="/tmp/kikit-config",
        )

        self.assertEqual(config.kicad_dir, "/tmp/kicad")
        self.assertEqual(config.resource_dir, "/tmp/kicad-resources")
        self.assertEqual(kk.runtime_config().config_dir, "/tmp/kikit-config")
        self.assertEqual(kk.target_kicad_major(), 10)
        self.assertIn(kk.backend_version(), {"kicad-10-native-extension-unavailable", "kicad-10-native-scaffold-pybind11-kicad-0.1"})

    def test_open_requires_native_backend(self):
        with self.assertRaises(Exception) as raised:
            kk.Board.open(FIXTURE)

        message = str(raised.exception).lower()
        self.assertIn("native", message)
        self.assertTrue(
            "alternate board-file implementation" in message
            or "target kicad" in message
        )


if __name__ == "__main__":
    unittest.main()
