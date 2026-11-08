#pragma once
#include <windows.h>
#include <string>

namespace capture {

    // Must be called once at app startup (initializes GDI+)
    bool Init();

    // Call at app shutdown
    void Shutdown();

    // Captures the whole screen (all monitors / virtual screen) into a
    // new HBITMAP. Caller owns the result and must DeleteObject it.
    HBITMAP CaptureFullScreenToBitmap();

    // Captures a specific rectangle (absolute screen coordinates) into a
    // new HBITMAP. Caller owns the result and must DeleteObject it.
    HBITMAP CaptureRectToBitmap(const RECT& screenRect);

    // Saves a bitmap to a PNG file.
    bool SaveBitmapToPngFile(HBITMAP bitmap, const std::wstring& filePath);

    // Puts a copy of the bitmap on the clipboard (as a standard DIB),
    // so it can be pasted into other apps.
    bool CopyBitmapToClipboard(HWND ownerWindow, HBITMAP bitmap);

    // Convenience: capture + save in one call (used by simple one-shot
    // actions that don't go through the editor).
    bool CaptureFullScreenToFile(const std::wstring& filePath);
    bool CaptureRectToFile(const RECT& screenRect, const std::wstring& filePath);

} // namespace capture
