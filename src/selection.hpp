// SPDX-License-Identifier: MIT
#pragma once
#include <string_view>

namespace armor_swap {

// -1 means leave the game's model selection untouched. The equipped armour is
// READ ONLY: RAC2 uses it for real damage protection and save progression.
// The selected appearance MAY exceed the owned tier: this is cosmetic only.
constexpr int desired_appearance(std::string_view setting, int equipped) noexcept {
    if (equipped < 0 || equipped > 4 || setting.size() != 1) return -1;
    const char c = setting[0];
    if (c < '0' || c > '4') return -1;
    // All of the game's five visual packs are always selectable.
    return c - '0';
}

// Models 5..8 are the game's special appearances; never override those.
constexpr bool may_reload(int game_state, int level, int loaded, int desired) noexcept {
    return game_state == 0 && level >= 0 && loaded >= 0 && loaded <= 4 &&
           desired >= 0 && desired <= 4 && loaded != desired;
}

} // namespace armor_swap
