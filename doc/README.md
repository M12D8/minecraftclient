# C++ Injection System for Minecraft

This system allows you to inject a C++ DLL into Minecraft to run side-by-side with your Fabric mod.

## Components

1.  **Injector (`injector.exe`)**: A Windows GUI utility that finds the Minecraft process and injects the DLL.
2.  **DLL (`esp_mod.dll`)**: A C++ library that attaches to the Minecraft JVM using JNI (Java Native Interface).

## New Structure

The project has been simplified to only contain the C++ system:
- `cpp-system/injector/`: Source code for the GUI injector.
- `cpp-system/dll/`: Source code for the C++ DLL.
- `cpp-system/deps/`: JNI headers for compilation (portable).
- `cpp-system/build/`: Compiled binaries.

## How to build

1.  Open a **Developer Command Prompt for Visual Studio**.
2.  Navigate to `Mincraft/cpp-system`.
3.  Run `build.bat`.

The compiled files will be in the `build/` folder inside `cpp-system`.

## How to use

1.  Start Minecraft.
2.  Run `Mincraft/cpp-system/build/injector.exe`.
3.  The UI will appear. Ensure the process name (default: `Minecraft.exe`) is correct.
4.  Click **INJECT DLL**.
5.  A message box will confirm if the injection was successful.
