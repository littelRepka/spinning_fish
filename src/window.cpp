#include "window.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//#include<iostream>
// ============================================================
//  ЭТАП 1 — Прозрачное окно
// ============================================================
bool DesktopRat::Create(HINSTANCE hInstance, int nCmdShow) {
    hInst_ = hInstance;

    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"DesktopRatWindow";
    wc.hbrBackground = nullptr;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassEx(&wc)) return false;

    // --- Этап 2: спрайты ---
    if (!LoadSprites("resources/sprites")) {
        MessageBoxA(nullptr,
            "Не удалось загрузить спрайты из resources/sprites/\n"
            "Убедитесь, что папка есть рядом с exe.",
            "Desktop Rat", MB_ICONERROR);
        return false;
    }

    // Начальная позиция — центр экрана
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    posX_ = static_cast<float>((screenW - SPRITE_W) / 2);
    posY_ = static_cast<float>((screenH - SPRITE_H) / 2);
    BASE_Y = screenH - 155;
    BASE_X = screenW -130;

    DWORD exStyle = WS_EX_LAYERED
                  | WS_EX_TRANSPARENT
                  | WS_EX_TOPMOST
                  | WS_EX_TOOLWINDOW;  // скрыть из таскбара / Alt+Tab

    hwnd_ = CreateWindowEx(
        exStyle,
        L"DesktopRatWindow",
        L"Desktop Rat",
        WS_POPUP,
        static_cast<int>(posX_), static_cast<int>(posY_),
        SPRITE_W, SPRITE_H,
        nullptr, nullptr, hInstance, this
    );
    if (!hwnd_) return false;

    SetLayeredWindowAttributes(hwnd_, COLORKEY, 255, LWA_COLORKEY | LWA_ALPHA);

    // --- Этап 3: таймер анимации ---
    SetTimer(hwnd_, TIMER_ANIM_ID, TIMER_ANIM_MS, nullptr);

    // --- Этап 4: таймер физики ---
    SetTimer(hwnd_, TIMER_MOVE_ID, TIMER_MOVE_MS, nullptr);

    SetTimer(hwnd_ , TIMER_LONG_ID, TIMER_LONG_MS, nullptr );

    // --- Этап 5: иконка в трее ---
    if (!AddTrayIcon()) {
        // Не критично — продолжаем без трея
    }

    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
    return true;
}

void DesktopRat::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// ============================================================
//  ЭТАП 2 — Загрузка спрайтов
// ============================================================
bool DesktopRat::LoadFrame(const std::string& path, Frame& out) {
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!data) return false;

    BITMAPINFO bmi              = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits  = nullptr;
    HDC hdcScr   = GetDC(nullptr);
    HBITMAP hBmp = CreateDIBSection(hdcScr, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ReleaseDC(nullptr, hdcScr);
    if (!hBmp || !pBits) { stbi_image_free(data); return false; }

    int pixels         = w * h;
    unsigned char* src = data;
    unsigned char* dst = reinterpret_cast<unsigned char*>(pBits);
    for (int i = 0; i < pixels; ++i) {
        unsigned char r = src[0], g = src[1], b = src[2], a = src[3];
        if (a < 128) {
            dst[0] = 255; dst[1] = 0; dst[2] = 255; dst[3] = 0;
        } else {
            dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = 0;
        }
        src += 4; dst += 4;
    }
    stbi_image_free(data);

    out.hBitmap = hBmp;
    out.width   = w;
    out.height  = h;
    return true;
}

bool DesktopRat::LoadSprites(const std::string& dir) {
    for (int i = 1; i <= 6; ++i) {
        Frame f;
        if (!LoadFrame(dir + "/ezgif-frame-00" + std::to_string(i) + ".png", f)) return false;
        walkFrames_.push_back(f);
    }
    /*
    for (int i = 0; i < 2; ++i) {
        Frame f;
        if (!LoadFrame(dir + "/idle_" + std::to_string(i) + ".png", f)) return false;
        idleFrames_.push_back(f);
    }
    */
    return true;
    
}

