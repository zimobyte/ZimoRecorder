// =============================================================
//  Config.h
//  ** This is the ONLY file you need to edit to rename the app. **
//     Everything else in the code refers back to these few lines.
// =============================================================
#pragma once
#include <string>

// Display name shown in the tray icon tooltip, messages and About box
#define APP_NAME        L"ZIMO RECORDER"

// Version string
#define APP_VERSION     L"1.0.0"

// Company / publisher name (also used in the exe's version info, app.rc)
#define APP_COMPANY     L"Zimo"

// Name of the subfolder where screenshots/recordings are saved,
// inside the user's Pictures / Videos folder, e.g.:
// C:\Users\<user>\Pictures\ZimoRecorder
#define APP_FOLDER_NAME L"ZimoRecorder"

// Default keyboard shortcuts - every one is just TWO keys (a modifier + a key).
// Each action has: the modifier (MOD_CONTROL / MOD_SHIFT / MOD_ALT),
// the key (VK_ codes from winuser.h), and the text shown in the About box.
//
// Design: the TRAY MENU always acts on the full screen; these HOTKEYS
// always let you pick a window or drag out a region first. (Recording
// hotkeys toggle: press once to start, press again to stop.)
#define HOTKEY_MOD_SCREENSHOT  MOD_CONTROL
#define HOTKEY_VK_SCREENSHOT   VK_SNAPSHOT          // Ctrl+PrtSc -> screenshot
#define HOTKEY_TEXT_SCREENSHOT L"Ctrl+PrtSc"

#define HOTKEY_MOD_RECORD      MOD_SHIFT
#define HOTKEY_VK_RECORD       VK_SNAPSHOT          // Shift+PrtSc -> record video (mp4), toggle
#define HOTKEY_TEXT_RECORD     L"Shift+PrtSc"

#define HOTKEY_MOD_GIF         MOD_ALT
#define HOTKEY_VK_GIF          VK_SNAPSHOT          // Alt+PrtSc -> record GIF, toggle
#define HOTKEY_TEXT_GIF        L"Alt+PrtSc"

#define HOTKEY_MOD_OCR         MOD_CONTROL
#define HOTKEY_VK_OCR          VK_F9                // Ctrl+F9 -> extract text (OCR)
#define HOTKEY_TEXT_OCR        L"Ctrl+F9"

// Backup shortcuts, in case a keyboard has no PrtSc key (many laptops need Fn)
// or another program already uses the PrtSc ones. Both sets work at the same time.
#define HOTKEY2_MOD_SCREENSHOT MOD_CONTROL
#define HOTKEY2_VK_SCREENSHOT  VK_F6
#define HOTKEY2_TEXT_SCREENSHOT L"Ctrl+F6"

#define HOTKEY2_MOD_RECORD     MOD_CONTROL
#define HOTKEY2_VK_RECORD      VK_F7
#define HOTKEY2_TEXT_RECORD    L"Ctrl+F7"

#define HOTKEY2_MOD_GIF        MOD_CONTROL
#define HOTKEY2_VK_GIF         VK_F8
#define HOTKEY2_TEXT_GIF       L"Ctrl+F8"

// -------------------------------------------------------------
//  Updates (delivered from YOUR server, not GitHub)
// -------------------------------------------------------------
// Server that hosts latest.txt and the new exe files (HTTPS only).
#define UPDATE_HOST             L"updates.nettfreedaily.xyz"
// Path of the signed manifest file on that server.
#define UPDATE_MANIFEST_PATH    L"/zimo/latest.txt"
// Public key used to check update signatures (128 hex characters).
// Create it once with  tools/update_keygen.py  and paste it here.
// While this is empty, update checks are switched off (safe default).
#define UPDATE_PUBLIC_KEY_HEX   L"5b5c339cdbc2d4196dbf6d60ff838d380412b676313be450eb1b31b0e9955662081e8a44788ca0ff3af198e1496f31fb8d01b39efd61a9183c06bd5e70cf29bc"
// After an update has been known for this many days it becomes required.
#define UPDATE_MANDATORY_AFTER_DAYS 10
// Friendly reminder at most once every N days.
#define UPDATE_PROMPT_EVERY_DAYS    1
// How often a running app re-checks (milliseconds): 6 hours.
#define UPDATE_CHECK_INTERVAL_MS    (6 * 60 * 60 * 1000)

namespace cfg {
    // File name of the app icon, copied from the assets folder
    inline const wchar_t* IconAssetName() { return L"icon.ico"; }
}
