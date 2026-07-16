#pragma once

#include <windows.h>
#include <GL/gl.h>
#include "../deps/imgui/imgui.h"

// In-game HUD overlay renderer
class InGameHUD {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized() { return m_initialized; }
    
    // Call this from render hook to render the HUD
    static void Render();
    
    // Setup ImGui for OpenGL rendering
    static bool SetupImGuiOpenGL();
    
    // Get current context
    static ImGuiContext* GetContext() { return m_imguiContext; }

private:
    static bool m_initialized;
    static ImGuiContext* m_imguiContext;
    static bool m_openglSetup;
};

// Global HUD instance
extern InGameHUD* g_inGameHUD;
