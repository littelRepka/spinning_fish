#include "controler.h"
#include "resources.h"
#include "stb_image.h"
#include<random>
controler::controler() = default;

controler::~controler() {
    FreeFrames();
}

bool controler::Create(HINSTANCE hInstance) {
    hInst_ = hInstance;

    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"DesktopRatHost";
    if (!RegisterClassEx(&wc)) return false;

    if (!LoadSprites("resources/sprites")) {
        MessageBoxA(nullptr, " NOT LOADED exe.", "Desktop Rat", MB_ICONERROR);
        return false;
    }

    // Невидимое окно-хост контроллера
    hwnd_ = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        L"DesktopRatHost",
        L"",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr, nullptr, hInstance, this
    );
    if (!hwnd_) return false;

    SetTimer(hwnd_, TIMER_ANIM_ID, TIMER_ANIM_MS, nullptr);
    SetTimer(hwnd_, TIMER_MOVE_ID, TIMER_MOVE_MS, nullptr);
    SetTimer(hwnd_, TIMER_LONG_ID, TIMER_LONG_MS, nullptr);

    AddTrayIcon();

    Fish* f =new Fish;

    if(!f->Create(hInst_ , &walkFrames)){
        MessageBox(nullptr, L"Не удалось создать селедку", L"Ошибка", MB_ICONERROR);
    }
    fisch_.push_back(f);


    return true;
}

void controler::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool controler::LoadFrame(const void* ptr, int size, Frame& out) {
    int w, h, ch;
    unsigned char* data = stbi_load_from_memory(
        static_cast<const stbi_uc*>(ptr), size, &w, &h, &ch, 4
    );
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

bool controler::LoadSprites(const std::string& /*dir*/) {
    for (int i = 0; i < IDR_FRAME_COUNT; i++) {
        HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_FRAME_FIRST + i), RT_RCDATA);
        HGLOBAL data = LoadResource(nullptr, res);
        const void* ptr = LockResource(data);
        int size = SizeofResource(nullptr, res);

        Frame f;
        LoadFrame(ptr, size, f);
        walkFrames.push_back(f);
    }
    return true;
}

void controler::FreeFrames() {
    for (auto& f : walkFrames) {
        if (f.hBitmap) DeleteObject(f.hBitmap);
    }
    walkFrames.clear();
}

bool controler::AddTrayIcon() {
    HICON hIcon = (HICON)LoadImage(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCE(TRAY_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    );

    if (!hIcon) {
        hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    nid_                  = {};
    nid_.cbSize           = sizeof(NOTIFYICONDATA);
    nid_.hWnd             = hwnd_;
    nid_.uID              = CTRL_TRAY_ID;
    nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = CTRL_WM_TRAYICON;
    nid_.hIcon            = hIcon;
    lstrcpyn(nid_.szTip, L"Desktop Rat", ARRAYSIZE(nid_.szTip));

    trayAdded_ = Shell_NotifyIcon(NIM_ADD, &nid_) == TRUE;
    return trayAdded_;
}

void controler::RemoveTrayIcon() {
    if (trayAdded_) {
        Shell_NotifyIcon(NIM_DELETE, &nid_);
        trayAdded_ = false;
    }
    if (nid_.hIcon) {
        DestroyIcon(nid_.hIcon);
        nid_.hIcon = nullptr;
    }
}

void controler::ShowTrayMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;
    if(paused_anim_){
        AppendMenu(hMenu, MF_STRING, IDM_PAUSE, L" Танцуем");
    }else{
        AppendMenu(hMenu, MF_STRING, IDM_PAUSE, L" Пауза");
    }
    
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);

    if(paused_move_hard_){
        AppendMenu(hMenu, MF_STRING, IDM_POSIH, L"Летать");
    }else{
        AppendMenu(hMenu, MF_STRING, IDM_POSIH, L"В угол");
    }

    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    if(is_hidden_){
        AppendMenu(hMenu, MF_STRING, IDM_SHOW, L" Показать");
    }else{
        AppendMenu(hMenu, MF_STRING, IDM_HIDE, L" Скрыть");
    }
    
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_BASE, L"Место");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_NEW, L"ДОБАВИТЬ СЕЛЁДКУ");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Выход");

    SetForegroundWindow(hwnd_);

    TrackPopupMenu(hMenu,
                   TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                   pt.x, pt.y,
                   0, hwnd_, nullptr);

    PostMessage(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}
