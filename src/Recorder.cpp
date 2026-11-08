#include "Recorder.h"
#include <cstdio>   // swprintf_s

namespace rec {

    namespace {
        // If ffmpeg.exe sits next to the app's own exe, use that;
        // otherwise fall back to the system PATH.
        std::wstring FindFfmpegPath() {
            wchar_t exeDir[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
            if (len == 0 || len == MAX_PATH) return L"ffmpeg.exe";

            std::wstring path(exeDir);
            size_t slash = path.find_last_of(L"\\/");
            if (slash == std::wstring::npos) return L"ffmpeg.exe";

            std::wstring candidate = path.substr(0, slash + 1) + L"ffmpeg.exe";
            DWORD attrs = GetFileAttributesW(candidate.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                return candidate;
            }
            return L"ffmpeg.exe"; // rely on PATH
        }

        // Runs a short-lived ffmpeg command synchronously (used for the
        // final concat/merge step, which finishes almost instantly since
        // it's a plain stream copy, no re-encoding).
        bool RunFfmpegAndWait(const std::wstring& ffmpeg, const std::wstring& args, DWORD timeoutMs) {
            std::wstring cmd = L"\"" + ffmpeg + L"\" " + args;
            std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
            cmdBuf.push_back(L'\0');

            SECURITY_ATTRIBUTES sa{};
            sa.nLength = sizeof(sa);
            sa.bInheritHandle = TRUE;
            HANDLE nul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                                      &sa, OPEN_EXISTING, 0, nullptr);

            STARTUPINFOW si{};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = nul;
            si.hStdError  = nul;

            PROCESS_INFORMATION pi{};
            BOOL ok = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
            if (nul) CloseHandle(nul);
            if (!ok) return false;

            WaitForSingleObject(pi.hProcess, timeoutMs);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        }

