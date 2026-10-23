"""Reject wrong or modified inputs before an SDL3 port build."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("sdl3_port", Path(__file__).resolve().parents[1] / "scripts/build_sdl3_port.py")
PORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PORT)


class InputTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def test_wrong_stove_release(self):
        (self.root / "provenance.json").write_text(json.dumps({"release": "16.2.0-open1"}))
        with self.assertRaises(ValueError):
            PORT.verify_stove(self.root)

    def test_compiler_tampering(self):
        files = {"bin/m68k-amigaos-g++": "compiler"}
        files.update({f"lib/runtime-{i}": str(i) for i in range(100)})
        hashes = {}
        for name, content in files.items():
            path = self.root / "prefix" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
            hashes[name] = PORT.digest(path)
        (self.root / "provenance.json").write_text(json.dumps({
            "release": "16.2.0-open3", "archive_sha256": PORT.OPEN3,
            "compiler_runtime_sha256": hashes}))
        PORT.verify_stove(self.root)
        (self.root / "prefix/bin/m68k-amigaos-g++").write_text("replacement")
        with self.assertRaisesRegex(ValueError, "changed"):
            PORT.verify_stove(self.root)

    def test_wrong_revision_and_dirty_cache(self):
        def git(*args):
            return subprocess.check_output(["git", "-C", str(self.root), *args], text=True).strip()
        git("init", "-q")
        (self.root / "input").write_text("original")
        git("add", "input")
        git("-c", "user.name=Test", "-c", "user.email=test@example.invalid", "commit", "-qm", "test input")
        pin = git("rev-parse", "HEAD")
        PORT.verify_source(self.root, pin)
        with self.assertRaisesRegex(ValueError, "revision mismatch"):
            PORT.verify_source(self.root, "0" * 40)
        (self.root / "input").write_text("changed")
        with self.assertRaisesRegex(ValueError, "pristine"):
            PORT.verify_source(self.root, pin)
        git("restore", "input")
        (self.root / "extra.h").write_text("unrecorded header")
        with self.assertRaisesRegex(ValueError, "pristine"):
            PORT.verify_source(self.root, pin)


if __name__ == "__main__":
    unittest.main()
