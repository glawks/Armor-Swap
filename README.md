# Armor Swap — RAC2 armour transmog

Change Ratchet's **visible** armour without touching the strongest armour purchased or its damage reduction. The visual model-swapping mechanism has been tested in-game. This **v0.1.0-dev.6 test** keeps the working Triangle/Y vendor popup and permits all cosmetic armour looks on every save.

## Oozla gameplay vendor — test

- **Approach the ship landing bars/walkway on Planet Oozla.** The trigger uses a *tight* radius (default 8 units) and vertical limit (5 units), no longer the old 45-unit area.
- When within range, a dark-blue **Y / TRIANGLE : ARMOUR SWAP** interaction prompt appears.
- **Press Triangle / Xbox Y** to open the full panel; D-pad Up/Down moves the selection, Cross/Xbox A equips the selected appearance, Circle/Xbox B or Triangle closes.
- The modal only captures pad buttons while open and inside the trigger area. **Select / Back is not used.** Outside the trigger area, normal controls work.
- **All five armour looks can be selected**, even on a brand-new save. The protection value and purchases remain untouched.

### Cosmetic selection versus protection

**Every appearance is available immediately**: Original suit, Tetrafiber, Duraplate, Electrosteel, and Carbonox. The `Default (strongest)` option switches the visual model back to the armour the game currently records as equipped. The player's actual damage reduction remains whatever the game already had.

The panel's **Saved protection** line reflects the signed 16-bit armour field at offset `+0x20` within RAC2's **saved progress block**. It is read via the relocated progress-block base (as in OpenRAC NG++), rather than independently relocating the armour field. It does **not** represent which cosmetic appearances are unlocked. The previously used *directly relocated field* reported Carbonox on a fresh Oozla save, which may be an address-mapping issue. This build logs both the relocated progress-block address and the separately relocated armour address, with their values, once per loaded level. Look for `Armour address check` in the OpenRAC log. **If both values still report 4 on your fresh save**, please check the Save Editor's Armour field as well: the cause may be in the save or game state rather than the menu itself. This mod never writes to that game field.

### Fixed location by the bars

The interaction centre is now **Oozla X: 297, Y: 418, Z: 115**, using the position shown in the in-game screenshot. These displayed coordinates are rounded, so the precise centre may differ slightly from Ratchet's position. The interaction has an 8-unit horizontal radius and a 5-unit vertical limit.

The old **L3 + R3 + Triangle calibration shortcut has been removed** because R3 opens the game's map. No calibration is necessary: stand near the bars and press **Triangle/Y** normally. You can still adjust `Vendor interaction radius` in mod settings if needed.

The X/Y/Z debug strip is now **off by default**. Enable `Show Oozla test coordinates` only for troubleshooting. The vendor appears only on Oozla, never over the map or elsewhere in the game.

### What changed from v0.1.0-dev.2

- Removed the auto-opening, invisible HUD text popup.
- Removed Select/Back as an interaction key.
- Draws a **real overlay** with background, title, selected-row highlight, hints and the current protection value, using OpenRAC's supported game overlay API.
- Swallows input **only** for the active popup or Triangle pressed within range; all other controls pass through unchanged.
- Only enables this on Oozla during normal gameplay; not in the map, another menu, cutscenes, or planets elsewhere.

### Limits

This is a **vendor-style popup overlay**, not yet a placed 3D NPC or the game's underlying armour vendor script. There is no bolt purchase. OpenRAC's native `ui_page` API is opened from the game's existing options menus rather than directly from arbitrary gameplay, so an overlay is the stable way to test an in-world trigger. The panel's trigger has been moved to the player's confirmed position at the bars; this new default still needs an in-game test.

## Install and test

From the successful `dev` [GitHub Actions build](https://github.com/glawks/Armor-Swap/actions?query=branch%3Adev), download the `mod-release` artifact, extract it and install `armor-swap-0.1.0-dev.3.zip` in OpenRAC.

Back up a save before testing. Change the appearance, move away and come back, then save/reload and confirm both the cosmetic selection and damage reduction. Changes to appearance are saved with **mod-specific save data**, not the base game armour state.

OpenRAC SDK 1.20, RAC2 NTSC/PAL/NTSC2. Six-platform CI (Windows, Linux, macOS and Android ARM64). `main` remains untouched; test builds only from `dev`.

MIT licensed; no copyrighted game assets included.
