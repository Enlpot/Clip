#pragma once

#include <clip/image/codec.h>
#include <clip/image/io.h>

#include <QString>

#include <cstdint>
#include <memory>

class QIODevice;

namespace clip::image_viewer {

class EncodedArtifact final {
  public:
    [[nodiscard]] clip::image::Format format() const noexcept {
        return format_;
    }
    [[nodiscard]] std::uint64_t byteSize() const noexcept {
        return byteSize_;
    }
    [[nodiscard]] const QString& path() const noexcept {
        return path_;
    }
    [[nodiscard]] const clip::image::Input& input() const noexcept {
        return input_;
    }
    [[nodiscard]] const clip::image::EncodedArtifactReceipt& receipt() const noexcept {
        return receipt_;
    }

    [[nodiscard]] bool copyTo(QIODevice& destination, QString* error = nullptr) const;

    static std::shared_ptr<const EncodedArtifact> adopt(clip::image::Format format,
                                                        const QString& path,
                                                        clip::image::EncodedArtifactReceipt receipt,
                                                        std::shared_ptr<const void> cleanupOwner,
                                                        QString* error = nullptr);
    static std::shared_ptr<const EncodedArtifact> adopt(clip::image::Format format,
                                                        const QString& path,
                                                        std::shared_ptr<const void> cleanupOwner,
                                                        QString* error = nullptr);

  private:
    EncodedArtifact(clip::image::Format format, std::uint64_t byteSize, QString path,
                    clip::image::Input input, clip::image::EncodedArtifactReceipt receipt,
                    std::shared_ptr<const void> cleanupOwner);

    // Declared first so it is destroyed last, after the file-backed input closes.
    std::shared_ptr<const void> cleanupOwner_;
    clip::image::Format format_ = clip::image::Format::unknown;
    std::uint64_t byteSize_ = 0;
    QString path_;
    clip::image::Input input_;
    clip::image::EncodedArtifactReceipt receipt_;
};

} // namespace clip::image_viewer
