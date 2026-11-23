// SPDX-License-Identifier: MIT
// Experimental native RAC2 armour vendor: only its menu and preview are reused.
// Bought armour is never changed by a normal cosmetic selection. The original
// game vendor *does* write armour on close; a safety rollback checks for that.
#include "openrac/mod.hpp"
#include "openrac/games/rac2.hpp"
#include "selection.hpp"
#include "vendor_logic.hpp"
#include "native_vendor_logic.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace {

struct Disc {
    const char* version;
    std::uint32_t progress, loaded_pack, vendor_ui, game_mode;
    std::array<std::uint32_t, 4> special_skin_flags;
    const char* load_pack;
    const char* open_vendor;
};
constexpr Disc kDiscs[] = {
    {"ntsc", 0x001A79F8u, 0x001A7290u, 0x0021F240u, 0x001A8BB0u,
        {0x001A7A49u, 0x001A7A42u, 0x001A7A4Bu, 0x001A7A4Au},
        "func_002923C0", "func_002F8178"},
    {"pal", 0x001A7A80u, 0x001A7310u, 0x0021F240u, 0x001A8C60u,
        {0x001A7AD1u, 0x001A7ACAu, 0x001A7AD3u, 0x001A7AD2u},
        "func_00292CD0", "func_002F90E8"},
    {"ntsc2", 0x001A7A00u, 0x001A7290u, 0x0021F1C0u, 0x001A8BB0u,
        {0x001A7A51u, 0x001A7A4Au, 0x001A7A53u, 0x001A7A52u},
        "func_00292C70", "func_002F8FF8"}
};
using LoadPack = void (*)(std::uint32_t);
using OpenVendor = void (*)(std::uint32_t);
constexpr int kOozla = 1, kWarmup = 15;
constexpr std::uint8_t kSaveFormat = 1, kSaveAuto = 255;

struct Session {
    openrac::Mod* mod = nullptr;
    const Disc* disc = nullptr;
    LoadPack loader = nullptr;
    OpenVendor open_vendor = nullptr;
    armor_swap::Zone zone{297.f, 418.f, 115.f, 8.f, 5.f};
    std::string fallback = "auto";
    std::string choice = "auto";
    bool debug_coords = false;
    bool near = false;
    bool requested = false;
    bool native_active = false;
    bool native_seen = false;
    bool list_ready = false;
    bool force_restore = false;
    int owned = -1, protection_before = -1;
    int last_level = -2, warmup = 0, native_ticks = 0;
    int failed_level = -2, failed_from = -1, failed_to = -1;
    std::uint16_t last_buttons = 0;
    std::uint32_t loads = ~0u;
} s;

// These atomics are the only variables touched by the render-thread overlay.
std::atomic<unsigned> g_prompt{0};
std::atomic<int> g_x{0}, g_y{0}, g_z{0}, g_dist{0};

void overlay_draw(void*, const openrac_overlay* overlay) {
    if (!overlay) return;
    const unsigned flags = g_prompt.load(std::memory_order_acquire);
    if (flags & 2u) {
        char message[120];
        std::snprintf(message, sizeof message, "OOZLA  X:%d Y:%d Z:%d  DIST:%d",
            g_x.load(), g_y.load(), g_z.load(), g_dist.load());
        overlay->rect(overlay->context, 8, 8, 315, 25, 0x081E35D9u);
        overlay->text(overlay->context, 14, 12, 13, 0xC8E7FFFFu, message);
    }
    if (!(flags & 1u)) return;
    overlay->rect(overlay->context, 142, 351, 228, 40, 0x071829DFu);
    overlay->rect(overlay->context, 142, 351, 228, 3, 0x57C5F5FFu);
    const char* prompt = "Y / TRIANGLE : ARMOUR SWAP";
    const float text_width = overlay->text_width(overlay->context, 16, prompt);
    overlay->text(overlay->context, 256 - text_width / 2.f, 359, 16,
                  0xFFFFFFFFu, prompt);
}

