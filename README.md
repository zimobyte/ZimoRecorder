<div align="center">

<img src="docs/images/banner.png" alt="ZIMO RECORDER" width="100%">

# ZIMO RECORDER

**A fast, lightweight screenshot & screen recorder for Windows.**
Capture a window or any region, record video or GIF, edit in one click, and keep working.

[![Download](https://img.shields.io/badge/%E2%AC%87%20Download%20for%20Windows-E5142C?style=for-the-badge)](https://github.com/zimobyte/ZimoRecorder/releases/latest)
[![Latest release](https://img.shields.io/github/v/release/zimobyte/ZimoRecorder?style=for-the-badge&color=151c24&label=Latest)](https://github.com/zimobyte/ZimoRecorder/releases/latest)
![Platform](https://img.shields.io/badge/Windows-10%20%7C%2011-151c24?style=for-the-badge&logo=windows&logoColor=white)
![C++](https://img.shields.io/badge/Built%20with-C%2B%2B-151c24?style=for-the-badge&logo=cplusplus&logoColor=white)
![Languages](https://img.shields.io/badge/Languages-15-151c24?style=for-the-badge)

<br>

**[Download](#download--install)** &nbsp;&bull;&nbsp; **[Features](#features)** &nbsp;&bull;&nbsp; **[Shortcuts](#shortcuts)** &nbsp;&bull;&nbsp; **[Build](#build-from-source)** &nbsp;&bull;&nbsp; **[Changelog](CHANGELOG.md)**

</div>

---

<p align="center">
  <img src="docs/images/dashboard.png" alt="Main window" width="46%">
  &nbsp;
  <img src="docs/images/region-select.png" alt="Window and region picker" width="46%">
</p>

## Why ZIMO RECORDER

- **Always ready.** Starts with Windows (optional) and waits quietly in the tray. Your shortcuts work in any app, even before you open the main window.
- **Pick exactly what you want.** Hover a window to highlight it and click, or drag any region. Right-click while dragging to start over.
- **One tool for everything.** Screenshots, MP4 video, GIFs and text recognition (OCR).
- **Edit right after capture.** Line, arrow, text and blur tools with undo, then save or copy.
- **Light and private.** No account, no cloud, nothing uploaded. Files are saved to your own folders.
- **Speaks your language.** 15 interface languages, switchable at any time.

## Download & install

<p align="center">
  <a href="https://github.com/zimobyte/ZimoRecorder/releases/latest">
    <img src="https://img.shields.io/badge/%E2%AC%87%20%20Download%20ZIMO%20RECORDER-E5142C?style=for-the-badge&labelColor=E5142C" alt="Download ZIMO RECORDER" height="52">
  </a>
</p>

| | |
|---|---|
| **Requires** | Windows 10 or Windows 11 (64-bit) |
| **Installer** | `ZimoRecorder-Setup-x.y.z.exe` &mdash; installs for your user only, no administrator rights needed |

1. Open the [latest release](https://github.com/zimobyte/ZimoRecorder/releases/latest) and download the installer.
2. Run it. Keep *Create a desktop shortcut* and *Start with Windows* ticked, then finish.
3. Press a shortcut and capture. That's it.

> Windows may show a "SmartScreen" notice for new apps. Choose **More info** and then **Run anyway**.

## Shortcuts

| Action | Shortcut | Backup shortcut |
|---|---|---|
| Screenshot (window or region) | `Ctrl` + `PrtSc` | `Ctrl` + `F6` |
| Record video (MP4), press again to stop | `Shift` + `PrtSc` | `Ctrl` + `F7` |
| Record GIF, press again to stop | `Alt` + `PrtSc` | `Ctrl` + `F8` |
| Extract text from the screen (OCR) | `Ctrl` + `F9` | |

Every action is also available from the main window and the tray menu. Left-click the tray icon to open the main window, right-click for the menu.

## Features

| | |
|---|---|
| **Capture** | Full screen, a single window (auto-highlighted), or a free region with live size display |
| **Record** | MP4 video with pause/resume, GIF recording, floating timer that never appears in the recording |
| **Edit** | Line, arrow, text, blur/pixelate, undo, copy to clipboard |
| **Text recognition** | Select an area and the text is copied to your clipboard |
| **Notifications** | Click the "saved" card to open the folder with the file selected |
| **Sharp on any display** | DPI-aware, multi-monitor aware |
| **Updates** | Built-in update checks with release notes; required updates are announced well in advance |
| **Languages** | English, Persian, German, Russian, French, Spanish, Turkish, Arabic, Chinese, Italian, Portuguese, Japanese, Korean, Hindi, Dutch |

## Where are my files?

- Screenshots: `Pictures\ZimoRecorder`
- Videos and GIFs: `Videos\ZimoRecorder`

## Video & GIF recording

Recording uses [FFmpeg](https://ffmpeg.org/). The installer includes it when it was available at build time; otherwise place `ffmpeg.exe` next to `ZimoRecorder.exe`.

## Build from source

Requirements: Windows, a C++17 compiler (MinGW-w64 / GCC), CMake and Ninja.

```
cmake -B build -G "Ninja"
cmake --build build
```

The executable is created at `build/ZimoRecorder.exe`. To build the installer, open `installer/ZimoRecorder.iss` in [Inno Setup](https://jrsoftware.org/isinfo.php) and compile.

All names, version and shortcuts are configured in one file: `src/Config.h`.

## Publishing updates (maintainers)

Updates are delivered from the publisher's own server and are verified with a digital signature and SHA-256 before installing.

One-time setup:

1. `pip install cryptography`
2. Run `python tools/update_keygen.py` and keep `update_private_key.pem` secret.
3. Paste the printed public key into `UPDATE_PUBLIC_KEY_HEX` in `src/Config.h` and rebuild.

For every release:

1. Raise `APP_VERSION` in `src/Config.h` and rebuild.
2. Run `python tools/make_release.py --exe build/ZimoRecorder.exe --version 1.1.0 --notes-en "What's new"` (add `--notes-de`, `--notes-fr`, ... for other languages).
3. Upload the two files from `release/` to the update server, `latest.txt` last.

## Contributing

Bug reports and ideas are welcome. Please use the [issue templates](../../issues/new/choose).

---

<div align="center">

If you like ZIMO RECORDER, please give it a &#11088; &mdash; it helps a lot.<br><br>Made with care by **Zimo** &nbsp;&middot;&nbsp; &copy; 2026 Zimo. All rights reserved.

</div>
