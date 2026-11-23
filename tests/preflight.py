#!/usr/bin/env python3
"""Native vendor preflight: checks intent, not a substitute for runtime save tests."""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "openrac-mod.json").read_text())
source = (root / "src/mod.cpp").read_text()
assert manifest["id"] == "armor-swap"
assert ('OPENRAC_MOD_CPP("armor-swap", "' + manifest["version"] + '", load)') in source
assert manifest["games"] == {"rac2": ["ntsc", "pal", "ntsc2"]}
assert manifest["api"] == "1.34"
assert manifest["settings"][0]["key"] == "appearance"
assert "openrac::rac2::ArmorVendor_BuildList" in source
assert "openrac::rac2::ArmorVendor_Buy" in source
assert "openrac::rac2::Vendor_Preview" in source
assert "s.mod->memory().write(answer, std::int32_t{0});" in source, (
    "Native purchase confirmation must be cancelled"
)
assert "request_mode(5, 1, 3u, 0, 0u)" in source
assert "s.open_vendor(" not in source
assert "bytes[1] |= 0x40u;" in source, "Cross should not reach native purchase path"
assert set(manifest["hooks"]) == {
    "rac2.armor_vendor.build_list", "rac2.armor_vendor.buy", "rac2.vendor.preview"
}
# The only allowed write to the game's progress block is an emergency
# restoration of the value recorded BEFORE opening our cosmetic vendor.
assert source.count("memory().write(base + 0x20u") == 1, (
    "Unexpected write(s) to the saved armour field"
)
assert "static_cast<std::int16_t>(s.protection_before)" in source
assert (root / "LICENSE").is_file()
print("Preflight OK: native cosmetic vendor hooks and protection rollback")