void DesktopRat::FreeFrames() {
    for (auto& f : walkFrames_) if (f.hBitmap) DeleteObject(f.hBitmap);
  // for (auto& f : idleFrames_) if (f.hBitmap) DeleteObject(f.hBitmap);
    walkFrames_.clear();
   // idleFrames_.clear();
}

// ============================================================
//  ЭТАП 3 — Анимация
// ============================================================
std::vector<Frame>* DesktopRat::CurrentAnim() {
    return &walkFrames_;
    //return isWalking_ ? &walkFrames_ : &idleFrames_;
}

void DesktopRat::AdvanceFrame() {
    auto* anim = CurrentAnim();
    if (!anim || anim->empty()) return;

    if(reverse){
        curFrame_--; 
    }else{
        curFrame_++;
        
        //curFrame_ = (curFrame_ + 1) % static_cast<int>(anim->size());
    }
    //std::cout << curFrame_;
    if (curFrame_ == 0 || curFrame_ == (anim->size()-1)) reverse =!reverse;
    
}

void DesktopRat::DrawCurrentFrame(HDC hdc) {
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

// ============================================================
//  ЭТАП 4 — Физика
// ============================================================
void DesktopRat::UpdatePhysics() {
    if(send_base_mode_) return;
    if (paused_move_hard_ && paused_move_) {
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

    posX_ += velX_;
    posY_ += velY_;

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

// ============================================================
//  ЭТАП 5 — Системный трей
// ============================================================
bool DesktopRat::AddTrayIcon() {
    // Загружаем иконку из файла рядом с exe
    HICON hIcon = (HICON)LoadImage(
        nullptr,
        L"resources/tray_icon.ico",
        IMAGE_ICON,
        16, 16,
        LR_LOADFROMFILE
    );
    if (!hIcon) {
        // Запасной вариант — стандартная системная иконка
        hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    nid_               = {};
    nid_.cbSize        = sizeof(NOTIFYICONDATA);
    nid_.hWnd          = hwnd_;
    nid_.uID           = TRAY_ID;
    nid_.uFlags        = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;
    nid_.hIcon         = hIcon;
    lstrcpyn(nid_.szTip, L"Desktop Rat", ARRAYSIZE(nid_.szTip));

    trayAdded_ = Shell_NotifyIcon(NIM_ADD, &nid_) == TRUE;
    return trayAdded_;
}

void DesktopRat::RemoveTrayIcon() {
    if (trayAdded_) {
        Shell_NotifyIcon(NIM_DELETE, &nid_);
        trayAdded_ = false;
    }
    if (nid_.hIcon) {
        DestroyIcon(nid_.hIcon);
        nid_.hIcon = nullptr;
    }
}

void DesktopRat::ShowTrayMenu() {
    // Получаем позицию курсора для меню
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // Пункт "Пауза" / "Продолжить"
    if (!paused_anim_) {
        AppendMenu(hMenu, MF_STRING, IDM_PAUSE, L" Пауза");
    } else {
        AppendMenu(hMenu, MF_STRING, IDM_PAUSE, L"Продолжить");
    }

    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);

    // Пункты видимости
    bool visible = (GetWindowLong(hwnd_, GWL_STYLE) & WS_VISIBLE) != 0;
    if (visible) {
        AppendMenu(hMenu, MF_STRING, IDM_HIDE, L" Скрыть");
    } else {
        AppendMenu(hMenu, MF_STRING, IDM_SHOW, L" Показать");
    }

    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    if(paused_move_hard_){
        AppendMenu(hMenu, MF_STRING, IDM_POSIH, L"Летать");
    } else{
        AppendMenu(hMenu, MF_STRING, IDM_POSIH, L"В угол");
    }

    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_BASE, L"Место");

    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Выход");
    

    // Обязательно устанавливаем hwnd_ на передний план перед TrackPopupMenu,
    // иначе меню не закроется при клике вне него
    SetForegroundWindow(hwnd_);

    TrackPopupMenu(hMenu,
                   TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                   pt.x, pt.y,
                   0, hwnd_, nullptr);

    // Посылаем пустое сообщение — трюк для правильного закрытия меню
    PostMessage(hwnd_, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

// ============================================================
//  WndProc
// ============================================================
LRESULT CALLBACK DesktopRat::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<DesktopRat*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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

    case WM_TIMER: {
        if (!self) break;

        if (wParam == TIMER_ANIM_ID) {
            if (!self->paused_anim_) {
                self->AdvanceFrame();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (wParam == TIMER_MOVE_ID) {
            self->UpdatePhysics();
        }else if (wParam = TIMER_LONG_ID){
            // inactiviti
            if(GetLastInputInfo(&self->last_act_)){
                auto time = GetTickCount() - self->last_act_.dwTime;
                if(time >= TIME_INACTIVE){
                    self->paused_move_ = false;
                }else{
                    self->paused_move_ = true;
                }
            }
            //full screen block
            if(SUCCEEDED(SHQueryUserNotificationState(&self->state)) && 
            (self->state == QUNS_NOT_PRESENT || self->state == QUNS_RUNNING_D3D_FULL_SCREEN 
                ||  self->state == QUNS_BUSY)){
                    ShowWindow(hwnd, SW_HIDE);
                    self->paused_anim_ =true;
                    self->paused_move_ =true;
                }else{
                   // ShowWindow(hwnd, SW_SHOW);
                   // self->paused_anim_ =false;
                   // self->paused_move_ =true;
                }

        }
        return 0;
    }

    // ── Этап 5: сообщения от иконки трея ─────────────────────
    case WM_TRAYICON: {
        if (!self) break;

        // lParam содержит событие мыши
        switch (LOWORD(lParam)) {
        case WM_RBUTTONUP:          // правая кнопка — контекстное меню
            self->ShowTrayMenu();
            break;
        case WM_LBUTTONDBLCLK:     // двойной клик — переключить видимость
            if (IsWindowVisible(hwnd))
                ShowWindow(hwnd, SW_HIDE);
            else
                ShowWindow(hwnd, SW_SHOW);
            break;
        }
        return 0;
    }

    // ── Этап 5: команды из контекстного меню ─────────────────
    case WM_COMMAND: {
        if (!self) break;
        switch (LOWORD(wParam)) {

        case IDM_PAUSE:
            self->paused_anim_ = !self->paused_anim_;
            // Если ставим на паузу — показываем idle-анимацию
            self->isWalking_ = !self->paused_anim_;
            self->curFrame_  = 0;
            break;

        case IDM_SHOW:
            ShowWindow(hwnd, SW_SHOW);
            break;

        case IDM_HIDE:
            ShowWindow(hwnd, SW_HIDE);
            break;
        
        case IDM_POSIH:
            self->paused_move_hard_ = !self->paused_move_hard_;
            break;
        case IDM_BASE:
            self->send_base_mode_ = true;
            //self->paused_anim_ = true;
            self->paused_move_ = true;
            self->paused_move_hard_ = true;

            style &= ~WS_EX_TRANSPARENT;
            SetWindowLongPtr(hwnd, GWL_EXSTYLE , style);
            SetWindowPos(hwnd , nullptr , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |SWP_NOACTIVATE | SWP_FRAMECHANGED);
            
            break;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
    if ( self->send_base_mode_) { // таскание
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);

        
        if(GetWindowRect(hwnd , &self->fish_win_)){
            self->BASE_X = self->fish_win_.left;
            self->BASE_Y = self->fish_win_.top;
        }

        self->send_base_mode_ =false;
        self->paused_anim_ = false;
            
        //DWORD style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        style |= WS_EX_TRANSPARENT;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE , style);
        SetWindowPos(hwnd , nullptr , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |SWP_NOACTIVATE | SWP_FRAMECHANGED);
            
    }
    return 0;

    case WM_DESTROY: {
        if (self) {
            KillTimer(hwnd, TIMER_ANIM_ID);
            KillTimer(hwnd, TIMER_MOVE_ID);
            KillTimer(hwnd, TIMER_LONG_ID);
            self->RemoveTrayIcon();   
            self->FreeFrames();
        }
        PostQuitMessage(0);
        return 0;
    }

    } // switch

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
