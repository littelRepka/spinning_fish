#pragma once
#include <windows.h>
#include <shellapi.h>   // NOTIFYICONDATA, Shell_NotifyIcon
#include <vector>
#include <string>

// ── Таймеры ──────────────────────────────────────────────────
static constexpr UINT TIMER_ANIM_ID = 1;
static constexpr UINT TIMER_MOVE_ID = 2;
static constexpr UINT TIMER_ANIM_MS = 100; // ~10 FPS
static constexpr UINT TIMER_MOVE_MS = 16;  // ~60 FPS

// ── Размер спрайта ────────────────────────────────────────────
static constexpr int SPRITE_W = 150;
static constexpr int SPRITE_H = 150;

// ── Colorkey ─────────────────────────────────────────────────
static constexpr COLORREF COLORKEY = RGB(255, 0, 255);

// ── Физика ───────────────────────────────────────────────────
static constexpr float RAT_SPEED = 3.0f;
static constexpr int BASE_Y = 720;
static constexpr int BASE_X = 1400;

// ── Трей (Этап 5) ────────────────────────────────────────────
static constexpr UINT WM_TRAYICON   = WM_APP + 1; // кастомное сообщение от трея
static constexpr UINT TRAY_ID       = 1;           // ID иконки в трее

// ID пунктов меню
static constexpr UINT IDM_PAUSE  = 1001;
static constexpr UINT IDM_SHOW   = 1002;
static constexpr UINT IDM_HIDE   = 1003;
static constexpr UINT IDM_EXIT   = 1004;
static constexpr UINT IDM_POSIH = 1005;

struct Frame {
    HBITMAP hBitmap = nullptr;
    int     width   = 0;
    int     height  = 0;
};

class DesktopRat {
public:
    bool Create(HINSTANCE hInstance, int nCmdShow);
    void RunMessageLoop();
    HWND GetHwnd() const { return hwnd_; }

private:
    HWND      hwnd_  = nullptr;
    HINSTANCE hInst_ = nullptr;

    // ── Спрайты ───────────────────────────────────────────────
    std::vector<Frame> walkFrames_;
    std::vector<Frame> idleFrames_;

    bool LoadFrame(const std::string& path, Frame& out);
    bool LoadSprites(const std::string& dir);
    void FreeFrames();

    // ── Анимация ──────────────────────────────────────────────
    int  curFrame_  = 0;
    bool isWalking_ = true;
    bool reverse = false;

    std::vector<Frame>* CurrentAnim();
    void AdvanceFrame();
    void DrawCurrentFrame(HDC hdc);

    // ── Физика ───────────────────────────────────────────────
    float posX_      = 200.0f;
    float posY_      = 200.0f;
    float velX_      = RAT_SPEED;
    float velY_      = RAT_SPEED;
    bool  facingLeft_ = false;
    bool  paused_anim_    = false; 
    bool  paused_move_ = true;
    

    void UpdatePhysics();

    // ── Трей (Этап 5) ────────────────────────────────────────
    NOTIFYICONDATA nid_ = {};
    bool           trayAdded_ = false;

    bool  AddTrayIcon();
    void  RemoveTrayIcon();
    void  ShowTrayMenu();      // правый клик → контекстное меню

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
};
