# Building LookAway AppImage (Linux)

## 📋 Prerequisites & Requirements

Ensure you have a 64-bit Linux distribution (Ubuntu 22.04+, Debian 12+, Fedora, Arch) with the build tools and Qt 6 libraries:

```bash
# Ubuntu / Debian / Pop!_OS:
sudo apt update
sudo apt install -y build-essential cmake ninja-build \
                    qt6-base-dev qt6-multimedia-dev \
                    libgl1-mesa-dev libx11-dev wget
```

*(If you installed Qt via the Qt Online Installer, the script automatically detects your Qt 6 installation in `~/Qt/`)*.

---

## 🚀 Quick Build (1 Step)

From the project root directory, run:

```bash
./installer/build_appimage.sh
```

### What this automated script does:
1. **Detects Version:** Automatically extracts version from `CMakeLists.txt` (e.g., `2.0.0`).
2. **Locates Qt 6:** Discovers local Qt 6 / QMake toolchain.
3. **Compiles Release:** Runs `cmake` and `ninja` across all CPU cores (`-j$(nproc)`).
4. **Sets Up AppDir:** Populates `AppDir` with binary, `.desktop` file, and high-res icon.
5. **Downloads Packaging Tools:** Fetches `linuxdeploy` and `linuxdeploy-plugin-qt` if not present.
6. **Bundles Dependencies:** Packages Qt libraries, platform plugins (`xcb`), multimedia codecs, and offline runtime.
7. **Emits AppImage:** Creates portable, self-contained AppImages in `installer_output/linux/`.

---

## ⚙️ Useful Script Options

```bash
# Standard automated build & package
./installer/build_appimage.sh

# Fast package without recompiling (uses existing binary)
./installer/build_appimage.sh --no-build

# Override version in the filename
./installer/build_appimage.sh -v 2.0.0

# Specify custom Qt qmake path
./installer/build_appimage.sh -q ~/Qt/6.8.2/gcc_64/bin/qmake

# Display help
./installer/build_appimage.sh --help
```

---

## 📦 Output Artifact

Generated in `installer_output/linux/`:
- **`LookAway-<version>-x86_64.AppImage`** — Standalone version-tagged distribution package (e.g. `LookAway-2.0.0-x86_64.AppImage`).

---

## ▶️ Running & Testing the AppImage

```bash
# Grant execution permissions
chmod +x installer_output/linux/LookAway-*-x86_64.AppImage

# Run the AppImage
./installer_output/linux/LookAway-*-x86_64.AppImage

# On systems without FUSE 2 (e.g. Ubuntu 24.04, WSL, Docker):
APPIMAGE_EXTRACT_AND_RUN=1 ./installer_output/linux/LookAway-*-x86_64.AppImage
```

---

## 💡 Troubleshooting

| Problem | Cause | Solution |
| :--- | :--- | :--- |
| `Cannot mount AppImage, please check your FUSE setup` | Missing `libfuse2` | Prepend `APPIMAGE_EXTRACT_AND_RUN=1` or run `sudo apt install libfuse2` |
| `Qt 6 qmake executable not found` | Qt not in standard PATH | Pass path via `-q /path/to/qmake` or `export QMAKE=/path/to/qmake` |
| `LookAway executable not found` | Build skipped or failed | Run `./installer/build_appimage.sh` without `--no-build` |
