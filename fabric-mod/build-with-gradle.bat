@echo off
setlocal

REM Set paths
set JAVA_HOME=C:\Users\Rdp1\Desktop\Mincraft\fabric-mod\java21
set GRADLE_HOME=C:\Users\Rdp1\Desktop\Mincraft\fabric-mod\gradle-8.10

REM Add gradle bin to PATH
set PATH=%GRADLE_HOME%\bin;%PATH%

echo ===================================
echo Fabric Mod Build
echo ===================================
echo JAVA_HOME: %JAVA_HOME%
echo GRADLE_HOME: %GRADLE_HOME%
echo.

REM Check Java
echo Checking Java installation...
"%JAVA_HOME%\bin\java.exe" -version
if errorlevel 1 (
    echo ERROR: Java not found!
    exit /b 1
)
echo.

REM Clean build
echo Step 1: Cleaning previous build...
call "%GRADLE_HOME%\bin\gradle.bat" clean --no-daemon
if errorlevel 1 (
    echo ERROR: Clean failed!
    exit /b 1
)
echo Clean completed successfully!
echo.

REM Build mod
echo Step 2: Building mod (this may take 3-5 minutes)...
call "%GRADLE_HOME%\bin\gradle.bat" build -x test --no-daemon
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)
echo.
echo Build completed successfully!
echo.

REM Check output
echo ===================================
echo Build Output
echo ===================================
if exist build\libs (
    echo.
    echo Output files in build\libs:
    dir /s build\libs\*.jar
    echo.
    for /f "tokens=5" %%A in ('dir /s build\libs\espmod-1.0.0.jar ^| find "espmod-1.0.0.jar"') do (
        echo File size: %%A bytes
    )
) else (
    echo ERROR: build\libs directory not found!
    exit /b 1
)

echo.
echo ===================================
echo Build complete!
echo ===================================

endlocal
