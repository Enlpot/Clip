#ifndef CLIP_SHOT_PRESENTATION_TRANSLATIONPAGECONTROLLER_H
#define CLIP_SHOT_PRESENTATION_TRANSLATIONPAGECONTROLLER_H

#include "clip_shot/app/edition.h"

#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
#include "clip_shot/network/clipshotapiclient.h"
#include "clip_shot/translation/translationservice.h"

#include <QLocale>
#include <QObject>
#include <QTimer>
#endif

namespace clip_shot::storage {
class ConfigurationStore;
}

namespace clip_shot::presentation {
class TranslationPageController;

#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
class TranslationPageController final : public QObject {
    Q_OBJECT

  public:
    TranslationPageController(ClipShotApiClient& client, storage::ConfigurationStore& settings,
                              QLocale locale, QObject* parent = nullptr,
                              int debounceMilliseconds = 1500);
    ~TranslationPageController() override;

    void activate();
    void deactivate();
    void setSourceText(const QString& text);
    void setComposing(bool composing);
    bool setPreferences(const QString& source, const QString& target, const QString& model);
    void swapLanguages();
    void retry();
    void setLocale(const QLocale& locale);

    [[nodiscard]] const QString& sourceText() const {
        return m_source;
    }
    [[nodiscard]] const QString& resultText() const {
        return m_result;
    }
    [[nodiscard]] const translation::TranslationPreferences& preferences() const {
        static const translation::TranslationPreferences empty;
        return m_service != nullptr ? m_service->preferences() : empty;
    }
    [[nodiscard]] const QVector<ClipShotChatModel>& models() const {
        static const QVector<ClipShotChatModel> empty;
        return m_service != nullptr ? m_service->models() : empty;
    }
    [[nodiscard]] bool loadingModels() const {
        return m_active && m_service != nullptr && m_service->loadingModels();
    }
    [[nodiscard]] bool translating() const {
        return m_job != nullptr && m_job->busy();
    }
    [[nodiscard]] bool active() const {
        return m_active;
    }
    [[nodiscard]] QString errorText() const;

  signals:
    // Stream content is separate from preferences, request lifecycle, and error state.
    void resultChanged();
    void stateChanged();

  private:
    void invalidateTranslation();
    void scheduleTranslation();
    void startTranslation();
    QPointer<translation::TranslationService> m_service;
    QPointer<translation::TranslationJob> m_job;
    QTimer m_debounce;
    QString m_source;
    QString m_result;
    bool m_active = false;
    bool m_composing = false;
    bool m_requestDue = false;
    bool m_retryRequired = false;
};
#endif
} // namespace clip_shot::presentation

#endif
