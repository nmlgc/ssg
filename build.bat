@echo off

if "%VCINSTALLDIR%" == "" (
	echo Error: The build must be run from within Visual Studio's `x64_x86 Cross Tools Command Prompt`.
	exit 1
)

: `git submodule` is quite slow on Windows, so it's a good idea to just call it
: a single time in the optimal case.
for /f "delims=" %%s in ('git submodule status') do call:check_submodule "%%s"

tup %*
exit /b

:check_submodule
	set line=%~1
	set status=%line:~0,1%
	for /f "tokens=2 delims= " %%i IN ("%~1") DO set module=%%i
	if "%status%" == "-" (
		: Submodule not initialized
		goto init
	) else if "%status%" == "+" (
		goto mismatch
	)
	: Submodule not changed
	exit /b

:init
	git submodule init %module%
	if not exist %module:/=\%.sparse-checkout (
		: Do a regular submodule checkout
		git submodule update %module%
		exit /b
	)

	for /f "delims=" %%i in (
		'git submodule status --cached %module%'
	) do set hash="%%i"

	: Do a manual sparse checkout by
	: 1) setting up the repo from scratch to emulate `git submodule`'s shallow
	:    cloning of only the given ref,
	git -C %module% init
	for /f "delims=" %%f in (
		'git config submodule.%module%.url'
	) do git -C %module% remote add origin %%f
	git -C %module% fetch --depth=1 --filter=blob:none origin %hash:~2,40%

	: 2) setting the sparse-checkout parameters, and
	git -C %module% sparse-checkout set --stdin < %module:/=\%.sparse-checkout
	git -C %module% reset --hard %hash:~2,40%

	: 3) converting the result back into a submodule.
	: Can we declaratively specify all that in .gitmodules somehow, please?
	git submodule absorbgitdirs %module%
	exit /b

:mismatch
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
