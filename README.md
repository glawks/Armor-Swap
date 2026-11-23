# Armor Swap — Native RAC2 Armour Vendor Prototype

**Experimental test build `0.1.0-native.1` — NOT a stable release.**
This branch starts from the **tested dev5 commit** and does not modify it, `dev`, or `main`.

The previous test confirmed that cosmetic Carbonox and the original suit keep the game's real protection unchanged and that saves remain intact. This branch **replaces only the custom on-screen selection panel with RAC2's native armour vendor**.

## Test the real armour vendor

1. Back up your RAC2 save. Install the ZIP from a successful `native-vendor-dev` [Actions run](https://github.com/glawks/Armor-Swap/actions?query=branch%3Anative-vendor-dev).
2. Stand at the Oozla bars near **X297 Y418 Z115**, as for dev5. **Triangle/Y** should still show the in-world interaction prompt.
3. Press Triangle/Y to open RAC2's **actual Megacorp armour vendor UI** with the rotating Ratchet model. It offers all **five cosmetic looks** (Original suit, Tetrafiber, Duraplate, Electrosteel, Carbonox), even if unpurchased in the actual game.
4. Navigate using the game's *native* vendor controls. For this prototype the vendor still uses its normal **zero-bolt purchase confirmation wording**. Confirm an armour to choose its **appearance**, without paying bolts or buying the armour tier. Back out of the vendor normally.
5. After returning to gameplay, Ratchet should use the chosen appearance with unchanged saved protection.

## Technical and safety notes

- The test uses OpenRAC's **SDK 1.34** and three hooks: `rac2.armor_vendor.build_list`, `rac2.armor_vendor.buy`, `rac2.vendor.preview`. These hooks are required; the mod refuses loading on an older port that lacks them.
- Calls the game's own **OpenVendorMenu(mode 3)** (same Megacorp vendor class as in the retail game), rather than constructing an imitation window.
- While the **Oozla test vendor** is active, it overwrites the temporary vendor **menu list** with five zero-bolt armour rows, *not* the saved armour or bolt values.
- On confirming a choice, the purchase hook writes the mod-only cosmetic preference, then changes the **purchase answer** from Yes to No before forwarding to the original handler. Normal armour vendors elsewhere remain untouched.
- A native armour vendor ordinarily restores the saved armour value on close. This test checks protection against the value recorded at entry. If it unexpectedly changes, a **safety rollback restores the original 0–4 armour tier in game memory and logs a warning**. This rollback is the *only case* when the mod writes the base game's armour value; it is an emergency safeguard, not part of cosmetic selection. Test with a backed-up save.
- The game's native preview normally skips reloading pack 0; this prototype explicitly reloads it to show the Original suit if the current cosmetic is different.
- The original five-model list is not changed for other armour vendors, including their prices and progression.
- Cosmetic choices are stored in OpenRAC's mod-specific per-save data, as in dev5. Game save progression is never intentionally upgraded.
- Other mods that replace the armour vendor's purchase/list handling may conflict; test without them initially.
- The **native vendor interaction, especially its confirmation flow and preview, has not yet been tested in-game**. Build success is only a first step.

If the menu fails, exits incorrectly, gives permanent armour, charges bolts, or hangs input, disable this prototype and return to the known-good dev5 build. Do not use it as your release version.

## Building

Pinned OpenRAC mod SDK **1.34.0**, verified by SHA-256. RAC2 NTSC/PAL/NTSC2. CI builds Windows, Linux, macOS and Android (six architectures).

```bash
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

MIT licensed. No proprietary game assets included.
