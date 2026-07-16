/*
 * esp_mod.dll  -  In-game HUD overlay + JNI cheat modules
 *
 *  INSERT  = open / close the in-game HUD
 *  END     = unload DLL
 *
 *  This version renders ImGui directly into the Minecraft game window
 *  as an in-game HUD (not an external overlay).
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <jni.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "../deps/imgui/imgui.h"
#include "ingame_overlay.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "opengl32.lib")

// ─── JVM globals ─────────────────────────────────────────────────────────────
static JavaVM*  g_jvm         = nullptr;
static jobject  g_classLoader = nullptr;
static HMODULE  g_hModule     = nullptr;

// ─── Module state ─────────────────────────────────────────────────────────────
static volatile bool g_killAura   = false;
static volatile bool g_esp        = false;
static volatile bool g_autoSprint = false;
volatile bool g_menuOpen   = false;  // Made public for HUD access

// ─── ImGui Panel state ────────────────────────────────────────────────────────
static bool g_combatOpen = true;
static bool g_visualOpen = true;
static bool g_movementOpen = true;

// ─── Panels ──────────────────────────────────────────────────────────────────
struct ModEntry { const char* name; volatile bool* on; };

static ModEntry g_combatMods[] = {
    {"Kill Aura", &g_killAura},
};

static ModEntry g_visualMods[] = {
    {"ESP", &g_esp},
};

static ModEntry g_movementMods[] = {
    {"Auto Sprint", &g_autoSprint},
};

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

// ─── ImGui UI rendering ───────────────────────────────────────────────────────
void RenderImGuiUI() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("COMBAT", &g_combatOpen)) {
        for (int i = 0; i < 1; i++) {
            ImGui::Checkbox(g_combatMods[i].name, (bool*)g_combatMods[i].on);
        }
        ImGui::End();
    }

    ImGui::SetNextWindowPos(ImVec2(220, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("VISUAL", &g_visualOpen)) {
        for (int i = 0; i < 1; i++) {
            ImGui::Checkbox(g_visualMods[i].name, (bool*)g_visualMods[i].on);
        }
        ImGui::End();
    }

    ImGui::SetNextWindowPos(ImVec2(430, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("MOVEMENT", &g_movementOpen)) {
        for (int i = 0; i < 1; i++) {
            ImGui::Checkbox(g_movementMods[i].name, (bool*)g_movementMods[i].on);
        }
        ImGui::End();
    }
}

// ─── Main DLL thread ──────────────────────────────────────────────────────────
static DWORD WINAPI MainThread(LPVOID) {
    AllocConsole();
    FILE* f=nullptr; freopen_s(&f,"CONOUT$","w",stdout);

    std::cout<<"================================\n"
               "  ESP Mod  -  In-Game HUD\n"
               "  INSERT = open/close menu\n"
               "  END    = unload\n"
               "================================\n";

    // Initialize in-game HUD
    g_inGameHUD = new InGameHUD();
    if (!g_inGameHUD->Initialize()) {
        std::cout << "[DLL] Failed to initialize in-game HUD\n";
        delete g_inGameHUD;
        g_inGameHUD = nullptr;
    } else {
        std::cout << "[DLL] In-game HUD initialized\n";
    }

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

    // Key + tick loop
    {
        bool prevIns=false;
        while(true){
            if(GetAsyncKeyState(VK_END)&1) break;

            bool curIns=(GetAsyncKeyState(VK_INSERT)&0x8000)!=0;
            if(curIns&&!prevIns){
                g_menuOpen=!g_menuOpen;
                std::cout<<"[DLL] Menu: "<<(g_menuOpen?"OPEN":"CLOSED")<<"\n";
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

    if(g_jvm) g_jvm->DetachCurrentThread();

Cleanup:
    std::cout<<"[DLL] Unloading\n";
    
    if(g_inGameHUD) {
        g_inGameHUD->Shutdown();
        delete g_inGameHUD;
        g_inGameHUD = nullptr;
    }
    
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