void controler::NewFisch(){
    Fish* f = new Fish;
    if (!f->Create(hInst_, &walkFrames)) {
        delete f;
        return;
    }

    f->paused_anim_ = paused_anim_;
    f->paused_move_hard_ = paused_move_hard_;
    f->is_hidden_   = is_hidden_;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist_x(200, 1400);
    std::uniform_int_distribution<int> dist_y(200, 720);

    f->BASE_X = dist_x(gen);
    f->BASE_Y = dist_y(gen);

    if (is_hidden_) {
        ShowWindow(f->GetHwnd(), SW_HIDE);
    }

    fisch_.push_back(f);
}

LRESULT CALLBACK controler::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<controler*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    case WM_TIMER: {
        if (!self) break;
        switch (wParam) {

        case TIMER_ANIM_ID:

            for (auto &i : self->fisch_) i->NextFrame();
            break;

        case TIMER_MOVE_ID:

            for(auto &i : self->fisch_) i->UpdatePhysics();
            break;

        case TIMER_LONG_ID:

            if(self->active_move && self->end_move){
                self->active_move = false;
                self->end_move = false;
                for(auto &i : self->fisch_){
                    i->EndMoveBase();
                    i->paused_move_hard_ = self->paused_move_hard_;
                }
            }
            if(GetLastInputInfo(&self->last_act_)){
                auto time = GetTickCount() - self->last_act_.dwTime;

                if(time >= TIME_INACTIVE){

                    for(auto &i : self->fisch_) {
                        i->paused_move = false;
                    }
                    self->timer_inactive_ = true;

                }else if(self->timer_inactive_){

                    self->timer_inactive_ = false;
                    for(auto &i : self->fisch_) i->paused_move = true;
                }
            }
            break;
        }
        return 0;
    }

    case CTRL_WM_TRAYICON: {
        if (!self) break;
        switch (LOWORD(lParam)) {
        case WM_RBUTTONUP:
            self->ShowTrayMenu();
            break;
        case WM_LBUTTONDBLCLK:
            // дабл клик ?
            break;
        }
        return 0;
    }

    case WM_COMMAND: {
        if (!self) break;
        switch (LOWORD(wParam)) {

        case IDM_PAUSE:
            for(auto &i :self->fisch_){
                i->paused_anim_ = !i->paused_anim_;
            }
            self->paused_anim_ = !self->paused_anim_;
            break;

        case IDM_SHOW:
                for (auto &i : self->fisch_){
                    ShowWindow(i->hwnd_, SW_SHOW);
                    i->is_hidden_ = false;
                }
                self->is_hidden_ = false;
            break;

        case IDM_HIDE:
                for (auto &i : self->fisch_){
                    ShowWindow(i->hwnd_, SW_HIDE);
                    i->is_hidden_ = true;
                }
                self->is_hidden_ = true;
            break;

        case IDM_POSIH:
            for (auto &i : self->fisch_){ 
                i->paused_move_hard_ = !i->paused_move_hard_;
                i->posX_  = i->BASE_X;
                i->posY_ = i->BASE_Y;
            }
            self->paused_move_hard_ = !self->paused_move_hard_;
            break;

        case IDM_BASE:
            self->active_move =true;
            self->end_move =false;
            for(auto &i : self->fisch_){
                i->MoveBase(&self->end_move);
            }
            break;
        case IDM_NEW:
            
            self->NewFisch();
            break;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;
    }

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
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
