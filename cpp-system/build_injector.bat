@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" > build\vcvars_log.txt 2>&1
cl.exe /EHsc /W3 injector\main.cpp /Fe:build\injector_new.exe /link user32.lib advapi32.lib dwmapi.lib comdlg32.lib shell32.lib gdi32.lib /subsystem:windows > build\cl_log.txt 2>&1
echo BUILD_EXIT=%ERRORLEVEL% >> build\cl_log.txt
if exist main.obj del main.obj
