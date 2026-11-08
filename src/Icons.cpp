#include "Icons.h"

namespace icons {

    HBITMAP IconToArgbBitmap(HICON hIcon, int size) {
        if (!hIcon) return nullptr;

        BITMAPINFO bi{};
        bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth       = size;
        bi.bmiHeader.biHeight      = -size; // top-down DIB
        bi.bmiHeader.biPlanes      = 1;
        bi.bmiHeader.biBitCount    = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        HDC hdcScreen = GetDC(nullptr);
        void* bits = nullptr;
        HBITMAP hbmp = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);

        if (hbmp) {
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            HGDIOBJ hOld = SelectObject(hdcMem, hbmp);
            // Canvas starts fully transparent; DrawIconEx preserves the icon's own alpha
            DrawIconEx(hdcMem, 0, 0, hIcon, size, size, 0, nullptr, DI_NORMAL);
            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
        }

        ReleaseDC(nullptr, hdcScreen);
        return hbmp;
    }

    HBITMAP LoadMenuIconBitmap(HINSTANCE hInstance, int resourceId, int size) {
        HICON hico = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(resourceId),
                                        IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
        if (!hico) return nullptr;
        HBITMAP bmp = IconToArgbBitmap(hico, size);
        DestroyIcon(hico);
        return bmp;
    }

} // namespace icons
