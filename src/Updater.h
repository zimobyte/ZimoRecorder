#pragma once
#include <windows.h>
#include <string>

// =============================================================
//  Updater.h
//  In-app updates delivered from YOUR OWN server (not GitHub).
//
//  The app downloads a tiny manifest (latest.txt) over HTTPS from the
//  host set in Config.h. The manifest is digitally signed; the app only
//  trusts it if the signature matches the public key built into the
//  app. The downloaded installer file is also checked against the
//  SHA-256 inside the signed manifest before it is ever run.
// =============================================================

namespace updater {

    enum class Status { UpToDate, Available, Failed, NotConfigured };

    struct CheckResult {
        Status       status = Status::Failed;
        std::wstring version;     // newest version on the server
        std::wstring url;         // where the new exe lives (same host only)
        std::wstring sha256;      // expected hash of that exe (lowercase hex)
        std::wstring notes;       // "what's new" text in the app's current language
        bool         userInitiated = false;
    };

    struct DownloadResult {
        bool         ok = false;
        std::wstring path;        // verified file, ready to install
    };

    // Starts a background check. When finished, posts `msg` to `hwnd`
    // with lParam = CheckResult* (the receiver must delete it).
    void CheckAsync(HWND hwnd, UINT msg, bool userInitiated);

    // Starts a background download + verification. When finished, posts
    // `msg` with lParam = DownloadResult* (the receiver must delete it).
    void DownloadAsync(HWND hwnd, UINT msg, const CheckResult& r);

    // --- Reminder policy (stored per Windows user, in the registry) ---
    // True once the update has been known for UPDATE_MANDATORY_AFTER_DAYS days.
    // The first call for a new version starts its countdown.
    bool IsMandatory(const std::wstring& version);
    // At most one friendly reminder per UPDATE_PROMPT_EVERY_DAYS days.
    bool ShouldPromptToday();
    void MarkPromptedToday();

    // Starts the verified new exe as a helper that swaps itself in once
    // this process has exited. The caller must exit right afterwards.
    bool LaunchInstaller(const std::wstring& newExePath);

    // Call first thing in WinMain. If this process was started as the
    // update helper, it does the swap and returns true (then just exit
    // with exitCode). Otherwise returns false immediately.
    bool RunAsUpdateHelperIfRequested(int& exitCode);

} // namespace updater
