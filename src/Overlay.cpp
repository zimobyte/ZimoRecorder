// =============================================================
//  Overlay.cpp - region / window picker
//
//  How it works for the person using the app:
//   - Move the mouse: the window under the cursor gets a highlighted
//     frame. Click once to pick that whole window.
//   - Or press, drag and release to draw your own region.
//   - Right-click while dragging: cancels the drag and lets you start
//     again. Right-click (or Esc) with no drag: leaves the picker.
// =============================================================
#include "Overlay.h"
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM
#include <dwmapi.h>
#include <vector>
#include <cstdio>
#include <cstdlib>

namespace overlay {

    namespace {
        const wchar_t* kClassName = L"ZimoRecorderOverlayWnd";

        const BYTE     kOverlayAlpha = 130;
        const COLORREF kAccent       = RGB(229, 20, 44);
        const COLORREF kSelectionFill = RGB(110, 110, 110);

        // Virtual screen (all monitors together)
        int g_vLeft = 0, g_vTop = 0, g_vW = 0, g_vH = 0;

        // Visible top-level windows, front-most first (screen coordinates)
        std::vector<RECT> g_windows;

        bool  g_dragging  = false;
        bool  g_ok        = false;   // a selection was made
        bool  g_cancelled = false;
        POINT g_start{ 0, 0 };       // client coordinates
        POINT g_cur{ 0, 0 };         // client coordinates
        bool  g_hasHover  = false;
        RECT  g_hover{};             // screen coordinates
        RECT  g_result{};            // screen coordinates

        HDC     g_memDC  = nullptr;
        HBITMAP g_memBmp = nullptr;
        HGDIOBJ g_memOld = nullptr;

        void ResetState() {
            g_dragging = false;
            g_ok = false;
            g_cancelled = false;
            g_start = { 0, 0 };
            g_cur = { 0, 0 };
            g_hasHover = false;
            g_hover = {};
            g_result = {};
        }

        void FreeBackBuffer() {
            if (g_memDC) {
                if (g_memOld) SelectObject(g_memDC, g_memOld);
                if (g_memBmp) DeleteObject(g_memBmp);
                DeleteDC(g_memDC);
            }
            g_memDC = nullptr; g_memBmp = nullptr; g_memOld = nullptr;
        }

        // ---------------------------------------------------------
        // Window list (for "click to pick a window")
        // ---------------------------------------------------------
        BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM) {
            if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) return TRUE;

            // Skip windows Windows keeps "cloaked" (hidden UWP apps, other desktops)
            DWORD cloaked = 0;
            if (SUCCEEDED(DwmGetWindowAttribute(hwnd, 14 /*DWMWA_CLOAKED*/, &cloaked, sizeof(cloaked))) && cloaked) {
                return TRUE;
            }

            // The exact visible frame (without the invisible resize border)
            RECT rc{};
            if (FAILED(DwmGetWindowAttribute(hwnd, 9 /*DWMWA_EXTENDED_FRAME_BOUNDS*/, &rc, sizeof(rc)))) {
                if (!GetWindowRect(hwnd, &rc)) return TRUE;
            }

            // Keep it inside the screen area
            if (rc.left   < g_vLeft)          rc.left   = g_vLeft;
            if (rc.top    < g_vTop)           rc.top    = g_vTop;
            if (rc.right  > g_vLeft + g_vW)   rc.right  = g_vLeft + g_vW;
            if (rc.bottom > g_vTop  + g_vH)   rc.bottom = g_vTop  + g_vH;

