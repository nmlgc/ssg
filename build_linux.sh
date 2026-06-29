#!/bin/sh

hatoyama="$(dirname "$0")/hatoyama"

. "$hatoyama/configure_linux.sh" || exit
"$hatoyama/version_from_git.sh"
tup "$@"
