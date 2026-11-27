#include "fish.h"
#include <cmath>

bool Fish::Create(HINSTANCE hInstance, const std::vector<Frame> * frames) {
    hInst_ = hInstance;
    walkFrames_ = frames;

    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"FishWindow";
    wc.hbrBackground = nullptr;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassEx(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    posX_ = static_cast<float>((screenW - SPRITE_W) / 2);
    posY_ = static_cast<float>((screenH - SPRITE_H) / 2);
    BASE_Y = screenH - 155;
    BASE_X = screenW - 130;

    DWORD exStyle = WS_EX_LAYERED
                  | WS_EX_TRANSPARENT
                  | WS_EX_TOPMOST
                  | WS_EX_TOOLWINDOW;  // скрыть из таскбара / Alt+Tab

    hwnd_ = CreateWindowEx(
        exStyle,
        L"FishWindow",
        L"Desktop Rat",
        WS_POPUP,
        static_cast<int>(posX_), static_cast<int>(posY_),
        SPRITE_W, SPRITE_H,
        nullptr, nullptr, hInstance, this
    );
    //MessageBoxA(nullptr, " NOT LOADED exe.", "Desktop Rat", MB_ICONERROR);
    if (!hwnd_) return false;

    SetLayeredWindowAttributes(hwnd_, COLORKEY, 255, LWA_COLORKEY | LWA_ALPHA);

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);

    
    return true;
}

void Fish::SetFrames(const std::vector<Frame>* walkFrames) {
    walkFrames_ = walkFrames;
}

const std::vector<Frame>* Fish::CurrentAnim() const {
    return walkFrames_;
}

void Fish::AdvanceFrame() {
    auto* anim = CurrentAnim();
    if (!anim || anim->empty()) return;

    if (reverse) {
        curFrame_--;
    } else {
        curFrame_++;
    }

    if (curFrame_ <= 0 || curFrame_ >= static_cast<int>(anim->size()) - 1) {
        reverse = !reverse;
        reves_count_++;
    }
    if (reves_count_ == 3) {
        facingLeft_ = !facingLeft_;
        reves_count_ = 0;
    }
}

void Fish::NextFrame() {
    if (paused_anim_) return;
    AdvanceFrame();
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void Fish::DrawCurrentFrame(HDC hdc) {
    auto* anim = CurrentAnim();
    if (!anim || anim->empty()) return;

    int idx = curFrame_ % static_cast<int>(anim->size());
    const Frame& fr = (*anim)[idx];
    if (!fr.hBitmap) return;

    HDC hdcMem  = CreateCompatibleDC(hdc);
    HBITMAP old = (HBITMAP)SelectObject(hdcMem, fr.hBitmap);

    if (!facingLeft_) {
        BitBlt(hdc, 0, 0, fr.width, fr.height, hdcMem, 0, 0, SRCCOPY);
    } else {
        StretchBlt(hdc, fr.width - 1, 0, -fr.width, fr.height,
                   hdcMem,            0, 0,  fr.width, fr.height,
                   SRCCOPY);
    }

    SelectObject(hdcMem, old);
    DeleteDC(hdcMem);
}

// Летание
void Fish::UpdatePhysics() {
    if (send_base_mode_) return;
    if (paused_move_hard_ && paused_move) {
        SetWindowPos(hwnd_, HWND_TOPMOST,
                 static_cast<int>(BASE_X), static_cast<int>(BASE_Y),
                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
        return;
    }

    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int originX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int originY = GetSystemMetrics(SM_YVIRTUALSCREEN);

    float minX = static_cast<float>(originX);
    float maxX = static_cast<float>(originX + screenW - SPRITE_W);
    float minY = static_cast<float>(originY);
    float maxY = static_cast<float>(originY + screenH - SPRITE_H);

    //if (GetWindowRect(hwnd_, &fish_win_)) {
    posX_ = posX_ + velX_;
    posY_ = posY_ + velY_;
    //}

    if (posX_ < minX) {
        posX_ = minX; velX_ = fabsf(velX_); facingLeft_ = false;
    } else if (posX_ > maxX) {
        posX_ = maxX; velX_ = -fabsf(velX_); facingLeft_ = true;
    }

    if (posY_ < minY) {
        posY_ = minY; velY_ = fabsf(velY_);
    } else if (posY_ > maxY) {
        posY_ = maxY; velY_ = -fabsf(velY_);
    }

    SetWindowPos(hwnd_, HWND_TOPMOST,
                 static_cast<int>(posX_), static_cast<int>(posY_),
                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
}
void Fish::MoveBase(bool* end){
    end_move = end;
    send_base_mode_ = true;
    //self->paused_anim_ = true;
    paused_move = true;
    paused_move_hard_ = true;
    DWORD style = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
    style &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtr(hwnd_, GWL_EXSTYLE , style);
    SetWindowPos(hwnd_ , nullptr , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |SWP_NOACTIVATE | SWP_FRAMECHANGED);
}
void Fish::EndMoveBase(){
        ReleaseCapture();
        SendMessage(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        send_base_mode_ = false;
        //paused_anim_    = false;

        DWORD Sstyle = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
        Sstyle |= WS_EX_TRANSPARENT;
        SetWindowLongPtr(hwnd_, GWL_EXSTYLE, Sstyle);
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);


}

LRESULT CALLBACK Fish::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<Fish*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    DWORD style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    case WM_PAINT: {
        if (!self) return DefWindowProc(hwnd, msg, wParam, lParam);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH bgBrush = CreateSolidBrush(COLORKEY);
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        self->DrawCurrentFrame(hdc);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (self && self->send_base_mode_) {
            ReleaseCapture();
            SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);

            if (GetWindowRect(hwnd, &self->fish_win_)) {
                self->BASE_X = self->fish_win_.left;
                self->BASE_Y = self->fish_win_.top;
                self->posX_ = self->BASE_X;
                self->posY_ = self->BASE_Y;
            }

            self->send_base_mode_ = false;
            self->paused_anim_    = false;

            style |= WS_EX_TRANSPARENT;
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, style);
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

            *(self->end_move) =true;
        }

        return 0;
    }

    case WM_DESTROY: {
        return 0;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}