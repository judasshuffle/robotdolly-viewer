#include "viewport.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
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
        HWND window = CreateWindowW(wc.lpszClassName, L"test", 0, 0, 0, 100, 100,
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
        check(ai.start(window, exe.u8string()), "start worker with spaces in path and class");
        for (int i = 0; i < 100 && ai.status().find("Renderer running") == std::string::npos; ++i) {
            Sleep(25); ai.poll();
        }
        check(ai.running() && ai.status().find("Renderer running") != std::string::npos, "read worker status");
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
