#include "clip_shot/network/webdavclient.h"

#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <functional>

namespace clip_shot::webdav {
namespace {

constexpr int kTransferTimeoutMs = 30000;

QByteArray propfindBody() {
    return QByteArrayLiteral(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<d:propfind xmlns:d=\"DAV:\">"
        "<d:prop>"
        "<d:getlastmodified/><d:getcontentlength/><d:resourcetype/><d:getetag/>"
        "</d:prop>"
        "</d:propfind>");
}

QString decodedHref(const QString& raw) {
    // href is percent-encoded; decode for comparison with requested paths.
    return QUrl::fromPercentEncoding(raw.toUtf8());
}

} // namespace

WebDavClient::WebDavClient(QObject* parent) : QObject(parent) {}

void WebDavClient::setCredentials(const QUrl& baseUrl, const QString& username,
                                   const QString& password) {
    m_baseUrl = baseUrl;
    m_username = username;
    m_password = password;
}

void WebDavClient::setIgnoreSslErrors(bool ignore) {
    m_ignoreSslErrors = ignore;
}

QUrl WebDavClient::requestUrl(const QString& path) const {
    QUrl url = m_baseUrl;
    QString fullPath = url.path();
    if (!fullPath.endsWith(u'/') && !path.startsWith(u'/')) {
        fullPath += u'/';
    }
    fullPath += path;
    url.setPath(fullPath, QUrl::DecodedMode);
    return url;
}

QString WebDavClient::encodedPath(const QString& path) const {
    // Request paths are server paths relative to the sync root. Keep them
    // decoded internally; QUrl encodes when building the request.
    return path;
}

QNetworkReply* WebDavClient::send(const QString& method, const QString& path,
                                  const QByteArray& body, const QString& contentType,
                                  QVector<QPair<QByteArray, QByteArray>> extraHeaders,
                                  PendingRequest pending) {
    QNetworkRequest request(requestUrl(path));
    request.setTransferTimeout(kTransferTimeoutMs);
    if (!m_username.isEmpty()) {
        const QByteArray credentials =
            (m_username.toUtf8() + ':' + m_password.toUtf8()).toBase64();
        request.setRawHeader("Authorization", "Basic " + credentials);
    }
    if (!body.isEmpty() || method == QLatin1String("PUT")) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
        request.setHeader(QNetworkRequest::ContentLengthHeader, body.size());
    }
    for (const auto& header : extraHeaders) {
        request.setRawHeader(header.first, header.second);
    }
    QNetworkReply* reply = nullptr;
    if (method == QLatin1String("PROPFIND")) {
        reply = m_manager.sendCustomRequest(request, "PROPFIND", body);
    } else if (method == QLatin1String("MKCOL")) {
        reply = m_manager.sendCustomRequest(request, "MKCOL", body);
    } else if (method == QLatin1String("PUT")) {
        reply = m_manager.put(request, body);
    } else if (method == QLatin1String("GET")) {
        reply = m_manager.get(request);
    } else {
        reply = m_manager.deleteResource(request);
    }
    if (m_ignoreSslErrors) {
        connect(reply, &QNetworkReply::sslErrors, reply,
                [](const QList<QSslError>&) {});
        reply->ignoreSslErrors();
    }
    connect(reply, &QNetworkReply::finished, this, [this, reply, pending]() {
        handleFinished(reply, pending);
    });
    return reply;
}

void WebDavClient::list(const QString& path) {
    list(path, [this, path](QVector<WebDavResource> resources, const WebDavError& error) {
        emit listFinished(path, resources, error);
    });
}

void WebDavClient::list(const QString& path,
                        std::function<void(QVector<WebDavResource>, const WebDavError&)> callback) {
    PendingRequest pending{path, {}, nullptr, std::move(callback), nullptr};
    send(QStringLiteral("PROPFIND"), path, propfindBody(),
         QStringLiteral("application/xml"),
         {{"Depth", "1"}},
         std::move(pending));
}

void WebDavClient::exists(const QString& path) {
    exists(path, [this, path](bool exists, const WebDavError& error) {
        emit existsFinished(path, exists, error);
    });
}

