#pragma once
#include <windows.h>

namespace icons {

    // Converts an HICON (loaded from the app's resources) into a color +
    // alpha HBITMAP (32bpp ARGB), so it can be shown next to a menu item.
    // The caller must free the result with DeleteObject.
    HBITMAP IconToArgbBitmap(HICON hIcon, int size);

    // Shortcut: builds a 16x16 HBITMAP directly from an icon resource id.
    HBITMAP LoadMenuIconBitmap(HINSTANCE hInstance, int resourceId, int size = 16);

} // namespace icons
