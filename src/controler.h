#pragma once
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include "frame.h"
#include"fish.h"

// Таймеры 
static constexpr UINT TIMER_ANIM_ID = 1;
static constexpr UINT TIMER_MOVE_ID = 2;
static constexpr UINT TIMER_LONG_ID = 3;
static constexpr UINT TIMER_ANIM_MS = 100; // ~10 FPS
static constexpr UINT TIMER_MOVE_MS = 16;  // ~60 FPS
static constexpr UINT TIMER_LONG_MS = 1000;

static constexpr int TIME_INACTIVE = 2 * 60000;

// Трей 
static constexpr UINT CTRL_WM_TRAYICON = WM_APP + 1;
static constexpr UINT CTRL_TRAY_ID     = 1;

// меню
static constexpr UINT IDM_PAUSE = 1001;
static constexpr UINT IDM_SHOW  = 1002;
static constexpr UINT IDM_HIDE  = 1003;
static constexpr UINT IDM_EXIT  = 1004;
static constexpr UINT IDM_POSIH = 1005;
static constexpr UINT IDM_BASE  = 1006;
static constexpr UINT IDM_NEW  = 1007;

class controler
{
private:
    HWND      hwnd_  = nullptr;
    HINSTANCE hInst_ = nullptr;

    NOTIFYICONDATA nid_       = {};
    bool trayAdded_ = false;

    LASTINPUTINFO last_act_{sizeof(LASTINPUTINFO), 0};
    QUERY_USER_NOTIFICATION_STATE state_{};

    bool LoadFrame(const void* ptr, int size, Frame& out);
    bool LoadSprites(const std::string& dir);
    void FreeFrames();

    bool AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();

    void NewFisch();

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

    std::vector<Fish*> fisch_;

    bool timer_inactive_ = false;
    bool end_move = false;
    bool active_move = false;

    bool paused_anim_        = false;
    bool paused_move        = true;
    bool paused_move_hard_   = true;
    bool hidden_full_screen_ = false;
    bool is_hidden_          = false;
    bool send_base_mode_     = false;
    bool isWalking_          = true;

public:
    std::vector<Frame> walkFrames;
    std::vector<Frame> idleFrames;

    controler();
    ~controler();

    bool Create(HINSTANCE hInstance);
    void RunMessageLoop();
    HWND GetHwnd() const { return hwnd_; }
};
