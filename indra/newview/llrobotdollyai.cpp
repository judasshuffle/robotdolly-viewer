#include "llviewerprecompiledheaders.h"
#include "llrobotdollyai.h"
#include "robotdolly/ai/viewport.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llframetimer.h"
#include "lldir.h"
#include <filesystem>
#include "robotdolly/ai/intensity_config.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace {
robotdolly::AIViewport& renderer() {
    static robotdolly::AIViewport instance;
    return instance;
}
}
const std::string& robotDollyAIStatus() { return renderer().status(); }
void stopRobotDollyAI() { renderer().stop(); }
// Apply the user's preferred strength without rewriting other Sidecar settings.
// Sidecar reads sidecar.toml on startup and projects the neural settings into
// ReShade.ini. Restart only after a successful save, using the normal discovery.
bool applyRobotDollyAIIntensity(std::string& message) {
#ifdef _WIN32
    if (!gDirUtilp) { message = "Viewer installation directory unavailable."; return false; }
    namespace fs = std::filesystem;
    const fs::path bundled = fs::path(gDirUtilp->getExecutableDir()) / "ai-renderer" / "wowsidecar.exe";
    std::error_code ec;
    fs::path executable = bundled;
    if (!fs::is_regular_file(bundled, ec)) {
        executable = fs::path(gSavedSettings.getString("RobotDollyAIRenderer"));
        if (!fs::is_regular_file(executable, ec)) {
            message = "No Sidecar renderer found; intensity not applied.";
            return false;
        }
    }
    const fs::path config = executable.parent_path() / "sidecar.toml";
    if (!robotdolly::saveIntensity(config, gSavedSettings.getF32("RobotDollyAIIntensity"), message))
        return false;
    if (renderer().running()) {
        renderer().stop();
        // updateRobotDollyAI launches only when the checkbox is enabled.
        updateRobotDollyAI();
        message = renderer().running() ? "AI intensity applied; renderer restarting." :
                                     "Settings saved, but renderer could not restart.";
        return renderer().running();
    }
    message = "AI intensity saved; it will apply when enhancement starts.";
    return true;
#else
    message = "AI enhancement requires Windows.";
    return false;
#endif
}

void updateRobotDollyAI() {
    auto& ai = renderer();
    const bool requested = gSavedSettings.getBOOL("RobotDollyAIEnabled");
    if (!requested) {
        if (ai.running()) ai.stop();
        return;
    }
    if (!ai.running()) {
        std::string path = gSavedSettings.getString("RobotDollyAIRenderer");

// Prefer the renderer bundled with ROBOTDOLLY.
if (gDirUtilp)
{
    const std::filesystem::path bundled =
        std::filesystem::path(gDirUtilp->getExecutableDir()) /
        "ai-renderer" / "wowsidecar.exe";

    std::error_code ec;
    if (std::filesystem::is_regular_file(bundled, ec))
    {
        path = bundled.string();
    }
}
        if (!gViewerWindow || !gViewerWindow->getWindow() ||
            !ai.start(gViewerWindow->getWindow()->getPlatformWindow(), path)) {
            gSavedSettings.setBOOL("RobotDollyAIEnabled", false);
        }
        return;
    }
    const LLRect world = gViewerWindow->getWorldViewRectRaw();
    const S32 height = gViewerWindow->getWindowRectRaw().getHeight();
    ai.updateOverlay(world.mLeft, height - world.mTop,
                     world.mRight, height - world.mBottom);
    if (!ai.running()) {
        gSavedSettings.setBOOL("RobotDollyAIEnabled", false);
        return;
    }
    static LLFrameTimer poll_timer;
    if (poll_timer.getElapsedTimeF32() >= 1.f) {
        poll_timer.reset();
        ai.poll();
        if (!ai.running()) gSavedSettings.setBOOL("RobotDollyAIEnabled", false);
    }
}
