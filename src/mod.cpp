// SPDX-License-Identifier: MIT
// Experimental RAC2 cosmetic armour vendor: a game-overlay panel on Oozla.
// Never writes to the saved armour/protection state.
#include "openrac/mod.hpp"
#include "selection.hpp"
#include "vendor_logic.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {
struct Disc {
    const char* version;
    std::uint32_t equipped, loaded;
    std::array<std::uint32_t, 4> skin_flags;
    const char* load_function;
};
constexpr Disc kDiscs[] = {
    {"ntsc", 0x001A7A18u, 0x001A7290u,
        {0x001A7A49u, 0x001A7A42u, 0x001A7A4Bu, 0x001A7A4Au}, "func_002923C0"},
    {"pal", 0x001A7AA0u, 0x001A7310u,
        {0x001A7AD1u, 0x001A7ACAu, 0x001A7AD3u, 0x001A7AD2u}, "func_00292CD0"},
    {"ntsc2", 0x001A7A20u, 0x001A7290u,
        {0x001A7A51u, 0x001A7A4Au, 0x001A7A53u, 0x001A7A52u}, "func_00292C70"}
};
using LoadModel = void (*)(std::uint32_t);
constexpr int kOozla = 1, kWarmup = 15;
constexpr std::uint8_t kSaveFormat = 1, kSaveAuto = 255;

struct Session {
    openrac::Mod* mod = nullptr;
    const Disc* disc = nullptr;
    LoadModel loader = nullptr;
    std::string fallback = "auto", choice = "auto";
    armor_swap::Zone zone{297.0f, 418.0f, 115.0f, 8.0f, 5.0f};
    bool near = false, popup = false, overlay_installed = false;
    bool debug_coords = false, force_restore = false;
    int owned = -1, last_logged_protection = -1, row = 0, warmup = 0, last_level = -2;
    int failed_level = -2, failed_from = -1, failed_to = -1;
    std::uint16_t last_buttons = 0;
    std::uint32_t loads = ~0u;
    openrac_pose pose{};
    bool pose_valid = false;
} s;

// Only atomics cross from the game's thread to the overlay/window thread.
// No game-memory, mod API, or Session access is allowed inside draw_overlay.
std::atomic<std::uint32_t> g_ui{0};
std::atomic<int> g_x{0}, g_y{0}, g_z{0}, g_dist{0};

void publish_ui(int level, int phase) {
    std::uint32_t bits = 0;
    if (level == kOozla && phase == OPENRAC_PLAY_PLAYING && s.overlay_installed) {
        if (s.near && !s.popup) bits |= 1u;   // Interaction prompt.
        if (s.near && s.popup) bits |= 2u;    // Full screen vendor panel.
        if (s.debug_coords) bits |= 4u;      // Developer coordinates only.
        bits |= static_cast<std::uint32_t>(s.row & 7) << 8;
        bits |= static_cast<std::uint32_t>((s.owned >= 0 ? s.owned : 0) & 7) << 12;
    }
    g_ui.store(bits, std::memory_order_release);
}
void text_at(const openrac_overlay* o, float x, float y, float size,
             std::uint32_t color, const char* text) {
    o->text(o->context, x, y, size, color, text);
}
void centered(const openrac_overlay* o, float center_x, float y, float height,
              std::uint32_t color, const char* text) {
    const float w = o->text_width(o->context, height, text);
    text_at(o, center_x - w / 2.0f, y, height, color, text);
}

