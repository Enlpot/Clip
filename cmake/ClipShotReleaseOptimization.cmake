# Apply before Mini clones the Full targets so both editions use the same policy.
# These libraries contain configuration metadata and infrequent OS/network tasks.
# Mixed settings/storage libraries deliberately keep a speed policy: history
# thumbnails, PNG preparation, clipboard decoding and global shortcuts live there.
set(CLIP_SHOT_RELEASE_SIZE_TARGETS
    clip_shot_settings_catalog clip_shot_settings_search
    clip_shot_translation clip_shot_administrator
    clip_shot_permissions clip_shot_updates clip_shot_crash_bridge)
set(CLIP_SHOT_RELEASE_SPEED_TARGETS
    clip_shot clip_shot_storage clip_shot_settings clip_shot_shortcuts
    clip_shot_window_shortcuts clip_shot_global_mouse clip_shot_diagnostics
    clip_shot_image_codec clip_shot_image_codec_backend
    clip_shot_history_pin clip_shot_clipboard_placement)

# Serialization/import/export do not participate in image processing loops.
set(CLIP_SHOT_RELEASE_SIZE_STORAGE_SOURCES
    src/storage/configurationarchive.cpp
    src/storage/configurationstore.cpp
    src/storage/configurationschema.cpp
    src/storage/persistedselectioncodec.cpp
    src/storage/persistedwindowgeometry.cpp
    src/storage/settingsadapters.cpp
    src/storage/storagedirectorychange.cpp)

# Settings forms and schema adapters are cold; history, thumbnail/clipboard
# code, streaming translation, theme rendering and shortcut dispatch stay fast.
set(CLIP_SHOT_RELEASE_SIZE_SETTINGS_SOURCES
    src/presentation/components/aboutpagewidget.cpp
    src/presentation/components/customaimodelssettingswidget.cpp
    src/presentation/components/settingscustomwidget.cpp
    src/presentation/components/settingspagewidget.cpp
    src/presentation/components/storagestatussettingswidget.cpp
    src/presentation/components/texttranslationsettingswidget.cpp
    src/presentation/settings/settingsbackend.cpp
    src/presentation/settings/settingsformfield.cpp
    src/presentation/settings/settingsruntimesession.cpp)

# The main executable mixes the shell with the latency-sensitive capture UI.
set(CLIP_SHOT_RELEASE_SIZE_SHELL_SOURCES
    src/app/updateconfirmationdialog.cpp
    src/app/applicationrestart.cpp
    src/app/featureavailability.cpp
    src/presentation/components/contentcardwidget.cpp
    src/presentation/components/maincontentheaderwidget.cpp
    src/presentation/components/sidebarwidget.cpp
    src/presentation/components/titlebarwidget.cpp)

if(CLIP_SHOT_ENABLE_AGGRESSIVE_RELEASE_OPTIMIZATION OR CLIP_SHOT_RELEASE_STATIC)
    foreach(_target IN LISTS CLIP_SHOT_RELEASE_SPEED_TARGETS CLIP_SHOT_RELEASE_SIZE_TARGETS)
        if(TARGET "${_target}")
            set(_policy speed)
            if(CLIP_APPS_ENABLE_RELEASE_SIZE_OPTIMIZATION AND
               _target IN_LIST CLIP_SHOT_RELEASE_SIZE_TARGETS)
                set(_policy size)
            endif()
            clip_apply_release_options("${_target}" POLICY "${_policy}")
        endif()
    endforeach()

    clip_apply_release_size_sources(clip_shot_storage SOURCES
        ${CLIP_SHOT_RELEASE_SIZE_STORAGE_SOURCES})

    clip_apply_release_size_sources(clip_shot_settings SOURCES
        ${CLIP_SHOT_RELEASE_SIZE_SETTINGS_SOURCES})

    clip_apply_release_size_sources(clip_shot SOURCES
        ${CLIP_SHOT_RELEASE_SIZE_SHELL_SOURCES})
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-release-optimization-policy-tests
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-release-optimization.py")
    set_tests_properties(clip-shot-release-optimization-policy-tests PROPERTIES
        LABELS unit TIMEOUT 60)
    if(WIN32)
        add_test(NAME clip-shot-release-optimization-pch-tests
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-release-optimization-pch.py")
        set_tests_properties(clip-shot-release-optimization-pch-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 120)
    endif()
endif()
