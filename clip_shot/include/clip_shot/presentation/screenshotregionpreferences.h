#pragma once

#include "clip_shot/image/screenshotregiongeometry.h"
#include "clip_shot/storage/applicationstorage.h"
#include "clip_shot/storage/configurationstore.h"

inline ScreenshotRegionType screenshotRegionPreference() {
    auto& storage = clip_shot::storage::ApplicationStorage::instance();
    if (!storage.isInitialized())
        return ScreenshotRegionType::Rectangle;
    return screenshotRegionTypeFromId(storage.configuration()
                                          .value(QStringLiteral("screenshot_selection/region_type"))
                                          .toString());
}

inline void setScreenshotRegionPreference(ScreenshotRegionType type) {
    auto& storage = clip_shot::storage::ApplicationStorage::instance();
    if (storage.isInitialized()) {
        static_cast<void>(storage.configuration().setValue(
            QStringLiteral("screenshot_selection/region_type"), screenshotRegionTypeId(type)));
    }
}
