#!/bin/sh
# Unix configuration functions

# Configures the Tup repository in `$1` to use full dependency tracking.
#
# We can't commit `.tup/options` directly because Tup's database initialization
# check literally just looks for the directory, and refuses to auto-initialize
# the database if it already exists. (Also, Windows doesn't need it.)
ensure_full_deps() {
	root="$1"

	[ ! -d "$root/.tup" ] && tup init
	[ ! -f "$root/.tup/options" ] &&
	echo "[updater]
		full_deps = 1 ; Necessary to track compiler updates on Linux
	" > "$root/.tup/options"
}

# Prepares all libraries for Unix targets, using `$1` as the root path of
# Hatoyama.
configure_unix() {
	hatoyama="$1"

	# Libraries that either aren't packaged by any distro or that we always
	# want to use the vendored version of
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

	# Vendored libraries that we only use if they aren't installed system-wide
	pkg_config_env_optional libblake3
	! pkg-config --exists libblake3 && {
		"$hatoyama/submodules_check.sh" "$hatoyama/libs/BLAKE3" || exit;
	}
}
