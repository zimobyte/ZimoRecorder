#include "Editor.h"
#include "Language.h"
#include "Config.h"
#include "ScreenCapture.h"
#include "resource.h"
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM
#include <vector>
#include <cmath>
#include <cstdlib>      // abs
#include <cwchar>       // wcslen

namespace editor {

    namespace {
        const wchar_t* kClassName = L"ZimoRecorderEditorWnd";

        const int kToolbarH = 54;
        const COLORREF kAccentRed = RGB(229, 20, 44);
        const COLORREF kToolbarBg = RGB(21, 28, 36);   // brand dark navy
        const COLORREF kCanvasBg  = RGB(30, 30, 32);

        // Tool-icon handles, loaded once and kept for the app's lifetime
        // (small, cheap - same convention as the app's own tray icons).
        HICON g_iconLine = nullptr, g_iconArrow = nullptr, g_iconText = nullptr,
              g_iconBlur = nullptr, g_iconUndo = nullptr;

        void LoadToolIconsOnce(HINSTANCE hInstance) {
            if (g_iconLine) return; // already loaded
            auto load = [&](int resId) {
                return (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(resId),
                                          IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
            };
            g_iconLine  = load(IDI_TOOL_LINE);
            g_iconArrow = load(IDI_TOOL_ARROW);
            g_iconText  = load(IDI_TOOL_TEXT);
            g_iconBlur  = load(IDI_TOOL_BLUR);
            g_iconUndo  = load(IDI_TOOL_UNDO);
        }

        enum {
            ID_TOOL_LINE = 201, ID_TOOL_ARROW, ID_TOOL_TEXT, ID_TOOL_BLUR,
            ID_UNDO, ID_COPY, ID_SAVE, ID_CANCEL, ID_TEXT_EDIT
        };

        enum class Tool { Line, Arrow, Text, Blur };

        HWND   g_hwnd = nullptr;
        HWND   g_editHwnd = nullptr;
        HBITMAP g_bitmap = nullptr;
        int    g_bitmapW = 0, g_bitmapH = 0;
        double g_scale = 1.0;
        int    g_canvasW = 0, g_canvasH = 0;

        Tool   g_tool = Tool::Arrow;
        bool   g_dragging = false;
        POINT  g_dragStart{}, g_dragCur{};
        POINT  g_textAnchorClient{};

        std::wstring g_savePath;
        HWND   g_ownerForClipboard = nullptr;
        Result g_result = Result::Cancelled;

        std::vector<HBITMAP> g_undoStack;

        // ---------------------------------------------------------
        // Small helpers
        // ---------------------------------------------------------
        RECT NormalizedRect(POINT a, POINT b) {
            RECT r;
            r.left   = (a.x < b.x) ? a.x : b.x;
            r.right  = (a.x > b.x) ? a.x : b.x;
            r.top    = (a.y < b.y) ? a.y : b.y;
            r.bottom = (a.y > b.y) ? a.y : b.y;
            return r;
        }

        POINT ClientToBitmap(POINT client) {
            POINT p;
            p.x = (LONG)(client.x / g_scale);
            p.y = (LONG)((client.y - kToolbarH) / g_scale);
            if (p.x < 0) p.x = 0;
            if (p.y < 0) p.y = 0;
            if (p.x >= g_bitmapW) p.x = g_bitmapW - 1;
            if (p.y >= g_bitmapH) p.y = g_bitmapH - 1;
            return p;
        }

        HBITMAP DuplicateBitmap(HBITMAP src) {
            BITMAP bm{};
            GetObjectW(src, sizeof(bm), &bm);
            HDC screen = GetDC(nullptr);
            HDC s = CreateCompatibleDC(screen);
            HDC d = CreateCompatibleDC(screen);
            HBITMAP dst = CreateCompatibleBitmap(screen, bm.bmWidth, bm.bmHeight);
            HGDIOBJ os = SelectObject(s, src);
            HGDIOBJ od = SelectObject(d, dst);
            BitBlt(d, 0, 0, bm.bmWidth, bm.bmHeight, s, 0, 0, SRCCOPY);
            SelectObject(s, os);
            SelectObject(d, od);
            DeleteDC(s);
            DeleteDC(d);
            ReleaseDC(nullptr, screen);
            return dst;
        }

        void PushUndo() {
            g_undoStack.push_back(DuplicateBitmap(g_bitmap));
            if (g_undoStack.size() > 20) {
                DeleteObject(g_undoStack.front());
                g_undoStack.erase(g_undoStack.begin());
            }
        }

        void DoUndo() {
            if (g_undoStack.empty()) return;
            DeleteObject(g_bitmap);
            g_bitmap = g_undoStack.back();
            g_undoStack.pop_back();
            InvalidateRect(g_hwnd, nullptr, FALSE);
        }

        // Fast blocky pixelation using two StretchBlts (shrink then re-grow)
        void PixelateRegion(HDC targetDC, RECT rect, int blockSize) {
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            if (w <= 1 || h <= 1) return;

            int smallW = w / blockSize; if (smallW < 1) smallW = 1;
            int smallH = h / blockSize; if (smallH < 1) smallH = 1;

            HDC tmp = CreateCompatibleDC(targetDC);
            HBITMAP tmpBmp = CreateCompatibleBitmap(targetDC, smallW, smallH);
            HGDIOBJ old = SelectObject(tmp, tmpBmp);

            SetStretchBltMode(tmp, HALFTONE);
            StretchBlt(tmp, 0, 0, smallW, smallH, targetDC, rect.left, rect.top, w, h, SRCCOPY);

            SetStretchBltMode(targetDC, COLORONCOLOR); // no smoothing -> blocky look
            StretchBlt(targetDC, rect.left, rect.top, w, h, tmp, 0, 0, smallW, smallH, SRCCOPY);

            SelectObject(tmp, old);
            DeleteObject(tmpBmp);
            DeleteDC(tmp);
        }

        void DrawArrowOnDC(HDC hdc, POINT from, POINT to, COLORREF color, int thickness) {
            HPEN pen = CreatePen(PS_SOLID, thickness, color);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            MoveToEx(hdc, from.x, from.y, nullptr);
            LineTo(hdc, to.x, to.y);

            double angle = atan2((double)(to.y - from.y), (double)(to.x - from.x));
            double headLen = 14 + thickness * 2.0;
            double headAngle = 0.5;

            POINT p1, p2;
            p1.x = (LONG)(to.x - headLen * cos(angle - headAngle));
            p1.y = (LONG)(to.y - headLen * sin(angle - headAngle));
            p2.x = (LONG)(to.x - headLen * cos(angle + headAngle));
            p2.y = (LONG)(to.y - headLen * sin(angle + headAngle));

            POINT headPts[3] = { to, p1, p2 };
            HBRUSH brush = CreateSolidBrush(color);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            Polygon(hdc, headPts, 3);
            SelectObject(hdc, oldBrush);
            DeleteObject(brush);

            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        void DrawTextOnBitmap(const std::wstring& text, POINT bitmapPos) {
            HDC hdc = CreateCompatibleDC(nullptr);
            HGDIOBJ old = SelectObject(hdc, g_bitmap);
            HFONT font = CreateFontW(-28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            HGDIOBJ oldFont = SelectObject(hdc, font);
            SetTextColor(hdc, kAccentRed);
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, bitmapPos.x, bitmapPos.y, text.c_str(), (int)text.size());
            SelectObject(hdc, oldFont);
            DeleteObject(font);
            SelectObject(hdc, old);
            DeleteDC(hdc);
        }

        void CommitTextEdit() {
            if (!g_editHwnd) return;
            wchar_t buf[256]{};
            GetWindowTextW(g_editHwnd, buf, 256);
            HWND edit = g_editHwnd;
            g_editHwnd = nullptr;
            DestroyWindow(edit);

            if (wcslen(buf) > 0) {
                POINT bmpPos = ClientToBitmap(g_textAnchorClient);
                PushUndo();
                DrawTextOnBitmap(buf, bmpPos);
                InvalidateRect(g_hwnd, nullptr, FALSE);
            }
        }

        void BeginTextEntry(HWND hwnd, POINT clientPt) {
            g_textAnchorClient = clientPt;
            g_editHwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                clientPt.x, clientPt.y, 220, 26,
                hwnd, (HMENU)(UINT_PTR)ID_TEXT_EDIT,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);
            if (g_editHwnd) SetFocus(g_editHwnd);
        }

        void CommitShape(POINT startClient, POINT endClient) {
            int dx = endClient.x - startClient.x;
            int dy = endClient.y - startClient.y;
            if (abs(dx) < 3 && abs(dy) < 3) return; // essentially a click, not a drag

            POINT startBmp = ClientToBitmap(startClient);
            POINT endBmp   = ClientToBitmap(endClient);

            PushUndo();
            HDC hdc = CreateCompatibleDC(nullptr);
            HGDIOBJ old = SelectObject(hdc, g_bitmap);

            switch (g_tool) {
            case Tool::Line: {
                HPEN pen = CreatePen(PS_SOLID, 4, kAccentRed);
                HGDIOBJ oldPen = SelectObject(hdc, pen);
                MoveToEx(hdc, startBmp.x, startBmp.y, nullptr);
                LineTo(hdc, endBmp.x, endBmp.y);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
                break;
            }
            case Tool::Arrow:
                DrawArrowOnDC(hdc, startBmp, endBmp, kAccentRed, 4);
                break;
            case Tool::Blur: {
                RECT r = NormalizedRect(startBmp, endBmp);
                PixelateRegion(hdc, r, 12);
                break;
            }
            default:
                break;
            }

            SelectObject(hdc, old);
            DeleteDC(hdc);
        }

        void CreateToolbarButtons(HWND hwnd, HINSTANCE hInstance) {
            LoadToolIconsOnce(hInstance);

            // Left side: icon-only tool buttons (matches the convention
            // professional screenshot editors use for their tool palette)
            const int iw = 40, ih = 40, igap = 6;
            int x = 10, y = 7;

            // The icon is what's drawn, but each button still gets its
            // real (localized) name as its window text - that text isn't
            // drawn (BS_ICON), but it's what screen readers announce and
            // what a native tooltip would show, so these icon-only
            // buttons stay properly labeled, not just pictures.
            auto addIconBtn = [&](int id, HICON icon, const wchar_t* label) {
                HWND b = CreateWindowW(L"BUTTON", label,
                              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON,
                              x, y, iw, ih, hwnd, (HMENU)(UINT_PTR)id, hInstance, nullptr);
                if (b) SendMessageW(b, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)icon);
                x += iw + igap;
            };
            addIconBtn(ID_TOOL_LINE,  g_iconLine,  lang::Str(StrId::EditorToolLine));
            addIconBtn(ID_TOOL_ARROW, g_iconArrow, lang::Str(StrId::EditorToolArrow));
            addIconBtn(ID_TOOL_TEXT,  g_iconText,  lang::Str(StrId::EditorToolText));
            addIconBtn(ID_TOOL_BLUR,  g_iconBlur,  lang::Str(StrId::EditorToolBlur));
            x += 12;
            addIconBtn(ID_UNDO, g_iconUndo, lang::Str(StrId::EditorUndo));

            // Right side: clear text buttons for the critical finishing actions
            const int w = 76, h = 30;
            int ty = (kToolbarH - h) / 2;
            int rx = g_canvasW - 10;
            auto addBtnRight = [&](int id, const wchar_t* label) {
                rx -= w;
                CreateWindowW(L"BUTTON", label, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              rx, ty, w, h, hwnd, (HMENU)(UINT_PTR)id, hInstance, nullptr);
                rx -= 6;
            };
            addBtnRight(ID_CANCEL, lang::Str(StrId::EditorCancel));
            addBtnRight(ID_SAVE,   lang::Str(StrId::EditorSave));
            addBtnRight(ID_COPY,   lang::Str(StrId::EditorCopy));
        }

