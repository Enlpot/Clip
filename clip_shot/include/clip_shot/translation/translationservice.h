#ifndef CLIP_SHOT_TRANSLATION_TRANSLATIONSERVICE_H
#define CLIP_SHOT_TRANSLATION_TRANSLATIONSERVICE_H

#include "clip_shot/app/edition.h"

#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
#include "clip_shot/network/clipshotapiclient.h"

#include <QLocale>
#include <QStringList>
#endif

#include <QString>

namespace clip_shot::storage {
class ConfigurationStore;
}

namespace clip_shot::translation {
struct TranslationPreferences {
    QString sourceLanguage;
    QString targetLanguage;
    QString modelId;
    QString secondaryTargetLanguage = QStringLiteral("en");
    friend bool operator==(const TranslationPreferences&, const TranslationPreferences&) = default;
};

class TranslationService;
class TranslationJob;

#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
// One catalog and settings binding per client/store pair. The application owns the client;
// standalone consumers may supply an isolated client and store through the same boundary.
class TranslationService final : public QObject {
    Q_OBJECT
  public:
    enum class ChangeReason { UserPreferences, Catalog, ModelInvalidated };
    static TranslationService& forClient(ClipShotApiClient& client,
                                         storage::ConfigurationStore& settings,
                                         const QLocale& locale);
    ~TranslationService() override;
    const QVector<ClipShotChatModel>& models() const {
        return m_models;
    }
    const TranslationPreferences& preferences() const {
        return m_preferences;
    }
    bool loadingModels() const {
        return m_modelsToken != 0;
    }
    QString errorText() const;
    static QString modelConfigurationChangedText();
    bool savePreferences(const TranslationPreferences& preferences);
    void refreshModels(bool force = false);
    void setLocale(const QLocale& locale);
    TranslationJob* createJob(const QStringList& texts, QObject* owner);
    TranslationJob* createJob(const QStringList& texts, const TranslationPreferences& preferences,
                              QObject* owner);

  signals:
    void catalogChanged();
    void preferencesChanged(clip_shot::translation::TranslationService::ChangeReason reason);
    void modelInvalidated(const QString& id);
    void unavailable();

  private:
    friend class TranslationJob;
    TranslationService(ClipShotApiClient& client, storage::ConfigurationStore& settings,
                       QLocale locale);
    void syncPreferences(ChangeReason reason);
    void publishModels(bool resolveSelection);
    QPointer<ClipShotApiClient> m_client;
    QPointer<storage::ConfigurationStore> m_settings;
    QLocale m_locale;
    QString m_defaultTarget;
    QVector<ClipShotChatModel> m_models;
    TranslationPreferences m_preferences;
    ClipShotApiClient::RequestToken m_modelsToken = 0;
    QString m_catalogError;
    bool m_catalogAttempted = false;
    bool m_storageError = false;
    bool m_modelInvalidationPending = false;
    bool m_saving = false;
};

class TranslationJob final : public QObject {
    Q_OBJECT
  public:
    enum class State {
        Idle,
        WaitingForModels,
        Streaming,
        Completed,
        Failed,
        Cancelled,
        Invalidated
    };
    struct Unit {
        QString sourceText;
        QString text;
        State state = State::Idle;
    };
    ~TranslationJob() override;
    void start();
    void retry();
    void cancel();
    State state() const {
        return m_state;
    }
    bool busy() const {
        return m_state == State::WaitingForModels || m_state == State::Streaming;
    }
    const QVector<Unit>& units() const {
        return m_units;
    }
    const TranslationPreferences& preferences() const {
        return m_preferences;
    }
    const ClipShotTranslationResult& result() const {
        return m_result;
    }
    QString errorText() const;

  signals:
    void stateChanged();
    void unitChanged(int index);
    void finished();
    void invalidated();

  private:
    friend class TranslationService;
    TranslationJob(TranslationService& service, const QStringList& texts, QObject* owner);
    void prepare();
    void pump();
    void fail(const QString& error);
    void invalidateModel(const QString& id);
    QPointer<TranslationService> m_service;
    QPointer<ClipShotApiClient> m_client;
    QVector<Unit> m_units;
    TranslationPreferences m_preferences;
    QString m_translationMode;
    QHash<int, ClipShotApiClient::RequestToken> m_requests;
    ClipShotTranslationResult m_result;
    State m_state = State::Idle;
    quint64 m_generation = 0;
    bool m_rateLimited = false;
    bool m_fixedPreferences = false;
};
#endif
} // namespace clip_shot::translation
#endif
