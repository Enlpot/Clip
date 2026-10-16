#ifndef CLIP_SHOT_WEBDAVCLIENT_H
#define CLIP_SHOT_WEBDAVCLIENT_H

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QVector>

class QNetworkReply;

namespace clip_shot::webdav {

struct WebDavResource {
    QString path;                 // Server path, always absolute (starts with '/') and decoded.
    bool isDirectory = false;
    qint64 contentLength = 0;
    QDateTime lastModified;
    QByteArray etag;
};

struct WebDavError {
    bool failed = false;
    int httpStatus = 0;
    QString message;
    static WebDavError ok() { return {}; }
    static WebDavError of(int status, const QString& message) {
        return WebDavError{true, status, message};
    }
};

// Asynchronous WebDAV client built on QNetworkAccessManager. Every operation
// reports through a dedicated signal carrying a trailing WebDavError; callers
// must not assume ordering between unrelated operations. Basic authentication
// is applied to every request so servers such as Nutstore or Synology NAS
// accept the first request without a 401 round trip.
class WebDavClient : public QObject {
    Q_OBJECT
public:
    explicit WebDavClient(QObject* parent = nullptr);

    // baseUrl must point at the sync root (for example
    // https://dav.jianguoyun.com/dav/Clip or
    // https://nas.local:5006/remote.php/dav/files/user/Clip).
    void setCredentials(const QUrl& baseUrl, const QString& username,
                        const QString& password);
    void setIgnoreSslErrors(bool ignore);

    // PROPFIND depth:1. The reported resources exclude the queried directory
    // itself and include the directory entry for each child directory.
    void list(const QString& path);
    // PROPFIND depth:0; reports existence of a single resource.
    void exists(const QString& path);
    // Callback variants used by the sync engine to serialize dependent
    // operations without an event-loop hop per request.
    void list(const QString& path, std::function<void(QVector<WebDavResource>, const WebDavError&)> callback);
    void exists(const QString& path, std::function<void(bool, const WebDavError&)> callback);
    // Recursively creates missing directories for path (MKCOL per level).
    void ensureDirectory(const QString& path);
    // PUT a local file to path.
    void upload(const QString& path, const QString& localFilePath);
    // GET path into localFilePath (parent directory must exist).
    void download(const QString& path, const QString& localFilePath);
    // DELETE path.
    void remove(const QString& path);

    // Cancels every in-flight operation; pending signals are not emitted.
    void abortAll();

    // Parses a PROPFIND multistatus response body into resources. Exposed for
    // deterministic unit tests without a live server.
    static QVector<WebDavResource> parseMultiStatus(const QByteArray& body);

signals:
    void listFinished(const QString& path, QVector<WebDavResource> resources,
                      const webdav::WebDavError& error);
    void existsFinished(const QString& path, bool exists,
                        const webdav::WebDavError& error);
    void ensureDirectoryFinished(const QString& path, bool created,
                                 const webdav::WebDavError& error);
    void uploadFinished(const QString& path, const webdav::WebDavError& error);
    void downloadFinished(const QString& path, const QString& localFilePath,
                          const webdav::WebDavError& error);
    void removeFinished(const QString& path, const webdav::WebDavError& error);

private:
    struct PendingRequest {
        QString path;
        QString localFilePath;
        std::function<void(QNetworkReply*, const PendingRequest&)> handler;
        std::function<void(QVector<WebDavResource>, const WebDavError&)> resourcesCallback;
        std::function<void(bool, const WebDavError&)> existsCallback;
    };

    QNetworkReply* send(const QString& method, const QString& path,
                        const QByteArray& body, const QString& contentType,
                        QVector<QPair<QByteArray, QByteArray>> extraHeaders,
                        PendingRequest pending);

    QUrl requestUrl(const QString& path) const;
    QString encodedPath(const QString& path) const;
    void handleFinished(QNetworkReply* reply, const PendingRequest& pending);
    static void dispatch(const PendingRequest& pending, bool exists, const WebDavError& error);
    static WebDavError errorFor(QNetworkReply* reply);

    QNetworkAccessManager m_manager;
    QUrl m_baseUrl;
    QString m_username;
    QString m_password;
    bool m_ignoreSslErrors = false;
};

} // namespace clip_shot::webdav

#endif // CLIP_SHOT_WEBDAVCLIENT_H
