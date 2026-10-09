#ifndef CLIP_SHOT_APP_STARTUPREGISTRATIONREPORT_H
#define CLIP_SHOT_APP_STARTUPREGISTRATIONREPORT_H

#include <QDebug>
#include <QString>

namespace clip_shot::app {

// Automatic startup reconciliation must not bring the application to the foreground.
inline void reportStartupRegistrationFailure(const QString& error) {
    qWarning().noquote() << error;
}

} // namespace clip_shot::app

#endif // CLIP_SHOT_APP_STARTUPREGISTRATIONREPORT_H
