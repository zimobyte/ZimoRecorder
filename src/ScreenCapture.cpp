#include "ScreenCapture.h"
#include <objidl.h>   // must come before gdiplus.h (some compilers need this)
#include <gdiplus.h>
#include <cstring>    // memcpy
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

namespace capture {

    static ULONG_PTR g_gdiplusToken = 0;

    // ---------------------------------------------------------------
    // Helper: find the CLSID of the PNG encoder GDI+ needs for saving
    // ---------------------------------------------------------------
    static bool GetPngEncoderClsid(CLSID* clsid) {
        UINT numEncoders = 0, size = 0;
        GetImageEncodersSize(&numEncoders, &size);
        if (size == 0) return false;

        ImageCodecInfo* info = reinterpret_cast<ImageCodecInfo*>(malloc(size));
        if (!info) return false;

        GetImageEncoders(numEncoders, size, info);

        bool found = false;
        for (UINT i = 0; i < numEncoders; i++) {
            if (wcscmp(info[i].MimeType, L"image/png") == 0) {
                *clsid = info[i].Clsid;
                found = true;
                break;
            }
        }
        free(info);
        return found;
    }

    bool Init() {
        GdiplusStartupInput input;
        return GdiplusStartup(&g_gdiplusToken, &input, nullptr) == Ok;
    }

    void Shutdown() {
        if (g_gdiplusToken) {
            GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
    }

    HBITMAP CaptureRectToBitmap(const RECT& r) {
        const int width  = r.right  - r.left;
        const int height = r.bottom - r.top;
        if (width <= 0 || height <= 0) return nullptr;

        HDC hScreenDC = GetDC(nullptr);
        if (!hScreenDC) return nullptr;

        HDC hMemDC = CreateCompatibleDC(hScreenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);

        BitBlt(hMemDC, 0, 0, width, height, hScreenDC, r.left, r.top, SRCCOPY | CAPTUREBLT);
        // The mouse cursor is intentionally not captured (keeps this simple and consistent)

        SelectObject(hMemDC, hOld);
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        return hBitmap;
    }

    HBITMAP CaptureFullScreenToBitmap() {
        RECT r;
        r.left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
        r.top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
        r.right  = r.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
        r.bottom = r.top  + GetSystemMetrics(SM_CYVIRTUALSCREEN);
        return CaptureRectToBitmap(r);
    }

    bool SaveBitmapToPngFile(HBITMAP bitmap, const std::wstring& filePath) {
        if (!bitmap) return false;
        bool ok = false;
        {
            Bitmap bmp(bitmap, nullptr);
            CLSID pngClsid;
            if (GetPngEncoderClsid(&pngClsid)) {
                ok = (bmp.Save(filePath.c_str(), &pngClsid, nullptr) == Ok);
            }
        }
        return ok;
    }

    bool CopyBitmapToClipboard(HWND ownerWindow, HBITMAP bitmap) {
        if (!bitmap) return false;

        BITMAP bm{};
        if (!GetObjectW(bitmap, sizeof(bm), &bm)) return false;

        BITMAPINFOHEADER bi{};
        bi.biSize        = sizeof(BITMAPINFOHEADER);
        bi.biWidth       = bm.bmWidth;
        bi.biHeight      = bm.bmHeight; // positive -> bottom-up DIB (standard clipboard format)
        bi.biPlanes      = 1;
        bi.biBitCount    = 32;
        bi.biCompression = BI_RGB;

        DWORD imageSize = (DWORD)bm.bmWidth * (DWORD)bm.bmHeight * 4;
        HDC hdc = GetDC(nullptr);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + imageSize);
        if (!hMem) { ReleaseDC(nullptr, hdc); return false; }

        void* mem = GlobalLock(hMem);
        if (!mem) { ReleaseDC(nullptr, hdc); GlobalFree(hMem); return false; }

        memcpy(mem, &bi, sizeof(BITMAPINFOHEADER));
        int got = GetDIBits(hdc, bitmap, 0, (UINT)bm.bmHeight,
                             (BYTE*)mem + sizeof(BITMAPINFOHEADER),
                             (BITMAPINFO*)mem, DIB_RGB_COLORS);
        GlobalUnlock(hMem);
        ReleaseDC(nullptr, hdc);

        if (got == 0) { GlobalFree(hMem); return false; }

        if (!OpenClipboard(ownerWindow)) { GlobalFree(hMem); return false; }
        EmptyClipboard();
        bool ok = SetClipboardData(CF_DIB, hMem) != nullptr;
        CloseClipboard();
        if (!ok) GlobalFree(hMem); // only free on failure; on success the clipboard owns it
        return ok;
    }

    bool CaptureFullScreenToFile(const std::wstring& filePath) {
        HBITMAP bmp = CaptureFullScreenToBitmap();
        if (!bmp) return false;
        bool ok = SaveBitmapToPngFile(bmp, filePath);
        DeleteObject(bmp);
        return ok;
    }

    bool CaptureRectToFile(const RECT& r, const std::wstring& filePath) {
        HBITMAP bmp = CaptureRectToBitmap(r);
        if (!bmp) return false;
        bool ok = SaveBitmapToPngFile(bmp, filePath);
        DeleteObject(bmp);
        return ok;
    }

} // namespace capture
