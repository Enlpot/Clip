#ifndef CLIP_SHOT_NETWORK_CLIPSHOTAPICLIENT_H
#define CLIP_SHOT_NETWORK_CLIPSHOTAPICLIENT_H

#include "clip_shot/app/edition.h"
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION || CLIP_SHOT_ENABLE_TABLE_RECOGNITION ||                     \
    CLIP_SHOT_ENABLE_LATEX_RECOGNITION || CLIP_SHOT_ENABLE_IMAGE_CONVERSION ||                     \
    CLIP_SHOT_ENABLE_API_CONFIGURATION
#include "clip_shot/customaimodelconfiguration.h"
#include "clip_shot/texttranslationconfiguration.h"
#endif

#include <QHash>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVector>

#include <functional>

class QNetworkAccessManager;
class ClipShotApiClient;

struct ClipShotTableResult {
    QString html;
    QString error;
    QString code;
    int httpStatus = 0;

    [[nodiscard]] bool succeeded() const {
        return !html.trimmed().isEmpty() && error.isEmpty();
    }
};

struct ClipShotLatexResult {
    QString latex;
    QString error;
    QString code;
    int httpStatus = 0;

    [[nodiscard]] bool succeeded() const {
        return !latex.trimmed().isEmpty() && error.isEmpty();
    }
};

struct ClipShotChatModel {
    QString id;
    QString name;
    bool supportsReasoning = false;
    QString translationMode = QStringLiteral("default");
    bool supportsVision = false;
};

struct ClipShotChatModelsResult {
    QVector<ClipShotChatModel> models;
    QString error;
    QString code;
    int httpStatus = 0;

    [[nodiscard]] bool succeeded() const {
        return !models.isEmpty() && error.isEmpty();
    }
};

struct ClipShotTranslationRequest {
    QString model;
    QString sourceLanguage;
    QString targetLanguage;
    QString text;
    QString translationMode = QStringLiteral("default");
    QString secondaryTargetLanguage;
};

struct ClipShotTranslationResult {
    QString error;
    QString code;
    int httpStatus = 0;
    bool cancelled = false;

    [[nodiscard]] bool succeeded() const {
        return error.isEmpty() && !cancelled;
    }
};

enum class ClipShotImageConversionFormat { Markdown, Html };

struct ClipShotImageConversionRequest {
    QString model;
    QImage image;
    ClipShotImageConversionFormat format = ClipShotImageConversionFormat::Markdown;
};

using ClipShotImageConversionResult = ClipShotTranslationResult;

#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION || CLIP_SHOT_ENABLE_TABLE_RECOGNITION ||                     \
    CLIP_SHOT_ENABLE_LATEX_RECOGNITION || CLIP_SHOT_ENABLE_IMAGE_CONVERSION ||                     \
    CLIP_SHOT_ENABLE_API_CONFIGURATION
class ClipShotApiClient final : public QObject {
    Q_OBJECT

  public:
    using RequestToken = quint64;
    using Completion = std::function<void(ClipShotTableResult)>;
    using LatexCompletion = std::function<void(ClipShotLatexResult)>;
    using ChatModelsCompletion = std::function<void(ClipShotChatModelsResult)>;
    using TranslationDelta = std::function<void(const QString&)>;
    using TranslationCompletion = std::function<void(ClipShotTranslationResult)>;

    explicit ClipShotApiClient(QString baseUrl, QObject* parent = nullptr);
    ~ClipShotApiClient() override;

    [[nodiscard]] static QString configuredBaseUrl(const QString& savedUrl = {});
    [[nodiscard]] QString baseUrl() const {
        return m_baseUrl;
    }
    [[nodiscard]] bool setBaseUrl(const QString& baseUrl);

