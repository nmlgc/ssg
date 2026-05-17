@echo off

call %~dp0hatoyama\depcheck_windows.bat
if %errorlevel% neq 0 exit /b %errorlevel%

sh %~dp0hatoyama\version_from_git.sh
tup %*
exit /b
