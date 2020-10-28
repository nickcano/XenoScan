@rmdir /s /q .\build >nul 2>&1
@rmdir /s /q .\bin >nul 2>&1

@del /q .\deps\luajit\src\*.exe >nul 2>&1
@del /q .\deps\luajit\src\*.lib >nul 2>&1
@del /q .\deps\luajit\src\*.exp >nul 2>&1
@del /q .\deps\luajit\src\*.dll >nul 2>&1

@echo Cleared all build files