openrac::gaddr progress_addr() {
    return s.mod->relocate(s.disc->progress);
}
openrac::gaddr vendor_addr() {
    return s.mod->relocate(s.disc->vendor_ui);
}
int protection() {
    const auto base = progress_addr();
    return base ? s.mod->memory().read<std::int16_t>(base + 0x20u) : -1;
}
int game_mode() {
    const auto addr = s.mod->relocate(s.disc->game_mode);
    return addr ? s.mod->memory().read<std::int32_t>(addr) : -1;
}
int vendor_field(std::uint32_t offset) {
    const auto base = vendor_addr();
    return base ? s.mod->memory().read<std::int32_t>(base + offset) : -1;
}
bool our_vendor() {
    return s.native_active && s.list_ready &&
           game_mode() == armor_swap::native::kVendorGameMode &&
           vendor_field(armor_swap::native::kMode) == armor_swap::native::kArmourMode;
}
bool special_skin() {
    for (auto raw : s.disc->special_skin_flags) {
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
    s.requested = false;
    s.native_active = false;
    s.native_seen = false;
    s.list_ready = false;
}
void set_appearance(int tier) {
    if (tier < 0 || tier > 4) return;
    const std::uint8_t data[2] = {kSaveFormat, static_cast<std::uint8_t>(tier)};
    if (!s.mod->save_data_set(data, sizeof data)) {
        s.mod->warn("Native vendor could not save cosmetic selection");
        return;
    }
    s.choice = std::to_string(tier);
    s.force_restore = false;
    s.failed_level = -2;
    s.mod->info("Native vendor selected cosmetic " + std::string(armor_swap::kNames[tier]) +
                "; real protection should remain " + std::to_string(s.protection_before));
}

// Only replace the list for our own in-world Oozla test vendor. The original
// vanilla armour vendors continue working, including their normal purchases.
void on_build_list(openrac::Original<void()> original) {
    original();
    if (!s.native_active) return;
    const auto base = vendor_addr();
    if (!base || vendor_field(armor_swap::native::kMode) != armor_swap::native::kArmourMode) {
        s.mod->warn("Native vendor opened in an unexpected mode; no cosmetic rows installed");
        return;
    }
    // Five zero-bolt rows. Native preview still sees valid armour pack 0..4.
    // The 'buy' hook below must cancel its original transaction before any
    // saved armour or bolts can be changed.
    for (int tier = 0; tier < armor_swap::native::kCount; ++tier) {
        const auto row = base + armor_swap::native::kEntries +
            static_cast<std::uint32_t>(tier) * armor_swap::native::kRowBytes;
        s.mod->memory().write(row, std::int32_t{0xEA92 + tier});
        s.mod->memory().write(row + 4u, std::int32_t{0});
        s.mod->memory().write(row + 8u, static_cast<std::int32_t>(tier));
    }
    s.mod->memory().write(base + armor_swap::native::kCountOffset,
                          std::int32_t{armor_swap::native::kCount});
    s.list_ready = true;
    s.mod->info("Native armour vendor populated with five free cosmetic choices");
}

void on_buy(openrac::Original<void(std::uint32_t,std::uint32_t)> original,
            std::uint32_t answer, std::uint32_t unused) {
    if (!our_vendor()) {
        original(answer, unused);
        return;
    }
    const int cursor = vendor_field(armor_swap::native::kCursor);
    const int state = vendor_field(armor_swap::native::kDialog);
    const auto base = vendor_addr();
    if (armor_swap::native::is_valid_selection(cursor, state) &&
        answer >= 0x10000u && answer <= 0x1FFFFFCu && (answer % 4u) == 0u) {
        const auto row = base + armor_swap::native::kEntries +
            static_cast<std::uint32_t>(cursor) * armor_swap::native::kRowBytes;
        const int icon = s.mod->memory().read<std::int32_t>(row);
        const int price = s.mod->memory().read<std::int32_t>(row + 4u);
        const int tier = s.mod->memory().read<std::int32_t>(row + 8u);
        if (tier == cursor && icon == 0xEA92 + cursor && price == 0 &&
            s.mod->memory().read<std::int32_t>(answer) == 1) {
            set_appearance(tier);
            // Tell the native buy handler the player chose NO. The game still
            // closes its confirmation dialog, but performs NO armour purchase.
            s.mod->memory().write(answer, std::int32_t{0});
        }
    }
    original(answer, unused);
}

void on_preview(openrac::Original<void()> original) {
    // The game's preview skips loading pack 0, assuming it is already loaded.
    // The transmog has invalidated that assumption. Restore pack 0 first.
    if (our_vendor() &&
        vendor_field(armor_swap::native::kCursor) == 0) {
        const auto loaded = s.mod->relocate(s.disc->loaded_pack);
        if (loaded && s.mod->memory().read<std::int32_t>(loaded) != 0)
            s.loader(0);
    }
    original();
}

void on_pad(int port, std::uint8_t* bytes, int size) {
    if (port != 0 || bytes == nullptr || size < 2) return;
    const auto raw = static_cast<std::uint16_t>(bytes[0]) |
                     (static_cast<std::uint16_t>(bytes[1]) << 8);
    const auto buttons = static_cast<std::uint16_t>(~raw);
    const auto press = armor_swap::just_pressed(buttons, s.last_buttons);
    s.last_buttons = buttons;
    if (s.near && !s.native_active && !s.requested &&
        (press & armor_swap::kTriangle)) {
        s.requested = true;
        // Only consume the single Triangle press that opens the vendor.
        // The real game's vendor now owns its menu and the rest of the pad.
        bytes[1] |= 0x10u;
    }
}

void enforce_protection() {
    if (!s.native_active || s.protection_before < 0 ||
        s.protection_before > 4) return;
    const int current = protection();
    if (current == s.protection_before) return;
    // This is an emergency rollback for a native vendor's side effect, NOT
    // an armour purchase. Only the cosmetic menu can trigger this guard.
    const auto base = progress_addr();
    if (base && current >= 0 && current <= 4) {
        s.mod->memory().write(base + 0x20u, static_cast<std::int16_t>(s.protection_before));
        s.mod->warn("Native vendor unexpectedly changed armour protection; restored original tier " +
                    std::to_string(s.protection_before));
    } else {
        s.mod->warn("Native vendor protection mismatch but could not safely restore it");
    }
}

void apply_appearance(const openrac_play_status& status) {
    if (status.phase != OPENRAC_PLAY_PLAYING || s.warmup < kWarmup ||
        s.native_active || game_mode() != 0) return;
    const int wanted = s.choice == "auto"
        ? (s.force_restore ? s.owned : -1)
        : armor_swap::desired_appearance(s.choice, s.owned);
    if (wanted < 0 || special_skin()) return;
    const auto addr = s.mod->relocate(s.disc->loaded_pack);
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
        s.mod->warn("Native prototype: armour model reload failed");
    } else {
        s.failed_level = -2;
        s.force_restore = false;
    }
}