void WebDavClient::exists(const QString& path,
                          std::function<void(bool, const WebDavError&)> callback) {
    PendingRequest pending{path, {}, nullptr, nullptr, std::move(callback)};
    send(QStringLiteral("PROPFIND"), path, propfindBody(),
         QStringLiteral("application/xml"),
         {{"Depth", "0"}},
         std::move(pending));
}

void WebDavClient::ensureDirectory(const QString& path) {
    // Split into segments and create each missing level. The client issues the
    // checks serially: parent must exist before MKCOL creates the child.
    const QStringList segments = path.split(u'/', Qt::SkipEmptyParts);
    QString current;
    std::function<void(int)> createLevel = [this, &createLevel, &segments, &current,
                                            path](int index) {
        if (index >= segments.size()) {
            emit ensureDirectoryFinished(path, true, WebDavError::ok());
            return;
        }
        if (index > 0) {
            current += u'/';
        }
        current += segments[index];
        exists(current, [this, &createLevel, &segments, &current, index, path](
                            bool exists, const WebDavError& error) {
            if (error.failed) {
                emit ensureDirectoryFinished(path, false, error);
                return;
            }
            if (exists) {
                createLevel(index + 1);
                return;
            }
            send(QStringLiteral("MKCOL"), current, {}, {},
                 {},
                 PendingRequest{current, {},
                                [this, &createLevel, &segments, &current, index, path](
                                    QNetworkReply* reply, const PendingRequest&) {
                                    const int status =
                                        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                                            .toInt();
                                    const bool created = status == 201 || status == 301 ||
                                                         status == 405;
                                    if (!created) {
                                        emit ensureDirectoryFinished(
                                            path, false, errorFor(reply));
                                        return;
                                    }
                                    createLevel(index + 1);
                                }});
        });
    };
    // Guard against a base URL that already contains the sync root: the root
    // itself is expected to exist and is not created here.
    if (path.isEmpty() || path == QStringLiteral("/")) {
        emit ensureDirectoryFinished(path, true, WebDavError::ok());
        return;
    }
    createLevel(0);
}

void WebDavClient::upload(const QString& path, const QString& localFilePath) {
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadFinished(path, WebDavError::of(
                                     0, QStringLiteral("The local file could not be read: ") +
                                            localFilePath));
        return;
    }
    const QByteArray bytes = file.readAll();
    file.close();
    send(QStringLiteral("PUT"), path, bytes, QStringLiteral("application/octet-stream"), {},
         PendingRequest{path, {}, [this](QNetworkReply* reply, const PendingRequest& pending) {
             emit uploadFinished(pending.path, errorFor(reply));
         }});
}

void WebDavClient::download(const QString& path, const QString& localFilePath) {
    send(QStringLiteral("GET"), path, {}, {}, {},
         PendingRequest{path, localFilePath,
                        [this](QNetworkReply* reply, const PendingRequest& pending) {
                            const WebDavError error = errorFor(reply);
                            if (error.failed) {
                                emit downloadFinished(pending.path, pending.localFilePath, error);
                                return;
                            }
                            QFile file(pending.localFilePath);
                            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                                emit downloadFinished(
                                    pending.path, pending.localFilePath,
                                    WebDavError::of(
                                        0, QStringLiteral("The local file could not be written: ") +
                                               pending.localFilePath));
                                return;
                            }
                            file.write(reply->readAll());
                            file.close();
                            emit downloadFinished(pending.path, pending.localFilePath,
                                                  WebDavError::ok());
                        }});
}

void WebDavClient::remove(const QString& path) {
    send(QStringLiteral("DELETE"), path, {}, {}, {},
         PendingRequest{path, {}, [this](QNetworkReply* reply, const PendingRequest& pending) {
             emit removeFinished(pending.path, errorFor(reply));
         }});
}

void WebDavClient::abortAll() {
    const auto replies = m_manager.findChildren<QNetworkReply*>();
    for (auto* reply : replies) {
        reply->abort();
    }
}

