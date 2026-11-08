#pragma once

// App icon (idle state and recording state)
#define IDI_APPICON           101
#define IDI_APPICON_RECORDING 102

// Small icons shown next to each menu item
#define IDI_MENU_CAPTURE_FULL   110
#define IDI_MENU_RECORD_START   112
#define IDI_MENU_RECORD_STOP    113
#define IDI_MENU_FOLDER         114
#define IDI_MENU_ABOUT          115
#define IDI_MENU_EXIT           116
#define IDI_MENU_LANGUAGE       117
#define IDI_MENU_OCR            118

// Editor toolbar tool icons
#define IDI_TOOL_LINE   130
#define IDI_TOOL_ARROW  131
#define IDI_TOOL_TEXT   132
#define IDI_TOOL_BLUR   133
#define IDI_TOOL_UNDO   134

// Tray icon id
#define ID_TRAY_APP_ICON     1001

// Custom message Windows sends for tray icon clicks
#define WM_TRAYICON          (WM_APP + 1)

// Tray icon right-click menu items (all of these act on the FULL screen;
// the matching hotkeys below do the same actions but let you pick a
// region first - see Config.h)
#define IDM_CAPTURE_FULL     2001
#define IDM_TOGGLE_RECORD    2003
#define IDM_OPEN_FOLDER      2004
#define IDM_ABOUT            2005
#define IDM_EXIT             2006
#define IDM_OCR              2009
#define IDM_TOGGLE_GIF       2010
#define IDM_CHECK_UPDATE     2011
#define IDM_OPEN_DASHBOARD   2012

// Dashboard (main window) actions and messages
#define DASH_REGION   1
#define DASH_FULL     2
#define DASH_RECORD   3
#define DASH_GIF      4
#define DASH_OCR      5
#define DASH_FOLDER   6
#define DASH_STARTUP  7
#define DASH_LANG     8
#define DASH_UPDATE   9
#define WM_SHOW_DASHBOARD  (WM_APP + 12)   // sent by a second launch: "open the window"
#define WM_DASH_ACTION     (WM_APP + 13)   // dashboard button pressed (wParam = DASH_*)

// Language submenu items: one id per language, IDM_LANG_BASE + (int)Lang
// (15 languages -> ids 3000..3014, see Language.h)
#define IDM_LANG_BASE         3000

// Global hotkey ids (RegisterHotKey). Each of these opens the region
// selector first, then does its action (recording ones toggle stop on
// a second press).
#define HOTKEY_ID_SCREENSHOT  1   // pick a region -> screenshot
#define HOTKEY_ID_RECORD      3   // pick a region -> record (mp4), toggle
#define HOTKEY_ID_OCR         6   // pick a region -> extract text
#define HOTKEY_ID_GIF         7   // pick a region -> record (GIF), toggle
#define HOTKEY_ID_SCREENSHOT2 11  // backup shortcuts
#define HOTKEY_ID_RECORD2     12
#define HOTKEY_ID_GIF2        13
