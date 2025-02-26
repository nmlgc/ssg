#!/bin/sh

if [ "$#" -lt 1 ]; then
	>&2 echo "Usage: ROOT= PATH_SKELETON= $0 APP_NAME"
	exit 1
fi

set -x

ROOT=${ROOT:-/usr}
PATH_SKELETON=${PATH_SKELETON:-$ROOT/share/skel}

install -Dm755 "./bin/GIAN07" -t "$ROOT/bin/"
install -Dm644 "./bin/bgm/Folder structure.png" -t "$PATH_SKELETON/bgm/"
install -Dm755 -d "$ROOT/share/applications/"
sed "s/<icon>/$1/" "./GIAN07/GIAN07.desktop.in" > "$ROOT/share/applications/$1.desktop"
install -Dm644 "./art/016_box.png" "$ROOT/share/icons/hicolor/16x16/apps/$1.png"
install -Dm644 "./art/032_text.png" "$ROOT/share/icons/hicolor/32x32/apps/$1.png"
install -Dm644 "./art/048_text.png" "$ROOT/share/icons/hicolor/48x48/apps/$1.png"
install -Dm644 "./art/128_text.png" "$ROOT/share/icons/hicolor/128x128/apps/$1.png"
install -Dm644 "./LICENSE" -t "$ROOT/share/licenses/$1/"
