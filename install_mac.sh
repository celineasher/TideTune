#!/usr/bin/env bash
set -euo pipefail
if [[ "$(uname -s)" != Darwin ]]; then echo "macOS only" >&2; exit 1; fi
DEST="${TIDETUNE_HOME:-$HOME/TideTune}"
REPO=https://github.com/celineasher/TideTune.git
if [[ ! -d "$DEST/.git" ]]; then
  if [[ -e "$DEST" ]]; then echo "Folder $DEST already exists and is not a Git clone." >&2; exit 1; fi
  git clone "$REPO" "$DEST"
fi
cd "$DEST"
if [[ ! -d Source ]]; then
  bash ./import_existing_mac.sh
fi
bash ./update_mac.sh
