#include "imgui_overlay.h"
#include <dxgi.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

ImGuiOverlay* g_imguiOverlay = nullptr;

ImGuiOverlay::ImGuiOverlay()
    : m_device(nullptr), m_context(nullptr), m_swapChain(nullptr), 
      m_renderTargetView(nullptr), m_initialized(false) {
}

ImGuiOverlay::~ImGuiOverlay() {
    Shutdown();
}

bool ImGuiOverlay::Initialize(HWND hwnd) {
    if (m_initialized) return true;

    // Create D3D11 device and swap chain
    if (!CreateDeviceAndSwapchain(hwnd)) {
        std::cout << "[ImGui] Failed to create D3D11 device\n";
        return false;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
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

    // Setup backends
    if (!ImGui_ImplWin32_Init(hwnd)) {
        std::cout << "[ImGui] Failed to initialize Win32 backend\n";
        ImGui::DestroyContext();
        CleanupDeviceAndSwapchain();
        return false;
    }

    if (!ImGui_ImplDX11_Init(m_device, m_context)) {
        std::cout << "[ImGui] Failed to initialize D3D11 backend\n";
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceAndSwapchain();
        return false;
    }

    m_initialized = true;
    std::cout << "[ImGui] Initialized successfully\n";
    return true;
}

void ImGuiOverlay::Shutdown() {
    if (!m_initialized) return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceAndSwapchain();
    m_initialized = false;
}

bool ImGuiOverlay::CreateDeviceAndSwapchain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevels, 1, D3D11_SDK_VERSION, &sd, &m_swapChain,
        &m_device, &featureLevel, &m_context))) {
        return false;
    }

    // Create render target view
    ID3D11Texture2D* backBuffer = nullptr;
    if (FAILED(m_swapChain->GetBuffer(0, IID_ID3D11Texture2D, (LPVOID*)&backBuffer))) {
        return false;
    }

    if (FAILED(m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView))) {
        backBuffer->Release();
        return false;
    }

    backBuffer->Release();
    m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

    // Setup viewport
    D3D11_VIEWPORT vp;
    vp.Width = 0;
    vp.Height = 0;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_context->RSSetViewports(1, &vp);

    return true;
}

void ImGuiOverlay::CleanupDeviceAndSwapchain() {
    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }
    if (m_swapChain) {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
}

void ImGuiOverlay::NewFrame() {
    if (!m_initialized) return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiOverlay::Render() {
    if (!m_initialized || !m_swapChain) return;

    ImGui::Render();

    const ImVec4 clearColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    float clearColorVal[] = {
        clearColor.x * clearColor.w,
        clearColor.y * clearColor.w,
        clearColor.z * clearColor.w,
        0.0f
    };

    m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    m_context->ClearRenderTargetView(m_renderTargetView, clearColorVal);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_swapChain->Present(1, 0); // V-sync enabled
}

LRESULT CALLBACK ImGuiOverlayProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_imguiOverlay && g_imguiOverlay->GetDevice()) {
            // Resize D3D11 resources
            // This would be handled by ImGui D3D11 backend
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU) // Disable ALT menu
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
