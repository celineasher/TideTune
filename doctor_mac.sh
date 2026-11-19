#!/usr/bin/env bash
# Read-only macOS diagnostics for TideTune plug-in installation and Ableton crashes.
set -euo pipefail
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This diagnostic script requires macOS." >&2
  exit 1
fi
printf '\nTIDETUNE DOCTOR (read-only)\n===========================\n'
printf 'macOS: '; sw_vers -productVersion
printf 'CPU: '; uname -m
printf 'Timestamp: '; date

check_bundle () {
  local bundle="$1" kind="$2"
  echo ""
  echo "-- $kind: $bundle"
  if [[ ! -d "$bundle" ]]; then
    echo "Not installed here."
    return
  fi
  local executable="$bundle/Contents/MacOS/TideTune"
  local plist="$bundle/Contents/Info.plist"
  if [[ -f "$plist" ]]; then
    /usr/bin/plutil -lint "$plist" 2>&1 || true
    /usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist" 2>/dev/null || true
  else
    echo "ERROR: Info.plist is missing."
  fi
  if [[ -f "$executable" ]]; then
    /usr/bin/file "$executable"
    /usr/bin/lipo -archs "$executable" 2>&1 || true
    echo "Dynamic libraries:"
    /usr/bin/otool -L "$executable" 2>&1 | head -n 16 || true
  else
    echo "ERROR: TideTune executable is missing from the bundle."
    find "$bundle/Contents/MacOS" -maxdepth 1 -type f -print 2>/dev/null || true
  fi
  echo "Code signature validation (unsigned development builds may fail):"
  /usr/bin/codesign --verify --deep --strict --verbose=2 "$bundle" 2>&1 || true
}

check_bundle "$HOME/Library/Audio/Plug-Ins/VST3/TideTune.vst3" "User VST3"
check_bundle "$HOME/Library/Audio/Plug-Ins/Components/TideTune.component" "User AU"
check_bundle "/Library/Audio/Plug-Ins/VST3/TideTune.vst3" "System VST3"
check_bundle "/Library/Audio/Plug-Ins/Components/TideTune.component" "System AU"
check_bundle "/Volumes/Athar SSD/VST3/TideTune.vst3" "Custom SSD VST3"

# Also examine plugin bundles quarantined by the safe isolation instructions.
if [[ -d "$HOME/Desktop/TideTune-Disabled" ]]; then
  while IFS= read -r -d '' plugin; do
    check_bundle "$plugin" "Disabled copy"
  done < <(find "$HOME/Desktop/TideTune-Disabled" -maxdepth 4 -type d \( -name "TideTune.vst3" -o -name "TideTune.component" \) -print0)
fi

echo ""
echo "-- Latest relevant macOS crash reports"
report_dir="$HOME/Library/Logs/DiagnosticReports"
if [[ -d "$report_dir" ]]; then
  find "$report_dir" -maxdepth 1 -type f \( -iname '*PluginScanner*.ips' -o -iname '*PluginScanner*.crash' -o -iname '*Ableton*.ips' -o -iname '*Ableton*.crash' \) -print | sort | tail -n 12
else
  echo "No DiagnosticReports directory found."
fi
echo ""
echo "This script does not delete plugins, clear caches, launch Live, or change settings."
echo "If Live still crashes without TideTune, inspect one of the reports above."
