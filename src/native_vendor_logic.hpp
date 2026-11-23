// SPDX-License-Identifier: MIT
#pragma once

namespace armor_swap::native {
constexpr int kCount = 5;
constexpr int kArmourMode = 3;
constexpr int kVendorGameMode = 5;
constexpr unsigned kMode = 0x50u;
constexpr unsigned kCursor = 0x80u;
constexpr unsigned kDialog = 0x84u;
constexpr unsigned kEntries = 0x140u;
constexpr unsigned kRowBytes = 12u;
constexpr unsigned kCountOffset = 0x740u;
constexpr int kDecisionState = 3;

constexpr bool is_valid_selection(int cursor, int dialog) noexcept {
    return cursor >= 0 && cursor < kCount && dialog == kDecisionState;
}
} // namespace armor_swap::native
