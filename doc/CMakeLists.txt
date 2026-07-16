cmake_minimum_required(VERSION 3.10)
project(EspMod)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui source files
set(IMGUI_SOURCES
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/backends/imgui_impl_dx11.cpp
    deps/imgui/backends/imgui_impl_win32.cpp
)

# DLL target
add_library(esp_mod SHARED
    dll/main_imgui.cpp
    dll/imgui_overlay.cpp
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(esp_mod PRIVATE
    deps/include
    deps/include/win32
    deps
)

# Link libraries
target_link_libraries(esp_mod PRIVATE
    user32.lib
    gdi32.lib
    d3d11.lib
    dxgi.lib
)

# Set output directory
set_target_properties(esp_mod PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
)

# Injector executable
add_executable(injector
    injector/main.cpp
)

target_link_libraries(injector PRIVATE
    user32.lib
    advapi32.lib
    dwmapi.lib
    comdlg32.lib
    shell32.lib
    gdi32.lib
)

set_target_properties(injector PROPERTIES
    SUBSYSTEM WINDOWS
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
)
