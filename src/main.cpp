#include "App.h"

#include <cwchar>

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdLine, int showCmd) {
    // Optional: --deal N starts on a specific deal number.
    uint32_t deal = 0;
    if (const wchar_t* p = wcsstr(cmdLine, L"--deal ")) deal = static_cast<uint32_t>(wcstoul(p + 7, nullptr, 10));

    App app;
    if (!app.Init(inst, showCmd, deal)) return 1;
    return app.Run();
}
