#!/usr/bin/env bash
# ==============================================================================
# LookAway - Linux AppImage Creation Script
# ==============================================================================
# Equivalent to installer/setup_script.iss for Linux.
# Packages LookAway into a self-contained, portable AppImage for distribution.
#
# Usage:
#   ./installer/build_appimage.sh [options]
#
# Options:
#   -v, --version <ver>   Specify version string (default: read from CMakeLists.txt)
#   -q, --qmake <path>    Specify path to Qt qmake binary
#   --no-build            Skip cmake compilation step (use existing binary)
#   -h, --help            Show this help message
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_DIR="${REPO_ROOT}/installer_output/linux"

DO_BUILD=true
CUSTOM_VERSION=""
CUSTOM_QMAKE=""

# --- Parse Arguments ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        -v|--version)
            CUSTOM_VERSION="$2"
            shift 2
            ;;
        -q|--qmake)
            CUSTOM_QMAKE="$2"
            shift 2
            ;;
        --no-build)
            DO_BUILD=false
            shift
            ;;
        -h|--help)
            echo "LookAway AppImage Builder"
            echo "Usage: $0 [-v <version>] [-q <qmake_path>] [--no-build]"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage."
            exit 1
            ;;
    esac
done

echo "=========================================================="
echo "  LookAway Linux AppImage Generator"
echo "=========================================================="

# --- 1. Determine Version ---
if [ -n "$CUSTOM_VERSION" ]; then
    VERSION="$CUSTOM_VERSION"
elif [ -n "${VERSION:-}" ]; then
    VERSION="$VERSION"
