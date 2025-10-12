#!/bin/sh

./version_from_git.sh

# We can't commit this file directly because Tup's database initialization
# check literally just looks for the directory, and refuses to auto-initialize
# the database if it already exists. (Also, Windows doesn't need it.)
[ ! -d .tup ] && tup init
[ ! -f .tup/options ] &&
echo "[updater]
	full_deps = 1 ; Necessary to track compiler updates on Linux" > .tup/options

# Libraries that either aren't packaged by any distro or that we always want
# to use the vendored version of
./submodules_check.sh \
	libs/tupblocks \
	libs/dr_libs \
	libs/miniaudio \
	|| exit 1

. ./libs/tupblocks/tupblocks.sh

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
	./submodules_check.sh libs/BLAKE3 || exit 1;
}

toolchain_detect_via_cc
tup "$@"
