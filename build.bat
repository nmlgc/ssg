@echo off

if "%VCINSTALLDIR%" == "" (
	echo Error: The build must be run from within Visual Studio's `x64_x86 Cross Tools Command Prompt`.
	exit 1
)

: `git submodule` is quite slow on Windows, so it's a good idea to just call it
: a single time in the optimal case.
for /f "delims=" %%s in ('git submodule status') do call:check_submodule "%%s"

tup
exit /b

:check_submodule
	set line=%~1
	set status=%line:~0,1%
	for /f "tokens=2 delims= " %%i IN ("%~1") DO set module=%%i
	if "%status%" == "-" (
		: Submodule not initialized
		git submodule init %module%
		git submodule update %module%
		exit /b
	) else if not "%status%" == "+" (
		: Submodule not changed
		exit /b
	)

	: Everything in (brackets) is treated as a single line where all variables
	: are expanded immediately, so we want to keep this branch outside of
	: brackets to have the %hash_recorded% variable work as intended.
	for /f "delims=" %%i in (
		'git submodule status --cached %module%'
	) do set hash_recorded="%%i"
	@echo off
	echo Error: Submodule "%module%": Checked-out commit does not match the recorded commit in Git:
	echo:
	echo 	Recorded: %hash_recorded:~2,40%
	echo 	 Current: %line:~1,40%
	echo:
	echo If you just pulled this repository, run
	echo:
	echo 	git submodule update %module%
	echo:
	echo Otherwise, resolve the conflict manually.
	echo Exiting the build process just to be safe.
	exit 1

