// SPDX-License-Identifier: MIT
#pragma once
#include <cmath>
#include <cstdint>
namespace armor_swap {
// Documented Oozla ship landing point (level 1), RAC2_SAVE_FORMAT.md.
constexpr float kShipX = 293.84f, kShipY = 411.66f, kShipZ = 115.0f;
constexpr float kRadius = 45.0f, kVertical = 24.0f;
constexpr const char* kNames[] = {
    "Original suit", "Tetrafiber", "Duraplate", "Electrosteel", "Carbonox"
};
inline bool near_landing(float x, float y, float z) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
    const float dx = x - kShipX, dy = y - kShipY;
    return dx * dx + dy * dy <= kRadius * kRadius &&
        std::fabs(z - kShipZ) <= kVertical;
}
constexpr int row_count(int owned) noexcept {
    return owned >= 0 && owned <= 4 ? owned + 2 : 0;
}
// Row 0 is the strongest/default look; rows 1..5 are skins 0..4.
constexpr int row_to_tier(int row, int owned) noexcept {
    return row >= 0 && row < row_count(owned) ? row - 1 : -2;
}
constexpr int row_for(int tier, int owned) noexcept {
    return tier >= 0 && tier <= owned ? tier + 1 : 0;
}
constexpr int cycle_row(int row, int delta, int owned) noexcept {
    const int count = row_count(owned);
    return count ? (row + delta + count) % count : 0;
}
constexpr std::uint16_t kSelect = 0x0001u;
constexpr std::uint16_t kUp = 0x0010u, kDown = 0x0040u;
constexpr std::uint16_t kTriangle = 0x1000u, kCircle = 0x2000u, kCross = 0x4000u;
constexpr std::uint16_t just_pressed(std::uint16_t now, std::uint16_t before) noexcept {
    return now & static_cast<std::uint16_t>(~before);
}
}
