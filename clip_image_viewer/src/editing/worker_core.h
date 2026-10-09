#pragma once

#include "editing/edit_export_settings.h"

#include <clip/image/document.h>
#include <clip/image/processing.h>
#include <clip/image/result.h>

#include <QString>
#include <QJsonObject>

#include <stop_token>
#include <chrono>
#include <functional>
#include <optional>

namespace clip::image_viewer::worker_core {

struct PreparationResult final {
    clip::image::Document document;
    bool previewEquivalentToBase = false;
    clip::image::AlphaContent alphaContent = clip::image::AlphaContent::non_opaque;
    std::chrono::nanoseconds alphaClassificationDuration{0};
};

[[nodiscard]] clip::image::Result<PreparationResult>
prepareForExport(clip::image::Document document, const EditExportSettings& settings,
                 const clip::image::EncoderInfo& encoder, QString* warning,
                 std::stop_token stop = {},
                 std::optional<clip::image::AlphaContent> verifiedAlphaContent = {});

using ArtifactReadyCallback = std::function<void(QJsonObject)>;

[[nodiscard]] QJsonObject executeEncodeJob(const QJsonObject& job,
                                           const EditExportSettings& settings,
                                           const ArtifactReadyCallback& artifactReady,
                                           std::stop_token stop, bool* artifactPublished);

[[nodiscard]] QJsonObject executePreviewJob(const QString& artifactPath, const QString& previewPath,
                                            clip::image::Format format, std::stop_token stop);

} // namespace clip::image_viewer::worker_core