void draw_overlay(void*, const openrac_overlay* overlay) {
    if (overlay == nullptr) return;
    const auto ui = g_ui.load(std::memory_order_acquire);
    if (ui == 0) return;

    if ((ui & 4u) != 0) {
        const int x = g_x.load(), y = g_y.load(), z = g_z.load(), d = g_dist.load();
        char debug[120];
        std::snprintf(debug, sizeof debug, "OOZLA  X:%d Y:%d Z:%d  DIST:%d",
                      x, y, z, d);
        overlay->rect(overlay->context, 8, 8, 315, 25, 0x081E35D9u);
        text_at(overlay, 14, 12, 13, 0xC8E7FFFFu, debug);
    }

    if (ui & 1u) {
        overlay->rect(overlay->context, 142, 351, 228, 40, 0x071829DFu);
        overlay->rect(overlay->context, 142, 351, 228, 3, 0x57C5F5FFu);
        centered(overlay, 256, 359, 16, 0xFFFFFFFFu,
                 "Y / TRIANGLE : ARMOUR SWAP");
    }
    if (!(ui & 2u)) return;

    // Vendor-style modal panel (overlay 1.14). Drawn after the game world,
    // not by hud_text_set, whose text was not visible in the first test.
    overlay->rect(overlay->context, 86, 66, 340, 328, 0x061321F1u);
    overlay->rect(overlay->context, 86, 66, 340, 5, 0x48B9EDFFu);
    overlay->rect(overlay->context, 86, 389, 340, 5, 0x367DA6FFu);
    overlay->rect(overlay->context, 97, 112, 318, 1, 0x368EAEDF);
    centered(overlay, 256, 79, 26, 0xD7F2FFFFu, "ARMOUR SWAP");
    centered(overlay, 256, 117, 14, 0x95CEDDFFu, "Choose a cosmetic armour");
    const int owned = (ui >> 12) & 7;
    const int row = (ui >> 8) & 7;
    char stat[84];
    std::snprintf(stat, sizeof stat, "Saved protection: %s", armor_swap::kNames[owned]);
    centered(overlay, 256, 143, 16, 0xEED59CFFu, stat);

    const int count = armor_swap::row_count(owned);
    for (int index = 0; index < count; ++index) {
        const int tier = armor_swap::row_to_tier(index, owned);
        const char* label = tier < 0 ? "Default (strongest)" : armor_swap::kNames[tier];
        const float y = 175.0f + 26.0f * index;
        if (index == row) {
            overlay->rect(overlay->context, 106, y - 3, 300, 24, 0x21638AF0u);
            overlay->rect(overlay->context, 106, y - 3, 3, 24, 0xE9D39CFFu);
        }
        text_at(overlay, 122, y, 17, index == row ? 0xFFFFFFFFu : 0xB8C7D6FFu, label);
    }
    overlay->rect(overlay->context, 102, 344, 308, 1, 0x368EAEDF);
    centered(overlay, 256, 352, 14, 0xD5EAF4FFu,
             "UP / DOWN : BROWSE");
    centered(overlay, 256, 369, 14, 0xD5EAF4FFu,
             "A / CROSS : EQUIP       B / CIRCLE : EXIT");
}

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
void restore_from_save() {
    s.choice = s.fallback;
    const auto data = s.mod->save_data();
    if (data.size() == 2 && data[0] == kSaveFormat) {
        if (data[1] == kSaveAuto) s.choice = "auto";
        else if (data[1] <= 4) s.choice = std::to_string(data[1]);
    }
    s.force_restore = false;
    s.failed_level = -2;
    s.popup = false;
}
void choose() {
    const int tier = armor_swap::row_to_tier(s.row, s.owned);
    if (tier < -1 || tier > 4) return;
    s.choice = tier < 0 ? "auto" : std::to_string(tier);
    const std::uint8_t saved[2] = {kSaveFormat,
        tier < 0 ? kSaveAuto : static_cast<std::uint8_t>(tier)};
    if (!s.mod->save_data_set(saved, sizeof saved))
        s.mod->warn("Could not store armour preference alongside save");
    s.force_restore = tier == -1;
    s.failed_level = -2;
    s.mod->info("Visual armour: " + std::string(tier == -1 ? "default" :
                armor_swap::kNames[tier]) + " (protection unchanged)");
    s.popup = false;
}

