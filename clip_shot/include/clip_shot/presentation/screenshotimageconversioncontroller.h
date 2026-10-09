#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTIMAGECONVERSIONCONTROLLER_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTIMAGECONVERSIONCONTROLLER_H

#include "clip_shot/presentation/screenshotimageconversion.h"
#include "clip_shot/app/edition.h"

#include <QTimer>

class QWidget;
namespace adqt::widgets {
class AdModal;
}

#if CLIP_SHOT_ENABLE_IMAGE_CONVERSION
class ScreenshotImageConversionController final : public QObject {
    Q_OBJECT
  public:
    enum class State { Idle, LoadingModels, Converting, Completed, Failed };
    explicit ScreenshotImageConversionController(QObject* parent = nullptr);
    ~ScreenshotImageConversionController() override;
    void setProvider(ClipShotApiClient* provider);
    // The caller owns target identity through key; image is used for requests and retries.
    void activate(QString key, QImage image, ClipShotImageConversionFormat format);
    void deactivate();
    void invalidate();
    void retry();
    void openSettings(QWidget* owner);
    void seed(const QString& key, const QVector<ScreenshotImageConversionEntry>& entries);
    [[nodiscard]] QVector<ScreenshotImageConversionEntry> entries(const QString& key) const;

    [[nodiscard]] State state() const {
        return m_state;
    }
    [[nodiscard]] bool busy() const {
        return m_state == State::LoadingModels || m_state == State::Converting;
    }
    [[nodiscard]] bool active() const {
        return m_active;
    }
    [[nodiscard]] QString source() const {
        return m_source;
    }
    [[nodiscard]] QString error() const {
        return m_error;
    }
    [[nodiscard]] ClipShotImageConversionFormat format() const {
        return m_format;
    }

  signals:
    void changed();
    void resultsChanged();

  private:
    void start(bool refreshModels);
    void startWithModels(const QVector<ClipShotChatModel>& models);
    void fail(const QString& message);
    void cancelRequests();
    QPointer<ClipShotApiClient> m_api;
    QPointer<adqt::widgets::AdModal> m_modal;
    QHash<QString, QVector<ScreenshotImageConversionEntry>> m_cache;
    QTimer m_previewTimer;
    QString m_key;
    QString m_requestModel;
    QImage m_image;
    QString m_source;
    QString m_error;
    ClipShotImageConversionFormat m_format = ClipShotImageConversionFormat::Markdown;
    State m_state = State::Idle;
    bool m_active = false;
    quint64 m_generation = 0;
    ClipShotApiClient::RequestToken m_modelsToken = 0;
    ClipShotApiClient::RequestToken m_conversionToken = 0;
    ClipShotApiClient::RequestToken m_settingsToken = 0;
};

#endif // CLIP_SHOT_ENABLE_IMAGE_CONVERSION

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTIMAGECONVERSIONCONTROLLER_H
