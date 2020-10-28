@if not defined INCLUDE (
	echo Must be run from a "Developer Command Prompt for VS"
) else (
	if not exist "%3" mkdir %3
	if exist deps\\luajit\\src\\lua51.lib (
		echo NOT BUILDING LuaJIT BECAUSE lua51.lib ALREADY EXISTS.
		echo To force rebuild of LuaJIT, DELETE "deps\\luajit\\src\\lua51.lib"
		cd %3 && cmake -G %1 -A %2 ..\\ && cd ..
	) else (
		echo BUILDING LuaJIT BECAUSE lua51.lib DOESN'T EXIST.
		cd deps\\luajit\\src && msvcbuild.bat && cd ..\\..\\..\\%3 && cmake -G %1 -A %2 ..\\ && cd ..
	)
)