// SPDX-License-Identifier: MIT
#include "native_vendor_logic.hpp"
#include <cstdio>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"CHECK failed on line %d: %s\n",__LINE__,#x); return 1; } } while(0)

int main() {
    using namespace armor_swap::native;
    CHECK(kCount == 5);
    CHECK(kRowBytes == 12);
    CHECK(kEntries + kRowBytes*kCount <= kCountOffset);
    for (int i = 0; i < kCount; ++i) CHECK(is_valid_selection(i, kDecisionState));
    CHECK(!is_valid_selection(-1, kDecisionState));
    CHECK(!is_valid_selection(5, kDecisionState));
    CHECK(!is_valid_selection(0, 2));
    CHECK(!is_valid_selection(4, 1));
    return 0;
}
