# Armor Swap (experimental)

Choose Ratchet's visible armour in **Ratchet & Clank 2: Going Commando** without reducing the protection from the strongest armour you own.

> **Development test build – NOT a verified release.** This mod calls a decompiled game model-loading routine at runtime. It has not yet been tested in-game. Back up your save and disable the mod if it causes instability.

## How it works

RAC2 stores gameplay armour and the loaded visual armour pack separately. Armor Swap only reads the player's equipped armour (the protection value saved by the game), and asks the game's own armour model loader to load another visual pack. It **never writes** to the purchased armour/protection value.

In **OpenRAC Launcher → Mods → Armor Swap**, choose a **Visible armour** setting: Automatic (default), Original suit, or one of tiers 1–4. The chosen tier must be no higher than the armour in the save. An unavailable tier is ignored, not granted or unlocked. Changes take effect at the next game launch. Built-in special skins are left alone.

The first prototype waits for 15 consecutive normal gameplay ticks before asking the model loader to update the look. It will not deliberately load models during the vendor menu or other non-gameplay states.

## Testing

Install the ZIP from a successful `dev` branch Actions run (look for `mod-release` or a platform-specific `mod-*` artifact), using the OpenRAC launcher. These are **build artifacts, not public Releases**.

Suggested first test:

1. Back up a save with high-tier armour, ideally tier 4.
2. Set **Visible armour** to **Original suit**; restart RAC2.
3. Confirm the original model appears, while enemy damage is still reduced according to the equipped high-tier armour.
4. Travel to another planet, die and respawn, visit the armour vendor, and reload the save. Watch for model flicker or crashes.
5. Check the OpenRAC log for `Armor Swap` messages and share results before any release.

RAC2 discs targeted: NTSC-U v1.01 (`ntsc`), PAL (`pal`), NTSC-U v2.00 (`ntsc2`). OpenRAC mod SDK/API **1.20**. Intended builds: Windows x64, Linux x64/ARM64, macOS x64/ARM64, Android ARM64.

## Development

`main` has no public version yet; all prototype code and downloadable test builds are on `dev`. The workflow is adapted from glawks's Vendor Buy All workflow. It builds six platforms; only a stable version merged to `main` can create a GitHub Release.

```bash
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The SDK is pinned in `sdk.lock`. No copyrighted game assets are distributed. MIT licensed; see `LICENSE`.

## Engineering notes

- Player protection: NTSC `0x001A7A18`, PAL `0x001A7AA0`, NTSC2 `0x001A7A20`.
- Loaded model pack: NTSC/NTSC2 `0x001A7290`, PAL `0x001A7310`.
- Loader: NTSC `func_002923C0`, PAL `func_00292CD0`, NTSC2 `func_00292C70`.
- The runtime loader call and availability of these named functions **still need in-game verification**; if they cannot be resolved, the SDK will refuse to load the mod rather than guessing another address.

The next version could expose an in-game armour selection UI and store a cosmetic preference per save, once this prototype's rendering and damage behaviour are confirmed.
