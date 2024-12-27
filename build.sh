#!/bin/sh

submodule_init() {
	git submodule init "$1"
	if [ ! -f "$1.sparse-checkout" ]; then
		# Do a regular submodule checkout
		git submodule update "$1"
		return;
	fi

	hash=$(git submodule status --cached "$1" | cut -c 2-41)

	# Do a manual sparse checkout by
	# 1) setting up the repo from scratch to emulate `git submodule`'s shallow
	#    cloning of only the given ref,
	git -C "$1" init
	git -C "$1" remote add origin "$(git config submodule."$1".url)"
	git -C "$1" fetch --depth=1 --filter=blob:none origin "$hash"

	# 2) setting the sparse-checkout parameters, and
	git -C "$1" sparse-checkout set --stdin < "$1.sparse-checkout"
	git -C "$1" reset --hard "$hash"

	# 3) converting the result back into a submodule.
	# Can we declaratively specify all that in .gitmodules somehow, please?
	git submodule absorbgitdirs "$1"
}

submodules_check() {
	for module in "$@"; do
		line=$(git submodule status "$module")
		case $line in
			# Submodule not initialized?
			"-"*)
				submodule_init "$module";;

			# Mismatch?
			"+"*)
				recorded=$(git submodule status --cached "$1" | cut -c 2-41)
				current=$(echo "$line" | cut -c 2-41)
				sed \
					-e "s:<recorded>:$recorded:g" \
					-e "s:<current>:$current:g" \
					-e "s:<module>:$module:g" \
					./submodule_error.txt
				exit 1
				;;
		esac
	done
}

# We can't commit this file directly because Tup's database initialization
# check literally just looks for the directory, and refuses to auto-initialize
# the database if it already exists. (Also, Windows doesn't need it.)
[ ! -d .tup ] && tup init
[ ! -f .tup/options ] &&
echo "[updater]
	full_deps = 1 ; Necessary to track compiler updates on Linux" > .tup/options

# Libraries that either aren't packaged by any distro or that we always want
# to use the vendored version of
submodules_check libs/tupblocks libs/dr_libs libs/miniaudio
. ./libs/tupblocks/pkg_config_env.sh

# Libraries that are supposed to be installed through the system's package
# manager
pkg_config_env_required sdl2 ogg vorbis vorbisfile pangocairo fontconfig

# Vendored libraries that we only use when they aren't installed system-wide
pkg_config_env_optional libblake3
! pkg-config --exists libblake3 && submodules_check libs/BLAKE3

tup "$@"
