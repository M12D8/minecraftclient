#pragma once

#include <windows.h>
#include <GL/gl.h>
#include "../deps/imgui/imgui.h"

// OpenGL function pointers
typedef void(__stdcall* glSwapBuffers_t)(HDC hDc);
typedef void(__stdcall* glClear_t)(GLbitfield mask);

// Our hook manager
class OpenGLHook {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized() { return m_initialized; }
    
    // Called when OpenGL is ready to render
    static void OnRenderFrame();
    
    // Setup ImGui for OpenGL
    static bool SetupImGuiOpenGL();

private:
    static bool m_initialized;
    static HMODULE m_glModule;
    
    // Original function pointers
    static glSwapBuffers_t m_originalSwapBuffers;
    static glClear_t m_originalGlClear;
    
    // Hook functions
    static void __stdcall HookedSwapBuffers(HDC hDc);
    static void __stdcall HookedGlClear(GLbitfield mask);
};

// Detour library declarations
extern "C" {
    typedef int(__stdcall* DetourAttach_t)(PVOID* ppPointer, PVOID pDetour);
    typedef int(__stdcall* DetourDetach_t)(PVOID* ppPointer, PVOID pDetour);
    typedef int(__stdcall* DetourTransactionBegin_t)();
    typedef int(__stdcall* DetourTransactionCommit_t)();
}
