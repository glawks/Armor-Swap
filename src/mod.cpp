// SPDX-License-Identifier: MIT
// Experimental Oozla landing popup for cosmetically swapping RAC2 armour.
// The player's purchased-armour value is read-only throughout.
#include "openrac/mod.hpp"
#include "selection.hpp"
#include "vendor_logic.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {
struct Disc {
    const char* version;
    std::uint32_t equipped, loaded, game_state;
    std::array<std::uint32_t, 4> skin_flags;
    const char* load_function;
};
constexpr Disc kDiscs[] = {
    {"ntsc", 0x001A7A18u, 0x001A7290u, 0x001A8BB0u,
        {0x001A7A49u, 0x001A7A42u, 0x001A7A4Bu, 0x001A7A4Au}, "func_002923C0"},
    {"pal", 0x001A7AA0u, 0x001A7310u, 0x001A8C60u,
        {0x001A7AD1u, 0x001A7ACAu, 0x001A7AD3u, 0x001A7AD2u}, "func_00292CD0"},
    {"ntsc2", 0x001A7A20u, 0x001A7290u, 0x001A8BB0u,
        {0x001A7A51u, 0x001A7A4Au, 0x001A7A53u, 0x001A7A52u}, "func_00292C70"}
};
using LoadModel = void (*)(std::uint32_t);
constexpr int kOozla = 1, kWarmup = 15;
constexpr std::uint8_t kSaveFormat = 1, kSaveAuto = 255;
struct Session {
    openrac::Mod* mod = nullptr;
    const Disc* disc = nullptr;
    LoadModel loader = nullptr;
    std::string fallback = "auto", choice = "auto", hud_key;
    bool auto_popup = true, near = false, shown_zone = false, popup = false;
    bool force_restore = false, hud_visible = false;
    int owned = -1, row = 0, warmup = 0, last_level = -2;
    int failed_level = -2, failed_from = -1, failed_to = -1;
    std::uint16_t last_buttons = 0;
    std::uint32_t loads = ~0u;
} s;

int get_owned() {
    const auto addr = s.mod->relocate(s.disc->equipped);
    return addr ? s.mod->memory().read<std::int16_t>(addr) : -1;
}
bool special_skin() {
    for (auto raw : s.disc->skin_flags) {
        const auto addr = s.mod->relocate(raw);
        if (!addr || s.mod->memory().read<std::uint8_t>(addr)) return true;
    }
    return false;
}
void reset_hud() {
    if (s.hud_visible) s.mod->hud_text_set({});
    s.hud_visible = false;
    s.hud_key.clear();
}
void put(std::vector<openrac_hud_text>& lines, const std::string& str, float y,
         float size, bool bright = false) {
    openrac_hud_text line{};
    std::snprintf(line.text, sizeof line.text, "%s", str.c_str());
    line.x = 0.5f;
    line.y = y;
    line.scale = size;
    line.flags = OPENRAC_HUD_SHADOW | OPENRAC_HUD_SMALL;
    line.rgba[0] = bright ? 230 : 160;
    line.rgba[1] = bright ? 245 : 200;
    line.rgba[2] = bright ? 255 : 225;
    line.rgba[3] = 255;
    lines.push_back(line);
}
void draw() {
    const std::string key = std::string(s.popup ? "menu" : "prompt") +
        ":" + std::to_string(s.owned) + ":" + std::to_string(s.row) + ":" + s.choice;
    if (s.hud_visible && s.hud_key == key) return;
    std::vector<openrac_hud_text> lines;
    if (s.popup) {
        put(lines, "===== ARMOUR SWAP =====", 0.23f, 1.1f, true);
        put(lines, "OOZLA LANDING - TEST VENDOR", 0.29f, 0.9f);
        put(lines, std::string("Protection: ") + armor_swap::kNames[s.owned],
            0.35f, 0.9f, true);
        for (int i = 0; i < armor_swap::row_count(s.owned); ++i) {
            const int tier = armor_swap::row_to_tier(i, s.owned);
            const std::string label = tier < 0 ? "Default (strongest)" : armor_swap::kNames[tier];
            put(lines, std::string(i == s.row ? "> " : "  ") + label +
                (i == s.row ? " <" : ""), 0.415f + 0.052f * i, 0.9f, i == s.row);
        }
        put(lines, "UP / DOWN : PICK", 0.76f, 0.83f);
        put(lines, "A / CROSS : EQUIP    B / CIRCLE : CLOSE", 0.81f, 0.78f, true);
    } else {
        put(lines, "SELECT / BACK : ARMOUR SWAP", 0.88f, 0.95f, true);
    }
    if (s.mod->hud_text_set(lines)) { s.hud_visible = true; s.hud_key = key; }
}
void show_popup() {
    if (s.owned < 0 || s.owned > 4) return;
    const int tier = armor_swap::desired_appearance(s.choice, s.owned);
    s.row = armor_swap::row_for(tier, s.owned);
    s.popup = true;
    s.hud_key.clear();
}
void close_popup() { s.popup = false; s.hud_key.clear(); }

