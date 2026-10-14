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
    bool running() const;
    const std::string& status() const;
private:
    struct Impl;
    std::unique_ptr<Impl> m;
};
}
