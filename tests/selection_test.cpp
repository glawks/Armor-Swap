// SPDX-License-Identifier: MIT
#include "selection.hpp"
#include "vendor_logic.hpp"
#include <cstdio>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #expr); return 1; } } while (0)
int main() {
    using armor_swap::desired_appearance;
    using armor_swap::may_reload;
    CHECK(desired_appearance("auto", 4) == -1);
    CHECK(desired_appearance("0", 4) == 0);
    CHECK(desired_appearance("2", 4) == 2);
    CHECK(desired_appearance("4", 4) == 4);
    CHECK(desired_appearance("4", 2) == 4);
    CHECK(desired_appearance("4", 0) == 4);
    CHECK(desired_appearance("1", 0) == 1);
    CHECK(desired_appearance("0", 0) == 0);
    CHECK(desired_appearance("0", -1) == -1);
    CHECK(desired_appearance("0", 5) == -1);
    CHECK(desired_appearance("9", 4) == -1);
    CHECK(may_reload(0, 3, 4, 2));
    CHECK(!may_reload(5, 3, 4, 2));
    CHECK(!may_reload(0, -1, 4, 2));
    CHECK(!may_reload(0, 3, 2, 2));
    CHECK(!may_reload(0, 3, 6, 2));
    CHECK(!may_reload(0, 3, -1, 2));

    CHECK(armor_swap::row_count(0) == 6);
    CHECK(armor_swap::row_count(4) == 6);
    CHECK(armor_swap::row_count(9) == 0);
    CHECK(armor_swap::row_to_tier(0, 4) == -1);
    CHECK(armor_swap::row_to_tier(1, 4) == 0);
    CHECK(armor_swap::row_to_tier(5, 4) == 4);
    CHECK(armor_swap::row_to_tier(5, 0) == 4);
    CHECK(armor_swap::row_to_tier(6, 4) == -2);
    CHECK(armor_swap::cycle_row(0, -1, 4) == 5);
    CHECK(armor_swap::cycle_row(5, 1, 4) == 0);
    CHECK(armor_swap::row_for(2, 4) == 3);
    CHECK(armor_swap::row_for(4, 2) == 5);
    CHECK(armor_swap::row_for(4, 0) == 5);
    const armor_swap::Zone zone{297.0f, 418.0f, 115.0f, 8.0f, 5.0f};
    CHECK(armor_swap::in_zone(297.0f, 418.0f, 115.0f, zone));
    CHECK(armor_swap::in_zone(301.f, 420.f, 118.f, zone));
    CHECK(!armor_swap::in_zone(285.f, 418.f, 115.f, zone));
    CHECK(!armor_swap::in_zone(297.f, 418.f, 121.f, zone));
    CHECK(!armor_swap::in_zone(400.f, 411.66f, 115.f, zone));
    CHECK(armor_swap::just_pressed(0x40, 0) == 0x40);
    CHECK(armor_swap::just_pressed(0x40, 0x40) == 0);
    return 0;
}
