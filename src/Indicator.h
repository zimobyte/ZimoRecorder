#pragma once
#include <windows.h>

// =============================================================
//  Indicator.h
//  A small always-on-top floating window shown while recording:
//  a red dot, an elapsed-time counter, and Pause/Resume + Stop
//  buttons — a small on-screen recording toolbar/HUD.
// =============================================================

namespace indicator {

    // Shows the indicator (creates it if needed) and starts its timer
    // from 00:00. onTogglePause / onStop are called when the person
    // clicks the matching button. Pass supportsPause=false (GIF
    // recordings) to hide the Pause/Resume button entirely.
    void Show(HINSTANCE hInstance, void (*onTogglePause)(), void (*onStop)(),
              bool supportsPause = true);

    // Hides and destroys the indicator window.
    void Hide();

    // Updates the indicator's visual state (timer keeps running or
    // pauses, and the button label switches between Pause/Resume).
    void SetPaused(bool paused);

    bool IsVisible();

} // namespace indicator
