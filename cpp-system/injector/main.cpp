/*
 * ESP Injector  -  Premium Dark UI v2
 * Injection only: find PID by name, inject DLL via CreateRemoteThread/LoadLibraryA.
 * All Java/class-loader logic lives in the DLL itself.
 */

#include <windows.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comdlg32.lib")

// ─── Colour palette ───────────────────────────────────────────────────────────
#define C_BG        RGB( 13,  13,  20)
#define C_HEADER    RGB( 18,  18,  30)
#define C_ACCENT    RGB(  0, 210, 140)
#define C_ACCENT_H  RGB(  0, 175, 115)
#define C_TEXT      RGB(225, 230, 240)
#define C_DIM       RGB(100, 110, 135)
#define C_INPUT_BG  RGB( 22,  22,  36)
#define C_INPUT_BR  RGB( 50,  55,  80)
#define C_SEP       RGB( 35,  35,  55)
#define C_BTN_TXT   RGB( 10,  15,  12)
#define C_SMBTN     RGB( 32,  34,  54)
#define C_SMBTN_H   RGB( 52,  56,  84)
#define C_STATUS_BG RGB( 10,  10,  16)
#define C_ERR       RGB(220,  60,  60)
#define C_DOT_OFF   RGB( 45,  50,  75)

// ─── Layout (client size: W=340, H=330) ───────────────────────────────────────
static const int MARGIN  = 17;
static const int SMBTN_W = 35;   // ↺ and … button width
static const int SMBTN_H = 26;   // button / input-shell height
static const int GAP     = 6;    // gap between control and small button

// W=340 → cW = 340-34-6-35 = 265,  bX = 17+265+6 = 288
static inline int CtrlW (int W) { return W - 2*MARGIN - GAP - SMBTN_W; }
static inline int SmBtnX(int W) { return MARGIN + CtrlW(W) + GAP; }

// Row y-coordinates (spec layout)
static const int ROW_PROC_LABEL = 85;
static const int ROW_PROC_BOX   = 103;  // drawn shell + small button
static const int ROW_PROC_CTRL  = 107;  // HWND combo (inside shell)
static const int ROW_DLL_LABEL  = 142;
static const int ROW_DLL_BOX    = 160;  // drawn shell + small button
static const int ROW_DLL_CTRL   = 164;  // HWND edit (inside shell)
static const int ROW_INJECT     = 200;
static const int INJECT_H       = 44;
static const int STATUS_SEP     = 248;  // separator line above status bar
static const int ROW_STATUS     = 259;

// ─── Control IDs ─────────────────────────────────────────────────────────────
#define ID_COMBO  101
#define ID_EDIT   102
#define ID_TIMER  1

// ─── Globals ──────────────────────────────────────────────────────────────────
static HWND   g_hwnd, g_hCombo, g_hEdit;
static HFONT  g_fTitle, g_fSub, g_fLabel, g_fInput, g_fBtn, g_fStatus, g_fSmBtn;
static HBRUSH g_brInput;

static BOOL g_hovInject  = FALSE;
static BOOL g_hovRefresh = FALSE;
static BOOL g_hovBrowse  = FALSE;
static BOOL g_tracking   = FALSE;

static BOOL         g_success   = FALSE;
static std::wstring g_statusMsg = L"Ready";

static BOOL g_procRunning = FALSE;   // green / dim dot next to process label
static BOOL g_fileExists  = FALSE;   // ✔ / ✘ next to DLL label

// ─── Process helpers ──────────────────────────────────────────────────────────

// EnumWindows callback — collects exe names of visible, titled, top-level windows.
// This matches what Task Manager calls "Apps" (not background processes).
static BOOL CALLBACK CollectApps(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (GetWindowTextLengthW(hwnd) == 0) return TRUE;
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
    if (GetWindow(hwnd, GW_OWNER) != NULL) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return TRUE;
    HANDLE hp = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hp) return TRUE;
    wchar_t path[MAX_PATH] = {};
    DWORD sz = MAX_PATH;
    if (QueryFullProcessImageNameW(hp, 0, path, &sz)) {
        std::wstring full = path;
        auto pos = full.find_last_of(L"\\/");
        std::wstring name = (pos != std::wstring::npos) ? full.substr(pos + 1) : full;
        reinterpret_cast<std::vector<std::wstring>*>(lParam)->push_back(name);
    }
    CloseHandle(hp);
    return TRUE;
}

