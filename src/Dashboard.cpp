#include "Dashboard.h"
#include "Config.h"
#include "Language.h"
#include "resource.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <vector>

namespace dashboard {

    namespace {
        const wchar_t* kClass = L"ZimoRecorderDashboard";
        const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        const wchar_t* kRunValue = L"ZimoRecorder";
        const UINT_PTR kRefreshTimer = 1;

        // Brand colours
        const COLORREF kBg      = RGB(18, 24, 31);
        const COLORREF kHeader  = RGB(24, 32, 42);
        const COLORREF kCard    = RGB(31, 41, 54);
        const COLORREF kCardHot = RGB(42, 56, 73);
        const COLORREF kAccent  = RGB(229, 20, 44);
        const COLORREF kAccentDark = RGB(170, 14, 32);
        const COLORREF kText    = RGB(255, 255, 255);
        const COLORREF kSubText = RGB(150, 160, 172);

        enum class Kind { Button, Toggle, Link };
        struct Item { int action; Kind kind; RECT rc; };

        HINSTANCE        g_inst = nullptr;
        HWND             g_main = nullptr;
        HWND             g_wnd  = nullptr;
        RecordingStateFn g_state = nullptr;
        HICON            g_icon = nullptr;
        double           g_scale = 1.0;
        std::vector<Item> g_items;
        int  g_hover = -1, g_pressed = -1;
        bool g_tracking = false;

        int S(int v) { return (int)(v * g_scale + 0.5); }
        int ClientW() { return S(28) * 2 + S(240) * 2 + S(16); }
        int ClientH() { return S(520); }

        bool Rtl() { Lang l = lang::Current(); return l == Lang::FA || l == Lang::AR; }

        void BuildLayout() {
            g_items.clear();
            const int left = S(28), colW = S(240), gap = S(16), top = S(132), bh = S(76), rowGap = S(14);
            const int acts[6] = { DASH_REGION, DASH_FULL, DASH_RECORD, DASH_GIF, DASH_OCR, DASH_FOLDER };
            for (int i = 0; i < 6; ++i) {
                int col = i % 2, row = i / 2;
                RECT rc;
                rc.left = left + col * (colW + gap);
                rc.top = top + row * (bh + rowGap);
                rc.right = rc.left + colW;
                rc.bottom = rc.top + bh;
                g_items.push_back({ acts[i], Kind::Button, rc });
            }
            RECT t = { left, S(412), left + S(320), S(412) + S(28) };
            g_items.push_back({ DASH_STARTUP, Kind::Toggle, t });
            RECT l1 = { left, S(456), left + S(200), S(456) + S(26) };
            g_items.push_back({ DASH_LANG, Kind::Link, l1 });
            int right = ClientW() - left;
            RECT l2 = { right - S(260), S(456), right, S(456) + S(26) };
            g_items.push_back({ DASH_UPDATE, Kind::Link, l2 });
        }

        int HitTest(int x, int y) {
            POINT p = { x, y };
            for (size_t i = 0; i < g_items.size(); ++i) {
                if (PtInRect(&g_items[i].rc, p)) return (int)i;
            }
            return -1;
        }

        const wchar_t* LabelFor(int action) {
            bool mp4 = g_state && g_state(false);
            bool gif = g_state && g_state(true);
            switch (action) {
            case DASH_REGION:  return lang::Upd(UpdStr::DashRegion);
            case DASH_FULL:    return lang::Str(StrId::MenuCaptureFull);
            case DASH_RECORD:  return mp4 ? lang::Str(StrId::MenuRecordStop) : lang::Str(StrId::MenuRecordStart);
            case DASH_GIF:     return gif ? lang::Str(StrId::MenuGifStop) : lang::Str(StrId::MenuGifStart);
            case DASH_OCR:     return lang::Str(StrId::MenuOcr);
            case DASH_FOLDER:  return lang::Str(StrId::MenuOpenFolder);
            case DASH_STARTUP: return lang::Upd(UpdStr::DashStartup);
            case DASH_LANG:    return lang::Str(StrId::MenuLanguage);
            case DASH_UPDATE:  return lang::Upd(UpdStr::MenuCheck);
            }
            return L"";
        }

