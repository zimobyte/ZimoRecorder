#pragma once
#include <windows.h>
#include <string>

// =============================================================
//  Editor.h
//  The screenshot editor window: shown right after a capture, so
//  the person can draw a line/arrow, add text, or blur a region
//  before the image is saved or copied. Mirrors the "annotate
//  before you save" step.
// =============================================================

namespace editor {

    enum class Result {
        Saved,      // written to the path passed into Open()
        Copied,     // put on the clipboard instead
        Cancelled,  // the person closed the editor without keeping it
    };

    // Opens the editor with the given bitmap. The editor takes
    // ownership of 'bitmap' (it will delete it). Blocks until the
    // person clicks Save, Copy, or Cancel; 'savePath' is only used
    // if they click Save.
    Result Open(HINSTANCE hInstance, HWND ownerForClipboard, HBITMAP bitmap,
                const std::wstring& savePath);

} // namespace editor
