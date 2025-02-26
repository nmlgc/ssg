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

# `git submodule` is quite slow on Windows, so it's a good idea to just call it
# a single time in the optimal case.
git submodule status "$@" | while read -r line; do
	case $line in
		# Submodule not initialized?
		"-"*)
			module=$(echo "$line" | cut -d ' ' -f2)
			submodule_init "$module";;

		# Mismatch?
		"+"*)
			module=$(echo "$line" | cut -d ' ' -f2)
			recorded=$(git submodule status --cached "$module" | cut -c 2-41)
			current=$(echo "$line" | cut -c 2-41)
			echo Error: Submodule "$module": Checked-out commit does not match the recorded commit in Git:
			echo
			echo "	Recorded: $recorded"
			echo "	 Current: $current"
			echo
			echo If you just pulled this repository, run
			echo
			echo "	git submodule update $module"
			echo
			echo Otherwise, resolve the conflict manually.
			echo Exiting the build process just to be safe.
			exit 1
			;;
	esac
done
