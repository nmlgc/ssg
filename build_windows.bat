@echo off

sh %~dp0hatoyama\version_from_git.sh
tup %*
exit /b
