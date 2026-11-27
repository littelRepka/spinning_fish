//#include <windows.h>
//#include "window.h"
#include"controler.h"
bool EnsureAutoStart() {
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hKey
    );
    if (status != ERROR_SUCCESS) {
        return false;
    }

    // Проверяем, есть ли уже запись
    status = RegQueryValueExW(hKey, L"SpiningFish", nullptr, nullptr, nullptr, nullptr);
    if (status == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }

    // Если нет — получаем путь к текущему exe и записываем
    wchar_t exePath[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        RegCloseKey(hKey);
        return false;
    }

    std::wstring quotedPath = L"\"" + std::wstring(exePath, len) + L"\"";
    DWORD bytes = static_cast<DWORD>((quotedPath.size() + 1) * sizeof(wchar_t));

    status = RegSetValueExW(
        hKey,
        L"SpiningFish",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(quotedPath.c_str()),
        bytes
    );

    RegCloseKey(hKey);
    return status == ERROR_SUCCESS;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    controler control;

    if (!control.Create(hInstance)) {
        MessageBox(nullptr, L"Не удалось создать окно", L"Ошибка", MB_ICONERROR);
        return 1;
    }
    if(!EnsureAutoStart()){
        MessageBox(nullptr, L"Не удалось создать окно", L"Ошибка", MB_ICONERROR);
    }
    control.RunMessageLoop();
    return 0;
}