void WebDavClient::handleFinished(QNetworkReply* reply, const PendingRequest& pending) {
    reply->deleteLater();
    if (pending.handler) {
        pending.handler(reply, pending);
    } else if (pending.resourcesCallback) {
        const WebDavError error = errorFor(reply);
        QVector<WebDavResource> resources;
        if (!error.failed) {
            resources = parseMultiStatus(reply->readAll());
            // Drop the queried directory itself and relativize children.
            QVector<WebDavResource> children;
            const QString base =
                pending.path.endsWith(u'/') ? pending.path : pending.path + u'/';
            for (const auto& resource : resources) {
                if (resource.path == pending.path || resource.path == base) {
                    continue;
                }
                QString relative = resource.path;
                if (relative.startsWith(base)) {
                    relative = relative.mid(base.size());
                } else if (relative.startsWith(pending.path + u'/')) {
                    relative = relative.mid(pending.path.size() + 1);
                } else {
                    continue;
                }
                WebDavResource child = resource;
                child.path = relative;
                children.append(child);
            }
            resources = children;
        }
        pending.resourcesCallback(resources, error);
    } else if (pending.existsCallback) {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const WebDavError error = errorFor(reply);
        dispatch(pending, !error.failed && status >= 200 && status < 300, error);
    }
}

void WebDavClient::dispatch(const PendingRequest& pending, bool exists,
                            const WebDavError& error) {
    if (pending.existsCallback) {
        pending.existsCallback(exists, error);
    }
}

WebDavError WebDavClient::errorFor(QNetworkReply* reply) {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError) {
        return WebDavError::ok();
    }
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return WebDavError::of(0, QStringLiteral("The operation was cancelled"));
    }
    QString message;
    switch (status) {
    case 401:
        message = QStringLiteral("Authentication failed (401). Check the account and password.");
        break;
    case 403:
        message = QStringLiteral("Access denied (403). The account may lack write permission.");
        break;
    case 404:
        message = QStringLiteral("The remote resource does not exist (404).");
        break;
    case 409:
        message = QStringLiteral("Conflict (409). A remote collection may already exist.");
        break;
    case 423:
        message = QStringLiteral("The remote resource is locked (423).");
        break;
    case 507:
        message = QStringLiteral("Insufficient storage (507). The server is out of space.");
        break;
    default:
        message = QStringLiteral("WebDAV request failed: %1")
                      .arg(reply->errorString());
        break;
    }
    return WebDavError::of(status, message);
}

QVector<WebDavResource> WebDavClient::parseMultiStatus(const QByteArray& body) {
    QVector<WebDavResource> resources;
    QXmlStreamReader reader(body);
    WebDavResource current;
    bool insideResponse = false;
    bool insideProp = false;
    bool currentIsDirectory = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            const QStringView name = reader.name();
            if (name == QLatin1String("response")) {
                insideResponse = true;
                current = {};
                currentIsDirectory = false;
            } else if (insideResponse && name == QLatin1String("href")) {
                current.path = decodedHref(reader.readElementText());
            } else if (name == QLatin1String("prop")) {
                insideProp = true;
            } else if (insideProp && name == QLatin1String("collection")) {
                currentIsDirectory = true;
            } else if (insideProp && name == QLatin1String("getlastmodified")) {
                current.lastModified = QDateTime::fromString(
                    reader.readElementText(), Qt::RFC2822Date);
                if (!current.lastModified.isValid()) {
                    // Some servers emit a slightly different format; fall back
                    // to parsing the ISO form.
                    current.lastModified =
                        QDateTime::fromString(reader.text().toString(),
                                              Qt::ISODate);
                }
            } else if (insideProp && name == QLatin1String("getcontentlength")) {
                current.contentLength = reader.readElementText().toLongLong();
            } else if (insideProp && name == QLatin1String("getetag")) {
                current.etag = reader.readElementText().toLatin1();
            }
        } else if (reader.isEndElement()) {
            const QStringView name = reader.name();
            if (name == QLatin1String("response")) {
                if (!current.path.isEmpty()) {
                    current.isDirectory = currentIsDirectory;
                    resources.append(current);
                }
                insideResponse = false;
            } else if (name == QLatin1String("prop")) {
                insideProp = false;
            }
        }
    }
    return resources;
}

} // namespace clip_shot::webdav