        // Splits a path into "folder + name without extension" and "extension"
        void SplitExt(const std::wstring& path, std::wstring& base, std::wstring& ext) {
            size_t slash = path.find_last_of(L"\\/");
            size_t dot = path.find_last_of(L'.');
            if (dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash)) {
                base = path.substr(0, dot);
                ext  = path.substr(dot);
            } else {
                base = path;
                ext  = L".mp4";
            }
        }
    }

    ScreenRecorder::~ScreenRecorder() {
        if (m_recording) Stop();
        CleanupProcessHandles();
    }

    void ScreenRecorder::CleanupProcessHandles() {
        if (m_stdinWrite)  { CloseHandle(m_stdinWrite);  m_stdinWrite = nullptr; }
        if (m_pi.hProcess) { CloseHandle(m_pi.hProcess); m_pi.hProcess = nullptr; }
        if (m_pi.hThread)  { CloseHandle(m_pi.hThread);  m_pi.hThread  = nullptr; }
    }

    std::wstring ScreenRecorder::NextSegmentPath() const {
        std::wstring base, ext;
        SplitExt(m_finalOutputPath, base, ext);
        wchar_t suffix[32];
        swprintf_s(suffix, L"_part%d", m_segmentIndex + 1);
        return base + suffix + ext;
    }

    bool ScreenRecorder::StartSegment() {
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE stdinRead = nullptr;
        if (!CreatePipe(&stdinRead, &m_stdinWrite, &sa, 0)) return false;
        // The write end of the pipe must not be inherited by the child
        SetHandleInformation(m_stdinWrite, HANDLE_FLAG_INHERIT, 0);

        HANDLE nul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  &sa, OPEN_EXISTING, 0, nullptr);

        int width, height, offsetX = 0, offsetY = 0;
        if (m_hasRegion) {
            width  = m_region.right  - m_region.left;
            height = m_region.bottom - m_region.top;
            offsetX = m_region.left;
            offsetY = m_region.top;
        } else {
            width  = GetSystemMetrics(SM_CXSCREEN);
            height = GetSystemMetrics(SM_CYSCREEN);
        }
        // libx264 + yuv420p need even width/height
        width  -= (width  % 2);
        height -= (height % 2);
        if (width <= 0 || height <= 0) {
            if (nul) CloseHandle(nul);
            CloseHandle(stdinRead);
            CloseHandle(m_stdinWrite);
            m_stdinWrite = nullptr;
            return false;
        }

        std::wstring ffmpeg = FindFfmpegPath();
        std::wstring segmentPath = NextSegmentPath();

        int framerate = (m_format == Format::GIF) ? 10 : 30;
        wchar_t sizeBuf[32];
        swprintf_s(sizeBuf, L"%dx%d", width, height);
        wchar_t frBuf[16];
        swprintf_s(frBuf, L"%d", framerate);

        std::wstring cmd = L"\"" + ffmpeg + L"\" -y -f gdigrab -framerate " + frBuf + L" -video_size " + sizeBuf;
        if (m_hasRegion) {
            wchar_t offBuf[64];
            swprintf_s(offBuf, L" -offset_x %d -offset_y %d", offsetX, offsetY);
            cmd += offBuf;
        }
        cmd += L" -i desktop";
        if (m_format == Format::GIF) {
            cmd += L" \"" + segmentPath + L"\"";
        } else {
            cmd += L" -c:v libx264 -preset veryfast -pix_fmt yuv420p \"" + segmentPath + L"\"";
        }

        std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput  = stdinRead;
        si.hStdOutput = nul;
        si.hStdError  = nul;

        ZeroMemory(&m_pi, sizeof(m_pi));

        BOOL ok = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &m_pi);

        CloseHandle(stdinRead);
        if (nul) CloseHandle(nul);

        if (!ok) {
            CloseHandle(m_stdinWrite);
            m_stdinWrite = nullptr;
            return false;
        }

        m_segmentPaths.push_back(segmentPath);
        m_segmentIndex++;
        return true;
    }

    void ScreenRecorder::FinalizeSegment() {
        if (m_stdinWrite) {
            DWORD written = 0;
            WriteFile(m_stdinWrite, "q\n", 2, &written, nullptr);
            CloseHandle(m_stdinWrite);
            m_stdinWrite = nullptr;
        }
        if (m_pi.hProcess) {
            if (WaitForSingleObject(m_pi.hProcess, 5000) == WAIT_TIMEOUT) {
                TerminateProcess(m_pi.hProcess, 1);
            }
        }
        CleanupProcessHandles();
    }

    bool ScreenRecorder::Start(const std::wstring& outputPath, const RECT* region, Format format) {
        if (m_recording) return false;

        m_finalOutputPath = outputPath;
        m_hasRegion = (region != nullptr);
        if (m_hasRegion) m_region = *region;
        m_format = format;
        m_segmentPaths.clear();
        m_segmentIndex = 0;
        m_paused = false;

        if (!StartSegment()) return false;

        m_recording = true;
        return true;
    }

    void ScreenRecorder::Pause() {
        if (!m_recording || m_paused || !SupportsPause()) return;
        FinalizeSegment();
        m_paused = true;
    }

    void ScreenRecorder::Resume() {
        if (!m_recording || !m_paused || !SupportsPause()) return;
        if (StartSegment()) {
            m_paused = false;
        }
    }

    void ScreenRecorder::MergeSegmentsIntoOutput() {
        if (m_segmentPaths.empty()) return;

        if (m_segmentPaths.size() == 1) {
            // Nothing to merge - just put the single segment at the final path
            MoveFileExW(m_segmentPaths[0].c_str(), m_finalOutputPath.c_str(),
                        MOVEFILE_REPLACE_EXISTING);
            return;
        }

        // Build a concat list file ffmpeg's concat demuxer understands
        std::wstring base, ext;
        SplitExt(m_finalOutputPath, base, ext);
        std::wstring listPath = base + L"_concat.txt";

        FILE* f = nullptr;
        if (_wfopen_s(&f, listPath.c_str(), L"w") != 0 || !f) return;
        for (const auto& seg : m_segmentPaths) {
            fwprintf(f, L"file '%s'\n", seg.c_str());
        }
        fclose(f);

        std::wstring ffmpeg = FindFfmpegPath();
        std::wstring args = L"-y -f concat -safe 0 -i \"" + listPath +
                             L"\" -c copy \"" + m_finalOutputPath + L"\"";
        RunFfmpegAndWait(ffmpeg, args, 30000);

        DeleteFileW(listPath.c_str());
        for (const auto& seg : m_segmentPaths) {
            DeleteFileW(seg.c_str());
        }
    }

    void ScreenRecorder::Stop() {
        if (!m_recording) return;

        if (!m_paused) {
            FinalizeSegment();
        }

        MergeSegmentsIntoOutput();

        m_recording = false;
        m_paused = false;
        m_segmentPaths.clear();
        m_segmentIndex = 0;
    }

} // namespace rec
