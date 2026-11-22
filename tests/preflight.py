#!/usr/bin/env python3
"""Cheap checks before six-platform build. Not an in-game verification."""
from pathlib import Path
import json
root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'openrac-mod.json').read_text())
source = (root / 'src/mod.cpp').read_text()
assert manifest['id'] == 'armor-swap'
assert manifest['version'] == '0.1.0-dev.1'
assert 'OPENRAC_MOD_CPP("armor-swap", "0.1.0-dev.1", load)' in source
assert manifest['games'] == {'rac2': ['ntsc','pal','ntsc2']}
assert manifest['settings'][0]['key'] == 'appearance'
assert 'memory().write' not in source, 'Armour progress must not be overwritten'
assert (root / 'LICENSE').is_file()
print('Preflight OK: prototype manifest, version and read-only armour logic')
