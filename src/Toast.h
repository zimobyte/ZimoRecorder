#pragma once
#include <windows.h>
#include <string>

// =============================================================
//  Toast.h
//  Our own small notification card (bottom-right corner of the
//  screen). It does not depend on Windows notification settings,
//  so it always shows. Clicking it opens the folder with the saved
//  file selected (when a file path is given).
// =============================================================
namespace toast {

    // title/text: what to show. openPath: full path of a saved file
    // (empty = the card is just a message and a click only closes it).
    void Show(HINSTANCE hInstance, const std::wstring& title,
              const std::wstring& text, const std::wstring& openPath);

} // namespace toast
