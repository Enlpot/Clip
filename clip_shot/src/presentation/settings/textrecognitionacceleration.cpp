#include "clip_shot/presentation/settings/textrecognitionacceleration.h"

#include <QLibrary>

namespace clip_shot::presentation::settings {

bool directMlTextRecognitionSupported() {
    // Loading the provider DLL is a cheap, process-local capability hint. The
    // OCR child performs the definitive ONNX provider probe once at startup.
    static const bool supported = [] {
#ifdef Q_OS_WIN
        QLibrary directMl(QStringLiteral("DirectML"));
        return directMl.load();
#else
        return false;
#endif
    }();
    return supported;
}

} // namespace clip_shot::presentation::settings
