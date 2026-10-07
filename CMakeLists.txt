cmake_minimum_required(VERSION 3.16)

project(LookAway VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Network)

set(SOURCES
    src/main.cpp
    src/TimerEngine.h
    src/TimerEngine.cpp
    src/SettingsManager.h
    src/SettingsManager.cpp
    src/AudioManager.h
    src/AudioManager.cpp
    src/SystemTrayManager.h
    src/SystemTrayManager.cpp
    src/BreakOverlayWidget.h
    src/BreakOverlayWidget.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/CustomPresetDialog.h
    src/CustomPresetDialog.cpp
    src/FullscreenDetector.h
    src/FullscreenDetector.cpp
    src/WeeklyAnalyticsWidget.h
    src/WeeklyAnalyticsWidget.cpp
    src/ErgonomicTipCatalog.h
    src/ErgonomicTipCatalog.cpp
    src/ScreenFlashWidget.h
    src/ScreenFlashWidget.cpp
    src/LinuxIdleDetector.h
    src/LinuxIdleDetector.cpp
    src/GlobalHotkeyManager.h
    src/GlobalHotkeyManager.cpp
    resources/resources.qrc
)

if(UNIX AND NOT APPLE)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    find_package(X11 REQUIRED)
endif()

if(WIN32)
    # Copy app_icon.ico and lookaway.rc into the binary directory
    # This prevents any path-with-spaces or UNC path issues with windres / MinGW
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.ico"
        "${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico"
        COPYONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/lookaway.rc"
        "${CMAKE_CURRENT_BINARY_DIR}/lookaway.rc"
        COPYONLY
    )
    list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/lookaway.rc")

    if(MINGW)
        set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/lookaway.rc"
            PROPERTIES COMPILE_FLAGS "--no-cpp"
        )
    endif()
endif()

add_executable(LookAway ${SOURCES})

target_link_libraries(LookAway PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

if(UNIX AND NOT APPLE)
    target_link_libraries(LookAway PRIVATE Qt6::DBus X11::X11)
endif()

if(WIN32)
    set_target_properties(LookAway PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