        const wchar_t* HintFor(int action) {
            switch (action) {
            case DASH_REGION: return HOTKEY_TEXT_SCREENSHOT;
            case DASH_RECORD: return HOTKEY_TEXT_RECORD;
            case DASH_GIF:    return HOTKEY_TEXT_GIF;
            case DASH_OCR:    return HOTKEY_TEXT_OCR;
            }
            return L"";
        }

        HFONT MakeFont(int px, int weight) {
            return CreateFontW(-S(px), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        }

        void FillSolid(HDC dc, const RECT& rc, COLORREF c) {
            HBRUSH b = CreateSolidBrush(c);
            FillRect(dc, &rc, b);
            DeleteObject(b);
        }

        void RoundBox(HDC dc, const RECT& rc, int radius, COLORREF c) {
            HBRUSH b = CreateSolidBrush(c);
            HGDIOBJ ob = SelectObject(dc, b);
            HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
            RoundRect(dc, rc.left, rc.top, rc.right + 1, rc.bottom + 1, radius, radius);
            SelectObject(dc, op);
            SelectObject(dc, ob);
            DeleteObject(b);
        }

        void DrawLabel(HDC dc, const wchar_t* text, RECT rc, HFONT font, COLORREF color, UINT flags) {
            HGDIOBJ old = SelectObject(dc, font);
            SetTextColor(dc, color);
            DrawTextW(dc, text, -1, &rc, flags | DT_NOPREFIX | DT_END_ELLIPSIS | (Rtl() ? DT_RTLREADING : 0));
            SelectObject(dc, old);
        }

        void Paint(HWND hwnd) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT cr;
            GetClientRect(hwnd, &cr);
            int W = cr.right, H = cr.bottom;

            HDC dc = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, W, H);
            HGDIOBJ oldBmp = SelectObject(dc, bmp);
            SetBkMode(dc, TRANSPARENT);

            FillSolid(dc, cr, kBg);

            // Header
            RECT hd = { 0, 0, W, S(108) };
            FillSolid(dc, hd, kHeader);
            RECT stripe = { 0, 0, W, S(3) };
            FillSolid(dc, stripe, kAccent);

            if (g_icon) DrawIconEx(dc, S(28), S(26), g_icon, S(56), S(56), 0, nullptr, DI_NORMAL);

            HFONT fTitle = MakeFont(26, FW_BOLD);
            HFONT fSub   = MakeFont(14, FW_NORMAL);
            HFONT fBtn   = MakeFont(15, FW_SEMIBOLD);
            HFONT fHint  = MakeFont(12, FW_NORMAL);
            HFONT fLink  = MakeFont(13, FW_SEMIBOLD);

            RECT rTitle = { S(100), S(22), W - S(90), S(60) };
            DrawLabel(dc, APP_NAME, rTitle, fTitle, kText, DT_SINGLELINE | DT_LEFT);
            RECT rSub = { S(100), S(62), W - S(28), S(86) };
            DrawLabel(dc, lang::Upd(UpdStr::DashTagline), rSub, fSub, kSubText, DT_SINGLELINE | DT_LEFT);
            RECT rVer = { W - S(110), S(26), W - S(24), S(46) };
            DrawLabel(dc, (std::wstring(L"v") + APP_VERSION).c_str(), rVer, fHint, kSubText, DT_SINGLELINE | DT_RIGHT);

