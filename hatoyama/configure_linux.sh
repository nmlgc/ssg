#!/bin/sh
# Must be sourced from a build script at the root of the repo.

root="$(dirname "$0")"
hatoyama="$root/hatoyama"

. "$hatoyama/funcs_unix.sh"
ensure_full_deps "$root"
configure_unix "$hatoyama"
toolchain_detect_via_cc