// OpenRAC keeps this versioned cosmetic byte alongside this save. The game's
// actual armour progression and damage multipliers are never changed.
void restore_from_save() {
    s.choice = s.fallback;
    const auto data = s.mod->save_data();
    if (data.size() == 2 && data[0] == kSaveFormat) {
        if (data[1] == kSaveAuto) s.choice = "auto";
        else if (data[1] <= 4) s.choice = std::to_string(data[1]);
    }
    s.failed_level = -2;
    s.force_restore = false;
}
void choose() {
    const int tier = armor_swap::row_to_tier(s.row, s.owned);
    if (tier < -1 || tier > s.owned) return;
    s.choice = tier < 0 ? "auto" : std::to_string(tier);
    const std::uint8_t save[2] = {kSaveFormat,
        tier < 0 ? kSaveAuto : static_cast<std::uint8_t>(tier)};
    if (!s.mod->save_data_set(save, sizeof save))
        s.mod->warn("Could not save cosmetic preference to this game");
    s.force_restore = tier < 0;
    s.failed_level = -2;
    s.mod->info(std::string("Selected ") +
        (tier < 0 ? "Default (strongest)" : armor_swap::kNames[tier]) +
        "; protection unchanged");
    close_popup();
}
void on_pad(int port, std::uint8_t* bytes, int size) {
    if (port != 0 || bytes == nullptr || size < 2) return;
    const std::uint16_t down = static_cast<std::uint16_t>(~(
        static_cast<std::uint16_t>(bytes[0]) |
        (static_cast<std::uint16_t>(bytes[1]) << 8)));
    const auto press = armor_swap::just_pressed(down, s.last_buttons);
    s.last_buttons = down;
    bool consume = s.popup;
    if (s.near && !s.popup && (press & armor_swap::kSelect)) {
        show_popup();
        consume = true;
    } else if (s.popup) {
        if (press & armor_swap::kUp) s.row = armor_swap::cycle_row(s.row, -1, s.owned);
        else if (press & armor_swap::kDown) s.row = armor_swap::cycle_row(s.row, 1, s.owned);
        else if (press & armor_swap::kCross) choose();
        else if (press & (armor_swap::kCircle | armor_swap::kTriangle | armor_swap::kSelect))
            close_popup();
    }
    if (consume) {
        // Consume inputs before the game reads the report, including sticks.
        // Prevents equipping a weapon or boarding the ship while choosing armour.
        bytes[0] = 255; bytes[1] = 255;
        for (int i = 2; i < size && i < 6; ++i) bytes[i] = 128;
    }
}
void apply_appearance(const openrac_play_status& status) {
    if (status.phase != OPENRAC_PLAY_PLAYING || s.warmup < kWarmup) return;
    const int wanted = s.choice == "auto"
        ? (s.force_restore ? s.owned : -1)
        : armor_swap::desired_appearance(s.choice, s.owned);
    if (wanted < 0 || special_skin()) return;
    const auto addr = s.mod->relocate(s.disc->loaded);
    if (!addr) return;
    const int loaded = s.mod->memory().read<std::int32_t>(addr);
    if (!armor_swap::may_reload(0, status.level, loaded, wanted)) {
        if (loaded == wanted) s.force_restore = false;
        return;
    }
    if (s.failed_level == status.level && s.failed_from == loaded &&
        s.failed_to == wanted) return;
    s.loader(static_cast<std::uint32_t>(wanted));
    if (s.mod->memory().read<std::int32_t>(addr) != wanted) {
        s.failed_level = status.level;
        s.failed_from = loaded;
        s.failed_to = wanted;
        s.mod->warn("Armour model failed to reload; not repeating this frame");
    } else {
        s.failed_level = -2;
        s.force_restore = false;
        s.mod->info("Visual tier " + std::to_string(wanted) +
            ", protection tier " + std::to_string(s.owned));
    }
}
void on_frame() {
    const auto status = s.mod->play_status();
    if (s.loads != status.loads) {
        s.loads = status.loads;
        s.shown_zone = false; s.popup = false; s.near = false;
        s.warmup = 0; s.failed_level = -2;
        reset_hud();
    }
    if (s.last_level != status.level) {
        s.last_level = status.level; s.warmup = 0; s.failed_level = -2;
    }
    if (status.phase != OPENRAC_PLAY_PLAYING) {
        s.near = false; s.popup = false; reset_hud(); return;
    }
    if (s.warmup < kWarmup) ++s.warmup;
    s.owned = get_owned();
    if (s.owned < 0 || s.owned > 4) {
        s.near = false; s.popup = false; reset_hud(); return;
    }
    if (s.row >= armor_swap::row_count(s.owned))
        s.row = armor_swap::row_count(s.owned) - 1;

    openrac_pose pose{};
    const bool has_oozla_pose = s.mod->hero_pose(pose) && pose.level == kOozla;
    s.near = status.level == kOozla && has_oozla_pose &&
        armor_swap::near_landing(pose.position[0], pose.position[1], pose.position[2]);
    if (has_oozla_pose && !s.near) s.shown_zone = false;
    if (!s.near) { s.popup = false; reset_hud(); }
    else {
        // Auto-open once for each approach after gameplay has settled.
        if (!s.shown_zone && s.warmup >= kWarmup) {
            s.shown_zone = true;
            if (s.auto_popup) show_popup();
        }
        draw();
    }
    apply_appearance(status);
}
void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") throw openrac::Error("Armor Swap supports RAC2 only");
    const Disc* disc = nullptr;
    for (const auto& d : kDiscs)
        if (mod.version() == d.version) { disc = &d; break; }
    if (!disc) throw openrac::Error("Unknown RAC2 disc");
    s = Session{};
    s.mod = &mod; s.disc = disc;
    s.fallback = mod.setting<std::string>("appearance");
    s.choice = s.fallback;
    s.auto_popup = mod.setting<bool>("oozla_popup");
    s.loader = mod.required_function<LoadModel>(disc->load_function);
    restore_from_save();
    mod.on_save_data([] { restore_from_save(); s.shown_zone = false; });
    mod.on_pad(on_pad);
    mod.on_frame(on_frame);
    mod.info("Armor Swap 0.1.0-dev.2: Oozla landing popup ready (experimental)");
}
}
OPENRAC_MOD_CPP("armor-swap", "0.1.0-dev.2", load)
