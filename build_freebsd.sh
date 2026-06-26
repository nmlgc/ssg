#!/bin/sh
 
bash ./version_from_git.sh
 
[ ! -d .tup ] && tup init

# Unlike on Linux, we do NOT enable full_deps here: it requires running 
# sub-processes in a chroot with Linux namespaces for complete dependency 
# tracking outside the project tree. Linux namespaces is a feature exclusive 
# to that kernel: The BSDs use other technologies for containerized environments
# but Tup doesn't seem to support them.

./submodules_check.sh \
	libs/tupblocks \
	libs/dr_libs \
	libs/miniaudio \
	|| exit 1
 
. ./libs/tupblocks/tupblocks.sh
 
pkg_config_env_required \
	sdl3 \
	fontconfig \
	libwebp \
	ogg \
	pangocairo \
	vorbis \
	vorbisfile \
 
pkg_config_env_optional libblake3
! pkg-config --exists libblake3 && {
	./submodules_check.sh libs/BLAKE3 || exit 1;
}

# Unlike on Linux, we do not trust that `g++`/`gcc` will resolve to the 
# correct compiler: on FreeBSD, they can point to any version of lang/gcc* 
# installed as default, not necessarily the one that supports the 
# C++ Standard Library Modules we require.
: "${CC:=gcc16}"
: "${CXX:=g++16}"
export CC CXX

export TOOLCHAIN=gcc
tup "$@"
