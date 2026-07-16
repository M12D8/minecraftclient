#include "ingame_overlay.h"
#include <iostream>

// Try to include OpenGL3 backend if available
#ifdef IMGUI_IMPL_OPENGL3_H
    #include "../deps/imgui/backends/imgui_impl_opengl3.h"
#else
    // Fallback declarations if header not available
    namespace {
        void ImGui_ImplOpenGL3_NewFrame() {}
        void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* data) {}
        bool ImGui_ImplOpenGL3_Init(const char* glsl_version) { return true; }
        void ImGui_ImplOpenGL3_Shutdown() {}
    }
#endif

// Static members
bool InGameHUD::m_initialized = false;
ImGuiContext* InGameHUD::m_imguiContext = nullptr;
bool InGameHUD::m_openglSetup = false;

InGameHUD* g_inGameHUD = nullptr;

// Forward declarations
extern void RenderImGuiUI();
extern volatile bool g_menuOpen;

bool InGameHUD::Initialize() {
    if (m_initialized) return true;

    try {
        // Create ImGui context
        if (!ImGui::GetCurrentContext()) {
            m_imguiContext = ImGui::CreateContext();
        } else {
            m_imguiContext = ImGui::GetCurrentContext();
        }

        if (!m_imguiContext) {
            std::cout << "[HUD] Failed to create ImGui context\n";
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();

        // Setup style
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 1.0f;

        // Apply Vape-style colors
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(18 / 255.f, 18 / 255.f, 18 / 255.f, 0.8f);
        colors[ImGuiCol_TitleBg] = ImVec4(26 / 255.f, 26 / 255.f, 26 / 255.f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(26 / 255.f, 26 / 255.f, 26 / 255.f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(62 / 255.f, 128 / 255.f, 255 / 255.f, 0.6f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(62 / 255.f, 128 / 255.f, 255 / 255.f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(62 / 255.f, 128 / 255.f, 255 / 255.f, 1.0f);
        colors[ImGuiCol_Separator] = ImVec4(38 / 255.f, 38 / 255.f, 38 / 255.f, 1.0f);
        colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        colors[ImGuiCol_TextDisabled] = ImVec4(110 / 255.f, 110 / 255.f, 115 / 255.f, 1.0f);

        m_initialized = true;
        std::cout << "[HUD] Initialized successfully\n";
        return true;
    } catch (const std::exception& e) {
        std::cout << "[HUD] Exception during initialize: " << e.what() << "\n";
        return false;
    }
}

void InGameHUD::Shutdown() {
    if (!m_initialized) return;

    try {
        if (m_openglSetup) {
            ImGui_ImplOpenGL3_Shutdown();
            m_openglSetup = false;
        }

        if (m_imguiContext) {
            ImGui::DestroyContext(m_imguiContext);
            m_imguiContext = nullptr;
        }

        m_initialized = false;
        std::cout << "[HUD] Shutdown\n";
    } catch (const std::exception& e) {
        std::cout << "[HUD] Exception during shutdown: " << e.what() << "\n";
    }
}

bool InGameHUD::SetupImGuiOpenGL() {
    if (m_openglSetup) return true;

    try {
        if (ImGui_ImplOpenGL3_Init("#version 120")) {
            m_openglSetup = true;
            std::cout << "[HUD] OpenGL3 backend initialized\n";
            return true;
        } else {
            std::cout << "[HUD] Failed to initialize OpenGL3 backend\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "[HUD] Exception in SetupImGuiOpenGL: " << e.what() << "\n";
        return false;
    }
}

void InGameHUD::Render() {
    if (!m_initialized || !g_menuOpen) return;

    try {
        if (!m_openglSetup) {
            SetupImGuiOpenGL();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        RenderImGuiUI();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    } catch (const std::exception& e) {
        std::cout << "[HUD] Exception during render: " << e.what() << "\n";
    }
}
