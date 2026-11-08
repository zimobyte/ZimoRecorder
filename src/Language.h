#pragma once
#include <windows.h>

// =============================================================
//  Language.h
//  Runtime, user-switchable UI language. The person running the
//  app picks their language from the tray menu itself — nothing
//  needs to be rebuilt. The choice is remembered for next time.
//
//  To add a new language: add it to the Lang enum below, add its
//  native display name in Language.cpp's NativeName(), its 2-letter
//  save-code in LangToCode()/CodeToLang(), and one full string
//  table alongside the existing ones (EN, FA, DE, ...).
// =============================================================

enum class Lang : int {
    EN = 0,  // English  (default)
    FA,      // Persian / Farsi
    DE,      // German
    RU,      // Russian
    FR,      // French
    ES,      // Spanish
    TR,      // Turkish
    AR,      // Arabic
    ZH,      // Chinese (Simplified)
    IT,      // Italian
    PT,      // Portuguese
    JA,      // Japanese
    KO,      // Korean
    HI,      // Hindi
    NL,      // Dutch
    Count
};

enum class StrId : int {
    MenuCaptureFull = 0,  // menu label: "Screenshot" (always full screen, this path)
    MenuRecordStart,
    MenuRecordStop,
    MenuOpenFolder,
    MenuAbout,
    MenuExit,
    MenuLanguage,
    MsgScreenshotSaved,   // used for both the menu (full) and hotkey (region) screenshot path
    MsgSaveError,
    MsgRecordStart,
    MsgRecordStop,
    MsgRecordError,
    AlreadyRunning,
    AboutIntro,
    AboutMenuHint,        // explains: tray menu = full screen
    AboutHotkeysTitle,
    AboutHotkeyHint,      // explains: hotkeys = pick a region first
    AboutVersionLabel,
    HotkeyScreenshotLabel,// About-box line for the screenshot hotkey
    HotkeyRecordLabel,    // About-box line for the record hotkey
    HotkeyGifLabel,       // About-box line for the GIF hotkey
    IndicatorPause,
    IndicatorResume,
    IndicatorStop,
    EditorToolLine,
    EditorToolArrow,
    EditorToolText,
    EditorToolBlur,
    EditorUndo,
    EditorCopy,
    EditorSave,
    EditorCancel,
    MsgCopiedToClipboard,
    MenuGifStart,
    MenuGifStop,
    MsgGifStart,
    MsgGifStop,
    MenuOcr,
    MsgOcrCopied,
    MsgOcrNoText,
    MsgOcrError,
    Count
};

// Strings for the update feature (kept separate so the main tables stay small)
enum class UpdStr : int {
    MenuCheck = 0,   // tray menu: "Check for Updates"
    Title,           // dialog title
    Intro,           // "A new version is available:"  (+ version number)
    Notes,           // "What's new:"
    Ask,             // "Install the update now?"
    Required,        // shown when the update is mandatory
    Latest,          // "You are using the latest version."
    CheckFailed,
    Downloading,
    InstallFailed,
    DashRegion,      // dashboard: "Capture region"
    DashStartup,     // dashboard: "Start with Windows"
    DashTagline,     // dashboard header line
    DashHint,        // dashboard footer line
    Count
};

namespace lang {

    // Loads the saved language preference from disk.
    // Defaults to English if there's no saved preference yet (first run).
    void Load();

    // The currently active language.
    Lang Current();

    // Switches the active language and saves the preference to disk,
    // so it's remembered the next time the app is started.
    void Set(Lang l);

    // The text for a given string id, in the current language.
    const wchar_t* Str(StrId id);

    // Update-feature text in the current language
    const wchar_t* Upd(UpdStr id);

    // The current language's 2-letter code ("en", "fa", "de", ...)
    const wchar_t* Code();

    // A language's own name, written in that language itself
    // (e.g. "Deutsch", "Русский", "日本語") — used in the Language submenu.
    const wchar_t* NativeName(Lang l);

} // namespace lang
