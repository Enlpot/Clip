#ifndef CLIP_SHOT_STORAGE_PREPAREDPNGIMAGE_H
#define CLIP_SHOT_STORAGE_PREPAREDPNGIMAGE_H

#include <QByteArray>
#include <QSize>

#include <memory>
#include <optional>

namespace clip_shot::storage {

class PreparedPngImage final {
  public:
    PreparedPngImage() = default;

    [[nodiscard]] static std::optional<PreparedPngImage>
    fromBytes(QSize pixelSize, std::shared_ptr<const QByteArray> bytes);
    [[nodiscard]] static std::optional<PreparedPngImage> fromBytes(QSize pixelSize,
                                                                   QByteArray bytes);

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QSize pixelSize() const;
    [[nodiscard]] const QByteArray& bytes() const;
    [[nodiscard]] const std::shared_ptr<const QByteArray>& sharedBytes() const;

  private:
    PreparedPngImage(QSize pixelSize, std::shared_ptr<const QByteArray> bytes);

    QSize m_pixelSize;
    std::shared_ptr<const QByteArray> m_bytes;
    bool m_valid = false;
};

} // namespace clip_shot::storage

#endif // CLIP_SHOT_STORAGE_PREPAREDPNGIMAGE_H
