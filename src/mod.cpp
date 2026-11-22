// SPDX-License-Identifier: MIT
// EXPERIMENTAL: first in-game test of visual armour loading in RAC2.
//
// Sources: OpenRAC's docs/port/RAC2_IDENTIFIED.tsv and port/game/rac2/profile*.h,
// plus rac2-decomp's 00292CD0.c / 00292C70-cc.c.
//
// Never write to the save's equipped-armour value: the game uses that for
// damage reduction. LoadPlayerDisplayModel accepts an appearance pack directly.
#include "openrac/mod.hpp"
#include "selection.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

struct Disc {
    const char* version;
    std::uint32_t equipped;          // s16: strongest armour, 0..4. READ ONLY.
    std::uint32_t loaded;            // s32: display pack last loaded, 0..8.
    std::uint32_t game_state;        // s32: zero in gameplay, five at a vendor.
    std::array<std::uint32_t, 4> special_skin_flags;
    const char* load_model_function;
};

constexpr Disc kDiscs[] = {
    {"ntsc",  0x001A7A18u, 0x001A7290u, 0x001A8BB0u,
        {0x001A7A49u, 0x001A7A42u, 0x001A7A4Bu, 0x001A7A4Au}, "func_002923C0"},
    {"pal",   0x001A7AA0u, 0x001A7310u, 0x001A8C60u,
        {0x001A7AD1u, 0x001A7ACAu, 0x001A7AD3u, 0x001A7AD2u}, "func_00292CD0"},
    {"ntsc2", 0x001A7A20u, 0x001A7290u, 0x001A8BB0u,
        {0x001A7A51u, 0x001A7A4Au, 0x001A7A53u, 0x001A7A52u}, "func_00292C70"}
};

// The decompiled loader takes a void * but interprets its low 32 bits as the
// numbered model pack.  An integer passes through the same game ABI register.
using LoadModel = void (*)(std::uint32_t);

struct Session {
    const Disc* disc = nullptr;
    LoadModel loader = nullptr;
    std::string choice;
    int last_level = -2;
    int consecutive_play_ticks = 0;
    int failed_level = -2;
    int failed_variant = -1;
    int failed_target = -1;
};

Session s_session;

bool special_skin_enabled(const openrac::Mod& mod, const Disc& disc) {
    for (const auto address : disc.special_skin_flags) {
        const auto relocated = mod.relocate(address);
        if (!relocated) return true; // fail closed if a build cannot resolve it
        if (mod.memory().read<std::uint8_t>(relocated)) return true;
    }
    return false;
}

void on_frame(openrac::Mod& mod) {
    Session& session = s_session;
    const Disc& disc = *session.disc;
    const int level = mod.loaded_level();
    const auto state_address = mod.relocate(disc.game_state);
    const auto equipped_address = mod.relocate(disc.equipped);
    const auto loaded_address = mod.relocate(disc.loaded);
    if (!state_address || !equipped_address || !loaded_address) return;

    const int game_state = mod.memory().read<std::int32_t>(state_address);
    // Never touch model textures in menus, vendor previews or while a level
    // is changing. Require 15 consecutive normal gameplay ticks.
    if (game_state != 0 || level < 0) {
        session.consecutive_play_ticks = 0;
        session.last_level = -2;
        session.failed_level = -2;
        return;
    }
    if (session.last_level != level) {
        session.last_level = level;
        session.consecutive_play_ticks = 0;
        session.failed_level = -2;
    }
    if (session.consecutive_play_ticks < 15) {
        ++session.consecutive_play_ticks;
        return;
    }

    const int owned = mod.memory().read<std::int16_t>(equipped_address);
    const int desired = armor_swap::desired_appearance(session.choice, owned);
    if (desired < 0 || special_skin_enabled(mod, disc)) return;

    const int loaded = mod.memory().read<std::int32_t>(loaded_address);
    if (!armor_swap::may_reload(game_state, level, loaded, desired)) return;
    // A loader that did not update the game's loaded-pack word should not
    // be retried every frame; allow another attempt after a level/menu change.
    if (session.failed_level == level &&
        session.failed_variant == loaded &&
        session.failed_target == desired) return;

    session.loader(static_cast<std::uint32_t>(desired));
    const int result = mod.memory().read<std::int32_t>(loaded_address);
    if (result != desired) {
        session.failed_level = level;
        session.failed_variant = loaded;
        session.failed_target = desired;
        mod.warn("Armour display pack did not update; will not retry until scene changes");
    } else {
        session.failed_level = -2;
        mod.info("Visible armour changed to tier " + std::to_string(desired) +
                 " (equipped protection remains tier " + std::to_string(owned) + ")");
    }
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") throw openrac::Error("Armor Swap supports RAC2 only");
    const Disc* disc = nullptr;
    for (const Disc& entry : kDiscs) {
        if (mod.version() == entry.version) { disc = &entry; break; }
    }
    if (!disc) throw openrac::Error("Unsupported RAC2 disc version");

    s_session = Session{};
    s_session.disc = disc;
    s_session.choice = mod.setting<std::string>("appearance");
    if (s_session.choice == "auto") {
        mod.info("Armor Swap: automatic appearance; vanilla behaviour preserved");
        return;
    }
    // A missing resolved loader is safer as a clean mod-load failure.
    s_session.loader = mod.required_function<LoadModel>(disc->load_model_function);
    mod.on_frame([&mod] { on_frame(mod); });
    mod.info("Armor Swap experimental model loader ready; protection is read-only");
}

} // namespace

OPENRAC_MOD_CPP("armor-swap", "0.1.0-dev.1", load)