            if (rc.right - rc.left < 20 || rc.bottom - rc.top < 20) return TRUE;
            g_windows.push_back(rc);
            return TRUE;
        }

        bool FindWindowAt(POINT screenPt, RECT& out) {
            for (const RECT& r : g_windows) {
                if (PtInRect(&r, screenPt)) { out = r; return true; }
            }
            return false;
        }

        void UpdateHover(HWND hwnd, POINT clientPt) {
            POINT sp{ clientPt.x + g_vLeft, clientPt.y + g_vTop };
            RECT r{};
            bool found = FindWindowAt(sp, r);
            bool changed = (found != g_hasHover) ||
                           (found && !EqualRect(&r, &g_hover));
            g_hasHover = found;
            if (found) g_hover = r;
            if (changed) InvalidateRect(hwnd, nullptr, FALSE);
        }

        bool Moved() {
            return abs(g_cur.x - g_start.x) >= 5 || abs(g_cur.y - g_start.y) >= 5;
        }

        // What is currently highlighted, in client coordinates
        bool CurrentSelection(RECT& out) {
            if (g_dragging && Moved()) {
                out.left   = (g_start.x < g_cur.x) ? g_start.x : g_cur.x;
                out.right  = (g_start.x > g_cur.x) ? g_start.x : g_cur.x;
                out.top    = (g_start.y < g_cur.y) ? g_start.y : g_cur.y;
                out.bottom = (g_start.y > g_cur.y) ? g_start.y : g_cur.y;
                return true;
            }
            if (g_hasHover) {
                out = g_hover;
                OffsetRect(&out, -g_vLeft, -g_vTop);
                return true;
            }
            return false;
        }

        void Paint(HWND hwnd) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            if (!g_memDC) {
                g_memDC  = CreateCompatibleDC(hdc);
                g_memBmp = CreateCompatibleBitmap(hdc, g_vW, g_vH);
                g_memOld = SelectObject(g_memDC, g_memBmp);
            }

            // 1) dark background over everything
            RECT all{ 0, 0, g_vW, g_vH };
            HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(g_memDC, &all, black);
            DeleteObject(black);

            // 2) highlighted window / region + frame + size label
            RECT sel{};
            if (CurrentSelection(sel)) {
                HBRUSH fill = CreateSolidBrush(kSelectionFill);
                FillRect(g_memDC, &sel, fill);
                DeleteObject(fill);

                HPEN pen = CreatePen(PS_SOLID, 3, kAccent);
                HGDIOBJ oldPen = SelectObject(g_memDC, pen);
                HGDIOBJ oldBrush = SelectObject(g_memDC, GetStockObject(NULL_BRUSH));
                Rectangle(g_memDC, sel.left, sel.top, sel.right, sel.bottom);
                SelectObject(g_memDC, oldBrush);
                SelectObject(g_memDC, oldPen);
                DeleteObject(pen);

                wchar_t label[48];
                swprintf_s(label, L"%d x %d", (int)(sel.right - sel.left), (int)(sel.bottom - sel.top));
                HFONT font = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                HGDIOBJ oldFont = SelectObject(g_memDC, font);
                SIZE ts{};
                GetTextExtentPoint32W(g_memDC, label, (int)wcslen(label), &ts);

                int bw = ts.cx + 16, bh = ts.cy + 6;
                int bx = sel.left;
                int by = sel.top - bh - 2;
                if (by < 0) by = sel.top + 4;          // not enough room above: put it inside
                if (bx + bw > g_vW) bx = g_vW - bw;
                if (bx < 0) bx = 0;
                RECT box{ bx, by, bx + bw, by + bh };
                HBRUSH boxBrush = CreateSolidBrush(kAccent);
                FillRect(g_memDC, &box, boxBrush);
                DeleteObject(boxBrush);

                SetBkMode(g_memDC, TRANSPARENT);
                SetTextColor(g_memDC, RGB(255, 255, 255));
                TextOutW(g_memDC, bx + 8, by + 3, label, (int)wcslen(label));
                SelectObject(g_memDC, oldFont);
                DeleteObject(font);
            }

            BitBlt(hdc, 0, 0, g_vW, g_vH, g_memDC, 0, 0, SRCCOPY);
            EndPaint(hwnd, &ps);
        }

        LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            switch (msg) {
            case WM_ERASEBKGND:
                return 1; // we paint everything ourselves (no flicker)

            case WM_LBUTTONDOWN:
                g_dragging = true;
                g_start.x = GET_X_LPARAM(lParam);
                g_start.y = GET_Y_LPARAM(lParam);
                g_cur = g_start;
                SetCapture(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;

            case WM_MOUSEMOVE: {
                POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (g_dragging) {
                    g_cur = p;
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else {
                    UpdateHover(hwnd, p);
                }
                return 0;
            }

            case WM_LBUTTONUP:
                if (g_dragging) {
                    g_cur.x = GET_X_LPARAM(lParam);
                    g_cur.y = GET_Y_LPARAM(lParam);
                    g_dragging = false;
                    ReleaseCapture();

                    if (Moved()) {
                        // Drawn region
                        RECT r;
                        r.left   = g_vLeft + ((g_start.x < g_cur.x) ? g_start.x : g_cur.x);
                        r.right  = g_vLeft + ((g_start.x > g_cur.x) ? g_start.x : g_cur.x);
                        r.top    = g_vTop  + ((g_start.y < g_cur.y) ? g_start.y : g_cur.y);
                        r.bottom = g_vTop  + ((g_start.y > g_cur.y) ? g_start.y : g_cur.y);
                        g_result = r;
                        g_ok = true;
                    } else if (g_hasHover) {
                        // A plain click: take the highlighted window
                        g_result = g_hover;
                        g_ok = true;
                    }
                    DestroyWindow(hwnd);
                }
                return 0;

            case WM_RBUTTONDOWN:
                if (g_dragging) {
                    // Right-click while dragging: throw the drag away, stay in the picker
                    g_dragging = false;
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else {
                    g_cancelled = true;
                    DestroyWindow(hwnd);
                }
                return 0;

            case WM_KEYDOWN:
                if (wParam == VK_ESCAPE) {
                    g_cancelled = true;
                    DestroyWindow(hwnd);
                }
                return 0;

            case WM_PAINT:
                Paint(hwnd);
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    } // namespace

    bool RunRegionSelector(HINSTANCE hInstance, RECT& outRect) {
        ResetState();

        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSW wc{};
            wc.lpfnWndProc   = WndProc;
            wc.hInstance     = hInstance;
            wc.hCursor       = LoadCursor(nullptr, IDC_CROSS);
            wc.hbrBackground = nullptr;
            wc.lpszClassName = kClassName;
            RegisterClassW(&wc);
            classRegistered = true;
        }

        g_vLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        g_vTop  = GetSystemMetrics(SM_YVIRTUALSCREEN);
        g_vW    = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        g_vH    = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // List the windows BEFORE our own overlay exists, so it isn't in the list
        g_windows.clear();
        EnumWindows(EnumWindowsProc, 0);

        HWND hwnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            kClassName, L"", WS_POPUP | WS_VISIBLE,
            g_vLeft, g_vTop, g_vW, g_vH,
            nullptr, nullptr, hInstance, nullptr);

        if (!hwnd) return false;

        SetLayeredWindowAttributes(hwnd, 0, kOverlayAlpha, LWA_ALPHA);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);

        // Highlight whatever is already under the cursor
        POINT cp;
        if (GetCursorPos(&cp)) {
            UpdateHover(hwnd, POINT{ cp.x - g_vLeft, cp.y - g_vTop });
        }
        InvalidateRect(hwnd, nullptr, FALSE);

        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        FreeBackBuffer();

        if (g_cancelled || !g_ok) return false;

        if (g_result.right - g_result.left < 2 || g_result.bottom - g_result.top < 2) return false;

        // Give Windows a moment to fully remove the dark overlay from the
        // screen, so it never ends up inside the screenshot / recording.
        Sleep(150);

        outRect = g_result;
        return true;
    }

} // namespace overlay
