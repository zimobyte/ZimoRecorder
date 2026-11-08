cmake_minimum_required(VERSION 3.15)
project(ZimoRecorder LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR "ZIMO RECORDER is Windows-only (it uses the Win32 API).")
endif()

add_definitions(-DUNICODE -D_UNICODE)

add_executable(ZimoRecorder WIN32
    src/main.cpp
    src/ScreenCapture.cpp
    src/Overlay.cpp
    src/Recorder.cpp
    src/Icons.cpp
    src/Language.cpp
    src/Indicator.cpp
    src/Editor.cpp
    src/Ocr.cpp
    src/Updater.cpp
    src/Toast.cpp
    src/Dashboard.cpp
    src/UpdateStrings.cpp
    src/app.rc
)

target_link_libraries(ZimoRecorder PRIVATE
    user32
    gdi32
    gdiplus
    shell32
    ole32
    comdlg32
    dwmapi
    winhttp
    bcrypt
    advapi32
)

# Auto-copy icon/logo next to the exe (makes later replacement easier)
add_custom_command(TARGET ZimoRecorder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/icon.ico" "$<TARGET_FILE_DIR:ZimoRecorder>/icon.ico"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/logo.png" "$<TARGET_FILE_DIR:ZimoRecorder>/logo.png"
)
