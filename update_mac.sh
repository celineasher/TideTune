#!/usr/bin/env bash
set -euo pipefail
if [[ "$(uname -s)" != "Darwin" ]]; then echo "macOS only" >&2; exit 1; fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
if [[ ! -d .git ]]; then echo "Please run from a TideTune Git clone." >&2; exit 1; fi
if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  echo "Tracked local changes found. Commit or stash them before updating." >&2; exit 1
fi
if [[ "$(git branch --show-current)" != main ]]; then echo "Switch to main before updating." >&2; exit 1; fi
git pull --ff-only origin main
if [[ ! -f CMakeLists.txt || ! -d Source ]]; then
  echo "TideTune source not imported yet. Run bash ./import_existing_mac.sh once." >&2
  exit 1
fi
bash ./test_dsp.sh
bash ./build_mac.sh --install
