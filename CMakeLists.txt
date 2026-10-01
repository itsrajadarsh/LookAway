cmake_minimum_required(VERSION 3.16)

project(LookAway VERSION 1.0.0 LANGUAGES CXX)

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
    resources/resources.qrc
)

add_executable(LookAway ${SOURCES})

target_link_libraries(LookAway PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

if(WIN32)
    set_target_properties(LookAway PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
