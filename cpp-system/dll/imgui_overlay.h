#pragma once

#include <windows.h>
#include <d3d11.h>
#include "../deps/imgui/imgui.h"
#include "../deps/imgui/backends/imgui_impl_win32.h"
#include "../deps/imgui/backends/imgui_impl_dx11.h"

// Forward declare the ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ImGui overlay state
class ImGuiOverlay {
public:
    ImGuiOverlay();
    ~ImGuiOverlay();

    bool Initialize(HWND hwnd);
    void Shutdown();
    void NewFrame();
    void Render();
    bool IsInitialized() const { return m_initialized; }

    // Accessors for D3D11 resources
    ID3D11Device* GetDevice() const { return m_device; }
    ID3D11DeviceContext* GetContext() const { return m_context; }

private:
    bool CreateDeviceAndSwapchain(HWND hwnd);
    void CleanupDeviceAndSwapchain();

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    bool m_initialized = false;
};

// Global ImGui overlay instance
extern ImGuiOverlay* g_imguiOverlay;

// Window procedure for ImGui overlay
extern LRESULT CALLBACK ImGuiOverlayProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
