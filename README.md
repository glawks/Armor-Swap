# Armor Swap (experimental)

RAC2 armour transmog: select how Ratchet looks **without changing the saved armour tier or its damage reduction**. The first visual model swap was confirmed working in-game; the **new Oozla popup is a test feature, not yet tested**.

## Oozla landing test vendor

The prototype shows an on-screen **ARMOUR SWAP** popup near the ship landing platform/walkway on **Oozla**, based on documented level-start coordinates (293.84, 411.66, 115).

- **Auto-popup:** When entering the landing area it should appear once (turn off `Oozla test popup` in launcher settings if unwanted).
- **Reopen:** Press **Select / Back** while near the ship.
- **Navigate:** D-pad Up/Down; **Cross / Xbox A** to equip; **Circle / Xbox B**, Triangle or Select to close.
- **Armour available:** Default (best owned), Original suit, Tetrafiber, Duraplate, Electrosteel, Carbonox. Only tiers already acquired are selectable. The player keeps the strongest purchased armour's protection.

The menu consumes gamepad buttons and sticks while open so moving the selection shouldn't also board the ship or move Ratchet. It is a **gameplay HUD text popup**, not yet a physical vendor NPC or a native shop panel, and doesn't charge bolts.

## Persistence and protection

In-game armour choices are stored in **OpenRAC's mod-specific per-save data**. **Save normally in-game** to keep the cosmetic selection for your next session. Existing saves without a cosmetic choice fall back to the launcher's `Visible armour` setting. Disabling this mod does not overwrite your game's actual armour. The model is loaded using RAC2's own armouring routine, without modifying the protection/stat field.

## Testing

1. Back up your save. Get the newest **dev** workflow artifact at [GitHub Actions](https://github.com/glawks/Armor-Swap/actions?query=branch%3Adev): `mod-release` contains a combined ZIP for all six platforms.
2. Install the combined ZIP through the OpenRAC launcher. Travel to Oozla, near the ship where your screenshots were taken.
3. Check whether the popup appears, whether the choices and button prompts respond, and whether the selected look appears immediately.
4. Exit and reopen the popup, travel to another planet and back, then save and reload. Verify that the strongest purchased armour still grants its normal protection.

If no popup appears, the proximity radius (45 units in the horizontal plane, 24 units vertically) may require calibration. Share a log and ideally a position readout. These builds are untested development artifacts, not stable releases.

## Supported

RAC2 NTSC-U v1.01, PAL, NTSC-U v2.00. OpenRAC mod SDK/API 1.20. Native CI builds for Windows x64, Linux x64/ARM64, macOS x64/ARM64, and Android ARM64.

`main` is intentionally not released; `dev` builds test artifacts and does not create a public GitHub Release.

## Development

```sh
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The save's equipped armour data is always read-only: NTSC `0x001A7A18`, PAL `0x001A7AA0`, NTSC2 `0x001A7A20`. Visual model pack: NTSC/NTSC2 `0x001A7290`, PAL `0x001A7310`.

MIT licensed. No copyrighted game assets included.
