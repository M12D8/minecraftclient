# ImGui Quick Start Guide

## Building the Project

```bash
cd cpp-system
build.bat
```

Output: `build/esp_mod.dll`

## Running

1. Start Minecraft
2. Run `build/injector.exe`
3. Click "INJECT DLL"
4. Press **INSERT** in-game to open/close menu
5. Press **END** to unload DLL

## Modifying the UI

### Change Panel Position

In `main_imgui.cpp`, `RenderImGuiUI()` function:

```cpp
ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);  // X=10, Y=10
```

### Add a New Toggle

1. Declare the variable:
```cpp
static volatile bool g_myFeature = false;
```

2. Add to UI:
```cpp
ImGui::Checkbox("My Feature", (bool*)&g_myFeature);
```

3. Use in logic (around line 512 in MainThread):
```cpp
if(g_myFeature) {
    // Do something
}
```

### Add a New Panel

1. Declare state:
```cpp
static bool g_myPanelOpen = true;
```

2. Add to UI:
```cpp
ImGui::SetNextWindowPos(ImVec2(640, 10), ImGuiCond_FirstUseEver);
ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

if (ImGui::Begin("MY PANEL", &g_myPanelOpen)) {
    ImGui::Checkbox("Option 1", &someVar);
    ImGui::Text("Some text");
    ImGui::End();
}
```

### Change Colors

In `imgui_overlay.cpp`, `ImGuiOverlay::Initialize()`:

```cpp
ImVec4* colors = style.Colors;
colors[ImGuiCol_WindowBg] = ImVec4(R/255.f, G/255.f, B/255.f, A);
// R, G, B = 0-255, A = 0.0-1.0 (opacity)
```

Common color slots:
- `ImGuiCol_WindowBg` - Panel background
- `ImGuiCol_TitleBg` - Title bar
- `ImGuiCol_Button` - Button default
- `ImGuiCol_ButtonHovered` - Button on hover
- `ImGuiCol_Text` - Text color
- `ImGuiCol_Separator` - Line separators

### Common ImGui Widgets

```cpp
// Checkbox
ImGui::Checkbox("Label", &bool_var);

// Text
ImGui::Text("Some text %d", variable);

// Button
if (ImGui::Button("Click Me")) {
    // Code when clicked
}

// Slider
ImGui::SliderFloat("Speed", &float_var, 0.0f, 10.0f);

// Combo (dropdown)
static int selected = 0;
ImGui::Combo("Mode", &selected, "Option1\0Option2\0Option3\0");

// Input text
static char buffer[256] = "";
ImGui::InputText("Name", buffer, IM_ARRAYSIZE(buffer));

// Separator line
ImGui::Separator();

// Spacing
ImGui::Spacing();
```

## File Structure

```
cpp-system/
├── dll/
│   ├── main.cpp              (Old GDI version - keep for reference)
│   ├── main_imgui.cpp        (NEW - ImGui version)
│   ├── imgui_overlay.h       (ImGui wrapper class)
│   └── imgui_overlay.cpp     (ImGui implementation)
├── deps/
│   ├── imgui/                (ImGui source code)
│   ├── include/              (JNI headers)
│   └── ...
├── build.bat                 (Build script)
├── CMakeLists.txt           (CMake build file)
└── IMGUI_IMPLEMENTATION.md   (This document)
```

## Common Issues

| Issue | Solution |
|-------|----------|
| DLL won't load | Check DirectX 11 is installed, update GPU drivers |
| UI doesn't appear | Press INSERT, check console for errors |
| Overlay flickering | Normal at 60 FPS, can reduce with VSync |
| Crashes on startup | Check all ImGui backends are compiled |

## Performance

- Overlay runs at 60 FPS (VSync enabled)
- Direct3D 11 hardware accelerated
- Minimal CPU usage in hidden state (click-through)
- ~2-5 MB memory overhead

## Next Steps

1. **Add more features** - Implement new JNI calls for game interactions
2. **Enhance UI** - Add sliders, input fields, color pickers
3. **Add settings** - Save/load configuration
4. **Performance monitoring** - Display FPS, memory usage
5. **Logging window** - Show mod activity in-game

## Resources

- ImGui Examples: `https://github.com/ocornut/imgui/tree/master/examples`
- DirectX 11 Tutorial: `https://www.rastertek.com/dx11tut03.html`
- JNI Reference: `https://docs.oracle.com/javase/8/docs/technotes/guides/jni/`
