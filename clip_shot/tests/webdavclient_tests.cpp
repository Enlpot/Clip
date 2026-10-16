#include "clip_shot/network/webdavclient.h"

#include <QCoreApplication>

#include <iostream>
#include <stdexcept>

namespace {
using clip_shot::webdav::WebDavClient;
using clip_shot::webdav::WebDavResource;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// A typical PROPFIND depth:1 response from a Nutstore / generic WebDAV server.
const char kMultiStatus[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<d:multistatus xmlns:d=\"DAV:\">"
    "<d:response>"
    "<d:href>/dav/Clip/</d:href>"
    "<d:propstat><d:prop>"
    "<d:resourcetype><d:collection/></d:resourcetype>"
    "</d:prop><d:status>HTTP/1.1 200 OK</d:status></d:propstat>"
    "</d:response>"
    "<d:response>"
    "<d:href>/dav/Clip/config.json</d:href>"
    "<d:propstat><d:prop>"
    "<d:getlastmodified>Tue, 13 Oct 2026 08:30:00 GMT</d:getlastmodified>"
    "<d:getcontentlength>4096</d:getcontentlength>"
    "<d:resourcetype/>"
    "<d:getetag>\"abc123\"</d:getetag>"
    "</d:prop><d:status>HTTP/1.1 200 OK</d:status></d:propstat>"
    "</d:response>"
    "<d:response>"
    "<d:href>/dav/Clip/capture_history/</d:href>"
    "<d:propstat><d:prop>"
    "<d:resourcetype><d:collection/></d:resourcetype>"
    "</d:prop><d:status>HTTP/1.1 200 OK</d:status></d:propstat>"
    "</d:response>"
    "<d:response>"
    "<d:href>/dav/Clip/capture_history/index.json</d:href>"
    "<d:propstat><d:prop>"
    "<d:getlastmodified>Wed, 14 Oct 2026 02:15:00 GMT</d:getlastmodified>"
    "<d:getcontentlength>128</d:getcontentlength>"
    "<d:resourcetype/>"
    "<d:getetag>\"xyz789\"</d:getetag>"
    "</d:prop><d:status>HTTP/1.1 200 OK</d:status></d:propstat>"
    "</d:response>"
    "</d:multistatus>";

void testParsesDirectoryAndFileEntries() {
    const auto resources =
        WebDavClient::parseMultiStatus(QByteArray(kMultiStatus));
    require(resources.size() == 4, "four resources should be parsed");
    const auto& root = resources[0];
    require(root.path == QStringLiteral("/dav/Clip/"), "root href should be decoded");
    require(root.isDirectory, "root resource should be a collection");
    const auto& config = resources[1];
    require(config.path == QStringLiteral("/dav/Clip/config.json"),
            "file href should be decoded");
    require(!config.isDirectory, "config should not be a collection");
    require(config.contentLength == 4096, "content length should parse");
    require(config.etag == "\"abc123\"", "etag should parse");
    require(config.lastModified.isValid(), "RFC2822 last-modified should parse");
    require(config.lastModified.year() == 2026 && config.lastModified.month() == 10,
            "last-modified date should parse");
    const auto& history = resources[2];
    require(history.isDirectory, "capture_history should be a collection");
    const auto& index = resources[3];
    require(index.contentLength == 128, "nested file length should parse");
}

void testParsesPercentEncodedHref() {
    const char body[] =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<d:multistatus xmlns:d=\"DAV:\">"
        "<d:response>"
        "<d:href>/dav/Clip/records/a%20b/display_0.png</d:href>"
        "<d:propstat><d:prop>"
        "<d:getcontentlength>42</d:getcontentlength>"
        "<d:resourcetype/>"
        "</d:prop><d:status>HTTP/1.1 200 OK</d:status></d:propstat>"
        "</d:response>"
        "</d:multistatus>";
    const auto resources =
        WebDavClient::parseMultiStatus(QByteArray(body));
    require(resources.size() == 1, "one resource should parse");
    require(resources[0].path == QStringLiteral("/dav/Clip/records/a b/display_0.png"),
            "percent-encoded href should be decoded");
}

void testEmptyBodyYieldsNoResources() {
    const auto resources = WebDavClient::parseMultiStatus(QByteArray());
    require(resources.isEmpty(), "empty body should yield no resources");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        testParsesDirectoryAndFileEntries();
        testParsesPercentEncodedHref();
        testEmptyBodyYieldsNoResources();
    } catch (const std::exception& error) {
        std::cerr << "webdav client test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "webdav client tests passed\n";
    return 0;
}
