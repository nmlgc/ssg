#!/bin/sh

root="$(dirname "$0")"
hatoyama="${1:-.}"

# We can't commit this file directly because Tup's database initialization
# check literally just looks for the directory, and refuses to auto-initialize
# the database if it already exists. (Also, Windows doesn't need it.)
[ ! -d "$root/.tup" ] && tup init
[ ! -f "$root/.tup/options" ] &&
echo "[updater]
	full_deps = 1 ; Necessary to track compiler updates on Linux
" > "$root/.tup/options"

# Libraries that either aren't packaged by any distro or that we always want
# to use the vendored version of
"$hatoyama/submodules_check.sh" \
	"$hatoyama/libs/tupblocks" \
	"$hatoyama/libs/dr_libs" \
	"$hatoyama/libs/miniaudio" \
	"$hatoyama/libs/printf" \
	|| exit

. "$hatoyama/libs/tupblocks/tupblocks.sh"

# Libraries that are supposed to be installed through the system's package
# manager
pkg_config_env_required \
	sdl3 \
	fontconfig \
	libwebp \
	ogg \
	pangocairo \
	vorbis \
	vorbisfile \

# Vendored libraries that we only use when they aren't installed system-wide
pkg_config_env_optional libblake3
! pkg-config --exists libblake3 && {
	"$hatoyama/submodules_check.sh" "$hatoyama/libs/BLAKE3" || exit;
}

toolchain_detect_via_cc
