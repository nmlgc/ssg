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
# correct compiler: on FreeBSD they can point to whatever lang/gcc* port is
# installed as the system default, not necessarily one that supports the
# C++ Standard Library Modules we require (GCC >14).
#
# GCC 15 is deliberately avoided where possible: there ara known GCC 15 compiler errors that make this ssg fail to build. We scan for the newest
# available g++NN >14 (excluding 15) first, and only fall back to 15 (with
# a warning) if nothing newer is installed.
if [ -z "$CXX" ]; then
	best=""
	best_ver=0
	for candidate in /usr/local/bin/g++[0-9]*; do
		[ -x "$candidate" ] || continue
		ver="${candidate##*g++}"
		case "$ver" in
			''|*[!0-9]*) continue ;; # not a clean numeric suffix
		esac
		[ "$ver" -le 14 ] && continue # doesn't support std modules
		[ "$ver" -eq 15 ] && continue # known GCC 15 module bug, skip for now
		if [ "$ver" -gt "$best_ver" ]; then
			best_ver="$ver"
			best="$candidate"
		fi
	done
 
	# Fallback: only use GCC 15 if nothing newer was found above.
	if [ -z "$best" ] && [ -x /usr/local/bin/g++15 ]; then
		>&2 echo "Warning: only GCC 15 was found, which has a known bug"
		>&2 echo "affecting this project. Install lang/gcc16 or newer if possible."
		best=/usr/local/bin/g++15
		best_ver=15
	fi
 
	if [ -z "$best" ]; then
		>&2 echo "No g++NN >14 found in /usr/local/bin/."
		>&2 echo "Install lang/gcc16 (or newer) via pkg/ports, or export"
		>&2 echo "CC/CXX manually pointing to a suitable compiler."
		exit 1
	fi
	CXX="$best"
	CC=$(echo "$best" | sed 's/g++/gcc/')
	echo "Using detected toolchain: CXX=$CXX CC=$CC"
fi
export CC CXX
 
# tupblocks.sh's automatic toolchain detector looks literally for "*GCC*"
# (uppercase) in the first line of `$CXX --version`. The GCC banner on the
# FreeBSD Ports Collection is:
#
#   g++x (FreeBSD Ports Collection) xx.x.x
#
# ...with no "GCC" substring anywhere, so the detector fails for any GCC
# installed via pkg/ports. This appears to be a bug in tupblocks itself  not something specific to ssg. In the meantime, we
# skip automatic detection entirely: we already know it's GCC, also CLang is currently broken on other UNIX-like systems.
export TOOLCHAIN=gcc
tup "$@"
