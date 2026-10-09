#ifndef CLIP_SHOT_PRESENTATION_SYSTEMNOTIFICATIONCONTROLLER_H
#define CLIP_SHOT_PRESENTATION_SYSTEMNOTIFICATIONCONTROLLER_H

#include "clip_shot/platform/systemnotification.h"
#include <QObject>

namespace clip_shot::presentation {
class SystemNotificationController final : public QObject {
    Q_OBJECT
  public:
    explicit SystemNotificationController(platform::SystemNotificationDelivery delivery,
                                          QObject* parent = nullptr);
    void show(platform::SystemNotificationRequest request);

  signals:
    void deliveryFinished(const clip_shot::platform::SystemNotificationRequest& request,
                          const clip_shot::platform::SystemNotificationResult& result);

  private:
    platform::SystemNotificationDelivery m_delivery;
};
} // namespace clip_shot::presentation
#endif
