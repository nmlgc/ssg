@echo off

if "%VCINSTALLDIR%" == "" (
	echo Error: The build must be run from within Visual Studio's `x64_x86 Cross Tools Command Prompt`.
	exit 1
)

sh ./version_from_git.sh
sh ./submodules_check.sh ^
	libs/9xcompat ^
	libs/BLAKE3 ^
	libs/dr_libs ^
	libs/libogg ^
	libs/libvorbis ^
	libs/libwebp_lossless ^
	libs/miniaudio ^
	libs/SDL2 ^
	libs/SDL3 ^
	libs/tupblocks
if %errorlevel% neq 0 exit /b %errorlevel%

tup %*
exit /b
