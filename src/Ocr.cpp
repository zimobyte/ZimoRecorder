#include "Ocr.h"
#include <vector>
#include <cstdio>
#include <cstring>

namespace ocr {

    namespace {

        // Uses the classic "Await" helper pattern to call Windows' WinRT
        // OCR engine from Windows PowerShell (ships on every Windows 10/11
        // machine). Prints a status marker line, then the payload.
        const char* kScript =
R"PS(param([string]$ImagePath)
$ErrorActionPreference = 'Stop'
try {
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    Add-Type -AssemblyName System.Runtime.WindowsRuntime
    $asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
    })[0]

    Function Await($WinRtTask, $ResultType) {
        $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
        $netTask = $asTask.Invoke($null, @($WinRtTask))
        $netTask.Wait(-1) | Out-Null
        $netTask.Result
    }

    [Windows.Storage.StorageFile, Windows.Storage, ContentType = WindowsRuntime] | Out-Null
    [Windows.Media.Ocr.OcrEngine, Windows.Media.Ocr, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null

    $file = Await ([Windows.Storage.StorageFile]::GetFileFromPathAsync($ImagePath)) ([Windows.Storage.StorageFile])
    $stream = Await ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    $decoder = Await ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
    $bitmap = Await ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])

    $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
    if ($null -eq $engine) {
        Write-Output "ZIMO_OCR_NO_ENGINE"
        exit 0
    }

    $result = Await ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
    Write-Output "ZIMO_OCR_OK"
    Write-Output $result.Text
} catch {
    Write-Output "ZIMO_OCR_EXCEPTION"
    Write-Output $_.Exception.Message
}
)PS";

        std::wstring GetTempScriptPath() {
            wchar_t tempDir[MAX_PATH];
            DWORD len = GetTempPathW(MAX_PATH, tempDir);
            if (len == 0 || len > MAX_PATH) return L"zimo_ocr_script.ps1";
            return std::wstring(tempDir) + L"zimo_ocr_script.ps1";
        }

        bool WriteScriptFile(const std::wstring& path) {
            FILE* f = nullptr;
            if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return false;
            fwrite(kScript, 1, strlen(kScript), f);
            fclose(f);
            return true;
        }

    } // namespace

    bool RecognizeTextFromFile(const std::wstring& imagePath, std::wstring& outText) {
        outText.clear();

        std::wstring scriptPath = GetTempScriptPath();
        if (!WriteScriptFile(scriptPath)) return false;

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE readPipe = nullptr, writePipe = nullptr;
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

        std::wstring cmd = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" +
                            scriptPath + L"\" -ImagePath \"" + imagePath + L"\"";
        std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError  = writePipe;

        PROCESS_INFORMATION pi{};
        BOOL ok = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        CloseHandle(writePipe);

        if (!ok) {
            CloseHandle(readPipe);
            return false;
        }

        std::string raw;
        char buf[4096];
        DWORD n = 0;
        while (ReadFile(readPipe, buf, sizeof(buf), &n, nullptr) && n > 0) {
            raw.append(buf, n);
        }
        CloseHandle(readPipe);

        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (raw.empty()) return false;

        int wlen = MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), (int)raw.size(), nullptr, 0);
        if (wlen <= 0) return false;
        std::wstring wide(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), (int)raw.size(), &wide[0], wlen);

        size_t firstBreak = wide.find_first_of(L"\r\n");
        std::wstring firstLine = (firstBreak == std::wstring::npos) ? wide : wide.substr(0, firstBreak);
        std::wstring rest;
        if (firstBreak != std::wstring::npos) {
            size_t restStart = wide.find_first_not_of(L"\r\n", firstBreak);
            if (restStart != std::wstring::npos) rest = wide.substr(restStart);
        }
        while (!rest.empty() && (rest.back() == L'\n' || rest.back() == L'\r' || rest.back() == L' ')) {
            rest.pop_back();
        }

        if (firstLine == L"ZIMO_OCR_OK") {
            outText = rest;
            return true;
        }
        return false; // ZIMO_OCR_NO_ENGINE, ZIMO_OCR_EXCEPTION, or anything unexpected
    }

} // namespace ocr
