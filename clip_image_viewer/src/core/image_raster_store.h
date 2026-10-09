#pragma once

#include <clip/image/raster.h>

#include <QString>

#include <memory>
#include <optional>
#include <utility>

namespace clip::image_viewer {

// Owns a committed raster store and the temporary file backing it. Sharing
// this object keeps the decoded source stable across render and edit users.
class ImageRasterStore final {
  public:
    ~ImageRasterStore();

    ImageRasterStore(const ImageRasterStore&) = delete;
    ImageRasterStore& operator=(const ImageRasterStore&) = delete;

    [[nodiscard]] static std::shared_ptr<ImageRasterStore>
    create(clip::image::DocumentDescriptor descriptor,
           const clip::image::RasterStoreOptions& options = {}, QString* error = nullptr);
    [[nodiscard]] static std::shared_ptr<ImageRasterStore>
    adopt(QString filePath, std::shared_ptr<clip::image::RasterStore> store);
    [[nodiscard]] static std::shared_ptr<ImageRasterStore>
    retain(QString filePath, std::shared_ptr<clip::image::RasterStore> store,
           std::shared_ptr<const void> cleanupOwner);

    [[nodiscard]] const QString& filePath() const noexcept {
        return filePath_;
    }
    [[nodiscard]] const std::shared_ptr<clip::image::RasterStore>& store() const noexcept {
        return store_;
    }
    [[nodiscard]] std::optional<clip::image::AlphaContent> verifiedAlphaContent() const noexcept {
        return analysis_.alpha_content;
    }
    void setVerifiedAlphaContent(clip::image::AlphaContent content) {
        analysis_.alpha_content = content;
        if (store_)
            (void)store_->set_analysis(analysis_);
    }
    [[nodiscard]] const clip::image::RasterAnalysis& analysis() const noexcept {
        return analysis_;
    }
    void setAnalysis(clip::image::RasterAnalysis analysis) {
        analysis_ = std::move(analysis);
        if (store_)
            (void)store_->set_analysis(analysis_);
    }
    [[nodiscard]] bool isValid() const noexcept {
        return !filePath_.isEmpty() && store_ != nullptr;
    }

  private:
    ImageRasterStore(QString filePath, std::shared_ptr<clip::image::RasterStore> store,
                     std::shared_ptr<const void> cleanupOwner = {});

    std::shared_ptr<const void> cleanupOwner_;
    QString filePath_;
    std::shared_ptr<clip::image::RasterStore> store_;
    clip::image::RasterAnalysis analysis_;
};

} // namespace clip::image_viewer
