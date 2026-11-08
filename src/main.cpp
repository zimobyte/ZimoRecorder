// =============================================================
//  ZIMO RECORDER - main.cpp
//  Tray-icon app for screenshots and screen recording.
//
//  Design: the tray MENU always acts on the full screen. The
//  matching HOTKEYS always let you drag out a region first.
//  Recording hotkeys/menu items toggle (press/click again to stop).
// =============================================================
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <string>
#include <cstdio>   // swprintf_s
#include <cstring>  // memcpy

#include "resource.h"
#include "Config.h"
#include "Language.h"
#include "ScreenCapture.h"
#include "Overlay.h"
#include "Recorder.h"
#include "Icons.h"
#include "Indicator.h"
#include "Editor.h"
#include "Ocr.h"
#include "Updater.h"
#include "Toast.h"
#include "Dashboard.h"
#include <vector>
#include <memory>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace {

    const wchar_t* kMainWndClass = L"ZimoRecorderMainWnd";

    HWND      g_hwnd = nullptr;
    HINSTANCE g_hInstance = nullptr;
    HICON     g_hIcon = nullptr;           // idle-state icon
    HICON     g_hIconRecording = nullptr;  // recording-state icon (red ring)
    NOTIFYICONDATAW g_nid{};
    rec::ScreenRecorder g_recorder;

    // Update feature
    const UINT     WM_UPDATE_CHECKED    = WM_APP + 10;
    const UINT     WM_UPDATE_DOWNLOADED = WM_APP + 11;
    const UINT_PTR TIMER_UPDATE         = 77;
    bool g_updateBusy      = false;   // a check or download is in progress
    bool g_updateMandatory = false;   // the update being installed is a required one

    // For the dashboard: is an MP4 (wantGif=false) / GIF (true) recording running?
    bool RecordingState(bool wantGif) {
        return g_recorder.IsRecording() && (g_recorder.IsGif() == wantGif);
    }

    // Swaps the tray icon between idle / recording state
    void SetTrayIcon(HICON icon) {
        g_nid.uFlags = NIF_ICON;
        g_nid.hIcon = icon;
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    }

    // -----------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------
    std::wstring GetKnownFolder(REFKNOWNFOLDERID id, const wchar_t* fallbackSubdir) {
        PWSTR raw = nullptr;
        std::wstring result;
        if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &raw))) {
            result = raw;
            CoTaskMemFree(raw);
        } else {
            wchar_t profile[MAX_PATH]{};
            DWORD n = GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
            if (n > 0) {
                result = std::wstring(profile) + L"\\" + fallbackSubdir;
            }
        }
        return result;
    }

    std::wstring EnsureAppSubfolder(const std::wstring& base) {
        std::wstring full = base + L"\\" + APP_FOLDER_NAME;
        SHCreateDirectoryExW(nullptr, full.c_str(), nullptr); // ignores the error if it already exists
        return full;
    }

    std::wstring TimestampedFileName(const wchar_t* prefix, const wchar_t* ext) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t buf[128];
        swprintf_s(buf, L"%s_%04d-%02d-%02d_%02d-%02d-%02d.%s",
                   prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ext);
        return buf;
    }

    // Shows our own notification card (see Toast.h)
    void ShowBalloon(const std::wstring& title, const std::wstring& text) {
        toast::Show(g_hInstance, title, text, L"");
    }

    // Same, but clicking the card opens the folder with this file selected
    void ShowSavedToast(const std::wstring& text, const std::wstring& filePath) {
        std::wstring name;
        size_t slash = filePath.find_last_of(L"\\/");
        name = (slash == std::wstring::npos) ? filePath : filePath.substr(slash + 1);
        toast::Show(g_hInstance, APP_NAME, name.empty() ? text : (text + L"\n" + name), filePath);
    }

    // -----------------------------------------------------------
    // Recording (shared by the tray menu, hotkeys, and the floating
    // indicator's own buttons). Only one recording can run at a time.
    // -----------------------------------------------------------
    void StartRecording(const RECT* region, rec::Format format) {
        std::wstring ext = (format == rec::Format::GIF) ? L"gif" : L"mp4";
        std::wstring folder = EnsureAppSubfolder(GetKnownFolder(FOLDERID_Videos, L"Videos"));
        std::wstring path = folder + L"\\" + TimestampedFileName(L"ZIMO", ext.c_str());

        if (g_recorder.Start(path, region, format)) {
            SetTrayIcon(g_hIconRecording);
            indicator::Show(g_hInstance,
                []() { // onTogglePause
                    if (g_recorder.IsPaused()) {
                        g_recorder.Resume();
                        indicator::SetPaused(false);
                    } else {
                        g_recorder.Pause();
                        indicator::SetPaused(true);
                    }
                },
                []() { // onStop
                    bool wasGif = g_recorder.IsGif();
                    g_recorder.Stop();
                    SetTrayIcon(g_hIcon);
                    indicator::Hide();
                    ShowSavedToast(wasGif ? lang::Str(StrId::MsgGifStop) : lang::Str(StrId::MsgRecordStop), g_recorder.OutputPath());
                },
                g_recorder.SupportsPause());
            ShowBalloon(APP_NAME, (format == rec::Format::GIF) ? lang::Str(StrId::MsgGifStart) : lang::Str(StrId::MsgRecordStart));
        } else {
            ShowBalloon(APP_NAME, lang::Str(StrId::MsgRecordError));
        }
    }

    void StopRecordingNow() {
        bool wasGif = g_recorder.IsGif();
        g_recorder.Stop();
        SetTrayIcon(g_hIcon);
        indicator::Hide();
        ShowSavedToast(wasGif ? lang::Str(StrId::MsgGifStop) : lang::Str(StrId::MsgRecordStop), g_recorder.OutputPath());
    }

    // Menu: full-screen mp4 toggle
    void DoToggleRecordFull() {
        if (g_recorder.IsRecording()) { StopRecordingNow(); return; }
        StartRecording(nullptr, rec::Format::MP4);
    }

    // Hotkey: pick a region, then mp4 toggle
    void DoRecordRegionMp4(HINSTANCE hInstance) {
        if (g_recorder.IsRecording()) { StopRecordingNow(); return; }
        RECT r;
        if (!overlay::RunRegionSelector(hInstance, r)) return; // cancelled
        StartRecording(&r, rec::Format::MP4);
    }

    // Menu: full-screen GIF toggle
    void DoToggleGifFull() {
        if (g_recorder.IsRecording()) { StopRecordingNow(); return; }
        StartRecording(nullptr, rec::Format::GIF);
    }

    // Hotkey: pick a region, then GIF toggle
    void DoRecordRegionGif(HINSTANCE hInstance) {
        if (g_recorder.IsRecording()) { StopRecordingNow(); return; }
        RECT r;
        if (!overlay::RunRegionSelector(hInstance, r)) return; // cancelled
        StartRecording(&r, rec::Format::GIF);
    }

    // -----------------------------------------------------------
    // Screenshot actions (both open the editor before saving)
    // -----------------------------------------------------------
    void HandleEditorResult(editor::Result r, const std::wstring& savedPath) {
        switch (r) {
        case editor::Result::Saved:  ShowSavedToast(lang::Str(StrId::MsgScreenshotSaved), savedPath); break;
        case editor::Result::Copied: ShowBalloon(APP_NAME, lang::Str(StrId::MsgCopiedToClipboard)); break;
        default: break; // cancelled - no balloon
        }
    }

    // Menu: full screen
    void DoScreenshotFull() {
        HBITMAP bmp = capture::CaptureFullScreenToBitmap();
        if (!bmp) { ShowBalloon(APP_NAME, lang::Str(StrId::MsgSaveError)); return; }

        std::wstring folder = EnsureAppSubfolder(GetKnownFolder(FOLDERID_Pictures, L"Pictures"));
        std::wstring path = folder + L"\\" + TimestampedFileName(L"ZIMO", L"png");
        editor::Result r = editor::Open(g_hInstance, g_hwnd, bmp, path);
        HandleEditorResult(r, path);
    }

    // Hotkey: pick a region first
    void DoScreenshotRegion(HINSTANCE hInstance) {
        RECT r;
        if (!overlay::RunRegionSelector(hInstance, r)) return; // user cancelled

        HBITMAP bmp = capture::CaptureRectToBitmap(r);
        if (!bmp) { ShowBalloon(APP_NAME, lang::Str(StrId::MsgSaveError)); return; }

        std::wstring folder = EnsureAppSubfolder(GetKnownFolder(FOLDERID_Pictures, L"Pictures"));
        std::wstring path = folder + L"\\" + TimestampedFileName(L"ZIMO", L"png");
        editor::Result res = editor::Open(g_hInstance, g_hwnd, bmp, path);
        HandleEditorResult(res, path);
    }

    // Used by both the OCR menu item and the OCR hotkey - OCR always
    // needs a bounded region, so there's no separate "full screen" path.
    void DoOcr(HINSTANCE hInstance) {
        RECT r;
        if (!overlay::RunRegionSelector(hInstance, r)) return;

        HBITMAP bmp = capture::CaptureRectToBitmap(r);
        if (!bmp) { ShowBalloon(APP_NAME, lang::Str(StrId::MsgOcrError)); return; }

        wchar_t tempDir[MAX_PATH];
        GetTempPathW(MAX_PATH, tempDir);
        std::wstring tempPath = std::wstring(tempDir) + L"zimo_ocr_capture.png";
        bool saved = capture::SaveBitmapToPngFile(bmp, tempPath);
        DeleteObject(bmp);
        if (!saved) { ShowBalloon(APP_NAME, lang::Str(StrId::MsgOcrError)); return; }

        std::wstring text;
        bool ok = ocr::RecognizeTextFromFile(tempPath, text);
        DeleteFileW(tempPath.c_str());

        if (!ok) {
            ShowBalloon(APP_NAME, lang::Str(StrId::MsgOcrError));
            return;
        }
        if (text.empty()) {
            ShowBalloon(APP_NAME, lang::Str(StrId::MsgOcrNoText));
            return;
        }

        if (OpenClipboard(g_hwnd)) {
            EmptyClipboard();
            size_t bytes = (text.size() + 1) * sizeof(wchar_t);
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
            if (hMem) {
                void* mem = GlobalLock(hMem);
                memcpy(mem, text.c_str(), bytes);
                GlobalUnlock(hMem);
                if (!SetClipboardData(CF_UNICODETEXT, hMem)) GlobalFree(hMem);
            }
            CloseClipboard();
        }
        ShowBalloon(APP_NAME, lang::Str(StrId::MsgOcrCopied));
        MessageBoxW(g_hwnd, text.c_str(), APP_NAME, MB_OK | MB_ICONINFORMATION);
    }

    void DoOpenFolder() {
        std::wstring folder = EnsureAppSubfolder(GetKnownFolder(FOLDERID_Pictures, L"Pictures"));
        ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    void DoAbout() {
        std::wstring msg = std::wstring(APP_NAME) + L"\n" +
            lang::Str(StrId::AboutVersionLabel) + L" " + APP_VERSION + L"\n\n" +
            lang::Str(StrId::AboutIntro) + L"\n" +
            lang::Str(StrId::AboutMenuHint) + L"\n\n" +
            lang::Str(StrId::AboutHotkeysTitle) + L"\n" +
            lang::Str(StrId::AboutHotkeyHint) + L"\n" +
            L"  " + std::wstring(HOTKEY_TEXT_SCREENSHOT) + L" / " + HOTKEY2_TEXT_SCREENSHOT + L"  : " + lang::Str(StrId::HotkeyScreenshotLabel) + L"\n" +
            L"  " + std::wstring(HOTKEY_TEXT_RECORD) + L" / " + HOTKEY2_TEXT_RECORD + L"  : " + lang::Str(StrId::HotkeyRecordLabel)     + L"\n" +
            L"  " + std::wstring(HOTKEY_TEXT_GIF) + L" / " + HOTKEY2_TEXT_GIF + L"  : " + lang::Str(StrId::HotkeyGifLabel)        + L"\n" +
            L"  " + std::wstring(HOTKEY_TEXT_OCR) + L"  : " + lang::Str(StrId::MenuOcr);
        MessageBoxW(g_hwnd, msg.c_str(), APP_NAME, MB_OK | MB_ICONINFORMATION);
    }


    // -----------------------------------------------------------
    // Updates (from our own server - see Updater.h)
    // -----------------------------------------------------------
    void StartUpdateDownload(const updater::CheckResult& r) {
        g_updateBusy = true;
        ShowBalloon(APP_NAME, lang::Upd(UpdStr::Downloading));
        updater::DownloadAsync(g_hwnd, WM_UPDATE_DOWNLOADED, r);
    }

    void StartUpdateCheck(bool userInitiated) {
        if (g_updateBusy) return;
        g_updateBusy = true;
        updater::CheckAsync(g_hwnd, WM_UPDATE_CHECKED, userInitiated);
    }

    void OnUpdateChecked(updater::CheckResult* raw) {
        std::unique_ptr<updater::CheckResult> r(raw);
        g_updateBusy = false;

        if (r->status != updater::Status::Available) {
            if (r->userInitiated) {
                ShowBalloon(APP_NAME, r->status == updater::Status::UpToDate
                                          ? lang::Upd(UpdStr::Latest)
                                          : lang::Upd(UpdStr::CheckFailed));
            }
            return;
        }

        bool mandatory = updater::IsMandatory(r->version);
        // Friendly reminders: at most once per day, unless the person asked or it is required.
        if (!mandatory && !r->userInitiated && !updater::ShouldPromptToday()) return;
        updater::MarkPromptedToday();

        std::wstring text = std::wstring(lang::Upd(UpdStr::Intro)) + L" " + r->version + L"\n\n";
        if (!r->notes.empty()) {
            text += std::wstring(lang::Upd(UpdStr::Notes)) + L"\n" + r->notes + L"\n\n";
        }
        text += mandatory ? lang::Upd(UpdStr::Required) : lang::Upd(UpdStr::Ask);

        UINT flags = mandatory ? (MB_OKCANCEL | MB_ICONWARNING) : (MB_YESNO | MB_ICONINFORMATION);
        int answer = MessageBoxW(g_hwnd, text.c_str(), lang::Upd(UpdStr::Title), flags | MB_SETFOREGROUND);

        if (answer == IDYES || answer == IDOK) {
            g_updateMandatory = mandatory;
            StartUpdateDownload(*r);
        } else if (mandatory) {
            DestroyWindow(g_hwnd); // a required update was declined: close the app
        }
    }

    void OnUpdateDownloaded(updater::DownloadResult* raw) {
        std::unique_ptr<updater::DownloadResult> d(raw);
        g_updateBusy = false;

        if (d->ok) {
            if (g_recorder.IsRecording()) StopRecordingNow(); // never lose a recording
            if (updater::LaunchInstaller(d->path)) {
                DestroyWindow(g_hwnd); // the helper swaps in the new exe and restarts it
                return;
            }
        }
        MessageBoxW(g_hwnd, lang::Upd(UpdStr::InstallFailed), APP_NAME, MB_OK | MB_ICONERROR);
        if (g_updateMandatory) DestroyWindow(g_hwnd);
    }

    // -----------------------------------------------------------
    // Tray menu
    // -----------------------------------------------------------

    // Appends one menu item together with its small icon (at the end of the menu)
    void InsertIconMenuItem(HMENU menu, UINT id, const wchar_t* text, HBITMAP bmp, bool enabled = true) {
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_STRING | MIIM_BITMAP | MIIM_STATE;
        mii.wID = id;
        mii.dwTypeData = const_cast<wchar_t*>(text);
        mii.hbmpItem = bmp; // can be nullptr (meaning: no icon)
        mii.fState = enabled ? MFS_ENABLED : MFS_GRAYED;
        InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &mii);
    }

    // Appends a submenu entry (icon + text + arrow to a child menu)
    void InsertIconSubMenuItem(HMENU menu, const wchar_t* text, HBITMAP bmp, HMENU subMenu) {
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING | MIIM_BITMAP | MIIM_SUBMENU;
        mii.dwTypeData = const_cast<wchar_t*>(text);
        mii.hbmpItem = bmp;
        mii.hSubMenu = subMenu;
        InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &mii);
    }

    // Builds the "Language" submenu: one entry per supported language,
    // with a checkmark on whichever one is currently active.
    HMENU BuildLanguageSubMenu() {
        HMENU sub = CreatePopupMenu();
        for (int i = 0; i < (int)Lang::Count; ++i) {
            Lang l = (Lang)i;
            UINT flags = MF_STRING | (l == lang::Current() ? MF_CHECKED : MF_UNCHECKED);
            AppendMenuW(sub, flags, IDM_LANG_BASE + i, lang::NativeName(l));
        }
        return sub;
    }

    void ShowTrayMenu(HWND hwnd, HINSTANCE hInstance) {
        HMENU menu = CreatePopupMenu();

        // These bitmaps are owned by the app; freed once the menu closes
        std::vector<HBITMAP> ownedBitmaps;
        auto bmpFor = [&](int resId) -> HBITMAP {
            HBITMAP b = icons::LoadMenuIconBitmap(hInstance, resId);
            ownedBitmaps.push_back(b);
            return b;
        };

        bool mp4Active = g_recorder.IsRecording() && !g_recorder.IsGif();
        bool gifActive = g_recorder.IsRecording() && g_recorder.IsGif();

        InsertIconMenuItem(menu, IDM_OPEN_DASHBOARD, APP_NAME, nullptr);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        InsertIconMenuItem(menu, IDM_CAPTURE_FULL, lang::Str(StrId::MenuCaptureFull), bmpFor(IDI_MENU_CAPTURE_FULL));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        InsertIconMenuItem(menu, IDM_TOGGLE_RECORD,
                            mp4Active ? lang::Str(StrId::MenuRecordStop) : lang::Str(StrId::MenuRecordStart),
                            bmpFor(mp4Active ? IDI_MENU_RECORD_STOP : IDI_MENU_RECORD_START),
                            /*enabled=*/ !gifActive);
        InsertIconMenuItem(menu, IDM_TOGGLE_GIF,
                            gifActive ? lang::Str(StrId::MenuGifStop) : lang::Str(StrId::MenuGifStart),
                            bmpFor(gifActive ? IDI_MENU_RECORD_STOP : IDI_MENU_RECORD_START),
                            /*enabled=*/ !mp4Active);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        InsertIconMenuItem(menu, IDM_OCR, lang::Str(StrId::MenuOcr), bmpFor(IDI_MENU_OCR));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        InsertIconMenuItem(menu, IDM_OPEN_FOLDER, lang::Str(StrId::MenuOpenFolder), bmpFor(IDI_MENU_FOLDER));
        InsertIconMenuItem(menu, IDM_CHECK_UPDATE, lang::Upd(UpdStr::MenuCheck), nullptr);
        InsertIconMenuItem(menu, IDM_ABOUT,       lang::Str(StrId::MenuAbout),      bmpFor(IDI_MENU_ABOUT));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        InsertIconSubMenuItem(menu, lang::Str(StrId::MenuLanguage), bmpFor(IDI_MENU_LANGUAGE), BuildLanguageSubMenu());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        InsertIconMenuItem(menu, IDM_EXIT, lang::Str(StrId::MenuExit), bmpFor(IDI_MENU_EXIT));

        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd); // so the menu closes properly on an outside click
        TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
        PostMessage(hwnd, WM_NULL, 0, 0);
        DestroyMenu(menu); // also destroys the Language submenu owned by it

        for (HBITMAP b : ownedBitmaps) {
            if (b) DeleteObject(b);
        }
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
            } else if (lParam == WM_LBUTTONUP) {
                dashboard::Show((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), hwnd, RecordingState);
            }
            return 0;

        case WM_SHOW_DASHBOARD:
            dashboard::Show((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), hwnd, RecordingState);
            return 0;

        case WM_DASH_ACTION: {
            HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
            switch (wParam) {
            case DASH_REGION: Sleep(200); DoScreenshotRegion(hi); break;
            case DASH_FULL:   Sleep(250); DoScreenshotFull(); break;
            case DASH_RECORD: Sleep(200); DoRecordRegionMp4(hi); break;
            case DASH_GIF:    Sleep(200); DoRecordRegionGif(hi); break;
            case DASH_OCR:    Sleep(200); DoOcr(hi); break;
            case DASH_FOLDER: DoOpenFolder(); break;
            case DASH_UPDATE: StartUpdateCheck(true); break;
            case DASH_LANG: {
                HMENU m = BuildLanguageSubMenu();
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                PostMessage(hwnd, WM_NULL, 0, 0);
                DestroyMenu(m);
                break;
            }
            }
            dashboard::Refresh();
            return 0;
        }

        case WM_HOTKEY:
            switch (wParam) {
            case HOTKEY_ID_SCREENSHOT:
            case HOTKEY_ID_SCREENSHOT2: DoScreenshotRegion((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE)); break;
            case HOTKEY_ID_RECORD:
            case HOTKEY_ID_RECORD2:     DoRecordRegionMp4((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE)); break;
            case HOTKEY_ID_GIF:
            case HOTKEY_ID_GIF2:        DoRecordRegionGif((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE)); break;
            case HOTKEY_ID_OCR:        DoOcr((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE)); break;
            }
            return 0;

        case WM_COMMAND: {
            UINT id = LOWORD(wParam);

            // Language submenu entries (one id per language)
            if (id >= IDM_LANG_BASE && id < IDM_LANG_BASE + (UINT)Lang::Count) {
                lang::Set((Lang)(id - IDM_LANG_BASE));
                dashboard::Refresh();
                return 0;
            }

            switch (id) {
            case IDM_OPEN_DASHBOARD: dashboard::Show((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), hwnd, RecordingState); break;
            case IDM_CAPTURE_FULL:  DoScreenshotFull(); break;
            case IDM_TOGGLE_RECORD: DoToggleRecordFull(); break;
            case IDM_TOGGLE_GIF:    DoToggleGifFull(); break;
            case IDM_OCR:           DoOcr((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE)); break;
            case IDM_OPEN_FOLDER:   DoOpenFolder(); break;
            case IDM_CHECK_UPDATE:  StartUpdateCheck(true); break;
            case IDM_ABOUT:         DoAbout(); break;
            case IDM_EXIT:          DestroyWindow(hwnd); break;
            }
            return 0;
        }

        case WM_UPDATE_CHECKED:
            OnUpdateChecked((updater::CheckResult*)lParam);
            return 0;

        case WM_UPDATE_DOWNLOADED:
            OnUpdateDownloaded((updater::DownloadResult*)lParam);
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_UPDATE) {
                // first check ~15 s after start, then every UPDATE_CHECK_INTERVAL_MS
                SetTimer(hwnd, TIMER_UPDATE, UPDATE_CHECK_INTERVAL_MS, nullptr);
                StartUpdateCheck(false);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_UPDATE);
            if (g_recorder.IsRecording()) g_recorder.Stop();
            indicator::Hide();
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            UnregisterHotKey(hwnd, HOTKEY_ID_SCREENSHOT);
            UnregisterHotKey(hwnd, HOTKEY_ID_RECORD);
            UnregisterHotKey(hwnd, HOTKEY_ID_GIF);
            UnregisterHotKey(hwnd, HOTKEY_ID_OCR);
            UnregisterHotKey(hwnd, HOTKEY_ID_SCREENSHOT2);
            UnregisterHotKey(hwnd, HOTKEY_ID_RECORD2);
            UnregisterHotKey(hwnd, HOTKEY_ID_GIF2);
            capture::Shutdown();
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInstance = hInstance;

    // If this exe was started by the updater to swap itself in, do that and stop.
    {
        int helperExit = 0;
        if (updater::RunAsUpdateHelperIfRequested(helperExit)) return helperExit;
    }

    // Make the app DPI-aware, so screen coordinates are real pixels on
    // scaled displays (125% / 150% ...). Without this, region selection
    // and recording can end up offset or blurry.
    {
        typedef BOOL (WINAPI *SetDpiCtxFn)(HANDLE);
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        SetDpiCtxFn setCtx = user32 ? (SetDpiCtxFn)GetProcAddress(user32, "SetProcessDpiAwarenessContext") : nullptr;
        if (!(setCtx && setCtx((HANDLE)-4 /* PER_MONITOR_AWARE_V2 */))) {
            SetProcessDPIAware();
        }
    }

    // Load the saved language preference first (defaults to English on first run),
    // so even the "already running" message below shows in the right language.
    lang::Load();

    // Don't allow a second instance to run at the same time
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"ZimoRecorderSingleInstanceMutex");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        // Already running: ask the running copy to open its window
        HWND running = FindWindowExW(HWND_MESSAGE, nullptr, kMainWndClass, nullptr);
        if (running) PostMessageW(running, WM_SHOW_DASHBOARD, 0, 0);
        else MessageBoxW(nullptr, lang::Str(StrId::AlreadyRunning), APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    dashboard::MigrateLegacyAutostart();

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    capture::Init();

    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kMainWndClass;
    RegisterClassW(&wc);

    g_hwnd = CreateWindowW(kMainWndClass, APP_NAME, 0, 0, 0, 0, 0,
                            HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!g_hwnd) return 1;

    g_hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPICON),
                                 IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    g_hIconRecording = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPICON_RECORDING),
                                          IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = ID_TRAY_APP_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_hIcon;
    wcsncpy_s(g_nid.szTip, APP_NAME, _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Register all shortcuts; remember any that another program already owns
    std::wstring failedKeys;
    auto reg = [&](int id, UINT mod, UINT vk, const wchar_t* label) {
        if (!RegisterHotKey(g_hwnd, id, mod | MOD_NOREPEAT, vk)) {
            if (!failedKeys.empty()) failedKeys += L", ";
            failedKeys += label;
        }
    };
    reg(HOTKEY_ID_SCREENSHOT,  HOTKEY_MOD_SCREENSHOT,  HOTKEY_VK_SCREENSHOT,  HOTKEY_TEXT_SCREENSHOT);
    reg(HOTKEY_ID_RECORD,      HOTKEY_MOD_RECORD,      HOTKEY_VK_RECORD,      HOTKEY_TEXT_RECORD);
    reg(HOTKEY_ID_GIF,         HOTKEY_MOD_GIF,         HOTKEY_VK_GIF,         HOTKEY_TEXT_GIF);
    reg(HOTKEY_ID_OCR,         HOTKEY_MOD_OCR,         HOTKEY_VK_OCR,         HOTKEY_TEXT_OCR);
    reg(HOTKEY_ID_SCREENSHOT2, HOTKEY2_MOD_SCREENSHOT, HOTKEY2_VK_SCREENSHOT, HOTKEY2_TEXT_SCREENSHOT);
    reg(HOTKEY_ID_RECORD2,     HOTKEY2_MOD_RECORD,     HOTKEY2_VK_RECORD,     HOTKEY2_TEXT_RECORD);
    reg(HOTKEY_ID_GIF2,        HOTKEY2_MOD_GIF,        HOTKEY2_VK_GIF,        HOTKEY2_TEXT_GIF);
    if (!failedKeys.empty()) {
        // (English on purpose: this is a technical hint for whoever sets the app up)
        ShowBalloon(APP_NAME, L"These shortcuts are already used by another program: " + failedKeys);
    }

    // Started by hand -> open the window. Started with Windows ("--background") -> stay in the tray.
    if (!wcsstr(GetCommandLineW(), L"--background")) {
        dashboard::Show(hInstance, g_hwnd, RecordingState);
    }

    // First update check shortly after start-up (then repeats on a timer)
    SetTimer(g_hwnd, TIMER_UPDATE, 15000, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (mutex) CloseHandle(mutex);
    return (int)msg.wParam;
}
