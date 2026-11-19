#!/usr/bin/env bash
# One-time import of the TideTune v0.2 source archive.
set -euo pipefail

ROOT="$(cd "$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
[[ -d .git ]] || { echo "Run this from your TideTune Git clone." >&2; exit 1; }

# Check the exact source tree before trying to commit anything.
source_ok() {
  local d="$1"
  [[ -f "$d/CMakeLists.txt" ]] &&
  grep -Eq 'project\(TideTune VERSION 0\.2\.0' "$d/CMakeLists.txt" &&
  [[ -f "$d/LICENSE" ]] &&
  [[ -f "$d/Source/PluginProcessor.cpp" ]] &&
  [[ -f "$d/Source/PitchEngine.h" ]] &&
  [[ -f "$d/Tests/dsp_test.cpp" ]] &&
  [[ -f "$d/build_mac.sh" ]] &&
  [[ -f "$d/test_dsp.sh" ]]
}

if source_ok "$ROOT"; then
  echo "Complete TideTune v0.2 source already found in the Git clone."
else
  ZIP="$HOME/Downloads/TideTune-v0.2.0-mac-source.zip"
  DIR="$HOME/Downloads/TideTune"
  TMP=""
  if [[ -f "$ZIP" ]]; then
    TMP="$(mktemp -d)"
    trap '[[ -z "$TMP" ]] || rm -rf "$TMP"' EXIT
    if ! unzip -q "$ZIP" -d "$TMP"; then
      echo "Could not extract $ZIP" >&2; exit 1
    fi
    DIR="$TMP/TideTune"
  fi

  if ! source_ok "$DIR"; then
    echo "Missing or incomplete TideTune v0.2 source." >&2
    echo "The existing ~/Downloads/TideTune folder may contain v0.1, not v0.2." >&2
    echo "Put TideTune-v0.2.0-mac-source.zip in ~/Downloads and rerun this script." >&2
    echo "Existing source files have not been deleted." >&2
    exit 1
  fi

  for item in CMakeLists.txt LICENSE Source Tests build_mac.sh test_dsp.sh; do
    if [[ -d "$DIR/$item" ]]; then
      mkdir -p "$ROOT/$item"
      ditto "$DIR/$item" "$ROOT/$item"
    else
      cp "$DIR/$item" "$ROOT/$item"
    fi
  done
fi

if ! source_ok "$ROOT"; then
  echo "Imported source validation failed; nothing has been committed." >&2
  exit 1
fi

git add -- CMakeLists.txt LICENSE Source Tests build_mac.sh test_dsp.sh

if ! git diff --cached --quiet; then
  git commit -m "Import TideTune v0.2 source and DSP tests"
fi

echo "Pushing TideTune source to GitHub..."
git push origin main
echo "Source is now committed and pushed. Next: bash ~/TideTune/update_mac.sh"
