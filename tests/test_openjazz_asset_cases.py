"""Check validation coverage and reject ambiguous asset names."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('jazz_assets', ROOT/'ports/openjazz/validation/run_assets.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class JazzAssetCases(unittest.TestCase):
    def test_inventory_selects_levels_and_cutscenes_without_executables(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ['MENU.000','STARTUP.0SC','END.0SC','LEVEL0.000','LEVEL7.030','LEVEL0.000.old','JAZZ.EXE']:
                (root/name).touch()
            self.assertEqual(module.cases_for(root), [('menu','MENU.000'),('scene','END.0SC'),
                ('scene','STARTUP.0SC'),('level','LEVEL0.000'),('level','LEVEL7.030')])
    def test_case_collision_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'PANEL.000').touch()
            (root/'panel.000').touch()
            with self.assertRaisesRegex(ValueError,'Case-colliding'):
                module.cases_for(root)

    def test_guest_script_has_explicit_results_and_rejects_unsafe_paths(self):
        script = module.amiga_script([('level', 'LEVEL0.000')], 'DH1:Games/Jazz')
        self.assertIn('Echo "CASE 000 RC $RC"', script)
        self.assertIn('>DH1:Games/Jazz/Results/000.txt', script)
        self.assertIn('Echo "MISSING RC $RC"', script)
        for root in ['relative', 'DH1:Jazz\nExecute bad', 'DH1:'+'x'*100]:
            with self.assertRaises(ValueError):
                module.amiga_script([('level', 'LEVEL0.000')], root)

    def test_sound_directory_bounds_and_sample_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)/'SOUNDS.000'
            entry = b'TEST'+b'\0'*8+(4).to_bytes(4,'little')+(3).to_bytes(2,'little')
            p.write_bytes(b'sfx\x1aabc'+entry+(7).to_bytes(4,'little'))
            result = module.audit_sound_bank(p)
            self.assertEqual(result['clips'],1)
            self.assertEqual(result['sample_names'],['TEST'])
            p.write_bytes(b'sfx\x1aabc'+entry[:16]+(4).to_bytes(2,'little')+(7).to_bytes(4,'little'))
            with self.assertRaisesRegex(ValueError,'sample exceeds'):
                module.audit_sound_bank(p)
            p.write_bytes(b'sfx\x1aabc'+entry+(999).to_bytes(4,'little'))
            with self.assertRaisesRegex(ValueError,'directory bounds'):
                module.audit_sound_bank(p)
