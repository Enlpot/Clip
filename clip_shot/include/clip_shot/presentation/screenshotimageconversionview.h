#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTIMAGECONVERSIONVIEW_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTIMAGECONVERSIONVIEW_H

#include "clip_shot/presentation/screenshotimageconversion.h"
#include <QWidget>

class QLabel;
class QTextBrowser;
class QUrl;
namespace adqt::widgets {
class AdButton;
} // namespace adqt::widgets

class ScreenshotImageConversionView final : public QWidget {
    Q_OBJECT
  public:
    explicit ScreenshotImageConversionView(QWidget* parent = nullptr);
    void setContent(ClipShotImageConversionFormat format, const QString& source, bool busy,
                    const QString& error);
    [[nodiscard]] QString source() const {
        return m_source;
    }
    [[nodiscard]] bool copyToClipboard() const;
    void selectAll();

  signals:
    void retryRequested();
    void linkActivated(const QUrl& url);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void render();
    void retranslate();
    void showContextMenu(const QPoint& globalPosition);
    QTextBrowser* m_browser = nullptr;
    QWidget* m_status = nullptr;
    QLabel* m_statusText = nullptr;
    adqt::widgets::AdButton* m_retry = nullptr;
    ClipShotImageConversionFormat m_format = ClipShotImageConversionFormat::Markdown;
    QString m_source;
    QString m_error;
    bool m_busy = false;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTIMAGECONVERSIONVIEW_H
