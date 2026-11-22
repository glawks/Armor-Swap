// SPDX-License-Identifier: MIT
#include "selection.hpp"
#include <cassert>
int main() {
    using armor_swap::desired_appearance;
    using armor_swap::may_reload;
    assert(desired_appearance("auto", 4) == -1);
    assert(desired_appearance("0", 4) == 0);
    assert(desired_appearance("2", 4) == 2);
    assert(desired_appearance("4", 4) == 4);
    assert(desired_appearance("4", 2) == -1);
    assert(desired_appearance("1", 0) == -1);
    assert(desired_appearance("0", 0) == 0);
    assert(desired_appearance("0", -1) == -1);
    assert(desired_appearance("0", 5) == -1);
    assert(desired_appearance("9", 4) == -1);
    assert(may_reload(0, 3, 4, 2));
    assert(!may_reload(5, 3, 4, 2));
    assert(!may_reload(0, -1, 4, 2));
    assert(!may_reload(0, 3, 2, 2));
    assert(!may_reload(0, 3, 6, 2));
    assert(!may_reload(0, 3, -1, 2));
    return 0;
}
