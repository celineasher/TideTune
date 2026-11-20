#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script builds macOS VST3/AU/Standalone. Run DSP tests on Linux instead." >&2
  exit 1
fi
if [[ ! -d "Source" || ! -f "CMakeLists.txt" ]]; then
  echo "Run build_mac.sh from inside the TideTune project folder." >&2
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is required: brew install cmake" >&2
  exit 1
fi
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Apple Command Line Tools required: xcode-select --install" >&2
  exit 1
fi
cd "$(dirname "${BASH_SOURCE[0]}")"
ARCH="$(uname -m)"
if [[ "$ARCH" != "arm64" && "$ARCH" != "x86_64" ]]; then
  echo "Unsupported architecture: $ARCH" >&2; exit 1
fi

printf '\n== TideTune v0.4 / macOS %s ==\n' "$ARCH"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$ARCH"
cmake --build build --config Release --parallel 4

VST3="$(find build -type d -name 'TideTune.vst3' -print -quit)"
AU="$(find build -type d -name 'TideTune.component' -print -quit)"
APP="$(find build -type d -name 'TideTune.app' -print -quit)"
if [[ -z "$VST3" || -z "$AU" ]]; then
  echo 'Plugin bundles not found; no installed files have been changed.' >&2
  exit 1
fi
printf '\nBuild complete:\nVST3: %s\nAU: %s\nStandalone: %s\n' "$VST3" "$AU" "$APP"

if [[ "${1:-}" != "--install" ]]; then
  printf '\nTo install, quit Ableton and run: ./build_mac.sh --install\n'
  exit 0
fi

if pgrep -if 'Ableton Live' >/dev/null 2>&1; then
  echo 'Please quit Ableton Live completely before installing the new plugin.' >&2
  echo 'The compiled bundles are safe in the build directory; no installed files changed.' >&2
  exit 1
fi

VST_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST_DIR" "$AU_DIR"
STAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP="$HOME/Desktop/TideTune-Backups/$STAMP"
mkdir -p "$BACKUP"

# Back up existing plugins before replacement. Even a failed replacement
# leaves the previous plugin inside this backup directory.
for pair in "$VST_DIR/TideTune.vst3" "$AU_DIR/TideTune.component"; do
  if [[ -d "$pair" ]]; then
    ditto "$pair" "$BACKUP/$(basename "$pair")"
  fi
done

install_bundle () {
  local source="$1"
  local destination="$2"
  local staging="${destination}.staging.$$"
  trap 'rm -rf "$staging"' RETURN
  ditto "$source" "$staging"
  rm -rf "$destination"
  mv "$staging" "$destination"
  trap - RETURN
}
install_bundle "$VST3" "$VST_DIR/TideTune.vst3"
install_bundle "$AU" "$AU_DIR/TideTune.component"
printf '\nInstalled successfully!\nBackups: %s\n' "$BACKUP"
printf 'VST3: %s/TideTune.vst3\nAU: %s/TideTune.component\n' "$VST_DIR" "$AU_DIR"
echo 'Now open Ableton Live, hold Option and click Rescan, then search for TideTune.'
