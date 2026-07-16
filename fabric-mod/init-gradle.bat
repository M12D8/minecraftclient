@echo off
setlocal enabledelayedexpansion

REM Set JAVA_HOME to the local Java 21 installation
set JAVA_HOME=C:\Users\Rdp1\Desktop\Mincraft\fabric-mod\java21

echo Downloading and initializing Gradle wrapper...

REM Check if gradle-wrapper.jar exists, if not we need to download it
if not exist gradle\wrapper\gradle-wrapper.jar (
    echo gradle-wrapper.jar is missing. Attempting to initialize...
    
    REM Create the gradle\wrapper directory if it doesn't exist
    if not exist gradle\wrapper mkdir gradle\wrapper
    
    REM Download gradle-wrapper.jar from Maven Central
    echo Downloading gradle-wrapper.jar...
    powershell -Command "^
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12; ^
        $url = 'https://repo.gradle.org/gradle/org/gradle/gradle-wrapper-maven/gradle-wrapper.jar'; ^
        $webClient = New-Object System.Net.WebClient; ^
        try { ^
            $webClient.DownloadFile('https://raw.githubusercontent.com/gradle/gradle/master/gradle/wrapper/gradle-wrapper.jar', 'gradle\wrapper\gradle-wrapper.jar'); ^
            Write-Host 'Downloaded gradle-wrapper.jar'; ^
        } catch { ^
            Write-Host 'Failed to download from GitHub, trying alternate source...'; ^
            $webClient.DownloadFile('https://services.gradle.org/distributions/gradle-8.10-bin.zip', 'gradle-8.10-bin.zip'); ^
            Write-Host 'Downloaded gradle-8.10-bin.zip'; ^
        } ^
    "
)

if exist gradle\wrapper\gradle-wrapper.jar (
    echo gradle-wrapper.jar found. Ready to build.
) else (
    echo Failed to initialize gradle-wrapper.jar
    exit /b 1
)

endlocal
