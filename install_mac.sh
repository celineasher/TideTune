#!/usr/bin/env bash
set -euo pipefail
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "TideTune's installer requires macOS." >&2
  exit 1
fi

DEST="${TIDETUNE_HOME:-$HOME/TideTune}"
REPO="https://github.com/celineasher/TideTune.git"

if [[ ! -d "$DEST/.git" ]]; then
  if [[ -e "$DEST" ]]; then
    echo "Folder $DEST exists but is not a Git clone. No files were changed." >&2
    exit 1
  fi
  git clone "$REPO" "$DEST"
fi

cd "$DEST"
if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  echo "Tracked local changes detected in $DEST. Commit or stash them before continuing." >&2
  exit 1
fi
git pull --ff-only origin main

# Importer validates source version and all required DSP test files.
bash ./import_existing_mac.sh
bash ./update_mac.sh
