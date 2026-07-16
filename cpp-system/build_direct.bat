@echo off
REM Direct build script - no hanging call statements
REM This assumes vcvars64 environment is already set up

setlocal EnableDelayedExpansion

set "JDK_INCLUDE=deps\include"
set "OUTPUT_DIR=build"

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo [BUILD] Compiling Injector UI...
cl.exe /EHsc /W3 injector\main.cpp /Fe:"%OUTPUT_DIR%\injector.exe" /link user32.lib advapi32.lib dwmapi.lib comdlg32.lib shell32.lib gdi32.lib /subsystem:windows

echo [BUILD] Compiling DLL with ImGui...
cl.exe /LD /EHsc /W3 ^
    /I"%JDK_INCLUDE%" ^
    /I"%JDK_INCLUDE%\win32" ^
    /I"deps" ^
    dll\main_imgui.cpp ^
    dll\imgui_overlay.cpp ^
    deps\imgui\imgui.cpp ^
    deps\imgui\imgui_draw.cpp ^
    deps\imgui\imgui_tables.cpp ^
    deps\imgui\imgui_widgets.cpp ^
    deps\imgui\backends\imgui_impl_dx11.cpp ^
    deps\imgui\backends\imgui_impl_win32.cpp ^
    /Fe:"%OUTPUT_DIR%\esp_mod.dll" ^
    /link user32.lib gdi32.lib d3d11.lib dxgi.lib

echo.
echo [DONE] Files are in %OUTPUT_DIR%
echo Cleaning up temporary files...
del *.obj 2>nul
del *.lib 2>nul
del *.exp 2>nul

echo [SUCCESS] Build complete!
dir "%OUTPUT_DIR%"