else
    # Auto-extract from CMakeLists.txt
    VERSION=$(grep -E 'project\s*\(.*VERSION\s+[0-9.]+' "${REPO_ROOT}/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo "1.1.0")
fi
echo "[+] Target Version: ${VERSION}"

# --- 2. Locate Qt 6 & QMake ---
if [ -n "$CUSTOM_QMAKE" ]; then
    QMAKE="$CUSTOM_QMAKE"
elif [ -n "${QMAKE:-}" ]; then
    QMAKE="$QMAKE"
else
    # Search common Qt installation paths
    POSSIBLE_QMAKES=(
        "/home/${USER}/Qt/6.8.2/gcc_64/bin/qmake"
        $(find /home/${USER}/Qt/6.*/gcc_64/bin/qmake 2>/dev/null | sort -V | tail -n 1)
        "/opt/Qt/6.8.2/gcc_64/bin/qmake"
        $(command -v qmake6 2>/dev/null || true)
        $(command -v qmake 2>/dev/null || true)
    )
    QMAKE=""
    for candidate in "${POSSIBLE_QMAKES[@]}"; do
        if [ -n "$candidate" ] && [ -x "$candidate" ]; then
            # Verify it is Qt 6
            if "$candidate" -query QT_VERSION 2>/dev/null | grep -q '^6\.'; then
                QMAKE="$candidate"
                break
            fi
        fi
    done
fi

if [ -z "$QMAKE" ] || [ ! -x "$QMAKE" ]; then
    echo "[-] ERROR: Qt 6 qmake executable not found!"
    echo "    Please specify with --qmake /path/to/Qt/6.x/gcc_64/bin/qmake"
    echo "    or set the QMAKE environment variable."
    exit 1
fi

QT_VERSION=$("$QMAKE" -query QT_VERSION)
QT_PREFIX=$("$QMAKE" -query QT_INSTALL_PREFIX)
echo "[+] Detected Qt: ${QT_VERSION} (${QMAKE})"
echo "[+] Qt Prefix:  ${QT_PREFIX}"

# --- 3. Build Release Binary (if requested) ---
BUILD_DIR="${REPO_ROOT}/build-linux/Desktop_Qt_6_8_2-Release"
if [ ! -d "$BUILD_DIR" ]; then
    BUILD_DIR="${REPO_ROOT}/build-linux"
fi

if [ "$DO_BUILD" = true ]; then
    echo "[+] Compiling Release binary..."
    mkdir -p "$BUILD_DIR"
    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${QT_PREFIX}"
    cmake --build "$BUILD_DIR" --config Release -j"$(nproc)"
fi

# Locate compiled executable
if [ -f "${BUILD_DIR}/LookAway" ]; then
    BINARY_PATH="${BUILD_DIR}/LookAway"
elif [ -f "${REPO_ROOT}/build/LookAway" ]; then
    BINARY_PATH="${REPO_ROOT}/build/LookAway"
else
    echo "[-] ERROR: LookAway executable not found in ${BUILD_DIR} or ${REPO_ROOT}/build!"
    exit 1
fi
echo "[+] Using binary: ${BINARY_PATH}"

# --- 4. Setup Packaging Workspace ---
mkdir -p "${OUTPUT_DIR}/AppDir/usr/bin"
mkdir -p "${OUTPUT_DIR}/AppDir/usr/share/applications"
mkdir -p "${OUTPUT_DIR}/AppDir/usr/share/icons/hicolor/256x256/apps"

# Copy binary to AppDir
cp -f "${BINARY_PATH}" "${OUTPUT_DIR}/AppDir/usr/bin/LookAway"
chmod +x "${OUTPUT_DIR}/AppDir/usr/bin/LookAway"

# Ensure Desktop entry exists
if [ ! -f "${OUTPUT_DIR}/LookAway.desktop" ]; then
    cat << 'EOF' > "${OUTPUT_DIR}/LookAway.desktop"
[Desktop Entry]
Type=Application
Name=LookAway
Exec=LookAway
Icon=LookAway
Categories=Utility;
EOF
fi

# Ensure Icon exists
if [ ! -f "${OUTPUT_DIR}/LookAway.png" ]; then
    if [ -f "${OUTPUT_DIR}/AppDir/usr/share/icons/hicolor/256x256/apps/LookAway.png" ]; then
        cp "${OUTPUT_DIR}/AppDir/usr/share/icons/hicolor/256x256/apps/LookAway.png" "${OUTPUT_DIR}/LookAway.png"
    fi
fi

# --- 5. Download Packaging Tools & Runtime (if missing) ---
cd "${OUTPUT_DIR}"

export APPIMAGE_EXTRACT_AND_RUN=1

if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
    echo "[+] Downloading linuxdeploy-x86_64.AppImage..."
    wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x linuxdeploy-x86_64.AppImage
fi

if [ ! -f "linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "[+] Downloading linuxdeploy-plugin-qt-x86_64.AppImage..."
    wget -c "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
fi

if [ ! -f "runtime-x86_64" ]; then
    echo "[+] Downloading runtime-x86_64..."
    wget -c "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64" -O "runtime-x86_64"
    chmod +x runtime-x86_64
fi
export LDAI_RUNTIME_FILE="${OUTPUT_DIR}/runtime-x86_64"

# --- 6. Bundle AppImage ---
echo "[+] Bundling AppImage via linuxdeploy..."
export QMAKE="${QMAKE}"
export VERSION="${VERSION}"
export LINUXDEPLOY_OUTPUT_VERSION="${VERSION}"

./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --desktop-file LookAway.desktop \
    --icon-file LookAway.png \
    --plugin qt \
    --output appimage

TARGET_APPIMAGE="LookAway-${VERSION}-x86_64.AppImage"
GENERIC_APPIMAGE="LookAway-x86_64.AppImage"

if [ -f "${TARGET_APPIMAGE}" ]; then
    chmod +x "${TARGET_APPIMAGE}"
    # Keep generic name in sync
    cp -f "${TARGET_APPIMAGE}" "${GENERIC_APPIMAGE}"
    chmod +x "${GENERIC_APPIMAGE}"
fi

echo "=========================================================="
echo "  BUILD SUCCESSFUL!"
echo "=========================================================="
echo "Generated AppImages in: ${OUTPUT_DIR}"
ls -lh "${OUTPUT_DIR}"/*.AppImage | grep -v 'linuxdeploy'
echo ""
echo "To test run:"
echo "  APPIMAGE_EXTRACT_AND_RUN=1 ${OUTPUT_DIR}/${TARGET_APPIMAGE}"
echo "=========================================================="
