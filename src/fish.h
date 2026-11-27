#pragma once
#include <windows.h>
#include <vector>
#include "frame.h"

// ── Размер спрайта ────────────────────────────────────────────
static constexpr int SPRITE_W = 150;
static constexpr int SPRITE_H = 150;

// ── Colorkey ─────────────────────────────────────────────────
static constexpr COLORREF COLORKEY = RGB(255, 0, 255);

// ── Физика ───────────────────────────────────────────────────
static constexpr float RAT_SPEED = 3.0f;

class Fish
{
public:
    Fish() = default;
    ~Fish() = default;

    HWND      hwnd_  = nullptr;
    HINSTANCE hInst_ = nullptr;

    bool Create(HINSTANCE hInstance, const std::vector<Frame> * frames);
    HWND GetHwnd() const { return hwnd_; }

    void SetFrames(const std::vector<Frame>* walkFrames);
    void NextFrame();
    void UpdatePhysics();

    void MoveBase(bool *end);
    void EndMoveBase();

    // Состояние (доступно контроллеру)
    int  BASE_Y = 720;
    int  BASE_X = 1400;
    bool paused_anim_        = false;
    bool paused_move        = true;
    bool paused_move_hard_   = true;
    bool hidden_full_screen_ = false;
    bool is_hidden_          = false;
    bool send_base_mode_     = false;
    bool isWalking_          = true;
    int  curFrame_           = 0;

    float posX_       = 200.0f;
    float posY_       = 200.0f;
    
    bool *end_move  ;

private:
   

    const std::vector<Frame>* walkFrames_ = nullptr;

    // Летание

    float velX_       = RAT_SPEED;
    float velY_       = RAT_SPEED;
    bool  facingLeft_ = false;
    RECT  fish_win_{};

    // Анимация
    bool reverse      = false;
    int  reves_count_ = 0;

    const std::vector<Frame>* CurrentAnim() const;
    void AdvanceFrame();
    void DrawCurrentFrame(HDC hdc);

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
};