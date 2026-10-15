#pragma once
#include <memory>
#include <string>

namespace robotdolly {
// Owns only the renderer process it starts. No NVIDIA code is loaded in SL.
class AIViewport {
public:
    AIViewport();
    ~AIViewport();
    AIViewport(const AIViewport&) = delete;
    AIViewport& operator=(const AIViewport&) = delete;
    bool start(void* window, const std::string& executable);
    void stop();
    void poll();
    // Raw client pixels, top-left origin (not UI-scaled coordinates).
    void updateOverlay(int left, int top, int right, int bottom);
    bool running() const;
    const std::string& status() const;
private:
    struct Impl;
    std::unique_ptr<Impl> m;
};
}
