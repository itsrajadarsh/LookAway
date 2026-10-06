# LookAway Installers & Packaging

This directory contains packaging scripts for generating production distribution packages on Linux and Windows.

---

## 🐧 Linux (AppImage)

The script `build_appimage.sh` automates the entire packaging workflow into a self-contained, portable Linux AppImage.

### Prerequisites (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-multimedia-dev libgl1-mesa-dev libx11-dev wget
```

### Quick Build
```bash
# Run from repository root
./installer/build_appimage.sh
```

### Script Options
* `./installer/build_appimage.sh` — Full build & package (auto-detects version & Qt 6)
* `./installer/build_appimage.sh --no-build` — Re-package without compiling (fast)
* `./installer/build_appimage.sh -v 2.0.0` — Override output version tag
* `./installer/build_appimage.sh -q <qmake_path>` — Specify custom Qt 6 qmake binary
* `./installer/build_appimage.sh --help` — Show help

**Output:** `installer_output/linux/LookAway-2.0.0-x86_64.AppImage`

> 📖 **Detailed Guide:** See [installer_output/linux/BUILD_APPIMAGE.md](../installer_output/linux/BUILD_APPIMAGE.md) for full prerequisites and troubleshooting.

**Running:**
```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./installer_output/linux/LookAway-2.0.0-x86_64.AppImage
```

---

## 🪟 Windows (Inno Setup)

The script `build_exe.iss` builds a modern, single-file Windows installer wizard with desktop shortcut creation and autostart registry options.

### Prerequisites
* Inno Setup 6.x installed
* Compiled Windows binary with deployed dependencies (`windeployqt build-windows/Release/LookAway.exe`)

### Quick Build
```cmd
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\build_exe.iss
```

**Output:** `installer_output\windows\LookAway-Setup-v2.0.0.exe`

> 📖 **Detailed Guide:** See [installer_output/windows/BUILD_EXE.md](../installer_output/windows/BUILD_EXE.md) for end-to-end Windows compilation, deployment, and GitHub release steps.
