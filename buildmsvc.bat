@if not defined INCLUDE (
	echo "Run from a 'Visual Studio Command Prompt'"
) else (
	cd deps\\luajit\\src && msvcbuild.bat && cd ..\\..\\.. && cmake -G %1 Win32 .
)