#include "opengl_hook.h"
#include <iostream>
#include "../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../deps/imgui/imgui.h"

// Static member initialization
bool OpenGLHook::m_initialized = false;
HMODULE OpenGLHook::m_glModule = nullptr;
glSwapBuffers_t OpenGLHook::m_originalSwapBuffers = nullptr;
glClear_t OpenGLHook::m_originalGlClear = nullptr;

// Forward declare render function
extern void RenderImGuiUI();
extern volatile bool g_menuOpen;

bool OpenGLHook::Initialize() {
    if (m_initialized) return true;

    // Load OpenGL module
    m_glModule = GetModuleHandleA("opengl32.dll");
    if (!m_glModule) {
        m_glModule = LoadLibraryA("opengl32.dll");
        if (!m_glModule) {
            std::cout << "[OpenGL Hook] Failed to load opengl32.dll\n";
            return false;
        }
    }

    // Get original function pointers
    m_originalSwapBuffers = (glSwapBuffers_t)GetProcAddress(m_glModule, "SwapBuffers");
    if (!m_originalSwapBuffers) {
        std::cout << "[OpenGL Hook] Failed to get SwapBuffers\n";
        return false;
    }

    std::cout << "[OpenGL Hook] Initialized\n";
    m_initialized = true;
    return true;
}

void OpenGLHook::Shutdown() {
    if (!m_initialized) return;

    // Detach hooks if needed
    m_initialized = false;
    m_glModule = nullptr;
}

bool OpenGLHook::SetupImGuiOpenGL() {
    try {
        // Setup ImGui context (if not already done)
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
        }

        ImGuiIO& io = ImGui::GetIO();

        // Setup style
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;

        // Apply Vape-style colors
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(18 / 255.f, 18 / 255.f, 18 / 255.f, 0.8f);
        colors[ImGuiCol_TitleBg] = ImVec4(26 / 255.f, 26 / 255.f, 26 / 255.f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(26 / 255.f, 26 / 255.f, 26 / 255.f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(62 / 255.f, 128 / 255.f, 255 / 255.f, 0.6f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(62 / 255.f, 128 / 255.f, 255 / 255.f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(62 / 255.f, 128 / 255.f, 255 / 255.f, 1.0f);
        colors[ImGuiCol_Separator] = ImVec4(38 / 255.f, 38 / 255.f, 38 / 255.f, 1.0f);

        // Setup OpenGL3 backend
        if (!ImGui_ImplOpenGL3_Init("#version 120")) {
            std::cout << "[OpenGL Hook] Failed to initialize OpenGL3 backend\n";
            return false;
        }

        std::cout << "[OpenGL Hook] ImGui OpenGL3 backend initialized\n";
        return true;
    } catch (const std::exception& e) {
        std::cout << "[OpenGL Hook] Exception in SetupImGuiOpenGL: " << e.what() << "\n";
        return false;
    }
}

void __stdcall OpenGLHook::HookedSwapBuffers(HDC hDc) {
    // Render ImGui before swapping buffers
    if (g_menuOpen) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        
        RenderImGuiUI();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Call original swap buffers
    if (m_originalSwapBuffers) {
        m_originalSwapBuffers(hDc);
    }
}

void __stdcall OpenGLHook::HookedGlClear(GLbitfield mask) {
    // Call original glClear
    // This hook is for future extensions
}

void OpenGLHook::OnRenderFrame() {
    // This could be called from a hooked render function if we intercept earlier in the pipeline
    if (!m_initialized || !g_menuOpen) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    RenderImGuiUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
