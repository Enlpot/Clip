#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_PERMISSIONGUIDEWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_PERMISSIONGUIDEWIDGET_H

#include "clip_shot/presentation/permissionguidecontroller.h"
#include <QWidget>

class QLabel;
class QPushButton;
class QMimeData;
namespace clip_shot::presentation {
class PermissionGuideDragRow;
class PermissionGuideWidget final : public QWidget {
    Q_OBJECT
  public:
    explicit PermissionGuideWidget(PermissionGuideApplication application);
    void setPermission(AppPermission permission, AppPermissionStatus status, bool pending,
                       bool fallback);
    int heightForGuideWidth(int width) const;
    void setColorScheme(Qt::ColorScheme scheme);
    bool interacting() const {
        return m_interacting;
    }
    // The caller owns the payload. An invalid/unbundled executable has no drag payload.
    QMimeData* createDragMimeData() const;
    void setInteracting(bool interacting);
  signals:
    void dismissed();
    void requestAccess();
    void interactionFinished();
    void contentSizeChanged();

  protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

  private:
    friend class PermissionGuideDragRow;
    void retranslate();
    bool m_dark = false;
    PermissionGuideApplication m_application;
    AppPermission m_permission = AppPermission::ScreenRecording;
    AppPermissionStatus m_status = AppPermissionStatus::Missing;
    bool m_pending = false;
    bool m_fallback = false;
    bool m_interacting = false;
    QLabel* m_instruction = nullptr;
    QPushButton* m_close = nullptr;
    QPushButton* m_request = nullptr;
    PermissionGuideDragRow* m_row = nullptr;
};
} // namespace clip_shot::presentation
#endif
