@echo off
setlocal EnableDelayedExpansion

:: --- Configuration ---
:: Using local headers copied from JDK
set "JDK_INCLUDE=deps\include"
set "OUTPUT_DIR=build"

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

:: --- Check for MSVC ---
where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo [INFO] cl.exe not found. Attempting to find Visual Studio...
    
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
    if exist "!VS_PATH!" (
        echo [INFO] Found Visual Studio 18 Insiders. Initializing environment...
        call "!VS_PATH!"
    ) else (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        if exist "!VS_PATH!" (
            call "!VS_PATH!"
        ) else (
            set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
            if exist "!VS_PATH!" (
                call "!VS_PATH!"
            ) else (
                echo [ERROR] Could not find Visual Studio vcvars64.bat.
                echo Please run this from a "Developer Command Prompt for VS" or install Visual Studio.
                exit /b 1
            )
        )
    )
)

:: --- Build Injector ---
echo [BUILD] Compiling Injector UI...
:: /subsystem:windows makes it a GUI app (no console window)
cl.exe /EHsc /W3 injector\main.cpp /Fe:"%OUTPUT_DIR%\injector.exe" /link user32.lib advapi32.lib dwmapi.lib comdlg32.lib shell32.lib gdi32.lib /subsystem:windows

:: --- Build DLL ---
echo [BUILD] Compiling DLL with ImGui...
:: Note: Using local JNI headers and ImGui
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
del *.obj
:: pause (Removed pause to prevent hanging)
