# ImGui Implementation for Minecraft ESP Mod

Welcome! This document provides a quick overview of the ImGui integration. For detailed information, see the other guides.

## 🎯 What's New

Your Minecraft injection DLL now features:
- **ImGui GUI Framework** - Modern, hardware-accelerated UI
- **Direct3D 11 Rendering** - 60 FPS overlay with transparency
- **Dark Theme** - Professional Vape-style appearance
- **Easy to Extend** - Simple API for adding new features

## 📁 Quick Start

### 1. Build
```bash
cd cpp-system
build.bat
```

Output: `build/esp_mod.dll` and `build/injector.exe`

### 2. Inject
```bash
1. Start Minecraft
2. Run build/injector.exe
3. Click "INJECT DLL"
```

### 3. Use
```bash
Press INSERT to toggle menu
Press END to unload
```

## 📚 Documentation

| Document | Purpose |
|----------|---------|
| **IMGUI_QUICK_START.md** | How to modify UI and build |
| **IMGUI_IMPLEMENTATION.md** | Technical architecture |
| **BUILD_GUIDE.md** | Detailed build instructions |
| **VALIDATION_CHECKLIST.md** | What was implemented |
| **IMPLEMENTATION_SUMMARY.md** | High-level overview |

## 🆕 New Files

```
cpp-system/
├── dll/
│   ├── main_imgui.cpp          ← Main DLL (ImGui version)
│   ├── imgui_overlay.h         ← ImGui wrapper
│   └── imgui_overlay.cpp       ← ImGui implementation
├── CMakeLists.txt              ← CMake support
└── [documentation files]       ← Guides and references
```

## 🔧 Customization Examples

### Add a Checkbox
```cpp
static volatile bool g_myFeature = false;

// In RenderImGuiUI():
ImGui::Checkbox("My Feature", (bool*)&g_myFeature);

// Use it:
if(g_myFeature) { /* do something */ }
```

### Add a Slider
```cpp
static float g_value = 1.0f;

ImGui::SliderFloat("Value", &g_value, 0.0f, 10.0f);
```

### Add a New Panel
```cpp
static bool g_panelOpen = true;

ImGui::SetNextWindowPos(ImVec2(640, 10), ImGuiCond_FirstUseEver);
ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

if (ImGui::Begin("MY PANEL", &g_panelOpen)) {
    ImGui::Checkbox("Option", &g_option);
    ImGui::End();
}
```

### Change Colors
In `imgui_overlay.cpp`, `ImGuiOverlay::Initialize()`:
```cpp
ImVec4* colors = style.Colors;
colors[ImGuiCol_WindowBg] = ImVec4(R/255.f, G/255.f, B/255.f, A);
```

## 🏗️ Architecture

```
┌─────────────────────────────────────┐
│  main_imgui.cpp                     │
│  - DLL entry point                  │
│  - Game logic (KillAura, ESP, etc.) │
│  - ImGui UI code                    │
└─────────────────────────────────────┘
           ↓
┌─────────────────────────────────────┐
│  ImGuiOverlay class                 │
│  - D3D11 device/swap chain          │
│  - ImGui context management         │
│  - Frame rendering                  │
└─────────────────────────────────────┘
           ↓
┌─────────────────────────────────────┐
│  ImGui + Backends                   │
│  - Win32 platform support           │
│  - D3D11 rendering                  │
└─────────────────────────────────────┘
           ↓
┌─────────────────────────────────────┐
│  Direct3D 11 + Windows API          │
└─────────────────────────────────────┘
```

## 📊 Features

### ✅ Implemented
- [x] Direct3D 11 overlay
- [x] ImGui UI framework
- [x] 60 FPS rendering
- [x] Dark theme styling
- [x] Transparent when closed
- [x] Click-through support
- [x] Window dragging
- [x] Menu toggle (INSERT key)
- [x] All original features (KillAura, ESP, AutoSprint)
- [x] JNI integration

### 🔜 Optional Enhancements
- [ ] Sliders and number input
- [ ] Text input fields
- [ ] Color picker
- [ ] Settings save/load
- [ ] Performance overlay
- [ ] Multiple themes

## 🐛 Troubleshooting

### Issue: "DLL won't load"
**Solution:** Ensure D3D11 is supported. Update graphics drivers.

### Issue: "UI doesn't appear"
**Solution:** Check console output. Press INSERT key. Verify Minecraft window detection.

### Issue: "Won't compile"
**Solution:** Run from Developer Command Prompt for Visual Studio, or manually run vcvars64.bat.

See **BUILD_GUIDE.md** for more solutions.

## 📖 Where to Go From Here

### Just Want to Build?
→ Follow **IMGUI_QUICK_START.md**

### Want to Understand How It Works?
→ Read **IMGUI_IMPLEMENTATION.md**

### Want Detailed Build Info?
→ Check **BUILD_GUIDE.md**

### Want to Know What Changed?
→ See **IMPLEMENTATION_SUMMARY.md**

### Want to Verify Everything?
→ Review **VALIDATION_CHECKLIST.md**

## 🎮 Using the Mod

1. **Start Minecraft** (Java Edition)
2. **Run injector:** `build/injector.exe`
3. **Inject DLL:** Click the button
4. **Toggle menu:** Press INSERT
5. **Use features:** Click checkboxes
6. **Unload:** Press END

## 📋 System Requirements

- **OS:** Windows 10/11 (x64)
- **GPU:** DirectX 11 compatible
- **Memory:** 100+ MB free
- **Minecraft:** Java Edition

## 🔗 References

- ImGui: https://github.com/ocornut/imgui
- DirectX 11: https://learn.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11
- JNI: https://docs.oracle.com/javase/8/docs/technotes/guides/jni/

## ⚙️ Configuration Files

- **build.bat** - Build script
- **CMakeLists.txt** - CMake configuration
- **imgui.ini** - ImGui layout (auto-generated)

## 📝 Notes

- Original `main.cpp` kept for reference
- All JNI functionality preserved
- ImGui is added, not replacing core logic
- Can be compiled with different backends if needed

## 🆘 Support

For issues:
1. Check the console window for error messages
2. Review the appropriate documentation file
3. Verify build prerequisites are met
4. Check graphics driver compatibility

---

**Implementation Date:** 2026-07-16  
**Status:** ✅ Production Ready  
**Framework:** ImGui + Direct3D 11  
**Platform:** Windows x64
