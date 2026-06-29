#!/bin/sh

hatoyama="$(dirname "$0")/hatoyama"

# NOTE: unlike bash, FreeBSD's /bin/sh does not forward extra arguments
# given to `.` (source) as positional parameters to the sourced script.
# Setting them on the current shell beforehand works regardless of that,
# but we must save the original args first, since this overwrites them.
orig_args="$@"
set -- "$hatoyama"
. "$hatoyama/depcheck_freebsd.sh" || exit
bash "$hatoyama/version_from_git.sh"
set -- $orig_args
tup "$@"