    [[nodiscard]] bool usesSystemProxy() const;
    void setUseSystemProxy(bool enabled);
    [[nodiscard]] const QVector<ClipShotChatModel>& cachedChatModels() const;
    [[nodiscard]] QString cachedChatModelsLocale() const {
        return m_cachedChatModelsLocale;
    }
    [[nodiscard]] RequestToken extractTable(const QImage& image, QObject* receiver,
                                            Completion completion);
    [[nodiscard]] RequestToken extractLatex(const QImage& image, QObject* receiver,
                                            LatexCompletion completion);
    [[nodiscard]] RequestToken fetchChatModels(const QString& locale, QObject* receiver,
                                               ChatModelsCompletion completion);
    [[nodiscard]] RequestToken streamTranslation(const ClipShotTranslationRequest& request,
                                                 QObject* receiver, TranslationDelta delta,
                                                 TranslationCompletion completion);
    [[nodiscard]] RequestToken
    streamImageConversion(const ClipShotImageConversionRequest& request, QObject* receiver,
                          TranslationDelta delta,
                          std::function<void(ClipShotImageConversionResult)> completion);
    void cancel(RequestToken token);
    void setTextTranslationConfigurations(const clip_shot::TextTranslationConfigurations& values);
    [[nodiscard]] bool isTextTranslation(const QString& id) const;
    [[nodiscard]] int translationConcurrency(const QString& id) const;
    void setCustomModels(const clip_shot::CustomAiModels& models);
    [[nodiscard]] bool isCustomModel(const QString& id) const;
    [[nodiscard]] QString fallbackModel(bool vision) const;
    [[nodiscard]] bool hasBuiltInModels(const QString& locale) const;
    [[nodiscard]] QString modelFingerprint(const QString& id) const;

  signals:
    void baseUrlChanged();
    void chatModelsChanged();
    void customModelInvalidated(const QString& id, bool translation, bool vision);

  public:
    [[nodiscard]] static QImage prepareImage(const QImage& image);
    [[nodiscard]] static QByteArray encodeWebp(const QImage& image);
    [[nodiscard]] static QString formatFailure(int httpStatus, const QString& failureCode,
                                               const QString& description);

  private:
    friend class ClipShotApiClientTestAccess;
    std::function<QByteArray(const QImage&)> m_tableImagePreparation;
    int m_latexTimeoutMs = 65000;
    int m_tableTimeoutMs = 35000;
    struct Request;
    void startLatexUpload(RequestToken token, const QByteArray& webp);
    void finishLatex(RequestToken token, ClipShotLatexResult result);
    void startTableUpload(RequestToken token, const QByteArray& webp);
    void cleanupRequest(Request* request);
    [[nodiscard]] QNetworkAccessManager* networkAccessManager();
    void finish(RequestToken token, ClipShotTableResult result);
    void finishChatModels(RequestToken token, ClipShotChatModelsResult result);
    void finishTranslation(RequestToken token, ClipShotTranslationResult result);
    void submitChatStream(RequestToken token, QByteArray body);
    void pumpCustomChatStreams();
    void startChatStream(RequestToken token, const QByteArray& body);

    const clip_shot::TextTranslationConfiguration* textTranslation(const QString& id) const;
    RequestToken enqueueTextTranslation(const ClipShotTranslationRequest& input, QObject* receiver,
                                        TranslationDelta delta, TranslationCompletion completion);
    void pumpTextTranslations();
    void startTextTranslation(RequestToken token);
    clip_shot::TextTranslationConfigurations m_textTranslations;
    QList<RequestToken> m_translationQueue;
    QList<RequestToken> m_customChatQueue;
    void rebuildAvailableModels();
    const clip_shot::CustomAiModelConfiguration* customModel(const QString& id) const;
    QString m_baseUrl;
    quint64 m_serverGeneration = 0;
    clip_shot::CustomAiModels m_customModels;
    QVector<ClipShotChatModel> m_availableModels;

    bool m_useSystemProxy = false;
    RequestToken m_nextToken = 0;
    QHash<RequestToken, Request*> m_requests;
    QVector<ClipShotChatModel> m_cachedChatModels;
    QString m_cachedChatModelsLocale;
};
#endif

#endif // CLIP_SHOT_NETWORK_CLIPSHOTAPICLIENT_H
