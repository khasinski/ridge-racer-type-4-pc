"""Corrupt cards must terminate parsing before export writes any files."""
import importlib.util
import pathlib
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('mcd', pathlib.Path(__file__).resolve().parents[2] / 'tools/port/mcd.py')
mcd = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mcd)

class CardTests(unittest.TestCase):
    def test_roundtrip_and_corrupt_chains(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'card.mcd'
            mcd.write_card(path, [('BESLES-01706R4', b'A' * 16384)])
            valid = path.read_bytes()
            self.assertEqual(mcd.read_card(path), [('BESLES-01706R4', b'A' * 16384)])
            for link in [0, 15, 0xfffe]:
                damaged = bytearray(valid)
                damaged[136:138] = link.to_bytes(2, 'little')
                path.write_bytes(damaged)
                with self.assertRaises(SystemExit): mcd.read_card(path)
            for name in [b'../escape', b'/absolute', b'..\\escape', b'C:escape', b'\xff', b'']:
                damaged = bytearray(valid)
                damaged[138:158] = name.ljust(20, b'\0')
                path.write_bytes(damaged)
                with self.assertRaises(SystemExit): mcd.read_card(path)
            damaged = bytearray(valid)
            damaged[132:136] = (200000).to_bytes(4, 'little')
            path.write_bytes(damaged)
            with self.assertRaises(SystemExit): mcd.read_card(path)

if __name__ == '__main__': unittest.main()
