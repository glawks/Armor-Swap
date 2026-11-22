// SPDX-License-Identifier: MIT
#include "selection.hpp"
#include <cstdio>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #expr); return 1; } } while (0)
int main() {
    using armor_swap::desired_appearance;
    using armor_swap::may_reload;
    CHECK(desired_appearance("auto", 4) == -1);
    CHECK(desired_appearance("0", 4) == 0);
    CHECK(desired_appearance("2", 4) == 2);
    CHECK(desired_appearance("4", 4) == 4);
    CHECK(desired_appearance("4", 2) == -1);
    CHECK(desired_appearance("1", 0) == -1);
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
    return 0;
}
