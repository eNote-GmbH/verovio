#!/usr/bin/env bash
# Regenerates include/Verovio/ with relative symlinks to every public header,
# mirroring Verovio.podspec's public_header_files globs so that Package.swift
# can expose a flat <Verovio/NAME.h> namespace that matches the CocoaPod.
#
# Run after pulling upstream changes that add/remove/rename headers.
set -euo pipefail
cd "$(dirname "$0")/.."

DEST="include/Verovio"
mkdir -p "$DEST"

find "$DEST" -mindepth 1 -not -name 'Verovio-umbrella.h' -delete

link_one() {
    local src="$1" base dst rel
    base="$(basename "$src")"
    dst="$DEST/$base"
    if [[ -e "$dst" || -L "$dst" ]]; then
        echo "collision: $src conflicts with existing $(readlink "$dst" 2>/dev/null || echo "$dst")" >&2
        exit 1
    fi
    rel="$(python3 -c "import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))" "$src" "$DEST")"
    ln -s "$rel" "$dst"
}

HEADER_DIRS=(include/crc include/hum include/json include/midi include/pugi include/vrv include/zip)
for d in "${HEADER_DIRS[@]}"; do
    [[ -d "$d" ]] || { echo "missing source dir: $d" >&2; exit 1; }
done
while IFS= read -r h; do link_one "$h"; done < <(find "${HEADER_DIRS[@]}" -maxdepth 1 -type f \( -name '*.h' -o -name '*.hpp' \) | sort)
while IFS= read -r h; do link_one "$h"; done < <(find libmei/dist libmei/addons -maxdepth 1 -type f -name '*.h' | sort)

echo "Regenerated $(find "$DEST" -maxdepth 1 -type l | wc -l | tr -d ' ') symlinks in $DEST"
