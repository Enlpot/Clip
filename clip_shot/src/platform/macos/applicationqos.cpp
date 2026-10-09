#include "clip_shot/platform/applicationqos.h"
#include "clip_shot/diagnostics/diagnostics.h"

namespace clip_shot::platform {
namespace {
extern "C" void reportQoSError(int code) {
    diagnostics::logEvent(QStringLiteral("clip_shot.platform"),
                          QStringLiteral("scheduling.qos_failed"), {{QStringLiteral("code"), code}},
                          QtWarningMsg);
}
} // namespace

void initializeApplicationQoS(ApplicationQoS qos) {
    const int code =
        clip_application_qos_initialize(static_cast<unsigned int>(qos), reportQoSError);
    if (code != 0)
        reportQoSError(code);
    applyApplicationQoSToCurrentThread();
}

} // namespace clip_shot::platform
