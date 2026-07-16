/*
 * esp_mod.dll  -  External overlay (Win32 layered window) + JNI cheat modules
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

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ─── Palette  (Vape v4 style) ────────────────────────────────────────────────
#define V_PANEL_BG  RGB( 18,  18,  18)
#define V_HEADER    RGB( 26,  26,  26)
#define V_ACCENT    RGB( 62, 128, 255)   // Vape blue
#define V_ON        RGB(255, 255, 255)
#define V_OFF       RGB(110, 110, 115)
#define V_HDRTXT    RGB(185, 185, 192)
#define V_SEP       RGB( 38,  38,  38)
#define V_HOV       RGB( 30,  30,  30)

// ─── Layout ──────────────────────────────────────────────────────────────────
static const int PANEL_W  = 150;
static const int HEADER_H = 22;
static const int ROW_H    = 20;

// ─── JVM globals ─────────────────────────────────────────────────────────────
static JavaVM*  g_jvm         = nullptr;
static jobject  g_classLoader = nullptr;
static HMODULE  g_hModule     = nullptr;

// ─── Module state ─────────────────────────────────────────────────────────────
static volatile bool g_killAura   = false;
static volatile bool g_esp        = false;
static volatile bool g_autoSprint = false;
static volatile bool g_menuOpen   = false;

// ─── Panels ──────────────────────────────────────────────────────────────────
struct ModEntry { const wchar_t* name; volatile bool* on; };
struct Panel {
    const wchar_t* cat;
    ModEntry       mods[4];
    int            count;
    int            x, y;          // position on overlay (pixels)
};

static Panel g_panels[3] = {
    { L"COMBAT",   { {L"Kill Aura",  &g_killAura}  }, 1, 10, 10 },
    { L"VISUAL",   { {L"ESP",        &g_esp}        }, 1, 170, 10 },
    { L"MOVEMENT", { {L"Auto Sprint",&g_autoSprint}  }, 1, 330, 10 },
};

// ─── Overlay window globals ───────────────────────────────────────────────────
static HWND  g_ovHwnd    = nullptr;
static HFONT g_fCat      = nullptr;   // category label  (bold 11pt)
static HFONT g_fMod      = nullptr;   // module name     (12pt)

// Drag state
static int   g_dragPanel = -1;
static int   g_dragOffX  = 0, g_dragOffY = 0;
static int   g_hovPanel  = -1, g_hovRow  = -1;
static bool  g_prevLBtn  = false;

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
static void ClearEx(JNIEnv* e){ if(e&&e->ExceptionCheck()) e->ExceptionClear(); }

static jclass FindMCClass(JNIEnv* env, const char* slash) {
    if (!g_classLoader) return nullptr;
    std::string dot=slash; for(char&c:dot) if(c=='/') c='.';
    jclass cc=env->FindClass("java/lang/Class");
    jmethodID fn=env->GetStaticMethodID(cc,"forName",
        "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
    jstring jn=env->NewStringUTF(dot.c_str());
    auto r=(jclass)env->CallStaticObjectMethod(cc,fn,jn,JNI_TRUE,g_classLoader);
    env->DeleteLocalRef(jn);
    if(env->ExceptionCheck()){env->ExceptionClear();return nullptr;}
    return r?(jclass)env->NewGlobalRef(r):nullptr;
}

static bool GrabClassLoader(JNIEnv* env) {
    jclass tc=env->FindClass("java/lang/Thread");
    jclass mc=env->FindClass("java/util/Map");
    jclass sc=env->FindClass("java/util/Set");
    jmethodID gat=env->GetStaticMethodID(tc,"getAllStackTraces","()Ljava/util/Map;");
    jmethodID ks=env->GetMethodID(mc,"keySet","()Ljava/util/Set;");
    jmethodID ta=env->GetMethodID(sc,"toArray","()[Ljava/lang/Object;");
    jmethodID gn=env->GetMethodID(tc,"getName","()Ljava/lang/String;");
    jmethodID gcl=env->GetMethodID(tc,"getContextClassLoader","()Ljava/lang/ClassLoader;");
    jobject map=env->CallStaticObjectMethod(tc,gat);
    auto arr=(jobjectArray)env->CallObjectMethod(env->CallObjectMethod(map,ks),ta);
    jint len=env->GetArrayLength(arr);
    const char* pref[]={"Render","main","Client","Minecraft",nullptr};
    std::cout<<"[DLL] Java threads:\n";
    for(int pass=0;pass<2&&!g_classLoader;pass++){
        for(int i=0;i<len;i++){
            jobject t=env->GetObjectArrayElement(arr,i);
            jstring jn=(jstring)env->CallObjectMethod(t,gn);
            const char* raw=env->GetStringUTFChars(jn,nullptr);
            std::string name=raw; env->ReleaseStringUTFChars(jn,raw);
            if(pass==0) std::cout<<"  "<<name<<"\n";
            bool want=false;
            if(pass==0){for(int k=0;pref[k];k++) if(name.find(pref[k])!=std::string::npos){want=true;break;}}
            else want=true;
            if(want&&!g_classLoader){
                jobject cl=env->CallObjectMethod(t,gcl);
                if(env->ExceptionCheck()) env->ExceptionClear();
                if(cl){g_classLoader=env->NewGlobalRef(cl);
                       std::cout<<"[DLL] ClassLoader from: "<<name<<"\n";}
            }
            env->DeleteLocalRef(t);
        }
    }
    return g_classLoader!=nullptr;
}

static bool InitRefs(JNIEnv* env) {
    if(!GrabClassLoader(env)) return false;
    g_mc.Minecraft          =FindMCClass(env,"net/minecraft/client/Minecraft");
    g_mc.ClientLevel        =FindMCClass(env,"net/minecraft/client/multiplayer/ClientLevel");
    g_mc.LocalPlayer        =FindMCClass(env,"net/minecraft/client/player/LocalPlayer");
    g_mc.Entity             =FindMCClass(env,"net/minecraft/world/entity/Entity");
    g_mc.LivingEntity       =FindMCClass(env,"net/minecraft/world/entity/LivingEntity");
    g_mc.PlayerEntity       =FindMCClass(env,"net/minecraft/world/entity/player/Player");
    g_mc.MultiPlayerGameMode=FindMCClass(env,"net/minecraft/client/multiplayer/MultiPlayerGameMode");
    g_mc.InteractionHand    =FindMCClass(env,"net/minecraft/world/InteractionHand");
    g_mc.MouseHandler       =FindMCClass(env,"net/minecraft/client/MouseHandler");
    g_mc.Iterable=(jclass)env->NewGlobalRef(env->FindClass("java/lang/Iterable"));
    g_mc.Iterator=(jclass)env->NewGlobalRef(env->FindClass("java/util/Iterator"));
    if(!g_mc.Minecraft||!g_mc.ClientLevel||!g_mc.LocalPlayer||!g_mc.Entity||
       !g_mc.LivingEntity||!g_mc.PlayerEntity||!g_mc.MultiPlayerGameMode||!g_mc.InteractionHand)
        return false;
    g_mc.mGetInstance=env->GetStaticMethodID(g_mc.Minecraft,"getInstance","()Lnet/minecraft/client/Minecraft;");
    g_mc.fPlayer  =env->GetFieldID(g_mc.Minecraft,"player","Lnet/minecraft/client/player/LocalPlayer;");
    g_mc.fLevel   =env->GetFieldID(g_mc.Minecraft,"level","Lnet/minecraft/client/multiplayer/ClientLevel;");
    g_mc.fGameMode=env->GetFieldID(g_mc.Minecraft,"gameMode","Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;");
    g_mc.mEntities=env->GetMethodID(g_mc.ClientLevel,"entitiesForRendering","()Ljava/lang/Iterable;");
    g_mc.mIsAlive =env->GetMethodID(g_mc.Entity,"isAlive","()Z");
    g_mc.mDistanceTo=env->GetMethodID(g_mc.Entity,"distanceTo","(Lnet/minecraft/world/entity/Entity;)F");
    g_mc.mAttack  =env->GetMethodID(g_mc.MultiPlayerGameMode,"attack","(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/entity/Entity;)V");
    g_mc.mSwing   =env->GetMethodID(g_mc.LocalPlayer,"swing","(Lnet/minecraft/world/InteractionHand;)V");
    g_mc.mSetSprinting=env->GetMethodID(g_mc.LocalPlayer,"setSprinting","(Z)V");
    g_mc.mIterator=env->GetMethodID(g_mc.Iterable,"iterator","()Ljava/util/Iterator;");
    g_mc.mHasNext =env->GetMethodID(g_mc.Iterator,"hasNext","()Z");
    g_mc.mNext    =env->GetMethodID(g_mc.Iterator,"next","()Ljava/lang/Object;");
    g_mc.fMainHand=env->GetStaticFieldID(g_mc.InteractionHand,"MAIN_HAND","Lnet/minecraft/world/InteractionHand;");
    if(g_mc.MouseHandler){
        g_mc.fMouseHandler =env->GetFieldID(g_mc.Minecraft,"mouseHandler","Lnet/minecraft/client/MouseHandler;");
        g_mc.mReleaseMouse =env->GetMethodID(g_mc.MouseHandler,"releaseMouse","()V");
        g_mc.mGrabMouse    =env->GetMethodID(g_mc.MouseHandler,"grabMouse","()V");
    }
    ClearEx(env);
    g_mc.valid=g_mc.mGetInstance&&g_mc.fPlayer&&g_mc.fLevel&&g_mc.fGameMode&&
               g_mc.mEntities&&g_mc.mIsAlive&&g_mc.mDistanceTo&&
               g_mc.mAttack&&g_mc.mSwing&&g_mc.fMainHand;
    std::cout<<"[DLL] Refs: "<<(g_mc.valid?"ready":"partial")<<"\n";
    return g_mc.valid;
}

static void TickKillAura(JNIEnv* env) {
    if(!g_mc.valid) return;
    jobject mc=env->CallStaticObjectMethod(g_mc.Minecraft,g_mc.mGetInstance);
    if(!mc||env->ExceptionCheck()){ClearEx(env);return;}
    jobject player=env->GetObjectField(mc,g_mc.fPlayer);
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
        env->CallVoidMethod(player,g_mc.mSwing,mh);       ClearEx(env);
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

// ─── GDI helpers ─────────────────────────────────────────────────────────────
static void GdiFillRect(HDC dc, int x, int y, int w, int h, COLORREF c) {
    HBRUSH b=CreateSolidBrush(c); RECT r{x,y,x+w,y+h}; FillRect(dc,&r,b); DeleteObject(b);
}
static void GdiText(HDC dc, const wchar_t* t, int x, int y, int w, int h, HFONT f, COLORREF c) {
    SetTextColor(dc,c); SetBkMode(dc,TRANSPARENT);
    SelectObject(dc,f);
    RECT r{x,y,x+w,y+h};
    DrawTextW(dc,t,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
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

// ─── Paint all panels onto the overlay DC ────────────────────────────────────
static void PaintOverlay(HDC dc, int W, int H) {
    // Clear to fully transparent black
    HBRUSH clr=CreateSolidBrush(RGB(0,0,0));
    RECT all{0,0,W,H}; FillRect(dc,&all,clr); DeleteObject(clr);

    if(!g_menuOpen) return;

    // Get mouse pos (already in overlay-client coords since overlay matches MC window)
    POINT mp; GetCursorPos(&mp);
    ScreenToClient(g_ovHwnd,&mp);
    int mx=mp.x, my=mp.y;

    bool lbtn=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
    bool clicked=lbtn&&!g_prevLBtn;

    // Drag
    if(g_dragPanel>=0){
        if(!lbtn) g_dragPanel=-1;
        else{
            g_panels[g_dragPanel].x=mx-g_dragOffX;
            g_panels[g_dragPanel].y=my-g_dragOffY;
        }
    }

    g_hovPanel=-1; g_hovRow=-1;

    for(int pi=0;pi<3;pi++){
        Panel& p=g_panels[pi];
        int px=p.x, py=p.y;
        int ph=HEADER_H+p.count*ROW_H;

        // Hover test
        if(mx>=px&&mx<px+PANEL_W&&my>=py&&my<py+ph){
            g_hovPanel=pi;
            if(my>=py+HEADER_H) g_hovRow=(my-(py+HEADER_H))/ROW_H;
        }

        // Body background
        GdiFillRect(dc,px,py+HEADER_H,PANEL_W,p.count*ROW_H,V_PANEL_BG);
        // Header
        GdiFillRect(dc,px,py,PANEL_W,HEADER_H,V_HEADER);
        // Header bottom separator
        GdiFillRect(dc,px,py+HEADER_H-1,PANEL_W,1,V_SEP);
        // Category name
        GdiText(dc,p.cat,px+6,py,PANEL_W-8,HEADER_H,g_fCat,V_HDRTXT);

        for(int mi=0;mi<p.count;mi++){
            int ry=py+HEADER_H+mi*ROW_H;
            bool on=*p.mods[mi].on;

            // Hover highlight
            if(g_hovPanel==pi&&g_hovRow==mi)
                GdiFillRect(dc,px,ry,PANEL_W,ROW_H,V_HOV);

            // 2px left accent bar when enabled
            if(on) GdiFillRect(dc,px,ry,2,ROW_H,V_ACCENT);

            // Row separator
            if(mi<p.count-1) GdiFillRect(dc,px,ry+ROW_H-1,PANEL_W,1,V_SEP);

            // Module name
            GdiText(dc,p.mods[mi].name,px+8,ry,PANEL_W-16,ROW_H,
                    g_fMod,on?V_ON:V_OFF);

            // Click → toggle
            if(clicked&&g_hovPanel==pi&&g_hovRow==mi){
                *p.mods[mi].on=!*p.mods[mi].on;
                InvalidateRect(g_ovHwnd,nullptr,FALSE);
            }
        }

        // Click in header → start drag
        if(clicked&&g_hovPanel==pi&&my>=py&&my<py+HEADER_H&&g_dragPanel<0){
            g_dragPanel=pi; g_dragOffX=mx-px; g_dragOffY=my-py;
        }
    }

    g_prevLBtn=lbtn;
}

// ─── Overlay window procedure ─────────────────────────────────────────────────
static LRESULT CALLBACK OverlayProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg){

    case WM_CREATE:
        g_fCat=CreateFontW(-11,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        g_fMod=CreateFontW(-12,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        // 60 fps repaint timer
        SetTimer(hw,1,16,nullptr);
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
        InvalidateRect(hw,nullptr,FALSE);
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc=BeginPaint(hw,&ps);
        RECT cr; GetClientRect(hw,&cr);

        // Draw into a back-buffer to avoid flicker
        HDC mdc=CreateCompatibleDC(dc);
        HBITMAP bmp=CreateCompatibleBitmap(dc,cr.right,cr.bottom);
        SelectObject(mdc,bmp);

        PaintOverlay(mdc,cr.right,cr.bottom);

        BitBlt(dc,0,0,cr.right,cr.bottom,mdc,0,0,SRCCOPY);
        DeleteObject(bmp); DeleteDC(mdc);
        EndPaint(hw,&ps);
        break;
    }

    // Allow dragging the whole overlay via empty areas
    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_DESTROY:
        KillTimer(hw,1);
        DeleteObject(g_fCat); DeleteObject(g_fMod);
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

    // WS_EX_TRANSPARENT makes it fully click-through initially
    // We remove it when the menu opens
    g_ovHwnd=CreateWindowExW(
        WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,
        L"EspOverlay",L"",WS_POPUP,
        rc.left,rc.top,W,H,
        nullptr,nullptr,g_hModule,nullptr);

    // Overall window opacity: 0=invisible when closed (we use WS_EX_TRANSPARENT),
    // but we also keep it slightly transparent so window border isn't visible
    SetLayeredWindowAttributes(g_ovHwnd,RGB(0,0,0),0,LWA_COLORKEY);
    ShowWindow(g_ovHwnd,SW_SHOW);

    MSG m{};
    while(GetMessage(&m,nullptr,0,0)){
        TranslateMessage(&m); DispatchMessage(&m);
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
                    InvalidateRect(g_ovHwnd,nullptr,FALSE);
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
