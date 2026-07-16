# ImGui Implementation Guide

This document explains the ImGui integration in the ESP Mod DLL.

## Overview

The overlay UI has been refactored from GDI (Windows graphics API) to use **ImGui** with **Direct3D 11** rendering. This provides:

- Modern, scalable UI framework
- Easier widget management and state handling
- Better performance with hardware acceleration
- Professional appearance with built-in themes

## Files

### Core ImGui Implementation

- **`dll/imgui_overlay.h`** - ImGuiOverlay class declaration
- **`dll/imgui_overlay.cpp`** - ImGuiOverlay class implementation with D3D11 setup
- **`dll/main_imgui.cpp`** - Main DLL entry point with ImGui UI rendering

### ImGui Dependencies

Located in `deps/imgui/`:
- Core ImGui library (imgui.h, imgui.cpp)
- Drawing backend (imgui_draw.cpp)
- Widget implementations (imgui_widgets.cpp, imgui_tables.cpp)
- **DirectX 11 Backend**: `backends/imgui_impl_dx11.cpp/h`
- **Windows Platform Backend**: `backends/imgui_impl_win32.cpp/h`

## Architecture

### ImGuiOverlay Class

Manages the Direct3D 11 device and ImGui rendering:

```cpp
class ImGuiOverlay {
public:
    bool Initialize(HWND hwnd);      // Setup D3D11 + ImGui
    void Shutdown();                  // Cleanup
    void NewFrame();                  // Begin frame
    void Render();                    // Render and present
};
```

**Key responsibilities:**
1. Creates D3D11 device and swap chain
2. Initializes ImGui context
3. Sets up ImGui backends (Win32 + D3D11)
4. Applies custom color scheme (Vape-style dark theme)
5. Handles frame rendering and buffer presentation

### Overlay Thread

The overlay runs on its own thread with:
- Win32 message pump for handling input
- ImGui frame rendering loop (60 FPS)
- Window repositioning to stay over Minecraft

### UI Rendering

The `RenderImGuiUI()` function creates the UI panels:

```cpp
static void RenderImGuiUI() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("COMBAT", &g_combatOpen)) {
        ImGui::Checkbox("Kill Aura", (bool*)&g_killAura);
        ImGui::End();
    }
    // ... more panels
}
```

Panels are:
1. **COMBAT** - Kill Aura toggle
2. **VISUAL** - ESP toggle
3. **MOVEMENT** - Auto Sprint toggle

## Building

### Using batch script (Windows):

```bash
cd cpp-system
build.bat
```

The build process:
1. Compiles all ImGui source files
2. Links D3D11 and DXGI libraries
3. Creates `build/esp_mod.dll`

### Using CMake (optional):

```bash
cd cpp-system
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## How It Works

### Initialization Phase

1. DLL is injected into Minecraft process
2. JVM is attached via JNI
3. Overlay thread is created
4. ImGuiOverlay initializes D3D11 resources
5. ImGui context is set up with custom styling

### Runtime Loop

```
Main Thread                    Overlay Thread
    |                              |
    ├─ Check INSERT key ────────────┤
    │  g_menuOpen toggle            │
    │                               ├─ Windows message pump
    │                               ├─ ImGui::NewFrame()
    │                               ├─ RenderImGuiUI()
    ├─ Tick active features         ├─ ImGui::Render()
    │  (KillAura, AutoSprint)       ├─ D3D11 render target
    │                               └─ Present (vsync)
    └─ Sleep(50ms)                  └─ Sleep(5ms)
```

### Input Handling

When **INSERT** is pressed:
1. `g_menuOpen` toggles between true/false
2. If opening: Remove `WS_EX_TRANSPARENT` from overlay window (accepts clicks)
3. If closing: Re-add `WS_EX_TRANSPARENT` (click-through)
4. Minecraft cursor is released/grabbed via JNI

ImGui handles all input within its context automatically.

## Customization

### Styling

Colors are defined in `ImGuiOverlay::Initialize()`:

```cpp
ImVec4* colors = style.Colors;
colors[ImGuiCol_WindowBg] = ImVec4(18/255.f, 18/255.f, 18/255.f, 0.8f);
colors[ImGuiCol_Button] = ImVec4(62/255.f, 128/255.f, 255/255.f, 0.6f);
```

Edit these values to customize colors.

### Adding UI Elements

To add a new checkbox to a panel:

```cpp
static bool g_newFeature = false;

// In RenderImGuiUI():
if (ImGui::Begin("COMBAT", &g_combatOpen)) {
    ImGui::Checkbox("Kill Aura", (bool*)&g_killAura);
    ImGui::Checkbox("New Feature", &g_newFeature);  // Add this
    ImGui::End();
}

// Then use g_newFeature in your logic
if(g_newFeature) {
    // Do something
}
```

### Adding New Panels

```cpp
static bool g_customPanelOpen = true;

// In RenderImGuiUI():
ImGui::SetNextWindowPos(ImVec2(640, 10), ImGuiCond_FirstUseEver);
ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

if (ImGui::Begin("CUSTOM", &g_customPanelOpen)) {
    // Add controls here
    ImGui::End();
}
```

## Known Limitations

1. **D3D11 Only** - Requires DirectX 11 (Minecraft Java Edition supports this)
2. **Overlay Window** - Uses separate swap chain, some games may not support overlays
3. **Window Positioning** - Overlay must stay on top of Minecraft window

## Troubleshooting

### DLL won't load
- Check that D3D11.dll and DXGI.dll are available on the system
- Verify Visual Studio runtime is installed

### UI doesn't appear
- Check console output for "[ImGui]" messages
- Verify INSERT key toggles menu state
- Ensure Minecraft window is found correctly

### Graphics glitches
- Try updating graphics drivers
- Check that D3D11 feature level 11.0 is supported
- Verify swap chain format is compatible with display

## Future Improvements

- [ ] Add ImGui text input for commands
- [ ] Slider controls for numeric parameters
- [ ] Color picker for customization
- [ ] Save/load UI layout
- [ ] Performance monitoring overlay
- [ ] Category collapsing/expanding

## References

- ImGui Documentation: https://github.com/ocornut/imgui
- DirectX 11 Documentation: https://learn.microsoft.com/en-us/windows/win32/directx
- Windows API: https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list
