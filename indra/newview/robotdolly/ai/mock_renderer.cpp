// Test-only process. No NVIDIA runtime or graphics enhancement.
#include "sidecar_control.h"
#include <cstring>
#include <cwchar>
int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || !FindWindowW(argv[1], nullptr)) return 2;
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"DLSS5SidecarControl";
    RegisterClassW(&wc);
    CreateWindowW(wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    HANDLE map = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(sidecar::SidecarStatus), L"Local\\DLSS5SidecarStatus");
    auto* status = static_cast<sidecar::SidecarStatus*>(MapViewOfFile(map, FILE_MAP_WRITE, 0, 0, 0));
    if (!status) return 3;
    wchar_t mode[32]{};
    GetEnvironmentVariableW(L"ROBOTDOLLY_AI_TEST_MODE", mode, 32);
    status->processId = GetCurrentProcessId();
    std::strcpy(status->passName, std::wcscmp(mode, L"passthrough") == 0 ? "passthrough" : "mock renderer");
    for (;;) {
        status->sequence += 1;
        MemoryBarrier();
        ++status->frames;
        MemoryBarrier();
        status->sequence += 1;
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_APP + 1 && msg.wParam == 1) return 0;
            DispatchMessageW(&msg);
        }
        Sleep(25);
    }
}
