#include <windows.h>
#include "window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    DesktopRat rat;

    if (!rat.Create(hInstance, nCmdShow)) {
        MessageBox(nullptr, L"Не удалось создать окно", L"Ошибка", MB_ICONERROR);
        return 1;
    }

    rat.RunMessageLoop();
    return 0;
}