void on_pad(int port, std::uint8_t* bytes, int size) {
    if (port != 0 || bytes == nullptr || size < 2) return;
    const auto raw = static_cast<std::uint16_t>(bytes[0]) |
                    (static_cast<std::uint16_t>(bytes[1]) << 8);
    const auto buttons = static_cast<std::uint16_t>(~raw);
    const auto pressed = armor_swap::just_pressed(buttons, s.last_buttons);
    s.last_buttons = buttons;

    // Only Triangle inside the fixed Oozla vendor zone starts this interaction.
    // Outside that zone every button (including L3, R3 and Select) reaches the game.
    bool consume = false;
    if (s.near && s.overlay_installed && s.owned >= 0 && s.owned <= 4) {
        if (!s.popup && (pressed & armor_swap::kTriangle)) {
            const int tier = armor_swap::desired_appearance(s.choice, s.owned);
            s.row = armor_swap::row_for(tier, s.owned);
            s.popup = true;
            consume = true;
        } else if (s.popup) {
            // Only hold controls while the draw callback has a real active panel.
            consume = true;
            if (pressed & armor_swap::kUp) s.row = armor_swap::cycle_row(s.row, -1, s.owned);
            else if (pressed & armor_swap::kDown)
                s.row = armor_swap::cycle_row(s.row, 1, s.owned);
            else if (pressed & armor_swap::kCross) choose();
            else if (pressed & (armor_swap::kCircle | armor_swap::kTriangle))
                s.popup = false;
        }
    } else {
        s.popup = false;
    }
    if (consume) {
        // Buttons and sticks are consumed ONLY on a vendor interaction tick.
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
        s.mod->warn("Model reload did not update armour state, not repeating this scene");
    } else {
        s.failed_level = -2;
        s.force_restore = false;
        s.mod->info("Visual tier " + std::to_string(wanted) +
                    ", retained protection tier " + std::to_string(s.owned));
    }
}
void on_frame() {
    const auto status = s.mod->play_status();
    if (status.loads != s.loads) {
        s.loads = status.loads;
        s.warmup = 0;
        s.failed_level = -2;
        s.popup = false; s.near = false; s.pose_valid = false;
        s.last_buttons = 0;
    }
    if (status.level != s.last_level) {
        s.last_level = status.level;
        s.warmup = 0;
        s.failed_level = -2;
    }
    if (status.phase != OPENRAC_PLAY_PLAYING) {
        s.popup = false; s.near = false; s.pose_valid = false;
        publish_ui(status.level, status.phase);
        return;
    }
    if (s.warmup < kWarmup) ++s.warmup;
    s.owned = get_owned();
    // This signed 16-bit field is what RAC2's AdjustPlayerHealth uses,
    // unlike the loaded model pack (which can be any chosen appearance).
    if (s.owned >= 0 && s.owned <= 4 && s.owned != s.last_logged_protection) {
        s.mod->info("Armour protection in game memory is tier " + std::to_string(s.owned) +
                    " (" + armor_swap::kNames[s.owned] + "); "
                    "cosmetic looks 0-4 are available independently");
        s.last_logged_protection = s.owned;
    }
    if (s.owned < 0 || s.owned > 4) {
        s.popup = false; s.near = false; s.pose_valid = false;
        publish_ui(status.level, status.phase);
        return;
    }
    s.pose_valid = s.mod->hero_pose(s.pose) && s.pose.level == kOozla;
    s.near = status.level == kOozla && s.pose_valid && s.warmup >= kWarmup &&
        armor_swap::in_zone(s.pose.position[0], s.pose.position[1],
                            s.pose.position[2], s.zone);
    if (!s.near) s.popup = false;
    if (s.pose_valid) {
        g_x.store(static_cast<int>(std::lround(s.pose.position[0])));
        g_y.store(static_cast<int>(std::lround(s.pose.position[1])));
        g_z.store(static_cast<int>(std::lround(s.pose.position[2])));
        g_dist.store(static_cast<int>(std::lround(
            armor_swap::distance_xy(s.pose.position[0], s.pose.position[1], s.zone))));
    }
    if (s.row >= armor_swap::row_count(s.owned))
        s.row = armor_swap::row_count(s.owned) - 1;
    publish_ui(status.level, status.phase);
    apply_appearance(status);
}
void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") throw openrac::Error("Armor Swap supports RAC2 only");
    const Disc* disc = nullptr;
    for (const auto& entry : kDiscs)
        if (mod.version() == entry.version) { disc = &entry; break; }
    if (!disc) throw openrac::Error("Unknown RAC2 disc");
    s = Session{};
    s.mod = &mod; s.disc = disc;
    s.fallback = mod.setting<std::string>("appearance");
    s.choice = s.fallback;
    s.zone.radius = static_cast<float>(mod.setting<double>("vendor_radius"));
    s.zone.vertical = 5.0f;
    s.debug_coords = mod.setting<bool>("debug_coords");
    s.loader = mod.required_function<LoadModel>(disc->load_function);
    if (!mod.has_overlay() || !mod.on_overlay(draw_overlay))
        throw openrac::Error("Armor Swap requires OpenRAC's in-game overlay API (1.14+)");
    s.overlay_installed = true;
    restore_from_save();
    mod.on_save_data([] { restore_from_save(); });
    mod.on_pad(on_pad);
    mod.on_frame(on_frame);
    mod.info("Armor Swap 0.1.0-dev.5: all visual armour tiers enabled, protection value read-only");
}
}
OPENRAC_MOD_CPP("armor-swap", "0.1.0-dev.5", load)
