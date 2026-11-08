#pragma once
#include <windows.h>

// =============================================================
//  Dashboard.h
//  The app's main window: big buttons for every action, the
//  keyboard shortcuts, language, "start with Windows" and update
//  check. The app still lives in the tray; closing this window
//  only hides it.
// =============================================================
namespace dashboard {

    // Asks the app whether an MP4 / GIF recording is running right now
    // (so the buttons can say "Stop" instead of "Record").
    typedef bool (*RecordingStateFn)(bool wantGif);

    // Creates (if needed), shows and focuses the window.
    void Show(HINSTANCE hInstance, HWND mainWindow, RecordingStateFn state);

    // Repaint with the current language / recording state.
    void Refresh();

    // "Start with Windows" (per user, no admin rights needed)
    bool IsAutostartEnabled();
    void SetAutostart(bool enable);

    // Carries a start-up entry created under the product's previous internal name
    // over to the current one (so nobody ends up with two copies starting).
    void MigrateLegacyAutostart();

} // namespace dashboard