            // Items
            for (size_t i = 0; i < g_items.size(); ++i) {
                const Item& it = g_items[i];
                bool hot = ((int)i == g_hover);
                bool down = hot && ((int)i == g_pressed);

                if (it.kind == Kind::Button) {
                    COLORREF fill = down ? kAccentDark : (hot ? kCardHot : kCard);
                    RoundBox(dc, it.rc, S(14), fill);
                    RECT bar = { it.rc.left, it.rc.top + S(18), it.rc.left + S(4), it.rc.bottom - S(18) };
                    FillSolid(dc, bar, kAccent);

                    RECT r1 = { it.rc.left + S(20), it.rc.top + S(14), it.rc.right - S(14), it.rc.top + S(42) };
                    DrawLabel(dc, LabelFor(it.action), r1, fBtn, kText, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
                    RECT r2 = { it.rc.left + S(20), it.rc.top + S(44), it.rc.right - S(14), it.rc.bottom - S(10) };
                    DrawLabel(dc, HintFor(it.action), r2, fHint, kSubText, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
                }
                else if (it.kind == Kind::Toggle) {
                    bool on = IsAutostartEnabled();
                    RECT sw = { it.rc.left, it.rc.top + S(3), it.rc.left + S(42), it.rc.top + S(25) };
                    RoundBox(dc, sw, S(22), on ? kAccent : RGB(62, 74, 90));
                    int knob = S(16);
                    int kx = on ? (sw.right - knob - S(3)) : (sw.left + S(3));
                    RECT kn = { kx, sw.top + S(3), kx + knob, sw.top + S(3) + knob };
                    RoundBox(dc, kn, knob, RGB(255, 255, 255));
                    RECT tx = { it.rc.left + S(54), it.rc.top, it.rc.right, it.rc.bottom };
                    DrawLabel(dc, LabelFor(it.action), tx, fLink, hot ? kText : RGB(205, 212, 220), DT_SINGLELINE | DT_LEFT | DT_VCENTER);
                }
                else { // Link
                    bool right = (it.action == DASH_UPDATE);
                    DrawLabel(dc, LabelFor(it.action), it.rc, fLink, hot ? kAccent : kSubText,
                              DT_SINGLELINE | DT_VCENTER | (right ? DT_RIGHT : DT_LEFT));
                }
            }

            RECT line = { S(28), S(446), W - S(28), S(446) + 1 };
            FillSolid(dc, line, RGB(40, 52, 66));

            RECT foot = { S(28), S(490), W - S(28), H - S(8) };
            DrawLabel(dc, lang::Upd(UpdStr::DashHint), foot, fHint, kSubText, DT_CENTER | DT_WORDBREAK);

            DeleteObject(fTitle); DeleteObject(fSub); DeleteObject(fBtn); DeleteObject(fHint); DeleteObject(fLink);

            BitBlt(hdc, 0, 0, W, H, dc, 0, 0, SRCCOPY);
            SelectObject(dc, oldBmp);
            DeleteObject(bmp);
            DeleteDC(dc);
            EndPaint(hwnd, &ps);
        }

        void Activate(int index) {
            if (index < 0 || index >= (int)g_items.size()) return;
            const Item& it = g_items[index];
            if (it.kind == Kind::Toggle) {
                SetAutostart(!IsAutostartEnabled());
                InvalidateRect(g_wnd, nullptr, FALSE);
                return;
            }
            // Capture actions need this window out of the way first
            if (it.action == DASH_REGION || it.action == DASH_FULL || it.action == DASH_RECORD ||
                it.action == DASH_GIF || it.action == DASH_OCR) {
                ShowWindow(g_wnd, SW_HIDE);
            }
            PostMessageW(g_main, WM_DASH_ACTION, (WPARAM)it.action, 0);
        }

        LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            switch (msg) {
            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT:
                Paint(hwnd);
                return 0;

            case WM_MOUSEMOVE: {
                int idx = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                if (idx != g_hover) {
                    g_hover = idx;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (!g_tracking) {
                    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                    TrackMouseEvent(&tme);
                    g_tracking = true;
                }
                return 0;
            }

            case WM_MOUSELEAVE:
                g_tracking = false;
                g_hover = -1;
                g_pressed = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;

            case WM_SETCURSOR:
                if (g_hover >= 0 && LOWORD(lParam) == HTCLIENT) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                break;

            case WM_LBUTTONDOWN:
                g_pressed = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;

            case WM_LBUTTONUP: {
                int idx = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                int pressed = g_pressed;
                g_pressed = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
                if (idx >= 0 && idx == pressed) Activate(idx);
                return 0;
            }

            case WM_TIMER:
                if (wParam == kRefreshTimer && IsWindowVisible(hwnd)) InvalidateRect(hwnd, nullptr, FALSE);
                return 0;

            case WM_CLOSE:
                ShowWindow(hwnd, SW_HIDE);   // the app keeps running in the tray
                return 0;

            case WM_DESTROY:
                KillTimer(hwnd, kRefreshTimer);
                if (g_icon) { DestroyIcon(g_icon); g_icon = nullptr; }
                g_wnd = nullptr;
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    } // namespace

    bool IsAutostartEnabled() {
        return RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    }

    void SetAutostart(bool enable) {
        if (!enable) {
            HKEY key;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                RegDeleteValueW(key, kRunValue);
                RegCloseKey(key);
            }
            return;
        }
        wchar_t self[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, self, MAX_PATH)) return;
        std::wstring cmd = std::wstring(L"\"") + self + L"\" --background";
        RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, REG_SZ, cmd.c_str(),
                        (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    }

    void MigrateLegacyAutostart() {
        const wchar_t* legacyName = L"ZimoRecordr";
        if (RegGetValueW(HKEY_CURRENT_USER, kRunKey, legacyName, RRF_RT_REG_SZ, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, legacyName);
            SetAutostart(true);
        }
    }

    void Refresh() {
        if (g_wnd && IsWindowVisible(g_wnd)) InvalidateRect(g_wnd, nullptr, FALSE);
    }

    void Show(HINSTANCE hInstance, HWND mainWindow, RecordingStateFn state) {
        g_inst = hInstance;
        g_main = mainWindow;
        g_state = state;

        if (!g_wnd) {
            HDC sdc = GetDC(nullptr);
            g_scale = GetDeviceCaps(sdc, LOGPIXELSX) / 96.0;
            ReleaseDC(nullptr, sdc);

            static bool registered = false;
            if (!registered) {
                WNDCLASSW wc{};
                wc.lpfnWndProc   = WndProc;
                wc.hInstance     = hInstance;
                wc.lpszClassName = kClass;
                wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
                wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
                wc.hbrBackground = nullptr;
                RegisterClassW(&wc);
                registered = true;
            }

            DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            RECT want = { 0, 0, ClientW(), ClientH() };
            AdjustWindowRectEx(&want, style, FALSE, 0);
            int w = want.right - want.left, h = want.bottom - want.top;

            RECT wa{};
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
            int x = wa.left + ((wa.right - wa.left) - w) / 2;
            int y = wa.top + ((wa.bottom - wa.top) - h) / 2;

            g_icon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, S(56), S(56), LR_DEFAULTCOLOR);
            BuildLayout();

            g_wnd = CreateWindowExW(0, kClass, APP_NAME, style, x, y, w, h, nullptr, nullptr, hInstance, nullptr);
            if (!g_wnd) return;

            // Dark title bar where Windows supports it (best effort)
            BOOL dark = TRUE;
            DwmSetWindowAttribute(g_wnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
            COLORREF cap = kHeader;
            DwmSetWindowAttribute(g_wnd, 35 /*DWMWA_CAPTION_COLOR*/, &cap, sizeof(cap));

            SetTimer(g_wnd, kRefreshTimer, 600, nullptr);
        }

        if (IsIconic(g_wnd)) ShowWindow(g_wnd, SW_RESTORE);
        ShowWindow(g_wnd, SW_SHOW);
        SetForegroundWindow(g_wnd);
        InvalidateRect(g_wnd, nullptr, FALSE);
    }

} // namespace dashboard
