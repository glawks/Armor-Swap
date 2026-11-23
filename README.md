# Armor Swap — RAC2 armour transmog

Change Ratchet's **visible** armour without touching the strongest armour purchased or its damage reduction. The visual model-swapping mechanism has been tested in-game. This **v0.1.0-dev.3 test** replaces the old invisible HUD/Select popup with a proper drawn vendor-style panel.

## Oozla gameplay vendor — test

- **Approach the ship landing bars/walkway on Planet Oozla.** The trigger uses a *tight* radius (default 8 units) and vertical limit (5 units), no longer the old 45-unit area.
- When within range, a dark-blue **Y / TRIANGLE : ARMOUR SWAP** interaction prompt appears.
- **Press Triangle / Xbox Y** to open the full panel; D-pad Up/Down moves the selection, Cross/Xbox A equips the selected appearance, Circle/Xbox B or Triangle closes.
- The modal only captures pad buttons while open and inside the trigger area. **Select / Back is not used.** Outside the trigger area, normal controls work.
- Only armour already purchased can be selected; protection and purchases remain untouched.

### Position calibration (important for this test)

The first trigger centre defaults to RAC2's documented Oozla ship arrival coordinates `(293.84, 411.66, 115)`. **The bars in the screenshot might be offset from that position.** To locate the popup *exactly* where Ratchet is standing beside your chosen bars:

1. On Oozla, stand at the desired interaction spot by the bars.
2. Hold **L3 + R3** and press **Triangle / Y** (pressing both sticks and Triangle at once). This moves the vendor trigger centre to Ratchet's current position and saves it in the launcher's settings.
3. Move away, return to the bars and check the Y/Triangle prompt. Regular Triangle/Y should open the vendor there, not elsewhere.

While testing, a small **X/Y/Z and DIST** readout appears at the top-left of the Oozla screen to help diagnose proximity. Turn off `Show Oozla test coordinates` in the launcher when no longer needed. You can also set the `Vendor position X/Y/Z` fields and radius manually. The new location is independent of game saves, while selected armour cosmetics are stored per save.

### What changed from v0.1.0-dev.2

- Removed the auto-opening, invisible HUD text popup.
- Removed Select/Back as an interaction key.
- Draws a **real overlay** with background, title, selected-row highlight, hints and the current protection value, using OpenRAC's supported game overlay API.
- Swallows input **only** for a visible popup or its dedicated interaction/calibration press; the rest of the controls are passed through unchanged.
- Only enables this on Oozla during normal gameplay; not in the map, another menu, cutscenes, or planets elsewhere.

### Limits

This is a **vendor-style popup overlay**, not yet a placed 3D NPC or the game's underlying armour vendor script. There is no bolt purchase. OpenRAC's native `ui_page` API is opened from the game's existing options menus rather than directly from arbitrary gameplay, so an overlay is the stable way to test an in-world trigger. The panel's position and input behaviour still need in-game verification.

## Install and test

From the successful `dev` [GitHub Actions build](https://github.com/glawks/Armor-Swap/actions?query=branch%3Adev), download the `mod-release` artifact, extract it and install `armor-swap-0.1.0-dev.3.zip` in OpenRAC.

Back up a save before testing. Change the appearance, move away and come back, then save/reload and confirm both the cosmetic selection and damage reduction. Changes to appearance are saved with **mod-specific save data**, not the base game armour state.

OpenRAC SDK 1.20, RAC2 NTSC/PAL/NTSC2. Six-platform CI (Windows, Linux, macOS and Android ARM64). `main` remains untouched; test builds only from `dev`.

MIT licensed; no copyrighted game assets included.
