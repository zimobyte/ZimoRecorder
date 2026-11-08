#pragma once
#include <windows.h>
#include <string>

// =============================================================
//  Ocr.h
//  Text recognition using Windows' own built-in OCR engine (the
//  same one Windows itself uses - no extra download needed). It's
//  driven through PowerShell rather than raw C++ WinRT headers,
//  since that works reliably across different compilers/toolchains
//  (MSVC and MinGW alike) without needing extra SDK pieces.
// =============================================================

namespace ocr {

    // Runs OCR on the image file at imagePath (PNG/JPG). Returns true
    // and fills outText on success - outText can legitimately be empty
    // if the image had no recognizable text. Returns false only on a
    // real failure (OCR engine unavailable, PowerShell missing, etc).
    bool RecognizeTextFromFile(const std::wstring& imagePath, std::wstring& outText);

} // namespace ocr
