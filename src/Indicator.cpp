#include "Indicator.h"
#include "Language.h"
#include <cstdio> // swprintf_s

// Some older Windows SDK headers don't define this yet (it was added in
// the Windows 10 2004 SDK) - define it ourselves if it's missing so the
// capture-exclusion call below always compiles.
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

namespace indicator {

    namespace {
        const wchar_t* kClassName = L"ZimoRecorderIndicatorWnd";

        const int kWndW = 230;
        const int kWndH = 56;
        const int kBtnW = 70;
        const int kBtnH = 26;
        const int kMargin = 16;

        const COLORREF kBg   = RGB(21, 28, 36);
        const COLORREF kRed  = RGB(251, 21, 35);
        const COLORREF kText = RGB(235, 235, 238);

        HWND g_hwnd = nullptr;
        HWND g_btnPause = nullptr;
        HWND g_btnStop  = nullptr;

        void (*g_onTogglePause)() = nullptr;
        void (*g_onStop)() = nullptr;

        int  g_elapsedSeconds = 0;
        bool g_paused = false;
        bool g_supportsPause = true;

        enum { ID_BTN_PAUSE = 1, ID_BTN_STOP = 2, ID_TIMER = 1 };

        void FormatElapsed(wchar_t* buf, size_t bufLen) {
            int h = g_elapsedSeconds / 3600;
            int m = (g_elapsedSeconds % 3600) / 60;
            int s = g_elapsedSeconds % 60;
            if (h > 0) {
                swprintf_s(buf, bufLen, L"%02d:%02d:%02d", h, m, s);
            } else {
                swprintf_s(buf, bufLen, L"%02d:%02d", m, s);
            }
        }

        void UpdatePauseButtonLabel() {
            if (!g_btnPause) return;
            SetWindowTextW(g_btnPause, g_paused ? lang::Str(StrId::IndicatorResume)
                                                 : lang::Str(StrId::IndicatorPause));
        }

        LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            switch (msg) {
            case WM_CREATE: {
                if (g_supportsPause) {
                    g_btnPause = CreateWindowW(L"BUTTON", lang::Str(StrId::IndicatorPause),
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        kWndW - (kBtnW * 2 + 8 + 10), kWndH - kBtnH - 8, kBtnW, kBtnH,
                        hwnd, (HMENU)(UINT_PTR)ID_BTN_PAUSE,
                        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);
                }

                g_btnStop = CreateWindowW(L"BUTTON", lang::Str(StrId::IndicatorStop),
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    kWndW - (kBtnW + 10), kWndH - kBtnH - 8, kBtnW, kBtnH,
                    hwnd, (HMENU)(UINT_PTR)ID_BTN_STOP,
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);
                return 0;
            }

            case WM_COMMAND:
                if (HIWORD(wParam) == BN_CLICKED) {
                    if (LOWORD(wParam) == ID_BTN_PAUSE && g_onTogglePause) {
                        g_onTogglePause();
                    } else if (LOWORD(wParam) == ID_BTN_STOP && g_onStop) {
                        g_onStop();
                    }
                }
                return 0;

            case WM_TIMER:
                if (!g_paused) {
                    g_elapsedSeconds++;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;

            case WM_ERASEBKGND:
                return 1; // avoid flicker; WM_PAINT paints the whole client area

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                RECT rc;
                GetClientRect(hwnd, &rc);
                HBRUSH bg = CreateSolidBrush(kBg);
                FillRect(hdc, &rc, bg);
                DeleteObject(bg);

                // Red (or dimmed, while paused) recording dot
                HBRUSH dot = CreateSolidBrush(g_paused ? RGB(130, 40, 46) : kRed);
                HGDIOBJ oldBrush = SelectObject(hdc, dot);
                HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
                Ellipse(hdc, 14, 14, 30, 30);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(dot);

                // Elapsed time text
                wchar_t buf[16];
                FormatElapsed(buf, 16);
                SetTextColor(hdc, kText);
                SetBkMode(hdc, TRANSPARENT);
                RECT textRc{ 40, 10, kWndW - 100, 34 };
                DrawTextW(hdc, buf, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_DESTROY:
                KillTimer(hwnd, ID_TIMER);
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    } // namespace

    void Show(HINSTANCE hInstance, void (*onTogglePause)(), void (*onStop)(), bool supportsPause) {
        g_onTogglePause = onTogglePause;
        g_onStop = onStop;
        g_elapsedSeconds = 0;
        g_paused = false;
        g_supportsPause = supportsPause;
        g_btnPause = nullptr;

        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSW wc{};
            wc.lpfnWndProc   = WndProc;
            wc.hInstance     = hInstance;
            wc.lpszClassName = kClassName;
            wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&wc);
            classRegistered = true;
        }

        if (g_hwnd) {
            DestroyWindow(g_hwnd);
            g_hwnd = nullptr;
        }

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int x = screenW - kWndW - kMargin;
        int y = kMargin;

        g_hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            kClassName, L"", WS_POPUP | WS_VISIBLE | WS_BORDER,
            x, y, kWndW, kWndH,
            nullptr, nullptr, hInstance, nullptr);

        if (g_hwnd) {
            // Keep this HUD out of screen captures/recordings (including our
            // own ffmpeg capture) - Windows 10 2004+. On older Windows this
            // call just fails harmlessly and the indicator stays visible to
            // capture as a fallback.
            SetWindowDisplayAffinity(g_hwnd, WDA_EXCLUDEFROMCAPTURE);

            UpdatePauseButtonLabel();
            SetTimer(g_hwnd, ID_TIMER, 1000, nullptr);
            InvalidateRect(g_hwnd, nullptr, TRUE);
        }
    }

    void Hide() {
        if (g_hwnd) {
            DestroyWindow(g_hwnd);
            g_hwnd = nullptr;
            g_btnPause = nullptr;
            g_btnStop = nullptr;
        }
    }

    void SetPaused(bool paused) {
        g_paused = paused;
        UpdatePauseButtonLabel();
        if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    bool IsVisible() {
        return g_hwnd != nullptr;
    }

} // namespace indicator
