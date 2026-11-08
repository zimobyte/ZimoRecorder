#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace rec {

    enum class Format { MP4, GIF };

    // Screen recording controller. Drives ffmpeg.exe (which must sit
    // next to the exe or be on PATH) as a child process.
    //
    // MP4 sessions support pausing and resuming: under the hood this is
    // done by stopping ffmpeg (which finalizes a valid, playable video
    // segment) on Pause, and starting a new segment on Resume. When
    // Stop() is called, every segment recorded during the session is
    // stitched back together (losslessly) into the single output file
    // requested in Start().
    //
    // GIF sessions are simpler (no pause/resume - see SupportsPause()):
    // one continuous ffmpeg process writes directly to the .gif file.
    class ScreenRecorder {
    public:
        ~ScreenRecorder();

        // Starts a new recording session, writing (eventually) to
        // outputPath. If region is null, the primary monitor is
        // recorded full-screen; otherwise just that screen rectangle
        // (absolute screen coordinates) is recorded.
        bool Start(const std::wstring& outputPath, const RECT* region = nullptr,
                   Format format = Format::MP4);

        // Whether the current session supports Pause()/Resume() (false
        // for GIF sessions).
        bool SupportsPause() const { return m_format == Format::MP4; }

        // Whether the current (or most recently started) session is a
        // GIF recording.
        bool IsGif() const { return m_format == Format::GIF; }

        // Pauses the session: the segment recorded so far is finalized
        // into a valid video file, so nothing is lost even if the app
        // were to close right now.
        void Pause();

        // Resumes a paused session: starts recording a new segment that
        // continues on from where Pause() left off.
        void Resume();

        // Stops the session for good. Finalizes the last segment and,
        // if the session was paused/resumed at least once, stitches all
        // segments into the single file requested by Start().
        void Stop();

        // The file this session saves (or saved) to
        const std::wstring& OutputPath() const { return m_finalOutputPath; }

        bool IsRecording() const { return m_recording; }
        bool IsPaused()    const { return m_paused; }

    private:
        bool StartSegment();
        void FinalizeSegment();       // stops the ffmpeg child process gracefully
        void MergeSegmentsIntoOutput();
        std::wstring NextSegmentPath() const;
        void CleanupProcessHandles();

        PROCESS_INFORMATION m_pi{};
        HANDLE  m_stdinWrite = nullptr;
        bool    m_recording  = false;
        bool    m_paused     = false;

        bool    m_hasRegion  = false;
        RECT    m_region{};
        Format  m_format     = Format::MP4;

        std::wstring              m_finalOutputPath;
        std::vector<std::wstring> m_segmentPaths;
        int                        m_segmentIndex = 0;
    };

} // namespace rec
