/*
 * esp_mod.dll  -  External overlay (Win32 layered window) + JNI cheat modules
 *  WITH ImGui rendering backend
 *
 *  INSERT  = open / close the ClickGUI overlay
 *  END     = unload DLL
 *
 *  The overlay window is positioned exactly over Minecraft's GLFW window,
 *  is WS_EX_TRANSPARENT when closed (fully click-through) and accepts
 *  mouse input only when the menu is open.  This is identical to how most
 *  real external clients (and Discord / GeForce overlay) work.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <jni.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "../deps/imgui/imgui.h"
#include "imgui_overlay.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ─── JVM globals ─────────────────────────────────────────────────────────────
static JavaVM*  g_jvm         = nullptr;
static jobject  g_classLoader = nullptr;
static HMODULE  g_hModule     = nullptr;

// ─── Module state ─────────────────────────────────────────────────────────────
static volatile bool g_killAura   = false;
static volatile bool g_esp        = false;
static volatile bool g_autoSprint = false;
static volatile bool g_menuOpen   = false;

// ─── ImGui Panel state ────────────────────────────────────────────────────────
static bool g_combatOpen = true;
static bool g_visualOpen = true;
static bool g_movementOpen = true;

// ─── Panels ──────────────────────────────────────────────────────────────────
struct ModEntry { const char* name; volatile bool* on; };
struct Panel {
    const char* cat;
    ModEntry   mods[4];
    int        count;
    bool*      open;
};

static Panel g_panels[3] = {
    { "COMBAT",   { {"Kill Aura",   &g_killAura}   }, 1, &g_combatOpen },
    { "VISUAL",   { {"ESP",         &g_esp}        }, 1, &g_visualOpen },
    { "MOVEMENT", { {"Auto Sprint", &g_autoSprint} }, 1, &g_movementOpen },
};

// ─── Overlay window globals ───────────────────────────────────────────────────
static HWND  g_ovHwnd = nullptr;

// ─── MC JNI refs ─────────────────────────────────────────────────────────────
struct MCRefs {
    jclass Minecraft=nullptr, ClientLevel=nullptr, LocalPlayer=nullptr;
    jclass Entity=nullptr, LivingEntity=nullptr, PlayerEntity=nullptr;
    jclass MultiPlayerGameMode=nullptr, InteractionHand=nullptr;
    jclass MouseHandler=nullptr, Iterable=nullptr, Iterator=nullptr;
    jfieldID fPlayer=nullptr, fLevel=nullptr, fGameMode=nullptr;
    jfieldID fMouseHandler=nullptr, fMainHand=nullptr;
    jmethodID mGetInstance=nullptr, mEntities=nullptr;
    jmethodID mIsAlive=nullptr, mDistanceTo=nullptr;
    jmethodID mAttack=nullptr, mSwing=nullptr, mSetSprinting=nullptr;
    jmethodID mIterator=nullptr, mHasNext=nullptr, mNext=nullptr;
    jmethodID mReleaseMouse=nullptr, mGrabMouse=nullptr;
    bool valid=false;
};
static MCRefs g_mc;

// ─── JNI helpers ─────────────────────────────────────────────────────────────
static JNIEnv* GetEnv() {
    if (!g_jvm) return nullptr;
    JNIEnv* env=nullptr;
    if (g_jvm->GetEnv((void**)&env,JNI_VERSION_1_8)==JNI_EDETACHED)
        g_jvm->AttachCurrentThread((void**)&env,nullptr);
    return env;
}

static void ClearEx(JNIEnv* env) {
    if(env&&env->ExceptionCheck()) env->ExceptionClear();
}

// ─── Reference initialization ─────────────────────────────────────────────────
static void InitRefs(JNIEnv* env) {
    auto findClass = [env](const char* name) {
        jclass c = env->FindClass(name);
        if(env->ExceptionCheck()) { ClearEx(env); return (jclass)nullptr; }
        return c;
    };

    g_mc.Minecraft         = findClass("net/minecraft/client/Minecraft");
    g_mc.ClientLevel       = findClass("net/minecraft/client/multiplayer/ClientLevel");
    g_mc.LocalPlayer       = findClass("net/minecraft/client/player/LocalPlayer");
    g_mc.Entity            = findClass("net/minecraft/world/entity/Entity");
    g_mc.LivingEntity      = findClass("net/minecraft/world/entity/LivingEntity");
    g_mc.PlayerEntity      = findClass("net/minecraft/world/entity/player/Player");
    g_mc.MultiPlayerGameMode = findClass("net/minecraft/client/multiplayer/MultiPlayerGameMode");
    g_mc.InteractionHand   = findClass("net/minecraft/world/InteractionHand");
    g_mc.MouseHandler      = findClass("net/minecraft/client/MouseHandler");
    g_mc.Iterable          = findClass("java/lang/Iterable");
    g_mc.Iterator          = findClass("java/util/Iterator");

    if(!g_mc.Minecraft) return;

    // Find field IDs
    g_mc.fPlayer = env->GetFieldID(g_mc.Minecraft, "player", "Lnet/minecraft/client/player/LocalPlayer;");
    g_mc.fLevel = env->GetFieldID(g_mc.Minecraft, "level", "Lnet/minecraft/client/multiplayer/ClientLevel;");
    g_mc.fGameMode = env->GetFieldID(g_mc.Minecraft, "gameMode", "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;");
    g_mc.fMouseHandler = env->GetFieldID(g_mc.Minecraft, "mouseHandler", "Lnet/minecraft/client/MouseHandler;");
    g_mc.fMainHand = env->GetStaticFieldID(g_mc.InteractionHand, "MAIN_HAND", "Lnet/minecraft/world/InteractionHand;");

    // Find method IDs
    g_mc.mGetInstance = env->GetStaticMethodID(g_mc.Minecraft, "getInstance", "()Lnet/minecraft/client/Minecraft;");
    g_mc.mEntities = env->GetMethodID(g_mc.ClientLevel, "entitiesForRendering", "()Ljava/lang/Iterable;");
    g_mc.mIsAlive = env->GetMethodID(g_mc.LivingEntity, "isAlive", "()Z");
    g_mc.mDistanceTo = env->GetMethodID(g_mc.Entity, "distanceTo", "(Lnet/minecraft/world/entity/Entity;)F");
    g_mc.mAttack = env->GetMethodID(g_mc.MultiPlayerGameMode, "attack", "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/entity/Entity;)V");
    g_mc.mSwing = env->GetMethodID(g_mc.MultiPlayerGameMode, "swing", "(Lnet/minecraft/world/InteractionHand;)V");
    g_mc.mSetSprinting = env->GetMethodID(g_mc.LocalPlayer, "setSprinting", "(Z)V");
    g_mc.mIterator = env->GetMethodID(g_mc.Iterable, "iterator", "()Ljava/util/Iterator;");
    g_mc.mHasNext = env->GetMethodID(g_mc.Iterator, "hasNext", "()Z");
    g_mc.mNext = env->GetMethodID(g_mc.Iterator, "next", "()Ljava/lang/Object;");
    g_mc.mReleaseMouse = env->GetMethodID(g_mc.MouseHandler, "releaseMouse", "()V");
    g_mc.mGrabMouse = env->GetMethodID(g_mc.MouseHandler, "grabMouse", "()V");

    ClearEx(env);
    g_mc.valid = (g_mc.Minecraft && g_mc.mGetInstance);
    std::cout << "[DLL] MC refs " << (g_mc.valid ? "OK" : "FAILED") << "\n";
}

// ─── KillAura logic ───────────────────────────────────────────────────────────
static void TickKillAura(JNIEnv* env) {
    if(!g_mc.valid||!g_mc.mGetInstance) return;
    jobject mc=env->CallStaticObjectMethod(g_mc.Minecraft,g_mc.mGetInstance);
    if(!mc||env->ExceptionCheck()){ClearEx(env);return;}
    jobject player =env->GetObjectField(mc,g_mc.fPlayer);
    jobject level =env->GetObjectField(mc,g_mc.fLevel);
    jobject gm    =env->GetObjectField(mc,g_mc.fGameMode);
    if(!player||!level||!gm){env->DeleteLocalRef(mc);return;}
    jobject ents=env->CallObjectMethod(level,g_mc.mEntities);
    if(!ents||env->ExceptionCheck()){ClearEx(env);env->DeleteLocalRef(mc);return;}
    jobject mh=env->GetStaticObjectField(g_mc.InteractionHand,g_mc.fMainHand);
    const float R=4.5f; jobject best=nullptr; float bd=R+1.f;
    jobject it=env->CallObjectMethod(ents,g_mc.mIterator);
    while(env->CallBooleanMethod(it,g_mc.mHasNext)){
        jobject e=env->CallObjectMethod(it,g_mc.mNext);
        if(!env->IsInstanceOf(e,g_mc.LivingEntity)||env->IsInstanceOf(e,g_mc.PlayerEntity)||
           !env->CallBooleanMethod(e,g_mc.mIsAlive)){env->DeleteLocalRef(e);continue;}
        float d=env->CallFloatMethod(player,g_mc.mDistanceTo,e);
        if(env->ExceptionCheck()){ClearEx(env);env->DeleteLocalRef(e);continue;}
        if(d<bd){if(best)env->DeleteLocalRef(best);best=e;bd=d;}else env->DeleteLocalRef(e);
    }
    env->DeleteLocalRef(it); env->DeleteLocalRef(ents);
    if(best&&bd<=R){
        env->CallVoidMethod(gm,g_mc.mAttack,player,best); ClearEx(env);
        env->CallVoidMethod(gm,g_mc.mSwing,mh);       ClearEx(env);
    }
    if(best) env->DeleteLocalRef(best);
    if(mh)   env->DeleteLocalRef(mh);
    env->DeleteLocalRef(player); env->DeleteLocalRef(level);
    env->DeleteLocalRef(gm);     env->DeleteLocalRef(mc);
}

static void TickAutoSprint(JNIEnv* env) {
    if(!g_mc.valid||!g_mc.mSetSprinting) return;
    jobject mc=env->CallStaticObjectMethod(g_mc.Minecraft,g_mc.mGetInstance);
    if(!mc||env->ExceptionCheck()){ClearEx(env);return;}
    jobject p=env->GetObjectField(mc,g_mc.fPlayer);
    if(p){env->CallVoidMethod(p,g_mc.mSetSprinting,JNI_TRUE);ClearEx(env);env->DeleteLocalRef(p);}
    env->DeleteLocalRef(mc);
}

// ─── Find Minecraft GLFW window (same process) ───────────────────────────────
static HWND FindMcWindow() {
    struct Ctx { DWORD pid; HWND found; };
    Ctx ctx{ GetCurrentProcessId(), nullptr };
    EnumWindows([](HWND hw, LPARAM lp) -> BOOL {
        auto* c = (Ctx*)lp;
        DWORD pid=0; GetWindowThreadProcessId(hw,&pid);
        if(pid==c->pid && IsWindowVisible(hw) && GetWindow(hw,GW_OWNER)==nullptr) {
            c->found=hw; return FALSE;
        }
        return TRUE;
    },(LPARAM)&ctx);
    return ctx.found;
}

// ─── ImGui UI rendering ───────────────────────────────────────────────────────
static void RenderImGuiUI() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("COMBAT", &g_combatOpen, ImGuiWindowFlags_NoMove)) {
        ImGui::Checkbox("Kill Aura", (bool*)&g_killAura);
        ImGui::End();
    }

    ImGui::SetNextWindowPos(ImVec2(220, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("VISUAL", &g_visualOpen, ImGuiWindowFlags_NoMove)) {
        ImGui::Checkbox("ESP", (bool*)&g_esp);
        ImGui::End();
    }

    ImGui::SetNextWindowPos(ImVec2(430, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("MOVEMENT", &g_movementOpen, ImGuiWindowFlags_NoMove)) {
        ImGui::Checkbox("Auto Sprint", (bool*)&g_autoSprint);
        ImGui::End();
    }
}

// ─── Overlay window procedure ─────────────────────────────────────────────────
static LRESULT CALLBACK OverlayProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    // Forward to ImGui handler
    if (ImGui_ImplWin32_WndProcHandler(hw, msg, wp, lp))
        return true;

    switch(msg){
    case WM_CREATE:
        SetTimer(hw, 1, 16, nullptr); // 60 fps repaint timer
        break;

    case WM_TIMER: {
        // Reposition over Minecraft window each tick
        HWND mc=FindMcWindow();
        if(mc&&mc!=hw){
            RECT rc; GetWindowRect(mc,&rc);
            int W=rc.right-rc.left, H=rc.bottom-rc.top;
            SetWindowPos(hw,HWND_TOPMOST,rc.left,rc.top,W,H,
                         SWP_NOACTIVATE|SWP_NOOWNERZORDER);
        }
        break;
    }

    case WM_PAINT: {
        ValidateRect(hw, nullptr);
        break;
    }

    case WM_DESTROY:
        KillTimer(hw,1);
        break;
    }
    return DefWindowProcW(hw,msg,wp,lp);
}

// ─── Overlay thread ───────────────────────────────────────────────────────────
static DWORD WINAPI OverlayThread(LPVOID) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc  =OverlayProc;
    wc.hInstance    =g_hModule;
    wc.lpszClassName=L"EspOverlay";
    wc.hCursor      =LoadCursor(nullptr,IDC_ARROW);
    RegisterClassExW(&wc);

    HWND mc=FindMcWindow();
    RECT rc{}; if(mc) GetWindowRect(mc,&rc);
    int W=rc.right-rc.left, H=rc.bottom-rc.top;
    if(!W) W=800; if(!H) H=600;

    // Create the overlay window
    g_ovHwnd=CreateWindowExW(
        WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,
        L"EspOverlay",L"",WS_POPUP,
        rc.left,rc.top,W,H,
        nullptr,nullptr,g_hModule,nullptr);

    if(!g_ovHwnd) {
        std::cout << "[DLL] Failed to create overlay window\n";
        return 1;
    }

    // Initialize ImGui overlay
    g_imguiOverlay = new ImGuiOverlay();
    if (!g_imguiOverlay->Initialize(g_ovHwnd)) {
        std::cout << "[DLL] Failed to initialize ImGui overlay\n";
        delete g_imguiOverlay;
        g_imguiOverlay = nullptr;
        DestroyWindow(g_ovHwnd);
        return 1;
    }

    SetLayeredWindowAttributes(g_ovHwnd,RGB(0,0,0),0,LWA_COLORKEY);
    ShowWindow(g_ovHwnd,SW_SHOW);

    // Main rendering loop - ALWAYS render to keep overlay responsive
    bool running = true;
    MSG m{};
    while(running) {
        // Reposition over Minecraft window EVERY frame
        HWND mc = FindMcWindow();
        if(mc && mc != g_ovHwnd) {
            RECT rc;
            GetWindowRect(mc, &rc);
            int W = rc.right - rc.left, H = rc.bottom - rc.top;
            SetWindowPos(g_ovHwnd, HWND_TOPMOST, rc.left, rc.top, W, H,
                         SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }

        // Handle messages without blocking
        while(PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
            if(m.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&m);
            DispatchMessage(&m);
        }

        // ALWAYS render the frame to keep overlay responsive
        if(g_imguiOverlay->IsInitialized()) {
            g_imguiOverlay->NewFrame();
            
            // Only draw UI if menu is open
            if(g_menuOpen) {
                RenderImGuiUI();
            }
            
            ImGui::EndFrame();
            g_imguiOverlay->Render();
        }
        
        Sleep(16); // 60 FPS
    }

    // Cleanup
    if(g_imguiOverlay) {
        g_imguiOverlay->Shutdown();
        delete g_imguiOverlay;
        g_imguiOverlay = nullptr;
    }

    if(g_ovHwnd) {
        DestroyWindow(g_ovHwnd);
        g_ovHwnd = nullptr;
    }

    return 0;
}

// ─── Main DLL thread ──────────────────────────────────────────────────────────
static DWORD WINAPI MainThread(LPVOID) {
    AllocConsole();
    FILE* f=nullptr; freopen_s(&f,"CONOUT$","w",stdout);

    std::cout<<"================================\n"
               "  ESP Mod  -  Injected!\n"
               "  INSERT = open/close menu\n"
               "  END    = unload\n"
               "================================\n";

    // Attach JVM
    HMODULE hJvm=GetModuleHandleA("jvm.dll");
    typedef jint(JNICALL* FnGetVMs)(JavaVM**,jsize,jsize*);
    auto fn=hJvm?(FnGetVMs)GetProcAddress(hJvm,"JNI_GetCreatedJavaVMs"):nullptr;
    jsize n=0;
    if(!fn||fn(&g_jvm,1,&n)!=JNI_OK||n==0)
        {std::cout<<"[DLL] JVM not found\n"; goto Cleanup;}
    {
        JNIEnv* env=nullptr;
        if(g_jvm->AttachCurrentThread((void**)&env,nullptr)!=JNI_OK)
            {std::cout<<"[DLL] Attach failed\n"; goto Cleanup;}
        std::cout<<"[DLL] Attached to JVM\n";
        InitRefs(env);
    }

    // Start overlay on its own thread (needs a Win32 message loop)
    CreateThread(nullptr,0,OverlayThread,nullptr,0,nullptr);
    Sleep(100); // let overlay window create

    // Key + tick loop
    {
        bool prevIns=false;
        while(true){
            if(GetAsyncKeyState(VK_END)&1) break;

            bool curIns=(GetAsyncKeyState(VK_INSERT)&0x8000)!=0;
            if(curIns&&!prevIns){
                g_menuOpen=!g_menuOpen;
                std::cout<<"[DLL] Menu: "<<(g_menuOpen?"OPEN":"CLOSED")<<"\n";

                if(g_ovHwnd){
                    LONG_PTR ex=GetWindowLongPtrW(g_ovHwnd,GWL_EXSTYLE);
                    if(g_menuOpen){
                        // Remove WS_EX_TRANSPARENT so the window receives clicks
                        SetWindowLongPtrW(g_ovHwnd,GWL_EXSTYLE,
                            ex & ~WS_EX_TRANSPARENT);
                        // Release Minecraft's cursor via JNI
                        JNIEnv* env=GetEnv();
                        if(env&&g_mc.fMouseHandler&&g_mc.mReleaseMouse){
                            jobject mc=env->CallStaticObjectMethod(g_mc.Minecraft,g_mc.mGetInstance);
                            if(mc&&!env->ExceptionCheck()){
                                jobject mh=env->GetObjectField(mc,g_mc.fMouseHandler);
                                if(mh){env->CallVoidMethod(mh,g_mc.mReleaseMouse);ClearEx(env);env->DeleteLocalRef(mh);}
                                env->DeleteLocalRef(mc);
                            } else ClearEx(env);
                        }
                    } else {
                        // Re-add WS_EX_TRANSPARENT (click-through when menu is closed)
                        SetWindowLongPtrW(g_ovHwnd,GWL_EXSTYLE,
                            ex | WS_EX_TRANSPARENT);
                        // Grab MC cursor back
                        JNIEnv* env=GetEnv();
                        if(env&&g_mc.fMouseHandler&&g_mc.mGrabMouse){
                            jobject mc=env->CallStaticObjectMethod(g_mc.Minecraft,g_mc.mGetInstance);
                            if(mc&&!env->ExceptionCheck()){
                                jobject mh=env->GetObjectField(mc,g_mc.fMouseHandler);
                                if(mh){env->CallVoidMethod(mh,g_mc.mGrabMouse);ClearEx(env);env->DeleteLocalRef(mh);}
                                env->DeleteLocalRef(mc);
                            } else ClearEx(env);
                        }
                    }
                }
            }
            prevIns=curIns;

            JNIEnv* env=GetEnv();
            if(env){
                if(g_killAura)   TickKillAura(env);
                if(g_autoSprint) TickAutoSprint(env);
            }
            Sleep(50);
        }
    }

    if(g_ovHwnd){ DestroyWindow(g_ovHwnd); g_ovHwnd=nullptr; }
    if(g_jvm) g_jvm->DetachCurrentThread();

Cleanup:
    std::cout<<"[DLL] Unloading\n";
    if(f) fclose(f); FreeConsole();
    FreeLibraryAndExitThread(g_hModule,0);
    return 0;
}

// ─── DllMain ─────────────────────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){
        g_hModule=hMod;
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr,0,MainThread,nullptr,0,nullptr);
    }
    return TRUE;
}
