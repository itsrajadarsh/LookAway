# LookAway

A lightweight, cross-platform desktop utility built with **C++17** and **Qt 6** to enforce the **20-20-20 eye care rule**: every 20 minutes of screen time, take a 20-second break to focus on an object at least 20 feet (6 meters) away.

---

## Downloads & Releases

Pre-compiled binaries for Windows and Linux are available on the [GitHub Releases](https://github.com/itsrajadarsh/LookAway/releases) page.

### Windows (Installer)
* **File:** `LookAway-Setup-v1.0.1.exe`
* **Installation:** Download and run the setup executable. Follow the installer wizard to install LookAway and optionally enable launch on system startup.

### Linux (AppImage)
* **File:** `LookAway-v1.0.1-x86_64.AppImage`
* **Execution:** Download the AppImage, grant executable permissions, and run:
  ```bash
  chmod +x LookAway-v1.0.1-x86_64.AppImage
  ./LookAway-v1.0.1-x86_64.AppImage
  ```

---

## Features

* **Background Engine:** Operates quietly in the system tray with minimal memory and CPU usage. Automatically manages break schedules.
* **System Tray Control:** Resides in the Windows taskbar system tray or Linux status area. Displays dynamic status icons and countdown hover tooltips.
* **Break Overlay:** Configurable full-screen translucent overlay during break sessions with eye relaxation visual prompts.
* **Smart Idle Detection:** Automatically pauses timer countdowns after 3 minutes of user inactivity (mouse/keyboard), preventing unnecessary break prompts while away from the desk.
* **Audio & Toast Notifications:** Plays low-latency chime notifications on session transitions paired with native system desktop notifications.
* **Timer Presets:** One-click toggling between `20-20-20 (Eye Care)`, `25-5 (Pomodoro)`, and `50-10 (Deep Work)`.
* **Daily Statistics:** Tracks completed breaks, skipped breaks, and total eye-rest minutes for the current day.
* **Persistent Configuration:** Saves custom durations, volume levels, overlay preferences, and startup behavior across system reboots via `QSettings`.

---

## Technology Stack

* **Language:** C++17
* **GUI Framework:** Qt 6 (Qt Widgets, Qt Multimedia)
* **Build System:** CMake 3.16+
* **Supported Compilers:** GCC / MinGW 13+, MSVC 2022, Clang
* **Supported OS:** Windows 10/11, Linux (X11 & Wayland)

---

## Project Structure

```
LookAway/
├── CMakeLists.txt              # CMake build configuration
├── installer/
│   └── setup_script.iss        # Inno Setup script for Windows installer
├── src/
│   ├── main.cpp                # Application entry point and CLI parsing
│   ├── TimerEngine.h/.cpp      # Timer engine and state machine logic
│   ├── SystemTrayManager.h/.cpp# QSystemTrayIcon and tray menu management
│   ├── MainWindow.h/.cpp       # Qt Dashboard and Settings interface
│   ├── BreakOverlayWidget.h/.cpp# Full-screen break overlay window
│   ├── AudioManager.h/.cpp     # QSoundEffect chime playback system
│   └── SettingsManager.h/.cpp  # QSettings persistence & statistics tracking
└── resources/
    ├── resources.qrc           # Qt resource manifest
    ├── icons/                  # Application SVG icons
    └── sounds/                 # Embedded audio chime WAV files
```

---

## Building from Source

### Prerequisites

* **Qt 6.x** (Widgets & Multimedia modules)
* **CMake 3.16+**
* **C++17 Compiler** (GCC/MinGW, MSVC, or Clang)
* **Ninja** or **Make**

### Windows (MinGW / MSVC)

1. Clone the repository:
   ```bash
   git clone https://github.com/itsrajadarsh/LookAway.git
   cd LookAway
   ```

2. Configure CMake:
   ```powershell
   cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
   ```

3. Build the project:
   ```powershell
   cmake --build build
   ```

4. Deploy dependencies for standalone distribution (optional):
   ```powershell
   windeployqt build/LookAway.exe
   ```

### Linux

1. Install build tools and Qt 6 development libraries (Debian/Ubuntu):
   ```bash
   sudo apt update
   sudo apt install qt6-base-dev qt6-multimedia-dev cmake build-essential ninja-build
   ```

2. Configure and build:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```

---

## Usage & CLI Options

Run the binary:
```bash
./build/LookAway
```

Launch directly to the system tray (minimized):
```bash
./build/LookAway --minimized
```

### System Tray Options
Right-clicking the system tray icon provides quick access to:
* **Show Dashboard**
* **Pause / Resume Timer**
* **Skip Break**
* **Settings...**
* **Quit LookAway**

---

## Documentation & Developer Guide

For in-depth architectural blueprints, internal data structures, finite state machine specifications, signal-slot workflows, and extension recipes, see the [Developer's Guide](DEVELOPER_GUIDE.md).

---

## License

This project is open-source and licensed under the [MIT License](LICENSE).

