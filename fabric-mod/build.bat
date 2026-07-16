@echo off
setlocal enabledelayedexpansion

REM Set JAVA_HOME to the local Java 21 installation
set JAVA_HOME=C:\Users\Rdp1\Desktop\Mincraft\fabric-mod\java21

REM Display Java version to verify it's working
echo Java version:
"%JAVA_HOME%\bin\java.exe" -version

REM Run gradle clean
echo Cleaning previous build...
call gradlew.bat clean

if errorlevel 1 (
    echo Clean failed!
    exit /b 1
)

REM Run gradle build without tests
echo Building mod...
call gradlew.bat build -x test

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!

REM List the output files
echo.
echo Output JARs:
if exist build\libs (
    dir /s build\libs\*.jar
) else (
    echo No build\libs directory found!
)

endlocal
