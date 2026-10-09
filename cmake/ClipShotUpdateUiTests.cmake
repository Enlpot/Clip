# Focused offscreen checks also work with the production Qt kit, which omits QtTest.
if(CLIP_SHOT_BUILD_UPDATE_TESTS AND NOT CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-update-about-tests
        tests/about_page_tests.cpp
        include/clip_shot/presentation/mainwindow.h
        include/clip_shot/presentation/components/contentcardwidget.h
        include/clip_shot/presentation/components/maincontentheaderwidget.h
        include/clip_shot/presentation/components/sidebarwidget.h
        include/clip_shot/presentation/components/titlebarwidget.h
        src/presentation/mainwindow.cpp
        src/presentation/components/contentcardwidget.cpp
        src/presentation/components/maincontentheaderwidget.cpp
        src/presentation/components/sidebarwidget.cpp
        src/presentation/components/titlebarwidget.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/platform/windows/windowchrome.cpp)
    target_link_libraries(clip-shot-update-about-tests PRIVATE clip_shot_settings clip_shot_image_codec)
    if(WIN32)
        target_link_libraries(clip-shot-update-about-tests PRIVATE PkgConfig::CLIP_SHOT_FFMPEG)
    endif()
    target_compile_definitions(clip-shot-update-about-tests PRIVATE
        CLIP_SHOT_TEST_VERSION="${CLIP_SHOT_VERSION}"
        CLIP_SHOT_TEST_WEBSITE_URL="${CLIP_SHOT_WEBSITE_URL}"
        CLIP_SHOT_TEST_PROJECT_URL="${CLIP_SHOT_PROJECT_URL}"
        CLIP_SHOT_TEST_QQ_GROUP_2_URL="${CLIP_SHOT_QQ_GROUP_2_URL}"
        CLIP_SHOT_TEST_QQ_GROUP_3_URL="${CLIP_SHOT_QQ_GROUP_3_URL}"
        CLIP_SHOT_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    add_dependencies(clip-shot-update-about-tests clip_shot_release_translations)
    clip_shot_import_offscreen_platform(clip-shot-update-about-tests)
    add_test(NAME clip-shot-update-about-tests COMMAND clip-shot-update-about-tests)
    set_tests_properties(clip-shot-update-about-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;windows" TIMEOUT 120)
    add_executable(clip-shot-update-settings-tests tests/settings_catalog_tests.cpp)
    target_link_libraries(clip-shot-update-settings-tests PRIVATE clip_shot_settings_search Qt6::Core)
    add_test(NAME clip-shot-update-settings-tests COMMAND clip-shot-update-settings-tests)
    set_tests_properties(clip-shot-update-settings-tests PROPERTIES LABELS "unit;windows" TIMEOUT 60)
endif()
