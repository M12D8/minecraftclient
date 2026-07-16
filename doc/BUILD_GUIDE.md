# ImGui Implementation - Build Guide

## Prerequisites

### Required Software
- **Windows 10/11** (64-bit)
- **Visual Studio 2019+** with C++ development tools
- **.NET Framework** (usually comes with VS)

### Optional
- **CMake** (for alternative build method)
- **Git** (for version control)

### Verify Installation

```bash
# Check for Visual Studio compiler
cl.exe /?

# Check Windows SDK
dir "C:\Program Files (x86)\Windows Kits\10\Lib"
```

## Build Methods

### Method 1: Batch Script (Recommended)

```bash
# Navigate to project
cd C:\Users\Rdp1\Desktop\Mincraft\cpp-system

# Run build script
build.bat
```

**What it does:**
1. Finds Visual Studio compiler
2. Initializes MSVC environment
3. Compiles injector.exe
4. Compiles esp_mod.dll with all ImGui sources
5. Links D3D11, DXGI, and Windows libraries
6. Places binaries in `build/` folder

**Output:**
```
build/
├── esp_mod.dll         ← Main DLL (with ImGui)
├── injector.exe        ← Injector UI
└── *.obj               ← Intermediate files (cleaned up)
```

### Method 2: Visual Studio IDE

1. **Open Developer Command Prompt for Visual Studio**
2. **Run:** `build.bat`

Or manually compile individual files:

```bash
# In Visual Studio Developer Command Prompt
cd cpp-system

# Compile main DLL
cl.exe /LD /EHsc /W3 ^
    /I"deps\include" /I"deps\include\win32" /I"deps" ^
    dll\main_imgui.cpp dll\imgui_overlay.cpp ^
    deps\imgui\imgui.cpp deps\imgui\imgui_draw.cpp ^
    deps\imgui\imgui_tables.cpp deps\imgui\imgui_widgets.cpp ^
    deps\imgui\backends\imgui_impl_dx11.cpp ^
    deps\imgui\backends\imgui_impl_win32.cpp ^
    /Fe:"build\esp_mod.dll" ^
    /link user32.lib gdi32.lib d3d11.lib dxgi.lib
```

### Method 3: CMake (Alternative)

```bash
cd C:\Users\Rdp1\Desktop\Mincraft\cpp-system

# Create build directory
mkdir build_cmake
cd build_cmake

# Generate build files
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build
cmake --build . --config Release

# Output in: build_cmake/build/
```

## Troubleshooting Build Issues

### Issue: "cl.exe not found"

**Solution:** 
Visual Studio compiler is not in PATH. The script will try to find vcvars64.bat automatically.

If still failing:
```bash
# Manually run vcvars
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Then run build
build.bat
```

### Issue: "Cannot find jni.h"

**Cause:** JNI headers not in `deps/include`

**Solution:**
```bash
# Copy from your JDK installation
xcopy "C:\Program Files\Java\jdk-17\include\*" "deps\include\" /E /I

# Then rebuild
build.bat
```

### Issue: "Cannot find d3d11.h"

**Cause:** Windows SDK not installed

**Solution:**
- Install Windows 10/11 SDK via Visual Studio Installer
- Or install full DirectX SDK

### Issue: Linker Error - "unresolved external symbol"

**Possible causes:**

1. Missing library: Add to `build.bat` `/link ... d3d11.lib dxgi.lib`
2. Object files conflict: Delete all `.obj` files and rebuild
3. ImGui implementation mismatch: Verify all backend files are compiled

**Solution:**
```bash
# Clean build
del *.obj *.lib *.exp
build.bat
```

### Issue: "IMGUI_API not defined"

**Cause:** ImGui header configuration issue

**Solution:**
- Verify `deps/imgui/imconfig.h` exists
- Check include paths in `build.bat`
- Make sure all ImGui source files are compiled

## Post-Build Verification

### Check Output Files

```bash
# Verify DLL was created
dir build\esp_mod.dll

# Verify injector was created  
dir build\injector.exe

# Check file sizes (should be substantial)
# esp_mod.dll: ~500 KB - 2 MB
# injector.exe: ~100-300 KB
```

### Test on Sandbox First

Create a test batch file `test_build.bat`:

```batch
@echo off
echo Verifying build output...

if not exist "build\esp_mod.dll" (
    echo ERROR: esp_mod.dll not found!
    exit /b 1
)

if not exist "build\injector.exe" (
    echo ERROR: injector.exe not found!
    exit /b 1
)

echo OK: All binaries found
echo.
echo To use:
echo 1. Start Minecraft
echo 2. Run: build\injector.exe
echo 3. Click INJECT DLL
echo 4. Press INSERT in Minecraft to toggle menu
echo.
pause
```

Run it:
```bash
test_build.bat
```

## Testing the Implementation

### Step 1: Start Minecraft
```bash
# Start Minecraft Java Edition
# Make sure you're on a test world (not multiplayer)
```