static DWORD PidByName(const std::wstring& name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe))
        do { if (name == pe.szExeFile) { pid = pe.th32ProcessID; break; } }
        while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    return pid;
}

// Updates the running-dot indicator for whatever is currently selected in the combo.
static void UpdateRunningDot() {
    int sel = (int)SendMessageW(g_hCombo, CB_GETCURSEL, 0, 0);
    BOOL running = FALSE;
    if (sel != CB_ERR) {
        wchar_t buf[256] = {};
        SendMessageW(g_hCombo, CB_GETLBTEXT, sel, (LPARAM)buf);
        running = (PidByName(buf) != 0) ? TRUE : FALSE;
    }
    if (running != g_procRunning) {
        g_procRunning = running;
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    }
}

// Resolves the current edit text to an absolute path (same logic as injection)
// and updates g_fileExists.
static void UpdateFileIndicator() {
    wchar_t dllW[MAX_PATH] = {};
    GetWindowTextW(g_hEdit, dllW, MAX_PATH);
    if (wcschr(dllW, L'\\') == nullptr && wcschr(dllW, L'/') == nullptr && dllW[0] != L'\0') {
        wchar_t exeDir[MAX_PATH] = {};
        GetModuleFileNameW(NULL, exeDir, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(exeDir, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wchar_t resolved[MAX_PATH] = {};
            wcscpy_s(resolved, exeDir);
            wcscat_s(resolved, dllW);
            wcscpy_s(dllW, resolved);
        }
    }
    BOOL exists = (dllW[0] != L'\0' &&
                   GetFileAttributesW(dllW) != INVALID_FILE_ATTRIBUTES) ? TRUE : FALSE;
    if (exists != g_fileExists) {
        g_fileExists = exists;
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    }
}

static void RefreshProcesses() {
    std::vector<std::wstring> list;
    EnumWindows(CollectApps, (LPARAM)&list);
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    SendMessageW(g_hCombo, CB_RESETCONTENT, 0, 0);
    for (auto& s : list)
        SendMessageW(g_hCombo, CB_ADDSTRING, 0, (LPARAM)s.c_str());
    // Pre-select javaw.exe (Minecraft's JVM), then Minecraft.exe, else first item
    LRESULT idx = SendMessageW(g_hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)L"javaw.exe");
    if (idx == CB_ERR)
        idx = SendMessageW(g_hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)L"Minecraft.exe");
    SendMessageW(g_hCombo, CB_SETCURSEL, (idx != CB_ERR) ? idx : 0, 0);
    UpdateRunningDot();
}

static bool InjectDLL(DWORD pid, const std::string& dll) {
    HANDLE hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hp) return false;
    SIZE_T sz = dll.size() + 1;
    LPVOID pm = VirtualAllocEx(hp, NULL, sz, MEM_COMMIT, PAGE_READWRITE);
    if (!pm) { CloseHandle(hp); return false; }
    WriteProcessMemory(hp, pm, dll.c_str(), sz, NULL);
    auto pLL = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA");
    HANDLE ht = CreateRemoteThread(hp, NULL, 0, pLL, pm, 0, NULL);
    if (!ht) { VirtualFreeEx(hp, pm, 0, MEM_RELEASE); CloseHandle(hp); return false; }
    WaitForSingleObject(ht, INFINITE);
    VirtualFreeEx(hp, pm, 0, MEM_RELEASE);
    CloseHandle(ht); CloseHandle(hp);
    return true;
}

// ─── Drawing helpers ──────────────────────────────────────────────────────────

static void FillC(HDC dc, RECT r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

static void RndRect(HDC dc, int x, int y, int w, int h, int rad,
                    COLORREF fill, COLORREF bord)
{
    HBRUSH fb = CreateSolidBrush(fill);
    HPEN   fp = CreatePen(PS_SOLID, 1, bord);
    SelectObject(dc, fb); SelectObject(dc, fp);
    RoundRect(dc, x, y, x+w, y+h, rad, rad);
    SelectObject(dc, GetStockObject(NULL_PEN));
    SelectObject(dc, GetStockObject(NULL_BRUSH));
    DeleteObject(fb); DeleteObject(fp);
}

static void DText(HDC dc, const wchar_t* t,
                  int x, int y, int w, int h,
                  HFONT f, COLORREF c, UINT fmt)
{
    RECT r{x, y, x+w, y+h};
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, c);
    auto old = (HFONT)SelectObject(dc, f);
    DrawTextW(dc, t, -1, &r, fmt);
    SelectObject(dc, old);
}

