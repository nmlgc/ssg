#!/bin/sh

hatoyama="$(dirname "$0")/hatoyama"

. "$hatoyama/depcheck_linux.sh" "$hatoyama" || exit
"$hatoyama/version_from_git.sh"
tup "$@"
