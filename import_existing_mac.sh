#!/usr/bin/env bash
# One-time import of previously downloaded TideTune v0.2 ZIP into GitHub.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
[[ -d .git ]] || { echo "Run from a Git clone of celineasher/TideTune" >&2; exit 1; }
if [[ -f CMakeLists.txt && -d Source ]]; then
  echo "TideTune source is already present; no import needed."
  exit 0
fi
ZIP="$HOME/Downloads/TideTune-v0.2.0-mac-source.zip"
DIR="$HOME/Downloads/TideTune"
TMP=""
if [[ -f "$ZIP" ]]; then
  TMP="$(mktemp -d)"
  trap '[[ -z "$TMP" ]] || rm -rf "$TMP"' EXIT
  unzip -q "$ZIP" -d "$TMP"
  DIR="$TMP/TideTune"
fi
if [[ ! -f "$DIR/CMakeLists.txt" || ! -f "$DIR/Source/PluginProcessor.cpp" ]]; then
  echo "Cannot find the v0.2 source in $ZIP or $HOME/Downloads/TideTune" >&2
  echo "Get the existing v0.2 source ZIP from this chat once, then rerun." >&2
  exit 1
fi
for item in CMakeLists.txt LICENSE Source Tests build_mac.sh test_dsp.sh; do
  if [[ -d "$DIR/$item" ]]; then
    ditto "$DIR/$item" "$ROOT/$item"
  elif [[ -f "$DIR/$item" ]]; then
    cp "$DIR/$item" "$ROOT/$item"
  fi
done
git add CMakeLists.txt LICENSE Source Tests build_mac.sh test_dsp.sh
if ! git diff --cached --quiet; then
  git commit -m "Import TideTune v0.2 plugin source"
  echo "Pushing the source to your GitHub repository..."
  git push origin main
fi
echo "Source imported. Now run: bash ~/TideTune/update_mac.sh"