        void PaintWindow(HWND hwnd) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT client;
            GetClientRect(hwnd, &client);

            RECT toolbarRect = { 0, 0, client.right, kToolbarH };
            HBRUSH toolbarBrush = CreateSolidBrush(kToolbarBg);
            FillRect(hdc, &toolbarRect, toolbarBrush);
            DeleteObject(toolbarBrush);

            HDC memDC = CreateCompatibleDC(hdc);
            HGDIOBJ old = SelectObject(memDC, g_bitmap);
            if (g_scale >= 0.999) {
                BitBlt(hdc, 0, kToolbarH, g_canvasW, g_canvasH, memDC, 0, 0, SRCCOPY);
            } else {
                SetStretchBltMode(hdc, HALFTONE);
                StretchBlt(hdc, 0, kToolbarH, g_canvasW, g_canvasH,
                           memDC, 0, 0, g_bitmapW, g_bitmapH, SRCCOPY);
            }
            SelectObject(memDC, old);
            DeleteDC(memDC);

            if (g_dragging) {
                if (g_tool == Tool::Blur) {
                    HPEN pen = CreatePen(PS_DASH, 2, kAccentRed);
                    HGDIOBJ oldPen = SelectObject(hdc, pen);
                    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    Rectangle(hdc, g_dragStart.x, g_dragStart.y, g_dragCur.x, g_dragCur.y);
                    SelectObject(hdc, oldPen);
                    SelectObject(hdc, oldBrush);
                    DeleteObject(pen);
                } else if (g_tool == Tool::Arrow) {
                    DrawArrowOnDC(hdc, g_dragStart, g_dragCur, kAccentRed, 3);
                } else if (g_tool == Tool::Line) {
                    HPEN pen = CreatePen(PS_SOLID, 3, kAccentRed);
                    HGDIOBJ oldPen = SelectObject(hdc, pen);
                    MoveToEx(hdc, g_dragStart.x, g_dragStart.y, nullptr);
                    LineTo(hdc, g_dragCur.x, g_dragCur.y);
                    SelectObject(hdc, oldPen);
                    DeleteObject(pen);
                }
            }

