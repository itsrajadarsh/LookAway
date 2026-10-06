# LookAway

A lightweight, cross-platform desktop utility built with **C++17** and **Qt 6** to enforce the **20-20-20 eye care rule**: every 20 minutes of screen time, take a 20-second break to focus on an object at least 20 feet (6 meters) away.

---

## 📥 Downloads & Releases

Pre-compiled binaries for Windows and Linux are available on the [GitHub Releases](https://github.com/itsrajadarsh/LookAway/releases) page.

### Windows (Installer)
* **File:** `LookAway-Setup-v1.1.0.exe`
* **Installation:** Download and run the setup executable. Follow the installer wizard to install LookAway and optionally enable launch on system startup.

### Linux (AppImage)
* **File:** `LookAway-1.1.0-x86_64.AppImage` (or `LookAway-x86_64.AppImage`)
* **Execution:** Download the AppImage, make it executable, and run:
  ```bash
  chmod +x LookAway-1.1.0-x86_64.AppImage
  ./LookAway-1.1.0-x86_64.AppImage
  ```

---

## ✨ Features

* **Background Operation:** Runs quietly in the system tray with minimal CPU and memory footprint.
* **Single-Instance Enforcement:** Inter-process communication brings the existing window to the front if launched again.
* **Compound Dual-Schedule:** Run Micro breaks (e.g., 20-20-20 eye rests) and Macro intervals (e.g., 50-10 work/rest stretches) simultaneously with automatic priority handling.
* **Presets & Custom Profiles:** Quick switching between `20-20-20 (Eye Care)`, `25-5 (Pomodoro)`, and `50-10 (Deep Work)`, plus create and save unlimited custom timer profiles.
* **Configurable Break Styles:** Select between a Floating Card, an Ambient Border Glow shield (non-intrusive luminous screen perimeter), or a Full-Screen rest window.
* **Smart Inactivity Auto-Pause:** Pauses the countdown after user inactivity (mouse and keyboard idle), resuming automatically when you return.
* **Fullscreen Detection:** Detects active fullscreen games, presentations, or video players and defers break popups until you exit fullscreen.
* **Strict Rest Mode (Optional):** Optional enforcement mode hiding the skip button and suppressing `Escape` dismissals during breaks.
* **Audio Chimes & System Tray:** Gentle audio cues on transitions, live countdown hover tooltips, and quick tray context menu controls.
* **Daily Statistics:** Dashboard tracking of completed breaks, skipped breaks, and total rest minutes for the day.
* **Cross-Platform Autostart:** Windows Registry `Run` key and Linux XDG Autostart (`~/.config/autostart/`) integration.

---

## 🛠️ Technology Stack

* **Language:** C++17
* **Framework:** Qt 6 (`QtCore`, `QtGui`, `QtWidgets`, `QtMultimedia`, `QtNetwork`)
* **Build System:** CMake 3.16+ with Ninja or GNU Make
* **Compilers:** GCC 11+ / MinGW 13+, MSVC 2022, Clang
* **Platforms:** Windows 10/11, Linux (X11 & Wayland)

---

## 📁 Project Structure

```
LookAway/
├── CMakeLists.txt              # CMake build configuration
├── installer/
│   ├── setup_script.iss        # Inno Setup script for Windows installer
│   └── build_appimage.sh       # Automated Linux AppImage packaging script
├── installer_output/
│   ├── windows/                # Generated Windows installer (.exe)
│   └── linux/                  # Generated AppImages, AppDir, and packaging tools
├── src/
│   ├── main.cpp                # Application entry point, CLI parser, and IPC server
│   ├── TimerEngine.h/.cpp      # FSM timer logic, dual scheduling, and idle hooks
│   ├── SettingsManager.h/.cpp  # QSettings persistence, stats, and autostart
│   ├── AudioManager.h/.cpp     # QSoundEffect audio chime engine
│   ├── SystemTrayManager.h/.cpp# System tray icon, context menu, and tooltips
│   ├── BreakOverlayWidget.h/.cpp# Break overlay (Popup, BorderGlow, FullScreen)
│   ├── MainWindow.h/.cpp       # Main dashboard and settings tab interface
│   ├── CustomPresetDialog.h/.cpp# Custom preset profile creation dialog
│   └── FullscreenDetector.h/.cpp# Fullscreen app and media inhibitor detection
└── resources/
    ├── resources.qrc           # Qt resource manifest
    ├── icons/                  # Application SVG and tray status icons
    └── sounds/                 # Embedded audio chime WAV files
```

---

## 🔨 Building from Source

### Prerequisites

* **C++17 Compiler** (GCC, Clang, or MSVC)
* **CMake 3.16+**
* **Qt 6.x** (`Widgets`, `Multimedia`, `Network`)
* **Ninja** or **Make**

### Linux

1. Install build dependencies (Debian/Ubuntu):
   ```bash
   sudo apt update
   sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-multimedia-dev libgl1-mesa-dev
   ```

2. Configure and build:
   ```bash
   cmake -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build-linux
   ```

3. Run the binary:
   ```bash
   ./build-linux/LookAway
   ```

4. Package as an AppImage:
   ```bash
   ./installer/build_appimage.sh
   ```

### Windows (MinGW / MSVC)

1. Configure and compile:
   ```powershell
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/mingw_64"
   cmake --build build
   ```

2. Deploy Qt dependencies:
   ```powershell
   windeployqt build/LookAway.exe
   ```

3. Create the installer (Inno Setup 6):
   ```cmd
   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\setup_script.iss
   ```

---

## 💻 Usage & CLI Options

```bash
# Launch normal window
./LookAway

# Launch directly into system tray (minimized)
./LookAway --minimized

# Show command help and options
./LookAway --help
```

### System Tray Quick Controls
Right-click the LookAway eye icon in your system tray to:
* **Show Dashboard**
* **Pause / Resume Timer**
* **Skip Break**
* **Settings...**
* **Quit LookAway**

---

## 📖 Developer Architecture Guide

For comprehensive documentation covering the Finite State Machine (FSM), MVC architecture, window compositing, platform idle hooks, and extension guides, see the **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)**.

---

## 📄 License

LookAway is open-source software licensed under the **[MIT License](LICENSE)**.
