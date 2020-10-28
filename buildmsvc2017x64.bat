@if "%VSCMD_ARG_TGT_ARCH%" == "x64" (
	buildmsvc.bat "Visual Studio 15 2017" "x64" "build"
) else (
	echo Must be run from a "x64 VS Developer Command Prompt"
)