#include "llviewerprecompiledheaders.h"
#include "llrobotdollyai.h"
#include "robotdolly/ai/viewport.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llframetimer.h"
#include "lldir.h"

namespace {
robotdolly::AIViewport& renderer() {
    static robotdolly::AIViewport instance;
    return instance;
}
}
const std::string& robotDollyAIStatus() { return renderer().status(); }
void stopRobotDollyAI() { renderer().stop(); }

void updateRobotDollyAI() {
    auto& ai = renderer();
    const bool requested = gSavedSettings.getBOOL("RobotDollyAIEnabled");
    if (!requested) {
        if (ai.running()) ai.stop();
        return;
    }
    if (!ai.running()) {
        const std::string path = gSavedSettings.getString("RobotDollyAIRenderer");
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
