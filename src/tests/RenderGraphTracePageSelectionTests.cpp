#include <catch2/catch_test_macros.hpp>
#include <cstddef>

#include "Debug/RenderGraphTracePage.h"

using namespace Muyo;

// Moved here from Muyo's `src/tests/RenderGraphLaneLayoutTest.cpp` with the page it tests (#318). That file
// was said to include `Debug/RenderGraphTracePage.h` without using it; it did use it - this case is the
// use - so the case moves to the repository that owns `AdjustSelectionAfterPush` and the lane-layout
// geometry cases stay beside `RenderGraphLaneLayout` in Muyo.

TEST_CASE("RenderGraphLaneLayout: the lane view's selection follows the data, and only when it moves", "[TracePage]")
{
    // P1 from the #168 review, and N1 from its round 2. The first fix decremented on every push, which is only right
    // once the window is full: during the fill phase nothing is dropped, so the selection walked one execution earlier
    // per frame and settled on the *oldest* execution - a wrong picture pointed the other way. Both regimes are
    // asserted here, because either one alone accepts a function that is wrong in the other.
    //
    // Fill phase: nothing dropped, so the indices did not move and the selection must not either.
    CHECK(AdjustSelectionAfterPush(2, 4, /*bDropped=*/false) == 2);
    CHECK(AdjustSelectionAfterPush(0, 1, false) == 0);
    CHECK(AdjustSelectionAfterPush(-1, 1, false) == -1);

    // Full window: the oldest entry is dropped and every surviving index shifts down by one.
    CHECK(AdjustSelectionAfterPush(1, 5, true) == 0);  // followed its execution
    CHECK(AdjustSelectionAfterPush(4, 5, true) == 3);
    CHECK(AdjustSelectionAfterPush(0, 5, true) == 0);  // it *was* the dropped entry: show the oldest survivor
    CHECK(AdjustSelectionAfterPush(9, 5, true) == 4);  // out of range: clamped to newest retained

    // Nothing retained: no range to clamp into.
    CHECK(AdjustSelectionAfterPush(3, 0, true) == 3);

    // The fill-phase property that N1 needed, as a sequence rather than one step: many pushes into a window that is
    // not yet full leave the selection exactly where it was.
    int selected = 0;
    std::size_t size = 0;
    for (int i = 0; i < 10; ++i)
    {
        size += 1;
        CHECK(size <= 240);
        selected = AdjustSelectionAfterPush(selected, size, /*bDropped=*/false);
        CHECK(selected == 0);
    }
    CHECK(selected == 0);
}
