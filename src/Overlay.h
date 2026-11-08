#pragma once
#include <windows.h>

namespace overlay {

    // Shows a semi-transparent full-screen window so the user can drag
    // with the mouse to select a region. This call blocks until the
    // selection is finished. Returns true and fills outRect on a
    // successful selection; returns false on Esc or a click with no drag.
    bool RunRegionSelector(HINSTANCE hInstance, RECT& outRect);

} // namespace overlay