// Draws a filled circle centred at (cx,cy) with radius r.
static void DrawDot(HDC dc, int cx, int cy, int r, COLORREF c) {
    HBRUSH  br = CreateSolidBrush(c);
    HGDIOBJ ob = SelectObject(dc, br);
    HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, cx-r, cy-r, cx+r+1, cy+r+1);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(br);
}

// ─── Hit-test helpers ─────────────────────────────────────────────────────────

static BOOL HitRefresh(int W, int px, int py) {
    return px >= SmBtnX(W) && px < SmBtnX(W)+SMBTN_W
        && py >= ROW_PROC_BOX && py < ROW_PROC_BOX+SMBTN_H;
}
static BOOL HitBrowse(int W, int px, int py) {
    return px >= SmBtnX(W) && px < SmBtnX(W)+SMBTN_W
        && py >= ROW_DLL_BOX && py < ROW_DLL_BOX+SMBTN_H;
}
static BOOL HitInject(int W, int px, int py) {
    return px >= MARGIN && px < W-MARGIN
        && py >= ROW_INJECT && py < ROW_INJECT+INJECT_H;
}

// ─── Window Procedure ─────────────────────────────────────────────────────────

LRESULT CALLBACK WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    // ── Create ───────────────────────────────────────────────────────────────
    case WM_CREATE: {
        g_hwnd = hw;  // needed by UpdateRunningDot / UpdateFileIndicator

        // Dark title bar: attr 19 = pre-20H1, attr 20 = 20H1+
        BOOL dk = TRUE;
        DwmSetWindowAttribute(hw, 19, &dk, sizeof(dk));
        DwmSetWindowAttribute(hw, 20, &dk, sizeof(dk));

        auto MF = [](int h, int wt, const wchar_t* face) -> HFONT {
            return CreateFontW(h, 0, 0, 0, wt, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
        };
        g_fTitle  = MF(22, FW_BOLD,    L"Segoe UI");
        g_fSub    = MF(12, FW_NORMAL,  L"Segoe UI");
        g_fLabel  = MF(11, FW_SEMIBOLD,L"Segoe UI");
        g_fInput  = MF(13, FW_NORMAL,  L"Consolas");
        g_fBtn    = MF(13, FW_BOLD,    L"Segoe UI");
        g_fStatus = MF(11, FW_NORMAL,  L"Segoe UI");
        g_fSmBtn  = MF(14, FW_NORMAL,  L"Segoe UI");

        g_brInput = CreateSolidBrush(C_INPUT_BG);

        RECT cr; GetClientRect(hw, &cr);
        int W  = cr.right;
        int cW = CtrlW(W);

        // Process combo — height 200 covers both collapsed bar and expanded list
        g_hCombo = CreateWindowExW(0, L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL,
            MARGIN, ROW_PROC_CTRL, cW, 200, hw, (HMENU)ID_COMBO, NULL, NULL);
        SendMessageW(g_hCombo, WM_SETFONT, (WPARAM)g_fInput, TRUE);

        // DLL path edit — borderless; we draw the shell ourselves
        g_hEdit = CreateWindowExW(0, L"EDIT", L"esp_mod.dll",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            MARGIN+4, ROW_DLL_CTRL, cW-8, 20, hw, (HMENU)ID_EDIT, NULL, NULL);
        SendMessageW(g_hEdit, WM_SETFONT, (WPARAM)g_fInput, TRUE);

        RefreshProcesses();
        UpdateFileIndicator();

        SetTimer(hw, ID_TIMER, 500, NULL);  // 500 ms auto-refresh for running dot
        break;
    }

    // ── Timer — keep running dot fresh ───────────────────────────────────────
    case WM_TIMER:
        if (wp == ID_TIMER) UpdateRunningDot();
        break;

    // ── Suppress default background erase (we paint everything) ──────────────
    case WM_ERASEBKGND:
        return 1;

    // ── Paint ────────────────────────────────────────────────────────────────
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hw, &ps);

        RECT cr; GetClientRect(hw, &cr);
        int W = cr.right, H = cr.bottom;
        int cW = CtrlW(W);
        int bX = SmBtnX(W);

        // ── Background
        FillC(dc, cr, C_BG);

        // ── Header (0-72)
        FillC(dc, RECT{0, 0, W, 72}, C_HEADER);
        FillC(dc, RECT{0, 0, W,  4}, C_ACCENT);   // 4 px accent stripe
        DText(dc, L"ESP INJECTOR",
              18, 13, W-36, 28, g_fTitle, C_TEXT, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        DText(dc, L"Select a process and DLL to inject",
              18, 44, W-36, 18, g_fSub,  C_DIM,  DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        FillC(dc, RECT{0, 72, W, 73}, C_SEP);

        // ── Section 1: Target Process (y=85)
        DText(dc, L"TARGET PROCESS",
              MARGIN, ROW_PROC_LABEL, 160, 14,
              g_fLabel, C_DIM, DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        // Running indicator: label + dot on the right of the header row
        {
            int dotCX   = W - MARGIN - 5;
            int dotCY   = ROW_PROC_LABEL + 7;
            DrawDot(dc, dotCX, dotCY, 4, g_procRunning ? C_ACCENT : C_DOT_OFF);
            // Text right-aligned, ending just before the dot
            int lblW = dotCX - 8 - (MARGIN + 160);
            if (lblW > 0) {
                const wchar_t* runTxt = g_procRunning ? L"Running" : L"Not found";
                COLORREF       runC   = g_procRunning ? C_ACCENT  : C_DIM;
                DText(dc, runTxt, MARGIN+160, ROW_PROC_LABEL, lblW, 14,
                      g_fStatus, runC, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
            }
        }

        // Shell behind combo
        RndRect(dc, MARGIN-1, ROW_PROC_BOX, cW+2, SMBTN_H, 5, C_INPUT_BG, C_INPUT_BR);

        // Refresh button ↺
        {
            COLORREF rc = g_hovRefresh ? C_SMBTN_H : C_SMBTN;
            RndRect(dc, bX, ROW_PROC_BOX, SMBTN_W, SMBTN_H, 5, rc, C_INPUT_BR);
            DText(dc, L"\u21BB", bX, ROW_PROC_BOX, SMBTN_W, SMBTN_H,
                  g_fSmBtn, C_TEXT, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }

        // ── Section 2: DLL to Inject (y=142)
        DText(dc, L"DLL TO INJECT",
              MARGIN, ROW_DLL_LABEL, 160, 14,
              g_fLabel, C_DIM, DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        // File-exists indicator: ✔ green / ✘ red
        {
            const wchar_t* sym  = g_fileExists ? L"\u2714" : L"\u2718";
            COLORREF       symC = g_fileExists ? C_ACCENT : C_ERR;
            DText(dc, sym, W-MARGIN-20, ROW_DLL_LABEL-1, 20, 16,
                  g_fSmBtn, symC, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }

        // Shell behind edit
        RndRect(dc, MARGIN-1, ROW_DLL_BOX, cW+2, SMBTN_H, 5, C_INPUT_BG, C_INPUT_BR);

        // Browse button …
        {
            COLORREF bc = g_hovBrowse ? C_SMBTN_H : C_SMBTN;
            RndRect(dc, bX, ROW_DLL_BOX, SMBTN_W, SMBTN_H, 5, bc, C_INPUT_BR);
            DText(dc, L"\u2026", bX, ROW_DLL_BOX, SMBTN_W, SMBTN_H,
                  g_fSmBtn, C_TEXT, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }

        // ── Inject button (y=200, h=44)
        {
            COLORREF ic = g_hovInject ? C_ACCENT_H : C_ACCENT;
            RndRect(dc, MARGIN, ROW_INJECT, W-2*MARGIN, INJECT_H, 7, ic, ic);
            DText(dc, L"INJECT DLL",
                  MARGIN, ROW_INJECT, W-2*MARGIN, INJECT_H,
                  g_fBtn, C_BTN_TXT, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }

        // ── Status bar (y=248 separator, y=259 text)
        FillC(dc, RECT{0, STATUS_SEP, W, H}, C_STATUS_BG);
        FillC(dc, RECT{0, STATUS_SEP, W, STATUS_SEP+1}, C_SEP);
        {
            std::wstring st = L"\u25CF  " + g_statusMsg;
            COLORREF     sc = g_success ? C_ACCENT : C_DIM;
            DText(dc, st.c_str(), MARGIN, ROW_STATUS, W-2*MARGIN, 18,
                  g_fStatus, sc, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        }

        EndPaint(hw, &ps);
        break;
    }

    // ── Dark-theme colours for child controls ─────────────────────────────────
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = (HDC)wp;
        SetTextColor(dc, C_TEXT);
        SetBkColor(dc, C_INPUT_BG);
        return (LRESULT)g_brInput;
    }

    // ── Hover tracking ────────────────────────────────────────────────────────
    case WM_MOUSEMOVE: {
        RECT cr; GetClientRect(hw, &cr);
        int W  = cr.right;
        int px = LOWORD(lp), py = HIWORD(lp);

        BOOL hI = HitInject (W, px, py);
        BOOL hR = HitRefresh(W, px, py);
        BOOL hB = HitBrowse (W, px, py);

        if (hI != g_hovInject || hR != g_hovRefresh || hB != g_hovBrowse) {
            g_hovInject  = hI;
            g_hovRefresh = hR;
            g_hovBrowse  = hB;
            InvalidateRect(hw, NULL, FALSE);
        }

        SetCursor(LoadCursor(NULL, (hI||hR||hB) ? IDC_HAND : IDC_ARROW));

        if ((hI||hR||hB) && !g_tracking) {
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hw, 0 };
            TrackMouseEvent(&tme);
            g_tracking = TRUE;
        }
        break;
    }
    case WM_MOUSELEAVE:
        g_hovInject = g_hovRefresh = g_hovBrowse = g_tracking = FALSE;
        InvalidateRect(hw, NULL, FALSE);
        break;

    // ── Child control notifications ───────────────────────────────────────────
    case WM_COMMAND: {
        int ctlId  = LOWORD(wp);
        int notif  = HIWORD(wp);
        if (ctlId == ID_COMBO && notif == CBN_SELCHANGE)
            UpdateRunningDot();
        if (ctlId == ID_EDIT  && notif == EN_CHANGE)
            UpdateFileIndicator();
        break;
    }

    // ── Click handling ────────────────────────────────────────────────────────
    case WM_LBUTTONDOWN: {
        RECT cr; GetClientRect(hw, &cr);
        int W  = cr.right;
        int px = LOWORD(lp), py = HIWORD(lp);

        // ── Refresh process list ──────────────────────────────────────────────
        if (HitRefresh(W, px, py)) {
            RefreshProcesses();
            g_statusMsg = L"Process list refreshed";
            g_success   = FALSE;
            InvalidateRect(hw, NULL, FALSE);
            break;
        }

        // ── Browse for DLL ────────────────────────────────────────────────────
        if (HitBrowse(W, px, py)) {
            wchar_t path[MAX_PATH] = {};
            GetWindowTextW(g_hEdit, path, MAX_PATH);
            OPENFILENAMEW ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hw;
            ofn.lpstrFilter = L"DLL Files\0*.dll\0All Files\0*.*\0";
            ofn.lpstrFile   = path;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            ofn.lpstrTitle  = L"Select DLL to inject";
            if (GetOpenFileNameW(&ofn)) {
                SetWindowTextW(g_hEdit, path);
                UpdateFileIndicator();
                g_statusMsg = L"DLL selected";
                g_success   = FALSE;
                InvalidateRect(hw, NULL, FALSE);
            }
            break;
        }

        // ── Inject DLL ────────────────────────────────────────────────────────
        if (HitInject(W, px, py)) {
            int sel = (int)SendMessageW(g_hCombo, CB_GETCURSEL, 0, 0);
            if (sel == CB_ERR) {
                g_statusMsg = L"No process selected";
                g_success   = FALSE;
                InvalidateRect(hw, NULL, FALSE);
                break;
            }
            wchar_t procW[256] = {};
            SendMessageW(g_hCombo, CB_GETLBTEXT, sel, (LPARAM)procW);

            // Resolve relative DLL path → absolute (relative to injector's own dir).
            // LoadLibraryA runs inside the target process, so relative paths would
            // resolve against its working directory, not ours.
            wchar_t dllW[MAX_PATH] = {};
            GetWindowTextW(g_hEdit, dllW, MAX_PATH);
            if (wcschr(dllW, L'\\') == nullptr && wcschr(dllW, L'/') == nullptr) {
                wchar_t exeDir[MAX_PATH] = {};
                GetModuleFileNameW(NULL, exeDir, MAX_PATH);
                wchar_t* lastSlash = wcsrchr(exeDir, L'\\');
                if (lastSlash) {
                    *(lastSlash + 1) = L'\0';
                    wchar_t resolved[MAX_PATH] = {};
                    wcscpy_s(resolved, exeDir);
                    wcscat_s(resolved, dllW);
                    wcscpy_s(dllW, resolved);
                }
            }

            // DLL not found → inline status, no popup
            if (GetFileAttributesW(dllW) == INVALID_FILE_ATTRIBUTES) {
                g_statusMsg = L"DLL file not found \u2014 check the path";
                g_success   = FALSE;
                InvalidateRect(hw, NULL, FALSE);
                break;
            }

            // Convert to ANSI for LoadLibraryA
            char dllA[MAX_PATH] = {};
            WideCharToMultiByte(CP_ACP, 0, dllW, -1, dllA, MAX_PATH, NULL, NULL);

            g_statusMsg = L"Searching for process\u2026";
            g_success   = FALSE;
            InvalidateRect(hw, NULL, FALSE);
            UpdateWindow(hw);

            DWORD pid = PidByName(procW);
            if (!pid) {
                // Process not running → inline status only, no popup
                g_statusMsg = std::wstring(L"Process not running: ") + procW;
                g_success   = FALSE;
                InvalidateRect(hw, NULL, FALSE);
                break;
            }

            g_statusMsg = L"Injecting\u2026";
            InvalidateRect(hw, NULL, FALSE);
            UpdateWindow(hw);

            if (InjectDLL(pid, dllA)) {
                // Success → green status bar, no popup
                g_statusMsg = std::wstring(L"Injected into ") + procW;
                g_success   = TRUE;
            } else {
                // Failure → popup only here (diagnostic: shows error code)
                DWORD err = GetLastError();
                g_statusMsg = L"Injection failed \u2014 try as Administrator";
                g_success   = FALSE;
                wchar_t errMsg[512];
                swprintf_s(errMsg,
                    L"Injection failed (error %lu).\n\n"
                    L"Common fixes:\n"
                    L"  \u2022 Run injector.exe as Administrator\n"
                    L"  \u2022 Make sure the target process is correct\n"
                    L"  \u2022 Temporarily disable Windows Defender / AV",
                    err);
                MessageBoxW(hw, errMsg, L"Injection Failed", MB_ICONERROR);
            }
            InvalidateRect(hw, NULL, FALSE);
            break;
        }
        break;
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    case WM_DESTROY:
        KillTimer(hw, ID_TIMER);
        DeleteObject(g_fTitle);  DeleteObject(g_fSub);
        DeleteObject(g_fLabel);  DeleteObject(g_fInput);
        DeleteObject(g_fBtn);    DeleteObject(g_fStatus);
        DeleteObject(g_fSmBtn);  DeleteObject(g_brInput);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hw, msg, wp, lp);
}

// ─── Entry point ──────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmd) {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"EspInjector";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Client area: 340 x 330, fixed size (no resize, no maximize)
    RECT wr{ 0, 0, 340, 330 };
    DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    AdjustWindowRect(&wr, style, FALSE);

    int ww = wr.right  - wr.left;
    int wh = wr.bottom - wr.top;
    int cx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int cy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    g_hwnd = CreateWindowExW(0, L"EspInjector", L"ESP Injector",
        style, cx, cy, ww, wh, NULL, NULL, hInst, NULL);

    ShowWindow(g_hwnd, nCmd);
    UpdateWindow(g_hwnd);

    MSG m{};
    while (GetMessageW(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return (int)m.wParam;
}