### Step 2: Inject DLL
```bash
# Run the injector
build\injector.exe

# In the UI:
# 1. Verify process name is "Minecraft.exe"
# 2. Click "INJECT DLL"
# 3. Wait for success message
```

### Step 3: Test Features
```
In-game:
1. Press INSERT to open menu
2. You should see 3 panels:
   - COMBAT (Kill Aura toggle)
   - VISUAL (ESP toggle)
   - MOVEMENT (Auto Sprint toggle)
3. Click checkboxes to toggle features
4. Press INSERT again to close menu
5. Press END to unload DLL
```

### Step 4: Console Output
A console window should show:
```
================================
  ESP Mod  - Injected!
  INSERT = open/close menu
  END    = unload
================================

[DLL] Attached to JVM
[DLL] MC refs OK
[ImGui] Initialized successfully
[DLL] Menu: OPEN
```

## Common Build Configurations

### Debug Build
```bash
# In build.bat, change:
cl.exe /LD /EHsc /W3 /Zi /D_DEBUG ...
```

### Release Build (Current)
```bash
# In build.bat:
cl.exe /LD /EHsc /W3 /O2 ...
```

### Static Runtime
```bash
# In build.bat, change:
cl.exe /LD /EHsc /MT /W3 ...
# (instead of /MD - dynamic runtime)
```

## Compiler Flags Explained

| Flag | Meaning |
|------|---------|
| `/LD` | Create DLL |
| `/EHsc` | Exception handling |
| `/W3` | Warning level 3 |
| `/O2` | Optimization level 2 |
| `/Zi` | Debug info |
| `/I<path>` | Include directory |
| `/Fe:<name>` | Output executable |
| `/link` | Start linker flags |

## Performance Optimization

For optimized builds, modify `build.bat`:

```batch
cl.exe /LD /EHsc /W3 /O2 /GL ^
    ...
    /link /LTCG ...
```

## Incremental Compilation

To speed up rebuilds during development:

```batch
:: Only recompile changed files
cl.exe /LD /EHsc /W3 /Gm ^
    ...
```

## Common Build Patterns

### Build Only DLL
```batch
cl.exe /LD /EHsc /W3 ^
    /I"deps\include" /I"deps\include\win32" /I"deps" ^
    dll\main_imgui.cpp dll\imgui_overlay.cpp ^
    deps\imgui\imgui.cpp deps\imgui\imgui_draw.cpp ^
    deps\imgui\imgui_tables.cpp deps\imgui\imgui_widgets.cpp ^
    deps\imgui\backends\imgui_impl_dx11.cpp ^
    deps\imgui\backends\imgui_impl_win32.cpp ^
    /Fe:"build\esp_mod.dll" ^
    /link user32.lib gdi32.lib d3d11.lib dxgi.lib
```

### Build Only Injector
```batch
cl.exe /EHsc /W3 ^
    injector\main.cpp ^
    /Fe:"build\injector.exe" ^
    /link user32.lib advapi32.lib dwmapi.lib comdlg32.lib shell32.lib gdi32.lib ^
    /subsystem:windows
```

## Continuous Integration (Optional)

Create a GitHub Actions workflow `.github/workflows/build.yml`:

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Setup Visual Studio
      uses: microsoft/setup-msbuild@v1
    
    - name: Build
      run: .\cpp-system\build.bat
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v2
      with:
        name: binaries
        path: cpp-system/build/
```

## Distribution

### Creating Distribution Package

```bash
# Create distribution folder
mkdir dist

# Copy binaries
copy build\injector.exe dist\
copy build\esp_mod.dll dist\

# Create README
copy README.md dist\

# Create archive
# (Use WinRAR, 7-Zip, or Windows Explorer)
```

### Zip Contents
```
esp_mod_release.zip
├── injector.exe
├── esp_mod.dll
├── README.md
└── USAGE.txt
```

## Security Notes

⚠️ **Important for Distribution:**
- Never distribute with API keys
- Remove debug symbols for release builds
- Sign DLL with certificate for production
- Include license file (if applicable)

## Build Stats

| Component | Compiled | Size |
|-----------|----------|------|
| ImGui Core | imgui.cpp | ~150 KB |
| ImGui Draw | imgui_draw.cpp | ~100 KB |
| ImGui Widgets | imgui_widgets.cpp | ~200 KB |
| ImGui Tables | imgui_tables.cpp | ~80 KB |
| D3D11 Backend | imgui_impl_dx11.cpp | ~40 KB |
| Win32 Backend | imgui_impl_win32.cpp | ~30 KB |
| Main DLL | main_imgui.cpp + overlay | ~100 KB |
| Injector | injector/main.cpp | ~50 KB |
| **Total DLL Size** | | **~1-2 MB** |

---

**Last Updated:** 2026-07-16  
**Build System:** MSVC 2019+  
**Target:** Windows 10/11 x64
