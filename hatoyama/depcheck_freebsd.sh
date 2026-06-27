#!/bin/sh

root="$(dirname "$0")"
hatoyama="${1:-.}"

[ ! -d "$root/.tup" ] && tup init

# Unlike on Linux, we do NOT enable full_deps here: it requires running
# sub-processes inside a chroot with Linux namespaces for full dependency
# tracking outside the project tree. Linux namespaces are exclusive to that
# kernel: While the different BSDs have thier own solutions for this Tup doesn't appear to support them
# but FUSE still should work if the user has the module loaded in kernel.

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

# Unlike on Linux, we do not trust that `g++`/`gcc` will resolve to the
# correct compiler: on FreeBSD they can point to whatever lang/gcc* port is
# installed as the system default, not necessarily one that supports the
# C++ Standard Library Modules we require (GCC >14).
#
# GCC 15 is deliberately avoided where possible: the maintainer has pointed
# out a known GCC 15 bug affecting this project. We scan for the newest
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
#   g++16 (FreeBSD Ports Collection) 16.1.0
#
# ...with no "GCC" substring anywhere, so the detector fails for any GCC
# installed via pkg/ports. This is a bug in tupblocks itself (worth
# reporting upstream), not something specific to ssg. In the meantime, we
# skip automatic detection entirely: we already know it's GCC.
export TOOLCHAIN=gcc
