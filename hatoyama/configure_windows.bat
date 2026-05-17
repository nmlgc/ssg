@echo off

if "%VCINSTALLDIR%" == "" (
	echo Error: The build must be run from within Visual Studio's `x64_x86 Cross Tools Command Prompt`.
	exit /b 1
)

sh %~dp0submodules_check.sh ^
	%~dp0libs\9xcompat ^
	%~dp0libs\BLAKE3 ^
	%~dp0libs\dr_libs ^
	%~dp0libs\libogg ^
	%~dp0libs\libvorbis ^
	%~dp0libs\libwebp_lossless ^
	%~dp0libs\miniaudio ^
	%~dp0libs\SDL3 ^
	%~dp0libs\tupblocks
exit /b
