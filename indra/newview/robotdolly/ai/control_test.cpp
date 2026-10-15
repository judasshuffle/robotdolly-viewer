#include "viewport.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#endif
void check(bool ok, const char* what) { if (!ok) throw std::runtime_error(what); }
int main(int argc, char** argv) {
    try {
        robotdolly::AIViewport ai;
        check(!ai.running(), "initial state");
        check(!ai.start(nullptr, ""), "invalid target rejected");
#ifdef _WIN32
        check(argc == 2, "mock path required");
        WNDCLASSW wc{};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"Robot Dolly AI test window";
        RegisterClassW(&wc);
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        HWND window = CreateWindowW(wc.lpszClassName, L"test", WS_OVERLAPPEDWINDOW, 100, 100, 640, 480,
            nullptr, nullptr, wc.hInstance, nullptr);
        check(window != nullptr, "create target");
        auto dir = std::filesystem::temp_directory_path() / ("robot dolly ai " + std::to_string(GetCurrentProcessId()));
        std::filesystem::create_directory(dir);
        auto exe = dir / "wowsidecar.exe";
        std::filesystem::copy_file(argv[1], exe);
        check(!ai.start(window, exe.u8string()), "missing dependencies rejected");
        for (auto name : {"dxgi.dll", "renodx-dlss5.addon64", "nvngx_dlssnr.dll"}) std::ofstream(dir / name).put('x');
        HWND second = CreateWindowW(wc.lpszClassName, L"second", 0, 0, 0, 100, 100,
            nullptr, nullptr, wc.hInstance, nullptr);
        check(!ai.start(window, exe.u8string()), "ambiguous capture target rejected");
        DestroyWindow(second);
        ShowWindow(window, SW_SHOW);
        SetForegroundWindow(window);
        DwmFlush();
        check(ai.start(window, exe.u8string()), "start worker with spaces in path and class");
        for (int i = 0; i < 100 && ai.status().find("Renderer running") == std::string::npos; ++i) {
            Sleep(25); ai.poll();
        }
        check(ai.running() && ai.status().find("Renderer running") != std::string::npos, "read worker status");
        ShowWindow(window, SW_SHOW);
        SetForegroundWindow(window);
        check(GetForegroundWindow() == window, "test target is foreground");
        HWND overlay = FindWindowW(L"SidecarOverlay", nullptr);
        check(overlay != nullptr, "mock overlay exists");
        auto pump = [&]() {
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg); DispatchMessageW(&msg);
            }
            Sleep(30);
        };
        auto verifyGeometry = [&]() {
            for (int i = 0; i < 5; ++i) { ai.updateOverlay(20, 40, 400, 300); pump(); }
            RECT frame{}, actual{}, region{};
            check(SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS,
                &frame, sizeof(frame))), "get visible window frame");
            GetWindowRect(overlay, &actual);
            if (!EqualRect(&frame, &actual)) {
                std::cerr << "frame " << frame.left << ',' << frame.top << ',' << frame.right << ',' << frame.bottom
                          << " overlay " << actual.left << ',' << actual.top << ',' << actual.right << ',' << actual.bottom
                          << " status " << ai.status() << '\n';
            }
            check(EqualRect(&frame, &actual), "whole capture aligned to visible frame");
            HRGN rgn = CreateRectRgn(0, 0, 0, 0);
            check(GetWindowRgn(overlay, rgn) == SIMPLEREGION, "viewport is clipped");
            GetRgnBox(rgn, &region); DeleteObject(rgn);
            POINT origin{}; ClientToScreen(window, &origin);
            RECT expected{20 + origin.x - frame.left, 40 + origin.y - frame.top,
                          400 + origin.x - frame.left, 300 + origin.y - frame.top};
            check(EqualRect(&region, &expected), "clip uses actual viewport and title-bar offset");
            const LONG_PTR flags = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
            check((GetWindowLongPtrW(overlay, GWL_EXSTYLE) & flags) == flags, "cross-process click-through styles");
        };
        verifyGeometry();
        SetWindowPos(window, nullptr, 250, 180, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        verifyGeometry();
        ShowWindow(overlay, SW_HIDE);
        ai.updateOverlay(20, 40, 400, 300); pump();
        check(!IsWindowVisible(overlay), "emergency hide is preserved");
        ShowWindow(overlay, SW_SHOWNOACTIVATE);
        ShowWindow(window, SW_MINIMIZE);
        ai.updateOverlay(20, 40, 400, 300);
        HRGN empty = CreateRectRgn(0, 0, 0, 0);
        check(GetWindowRgn(overlay, empty) == NULLREGION, "minimized viewer masks overlay");
        DeleteObject(empty);
        ShowWindow(window, SW_RESTORE); SetForegroundWindow(window);
        verifyGeometry();
        DWORD oldPid = 0; GetWindowThreadProcessId(overlay, &oldPid);
        SetWindowPos(window, nullptr, 0, 0, 800, 600, SWP_NOMOVE | SWP_NOZORDER);
        for (int i = 0; i < 35; ++i) { ai.updateOverlay(20, 40, 400, 300); pump(); }
        check(ai.running(), "renderer restarted after resize");
        overlay = FindWindowW(L"SidecarOverlay", nullptr);
        DWORD newPid = 0; GetWindowThreadProcessId(overlay, &newPid);
        check(newPid && oldPid != newPid, "resize reallocates via fresh renderer process");
        verifyGeometry();
        ShowWindow(window, SW_MAXIMIZE);
        for (int i = 0; i < 35; ++i) { ai.updateOverlay(20, 40, 400, 300); pump(); }
        overlay = FindWindowW(L"SidecarOverlay", nullptr);
        check(ai.running() && overlay, "renderer follows maximize");
        verifyGeometry();
        ShowWindow(window, SW_RESTORE);
        for (int i = 0; i < 35; ++i) { ai.updateOverlay(20, 40, 400, 300); pump(); }
        overlay = FindWindowW(L"SidecarOverlay", nullptr);
        verifyGeometry();
        ai.stop();
        check(!ai.running(), "stop clears ownership");
        check(!FindWindowExW(HWND_MESSAGE, nullptr, L"DLSS5SidecarControl", nullptr), "child exits on stop");
        SetEnvironmentVariableW(L"ROBOTDOLLY_AI_TEST_MODE", L"passthrough");
        check(ai.start(window, exe.u8string()), "start fallback worker");
        for (int i = 0; i < 100 && ai.running(); ++i) { Sleep(25); ai.poll(); }
        check(!ai.running() && ai.status().find("passthrough") != std::string::npos, "fallback is an error, not AI success");
        SetEnvironmentVariableW(L"ROBOTDOLLY_AI_TEST_MODE", nullptr);
        DestroyWindow(window);
        std::filesystem::remove_all(dir);
#endif
        std::cout << "AI control tests passed (no GPU inference tested)\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
