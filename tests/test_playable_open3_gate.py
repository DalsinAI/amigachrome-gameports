"""Regression guard for the playable-port compiler campaign."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
RECIPES = (
    "ports/uqm/build-amigaos3.sh",
    "ports/assaultcube/build-amiga-client.sh",
    "ports/assaultcube/build-amiga-server.sh",
    "ports/nxengine-evo/build-amigaos3.sh",
    "ports/chocolate-doom/build-amigaos3.sh",
    "ports/sdlpop/build-amigaos3.sh",
)


class PlayableOpen3GateTests(unittest.TestCase):
    def test_priority_recipes_require_verified_open3_stove(self):
        for relative in RECIPES:
            with self.subTest(recipe=relative):
                text = (ROOT / relative).read_text(encoding="utf-8")
                self.assertIn("verify_gameport_stove.py", text)
                self.assertIn("16.2.0-open3", text)
                self.assertNotIn('STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}', text)

    def test_cmake_priority_recipes_pass_selected_stove_explicitly(self):
        for relative in ("ports/chocolate-doom/build-amigaos3.sh", "ports/sdlpop/build-amigaos3.sh"):
            with self.subTest(recipe=relative):
                text = (ROOT / relative).read_text(encoding="utf-8")
                self.assertIn('-DAMIGA_STOVE="$STOVE"', text)

    def test_gate_defaults_to_open3(self):
        text = (ROOT / "scripts/verify_gameport_stove.py").read_text(encoding="utf-8")
        self.assertIn('default="16.2.0-open3"', text)


if __name__ == "__main__":
    unittest.main()
