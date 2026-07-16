# PowerShell build script with proper MSVC environment

# Find Visual Studio
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcVarsPath = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"

if (-not (Test-Path $vcVarsPath)) {
    Write-Host "Error: Visual Studio 2022 not found at $vsPath"
    exit 1
}

# Set up environment
Write-Host "[INFO] Setting up Visual Studio environment..."
$cmdOutput = cmd /c "`"$vcVarsPath`" && set"
foreach ($line in $cmdOutput) {
    if ($line -match "^([^=]+)=(.*)$") {
        $varName = $matches[1]
        $varValue = $matches[2]
        [Environment]::SetEnvironmentVariable($varName, $varValue, "Process")
    }
}

# Change to project directory
cd "C:\Users\Rdp1\Desktop\Mincraft\cpp-system"
Write-Host "[INFO] Working directory: $(Get-Location)"

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
    Write-Host "[BUILD] Created build directory"
}

# Build Injector
Write-Host "[BUILD] Compiling Injector UI..."
$injectorCmd = 'cl.exe /EHsc /W3 injector\main.cpp /Fe:"build\injector.exe" /link user32.lib advapi32.lib dwmapi.lib comdlg32.lib shell32.lib gdi32.lib /subsystem:windows'
Invoke-Expression $injectorCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Injector compilation failed with exit code $LASTEXITCODE"
    exit 1
}
Write-Host "[OK] Injector compiled successfully"

# Build DLL with corrected include paths
Write-Host "[BUILD] Compiling DLL with ImGui..."
$dllCmd = @'
cl.exe /LD /EHsc /W3 `
    /I"deps\include" `
    /I"deps\include\win32" `
    /I"deps" `
    /I"deps\imgui" `
    /I"deps\imgui\backends" `
    dll\main_imgui.cpp `
    dll\imgui_overlay.cpp `
    deps\imgui\imgui.cpp `
    deps\imgui\imgui_draw.cpp `
    deps\imgui\imgui_tables.cpp `
    deps\imgui\imgui_widgets.cpp `
    deps\imgui\backends\imgui_impl_dx11.cpp `
    deps\imgui\backends\imgui_impl_win32.cpp `
    /Fe:"build\esp_mod.dll" `
    /link user32.lib gdi32.lib d3d11.lib dxgi.lib dxguid.lib
'@
Invoke-Expression $dllCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] DLL compilation failed with exit code $LASTEXITCODE"
    exit 1
}
Write-Host "[OK] DLL compiled successfully"

# Clean up temporary files
Write-Host "[BUILD] Cleaning up temporary files..."
Remove-Item -Force -ErrorAction SilentlyContinue *.obj
Remove-Item -Force -ErrorAction SilentlyContinue *.lib
Remove-Item -Force -ErrorAction SilentlyContinue *.exp

# Report results
Write-Host ""
Write-Host "[SUCCESS] Build complete!"
Write-Host ""
Write-Host "=== Build Output ==="
if (Test-Path "build\esp_mod.dll") {
    $dllSize = (Get-Item "build\esp_mod.dll").Length / 1MB
    Write-Host "[OK] build\esp_mod.dll ($([math]::Round($dllSize, 2)) MB)"
} else {
    Write-Host "[MISSING] build\esp_mod.dll NOT FOUND"
}

if (Test-Path "build\injector.exe") {
    $exeSize = (Get-Item "build\injector.exe").Length / 1KB
    Write-Host "[OK] build\injector.exe ($([math]::Round($exeSize, 2)) KB)"
} else {
    Write-Host "[MISSING] build\injector.exe NOT FOUND"
}

Write-Host ""
Write-Host "Build contents:"
Get-ChildItem -Path "build" | Format-Table -AutoSize
