#ifndef CLIP_SHOT_APP_APPLICATIONRESTART_H
#define CLIP_SHOT_APP_APPLICATIONRESTART_H

#include <QString>
#include <QStringList>

#include <functional>

class QCoreApplication;

namespace clip_shot::app {

struct ApplicationRestartResult {
    bool success = false;
    QString error;
};

struct ApplicationRestartTransactionOperations {
    std::function<ApplicationRestartResult()> prepare;
    std::function<bool()> guard;
    std::function<bool()> flush;
    std::function<ApplicationRestartResult()> commit;
    std::function<void()> cancel;
    std::function<void()> quit;
};

[[nodiscard]] ApplicationRestartResult
runApplicationRestartTransaction(const ApplicationRestartTransactionOperations& operations);

class ApplicationRestartCoordinator final {
  public:
    explicit ApplicationRestartCoordinator(QCoreApplication& application);

    [[nodiscard]] ApplicationRestartResult restart(const std::function<bool()>& guard,
                                                   const std::function<bool()>& flush);

  private:
    QCoreApplication& m_application;
    bool m_pending = false;
};

// Returns -1 when the launch is ordinary or when a committed helper should continue through
// normal startup. Other values are helper failure exit codes.
[[nodiscard]] int dispatchApplicationRestartHelper(const QStringList& arguments);
[[nodiscard]] bool isApplicationRestartHelperLaunch(const QStringList& arguments);
[[nodiscard]] QStringList normalApplicationArguments(const QStringList& arguments);

} // namespace clip_shot::app

#endif // CLIP_SHOT_APP_APPLICATIONRESTART_H
