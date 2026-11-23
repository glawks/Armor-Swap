// SPDX-License-Identifier: MIT
#pragma once
#include <cmath>
#include <cstdint>
namespace armor_swap {
struct Zone {
    float x, y, z, radius, vertical;
};
inline float distance_xy(float x, float y, const Zone& zone) noexcept {
    const float dx = x - zone.x, dy = y - zone.y;
    return std::sqrt(dx * dx + dy * dy);
}
inline bool in_zone(float x, float y, float z, const Zone& zone) noexcept {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
        std::isfinite(zone.x) && std::isfinite(zone.y) && std::isfinite(zone.z) &&
        zone.radius >= 0 && zone.vertical >= 0 &&
        distance_xy(x, y, zone) <= zone.radius &&
        std::fabs(z - zone.z) <= zone.vertical;
}
constexpr const char* kNames[] = {
    "Original suit", "Tetrafiber", "Duraplate", "Electrosteel", "Carbonox"
};
constexpr int row_count(int owned) noexcept {
    return owned >= 0 && owned <= 4 ? owned + 2 : 0;
}
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
constexpr std::uint16_t kTriangle = 0x1000u, kCircle = 0x2000u, kCross = 0x4000u;
constexpr std::uint16_t kUp = 0x0010u, kDown = 0x0040u;
constexpr std::uint16_t just_pressed(std::uint16_t now, std::uint16_t before) noexcept {
    return now & static_cast<std::uint16_t>(~before);
}
}
