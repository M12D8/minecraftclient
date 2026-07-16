$vcvars = "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
$script = "`"$vcvars`" && cl.exe /EHsc /W3 injector\main.cpp /Fe:build\injector_new.exe /link user32.lib advapi32.lib dwmapi.lib comdlg32.lib shell32.lib gdi32.lib /subsystem:windows"
$proc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c", $script -NoNewWindow -PassThru -RedirectStandardOutput "build\cl_out.txt" -RedirectStandardError "build\cl_err.txt" -Wait
$exit = $proc.ExitCode
Get-Content "build\cl_out.txt" -ErrorAction SilentlyContinue
Get-Content "build\cl_err.txt" -ErrorAction SilentlyContinue
Write-Host "--- BUILD EXIT CODE: $exit ---"
if (Test-Path "main.obj") { Remove-Item "main.obj" }
