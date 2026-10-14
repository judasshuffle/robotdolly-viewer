#include "viewport.h"
#include <chrono>
#include <filesystem>
#include <cstring>
#include <cwchar>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "sidecar_control.h"
#endif

namespace robotdolly {
struct AIViewport::Impl {
    std::string message = "Off. Choose a DLSS Sidecar renderer to set up.";
#ifdef _WIN32
    HANDLE process = nullptr;
    HANDLE job = nullptr;
    DWORD pid = 0;
    std::chrono::steady_clock::time_point started;
    std::chrono::steady_clock::time_point updated;
    unsigned int sequence = 0;
    bool received = false;

    void close() {
        // Only address the process we created, never another user's overlay.
        HWND control = FindWindowExW(HWND_MESSAGE, nullptr, L"DLSS5SidecarControl", nullptr);
        DWORD owner = 0;
        if (control) GetWindowThreadProcessId(control, &owner);
        if (process && owner == pid) {
            PostMessageW(control, WM_APP + 1, 1, 0); // SidecarCommand::Stop
            WaitForSingleObject(process, 250);
        }
        // KILL_ON_JOB_CLOSE also handles a hung renderer, or viewer shutdown.
        if (job) CloseHandle(job);
        if (process) CloseHandle(process);
        job = process = nullptr;
        pid = 0;
    }
#endif
};

AIViewport::AIViewport() : m(new Impl) {}
AIViewport::~AIViewport() { stop(); }
const std::string& AIViewport::status() const { return m->message; }
bool AIViewport::running() const {
#ifdef _WIN32
    return m->process != nullptr;
#else
    return false;
#endif
}
void AIViewport::stop() {
#ifdef _WIN32
    m->close();
#endif
    m->message = "Off";
}

bool AIViewport::start(void* window, const std::string& executable) {
    stop();
#ifndef _WIN32
    m->message = "AI window enhancement currently requires Windows.";
    return false;
#else
    auto fail = [this](const std::string& message) {
        m->close();
        m->message = message;
        return false;
    };
    HWND target = static_cast<HWND>(window);
    DWORD target_pid = 0;
    if (!IsWindow(target)) return fail("No viewer window is available.");
    GetWindowThreadProcessId(target, &target_pid);
    if (target_pid != GetCurrentProcessId()) return fail("Capture target is not this viewer.");
    wchar_t class_name[256]{};
    if (!GetClassNameW(target, class_name, 256)) return fail("Cannot identify the viewer window.");
    // The existing renderer accepts a window class, not a window handle.
    // Reject ambiguous classes instead of enhancing another viewer instance.
    HWND found = nullptr;
    unsigned int count = 0;
    while ((found = FindWindowExW(nullptr, found, class_name, nullptr))) ++count;
    if (count != 1) return fail("Close other viewer instances before enabling AI.");
    if (FindWindowExW(HWND_MESSAGE, nullptr, L"DLSS5SidecarControl", nullptr))
        return fail("A DLSS Sidecar is already running. Stop it first.");
    if (executable.empty()) return fail("Choose wowsidecar.exe from a configured DLSS Sidecar installation.");

    std::filesystem::path exe;
    try {
        exe = std::filesystem::u8path(executable);
        if (!exe.is_absolute() || !std::filesystem::is_regular_file(exe))
            return fail("The renderer path must point to an existing executable.");
        if (_wcsicmp(exe.filename().c_str(), L"wowsidecar.exe") != 0)
            return fail("Select wowsidecar.exe, not NeuralScreen or the manager.");
        // Fail before launching a misleading passthrough overlay.
        for (const auto* file : {L"dxgi.dll", L"renodx-dlss5.addon64", L"nvngx_dlssnr.dll"}) {
            if (!std::filesystem::is_regular_file(exe.parent_path() / file))
                return fail("Renderer dependencies are missing. Complete DLSS Sidecar setup first.");
        }
    } catch (const std::filesystem::filesystem_error&) {
        return fail("Cannot read the renderer installation.");
    }
    // Windows paths cannot contain quotes. Explicit application path; no shell.
    std::wstring command = L"\"" + exe.wstring() + L"\" \"" + class_name + L"\"";
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(0);
    m->job = CreateJobObjectW(nullptr, nullptr);
    if (!m->job) return fail("Cannot create renderer lifetime guard.");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(m->job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        return fail("Cannot configure renderer lifetime guard.");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    if (!CreateProcessW(exe.c_str(), buffer.data(), nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED, nullptr, exe.parent_path().c_str(), &startup, &info))
        return fail("Windows could not start the renderer (error " + std::to_string(GetLastError()) + ").");
    m->process = info.hProcess;
    m->pid = info.dwProcessId;
    if (!AssignProcessToJobObject(m->job, m->process)) {
        TerminateProcess(m->process, 1); // still suspended; only our child
        CloseHandle(info.hThread);
        return fail("Windows could not supervise the renderer.");
    }
    if (ResumeThread(info.hThread) == static_cast<DWORD>(-1)) {
        CloseHandle(info.hThread);
        return fail("Windows could not resume the renderer.");
    }
    CloseHandle(info.hThread);
    m->started = m->updated = std::chrono::steady_clock::now();
    m->sequence = 0;
    m->received = false;
    m->message = "Starting AI renderer...";
    return true;
#endif
}

void AIViewport::poll() {
#ifdef _WIN32
    if (!m->process) return;
    auto fail = [this](const std::string& reason) { m->close(); m->message = reason; };
    DWORD code = 0;
    if (!GetExitCodeProcess(m->process, &code) || code != STILL_ACTIVE) {
        fail("Renderer stopped. See sidecar.log in its installation folder.");
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Local\\DLSS5SidecarStatus");
    bool valid = false;
    sidecar::SidecarStatus status{};
    if (mapping) {
        const auto* view = static_cast<const sidecar::SidecarStatus*>(
            MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(status)));
        if (view) {
            for (int attempt = 0; attempt < 4; ++attempt) {
                const auto before = *reinterpret_cast<const volatile uint32_t*>(&view->sequence);
                if (before & 1) continue;
                MemoryBarrier();
                std::memcpy(&status, view, sizeof(status));
                MemoryBarrier();
                if (before == *reinterpret_cast<const volatile uint32_t*>(&view->sequence)
                    && status.processId == m->pid) { valid = true; break; }
            }
            UnmapViewOfFile(view);
        }
        CloseHandle(mapping);
    }
    if (valid && status.sequence != m->sequence) {
        m->sequence = status.sequence;
        m->updated = now;
        m->received = true;
        status.passName[sizeof(status.passName)-1] = 0;
        status.lastError[sizeof(status.lastError)-1] = 0;
        if (status.lastError[0] || std::strstr(status.passName, "passthrough")) {
            fail("AI unavailable: " + std::string(status.lastError[0] ? status.lastError : "renderer fell back to passthrough"));
            return;
        }
        // A pass name is not proof that NVIDIA actually changed the pixels.
        m->message = status.frames ? "Renderer running; verify the effect visually." : "Waiting for captured frames...";
    }
    if (!m->received && now - m->started > std::chrono::seconds(60))
        fail("Renderer did not report ready within 60 seconds.");
    // Capture may pause when minimized; do not declare that an error.
#endif
}
}
