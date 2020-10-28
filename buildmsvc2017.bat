@if "%VSCMD_ARG_TGT_ARCH%" == "x86" (
	buildmsvc.bat "Visual Studio 15 2017" "Win32" "build"
) else (
	echo Must be run from a "32bit VS Developer Command Prompt"
)