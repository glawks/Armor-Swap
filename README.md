# Armor Swap — Native RAC2 Armour Vendor Prototype

**Experimental test build `0.1.0-native.2` — NOT a stable release.**
This branch starts from the **tested dev5 commit** and does not modify it, `dev`, or `main`.

The previous test confirmed that cosmetic Carbonox and the original suit keep the game's real protection unchanged and that saves remain intact. This branch **replaces only the custom on-screen selection panel with RAC2's native armour vendor**.

## Test the real armour vendor

1. Back up your RAC2 save. Install the ZIP from a successful `native-vendor-dev` [Actions run](https://github.com/glawks/Armor-Swap/actions?query=branch%3Anative-vendor-dev).
2. Stand at the Oozla bars near **X297 Y418 Z115**, as for dev5. **Triangle/Y** should still show the in-world interaction prompt.
3. Press Triangle/Y to open RAC2's **actual Megacorp armour vendor UI** with the rotating Ratchet model. It offers all **five cosmetic looks** (Original suit, Tetrafiber, Duraplate, Electrosteel, Carbonox), even if unpurchased in the actual game.
4. Navigate with the game's native controls. **Press Cross/Xbox A on an appearance to equip it cosmetically**. This skips the game's purchase confirmation and never buys armour; the selected appearance is saved in mod-specific data.
5. Press **Triangle/Xbox Y to exit the vendor normally**, then verify Ratchet's new appearance in gameplay. The game must no longer freeze after exiting.
6. Check the Save Editor: protection and bolts should remain unchanged after saving and reloading.

## Technical and safety notes

- The test uses OpenRAC's **SDK 1.34** and three hooks: `rac2.armor_vendor.build_list`, `rac2.armor_vendor.buy`, `rac2.vendor.preview`. These hooks are required; the mod refuses loading on an older port that lacks them.
- Calls `request_begin_slot(5, 1, 3, 0, nullptr)` to enter the original Megacorp armour vendor through the **proper game-mode stack**. This is the crucial fix for the freeze after leaving the previous prototype: directly calling `OpenVendorMenu(3)` bypassed gameplay's saved return mode.
- While the **Oozla test vendor** is active, it overwrites the temporary vendor **menu list** with five zero-bolt armour rows, *not* the saved armour or bolt values.
- When the native vendor is active, **Cross/A directly sets the cosmetic choice and is swallowed** before RAC2's purchase code runs. This bypasses the native shop's `Can't buy any armor` restriction on Carbonox. The purchase hook remains as a defensive fallback and cancels any confirmation that slips through. Normal armour vendors elsewhere remain untouched.
- A native armour vendor ordinarily restores the saved armour value on close. This test checks protection against the value recorded at entry. If it unexpectedly changes, a **safety rollback restores the original 0–4 armour tier in game memory and logs a warning**. This rollback is the *only case* when the mod writes the base game's armour value; it is an emergency safeguard, not part of cosmetic selection. Test with a backed-up save.
- The game's native preview normally skips reloading pack 0; this prototype explicitly reloads it to show the Original suit if the current cosmetic is different.
- The original five-model list is not changed for other armour vendors, including their prices and progression.
- Cosmetic choices are stored in OpenRAC's mod-specific per-save data, as in dev5. Game save progression is never intentionally upgraded.
- Other mods that replace the armour vendor's purchase/list handling may conflict; test without them initially.
- The earlier native prototype proved that the native preview opens and displays armour, **but leaving the menu froze gameplay**. This version uses the proper entry transition and still needs an in-game test of opening, Cross/A equip, Triangle/Y exit, return to gameplay, and saved protection.

This is **still experimental**. The game has no physical armour-vendor moby at the bars; it summons the vanilla armour vendor *menu* in place. A physical vendor prop could be added separately. If the menu still hangs or alters protection, disable the prototype and return to the tested dev5 version. Do not distribute as a release yet.

## Building

Pinned OpenRAC mod SDK **1.34.0**, verified by SHA-256. RAC2 NTSC/PAL/NTSC2. CI builds Windows, Linux, macOS and Android (six architectures).

```bash
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

MIT licensed. No proprietary game assets included.
