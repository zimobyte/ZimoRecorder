#include "Toast.h"
#include "Language.h"
#include <shellapi.h>

namespace toast {

    namespace {
        const wchar_t* kClass = L"ZimoRecorderToastWnd";
        const UINT_PTR kTimer = 1;
        const COLORREF kBg     = RGB(21, 28, 36);
        const COLORREF kAccent = RGB(229, 20, 44);

        HWND         g_toast = nullptr;
        std::wstring g_title, g_text, g_path;
        double       g_scale = 1.0;

        int S(int v) { return (int)(v * g_scale + 0.5); }

        void OpenSavedLocation() {
            if (g_path.empty()) return;
            std::wstring args = L"/select,\"" + g_path + L"\"";
            ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
        }

        LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            switch (msg) {
            case WM_MOUSEACTIVATE:
                return MA_NOACTIVATE;   // never steal focus from what the person is doing

            case WM_SETCURSOR:
                SetCursor(LoadCursor(nullptr, g_path.empty() ? IDC_ARROW : IDC_HAND));
                return TRUE;

            case WM_LBUTTONUP:
                OpenSavedLocation();
                DestroyWindow(hwnd);
                return 0;

            case WM_TIMER:
                DestroyWindow(hwnd);
                return 0;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc;
                GetClientRect(hwnd, &rc);

                HBRUSH bg = CreateSolidBrush(kBg);
                FillRect(hdc, &rc, bg);
                DeleteObject(bg);

                RECT bar = rc;
                bar.right = bar.left + S(6);
                HBRUSH ac = CreateSolidBrush(kAccent);
                FillRect(hdc, &bar, ac);
                DeleteObject(ac);

                Lang cur = lang::Current();
                bool rtl = (cur == Lang::FA || cur == Lang::AR);
                UINT dir = rtl ? (DT_RTLREADING | DT_RIGHT) : DT_LEFT;

                SetBkMode(hdc, TRANSPARENT);

                HFONT fTitle = CreateFontW(-S(15), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                HGDIOBJ old = SelectObject(hdc, fTitle);
                SetTextColor(hdc, RGB(255, 255, 255));
                RECT tr = { S(20), S(10), rc.right - S(14), S(34) };
                DrawTextW(hdc, g_title.c_str(), -1, &tr, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | dir);

                HFONT fBody = CreateFontW(-S(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                SelectObject(hdc, fBody);
                SetTextColor(hdc, RGB(200, 205, 212));
                RECT br = { S(20), S(36), rc.right - S(14), rc.bottom - S(8) };
                DrawTextW(hdc, g_text.c_str(), -1, &br, DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX | dir);

                SelectObject(hdc, old);
                DeleteObject(fTitle);
                DeleteObject(fBody);
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_DESTROY:
                KillTimer(hwnd, kTimer);
                if (hwnd == g_toast) g_toast = nullptr;
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    } // namespace

    void Show(HINSTANCE hInstance, const std::wstring& title,
              const std::wstring& text, const std::wstring& openPath) {
        if (g_toast) { DestroyWindow(g_toast); g_toast = nullptr; }

        static bool registered = false;
        if (!registered) {
            WNDCLASSW wc{};
            wc.lpfnWndProc   = WndProc;
            wc.hInstance     = hInstance;
            wc.lpszClassName = kClass;
            RegisterClassW(&wc);
            registered = true;
        }

        g_title = title;
        g_text  = text;
        g_path  = openPath;

        HDC sdc = GetDC(nullptr);
        g_scale = GetDeviceCaps(sdc, LOGPIXELSX) / 96.0;
        ReleaseDC(nullptr, sdc);

        RECT wa{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        int w = S(380), h = S(88), margin = S(16);

        g_toast = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
            kClass, L"", WS_POPUP,
            wa.right - w - margin, wa.bottom - h - margin, w, h,
            nullptr, nullptr, hInstance, nullptr);
        if (!g_toast) return;

        SetLayeredWindowAttributes(g_toast, 0, 245, LWA_ALPHA);
        HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, S(14), S(14));
        SetWindowRgn(g_toast, rgn, TRUE);   // the window now owns the region
        ShowWindow(g_toast, SW_SHOWNOACTIVATE);
        SetTimer(g_toast, kTimer, 6000, nullptr);
    }

} // namespace toast