            EndPaint(hwnd, &ps);
        }

        void HandleCommand(HWND hwnd, int id, int notify) {
            switch (id) {
            case ID_TOOL_LINE:  g_tool = Tool::Line;  return;
            case ID_TOOL_ARROW: g_tool = Tool::Arrow; return;
            case ID_TOOL_TEXT:  g_tool = Tool::Text;  return;
            case ID_TOOL_BLUR:  g_tool = Tool::Blur;  return;
            case ID_UNDO:       DoUndo(); return;

            case ID_COPY:
                capture::CopyBitmapToClipboard(g_ownerForClipboard, g_bitmap);
                g_result = Result::Copied;
                DestroyWindow(hwnd);
                return;

            case ID_SAVE:
                capture::SaveBitmapToPngFile(g_bitmap, g_savePath);
                g_result = Result::Saved;
                DestroyWindow(hwnd);
                return;

            case ID_CANCEL:
                g_result = Result::Cancelled;
                DestroyWindow(hwnd);
                return;

            case ID_TEXT_EDIT:
                if (notify == EN_KILLFOCUS) CommitTextEdit();
                return;
            }
        }

        LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            switch (msg) {
            case WM_CREATE:
                CreateToolbarButtons(hwnd, (HINSTANCE)((CREATESTRUCTW*)lParam)->hInstance);
                return 0;

            case WM_LBUTTONDOWN: {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (g_editHwnd) CommitTextEdit(); // finalize any pending text first
                if (pt.y < kToolbarH) return 0;

                if (g_tool == Tool::Text) {
                    BeginTextEntry(hwnd, pt);
                    return 0;
                }
                g_dragging = true;
                g_dragStart = pt;
                g_dragCur = pt;
                SetCapture(hwnd);
                return 0;
            }

            case WM_MOUSEMOVE:
                if (g_dragging) {
                    g_dragCur.x = GET_X_LPARAM(lParam);
                    g_dragCur.y = GET_Y_LPARAM(lParam);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;

            case WM_LBUTTONUP:
                if (g_dragging) {
                    g_dragging = false;
                    ReleaseCapture();
                    POINT endPt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    CommitShape(g_dragStart, endPt);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;

            case WM_COMMAND:
                HandleCommand(hwnd, LOWORD(wParam), HIWORD(wParam));
                return 0;

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT:
                PaintWindow(hwnd);
                return 0;

            case WM_CLOSE:
                g_result = Result::Cancelled;
                DestroyWindow(hwnd);
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

    } // namespace

    Result Open(HINSTANCE hInstance, HWND ownerForClipboard, HBITMAP bitmap, const std::wstring& savePath) {
        g_bitmap = bitmap;
        g_savePath = savePath;
        g_ownerForClipboard = ownerForClipboard;
        g_tool = Tool::Arrow;
        g_dragging = false;
        g_editHwnd = nullptr;
        g_result = Result::Cancelled;
        for (HBITMAP b : g_undoStack) DeleteObject(b);
        g_undoStack.clear();

        BITMAP bm{};
        GetObjectW(bitmap, sizeof(bm), &bm);
        g_bitmapW = bm.bmWidth;
        g_bitmapH = bm.bmHeight;
        if (g_bitmapW <= 0) g_bitmapW = 1;
        if (g_bitmapH <= 0) g_bitmapH = 1;

        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        int availW = (work.right - work.left) - 40;
        int availH = (work.bottom - work.top) - kToolbarH - 80;

        double scaleW = (availW > 0) ? (double)availW / g_bitmapW : 1.0;
        double scaleH = (availH > 0) ? (double)availH / g_bitmapH : 1.0;
        g_scale = (scaleW < scaleH) ? scaleW : scaleH;
        if (g_scale > 1.0) g_scale = 1.0;
        if (g_scale < 0.1) g_scale = 0.1;

        g_canvasW = (int)(g_bitmapW * g_scale);
        g_canvasH = (int)(g_bitmapH * g_scale);
        if (g_canvasW < 480) g_canvasW = (480 < g_bitmapW ? 480 : g_bitmapW);
        if (g_canvasW < 1) g_canvasW = 1;
        if (g_canvasH < 1) g_canvasH = 1;

        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSW wc{};
            wc.lpfnWndProc   = WndProc;
            wc.hInstance     = hInstance;
            wc.lpszClassName = kClassName;
            wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = CreateSolidBrush(kCanvasBg);
            RegisterClassW(&wc);
            classRegistered = true;
        }

        int clientW = g_canvasW;
        int clientH = kToolbarH + g_canvasH;

        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
        RECT wr = { 0, 0, clientW, clientH };
        AdjustWindowRectEx(&wr, style, FALSE, WS_EX_TOPMOST);
        int outerW = wr.right - wr.left;
        int outerH = wr.bottom - wr.top;

        int winX = work.left + ((work.right - work.left) - outerW) / 2;
        int winY = work.top  + ((work.bottom - work.top) - outerH) / 2;

        g_hwnd = CreateWindowExW(WS_EX_TOPMOST, kClassName, APP_NAME,
            style, winX, winY, outerW, outerH,
            nullptr, nullptr, hInstance, nullptr);

        if (!g_hwnd) {
            DeleteObject(g_bitmap);
            g_bitmap = nullptr;
            return Result::Cancelled;
        }

        ShowWindow(g_hwnd, SW_SHOW);
        UpdateWindow(g_hwnd);

        MSG msg;
        while (g_hwnd && GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        for (HBITMAP b : g_undoStack) DeleteObject(b);
        g_undoStack.clear();
        if (g_bitmap) {
            DeleteObject(g_bitmap);
            g_bitmap = nullptr;
        }
        g_hwnd = nullptr;

        return g_result;
    }

} // namespace editor