void on_frame() {
    const auto status = s.mod->play_status();
    if (status.loads != s.loads) {
        s.loads = status.loads;
        s.last_level = -2;
        s.warmup = 0;
        s.near = false;
        s.requested = false;
        s.native_active = false;
        s.native_seen = false;
        s.list_ready = false;
        s.last_buttons = 0;
        g_prompt.store(0, std::memory_order_release);
    }
    if (status.level != s.last_level) {
        s.last_level = status.level;
        s.warmup = 0;
        s.failed_level = -2;
    }
    const int mode = game_mode();
    if (s.native_active) {
        if (mode == armor_swap::native::kVendorGameMode) {
            s.native_seen = true;
            enforce_protection();
        } else if (s.native_seen || ++s.native_ticks > 8) {
            enforce_protection();
            s.mod->info("Native armour vendor closed; restoring saved cosmetic look");
            s.native_active = false;
            s.native_seen = false;
            s.list_ready = false;
            s.requested = false;
            s.failed_level = -2;
        }
    }
    if (status.phase != OPENRAC_PLAY_PLAYING) {
        s.near = false;
        g_prompt.store(0, std::memory_order_release);
        return;
    }
    if (s.warmup < kWarmup) ++s.warmup;
    s.owned = protection();
    if (s.owned < 0 || s.owned > 4) {
        s.near = false;
        g_prompt.store(0, std::memory_order_release);
        return;
    }

    openrac_pose pose{};
    s.near = !s.native_active && mode == 0 && status.level == kOozla &&
        s.warmup >= kWarmup &&
        s.mod->hero_pose(pose) && pose.level == kOozla &&
        armor_swap::in_zone(pose.position[0], pose.position[1],
                            pose.position[2], s.zone);
    if (status.level == kOozla && s.debug_coords && pose.level == kOozla) {
        g_x.store(static_cast<int>(std::lround(pose.position[0])));
        g_y.store(static_cast<int>(std::lround(pose.position[1])));
        g_z.store(static_cast<int>(std::lround(pose.position[2])));
        g_dist.store(static_cast<int>(std::lround(armor_swap::distance_xy(
            pose.position[0], pose.position[1], s.zone))));
    }
    const unsigned shown = (s.near ? 1u : 0u) |
        ((s.debug_coords && status.level == kOozla && mode == 0) ? 2u : 0u);
    g_prompt.store(shown, std::memory_order_release);

    // Dispatch only on the game thread after a normal gameplay tick.
    if (s.requested) {
        s.requested = false;
        if (s.near && mode == 0 && !s.native_active) {
            s.native_active = true;
            s.native_seen = false;
            s.list_ready = false;
            s.native_ticks = 0;
            s.protection_before = s.owned;
            s.open_vendor(3u);  // The real Megacorp armour vendor, mode 3.
            if (game_mode() != armor_swap::native::kVendorGameMode ||
                !s.list_ready) {
                s.mod->warn("Native armour vendor did not open in mode 3 with cosmetic rows");
                s.native_active = false;
                s.list_ready = false;
            } else {
                s.native_seen = true;
                s.mod->info("Opened the native RAC2 armour vendor at the Oozla bars");
            }
            g_prompt.store(0, std::memory_order_release);
        }
        return;
    }
    apply_appearance(status);
}
void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") throw openrac::Error("Armor Swap supports RAC2 only");
    const Disc* disc = nullptr;
    for (const auto& entry : kDiscs)
        if (mod.version() == entry.version) { disc = &entry; break; }
    if (!disc) throw openrac::Error("Unknown RAC2 disc build");
    s = Session{};
    s.mod = &mod; s.disc = disc;
    s.fallback = mod.setting<std::string>("appearance");
    s.choice = s.fallback;
    s.zone.radius = static_cast<float>(mod.setting<double>("vendor_radius"));
    s.debug_coords = mod.setting<bool>("debug_coords");
    s.loader = mod.required_function<LoadPack>(disc->load_pack);
    s.open_vendor = mod.required_function<OpenVendor>(disc->open_vendor);
    if (!mod.has_overlay() || !mod.on_overlay(overlay_draw))
        throw openrac::Error("Native vendor prototype needs OpenRAC's overlay API");

    // SDK 1.34+ native vendor hooks. If not installed, refuse to start rather
    // than opening the game's armour shop without purchase interception.
    mod.hook<openrac::rac2::ArmorVendor_BuildList>(on_build_list);
    mod.hook<openrac::rac2::ArmorVendor_Buy>(on_buy);
    mod.hook<openrac::rac2::Vendor_Preview>(on_preview);
    restore_from_save();
    mod.on_save_data([] { restore_from_save(); });
    mod.on_pad(on_pad);
    mod.on_frame(on_frame);
    mod.info("Armor Swap native vendor TEST: Oozla, SDK 1.34, five free cosmetic looks");
}
}  // namespace
OPENRAC_MOD_CPP("armor-swap", "0.1.0-native.1", load)
