include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ClipTestCliGuard.cmake")
# Test binaries must report failures as stderr output plus a non-zero exit
# code instead of blocking on modal dialogs.
clip_link_test_cli_guard(qt)

add_executable(clip_image_viewer_tests viewer_tests.cpp)
target_link_libraries(clip_image_viewer_tests
    PRIVATE
        clip_image_viewer_lib
        Qt6::Test
)
target_compile_features(clip_image_viewer_tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(clip_image_viewer_tests PRIVATE
        /FS /utf-8 /permissive- /Zc:__cplusplus /W4
    )
    if(CLIP_IMAGE_VIEWER_STRICT_COMPILE)
        target_compile_options(clip_image_viewer_tests PRIVATE /WX)
    endif()
endif()

add_test(NAME clip_image_viewer_tests COMMAND clip_image_viewer_tests)
add_test(NAME clip_image_viewer_export_reuse_tests COMMAND clip_image_viewer_tests --export-reuse-only)
set_tests_properties(clip_image_viewer_export_reuse_tests PROPERTIES LABELS "unit" TIMEOUT 45)
add_test(NAME clip_image_viewer_worker_lifecycle_tests
    COMMAND clip_image_viewer_tests --worker-lifecycle-only)
set_tests_properties(clip_image_viewer_worker_lifecycle_tests PROPERTIES LABELS "unit" TIMEOUT 30)

add_executable(clip_image_viewer_editor_lifecycle_tests editor_lifecycle_tests.cpp)
target_link_libraries(clip_image_viewer_editor_lifecycle_tests PRIVATE clip_image_viewer_lib)
target_compile_features(clip_image_viewer_editor_lifecycle_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(clip_image_viewer_editor_lifecycle_tests PRIVATE
        /FS /utf-8 /permissive- /Zc:__cplusplus /W4
    )
    if(CLIP_IMAGE_VIEWER_STRICT_COMPILE)
        target_compile_options(clip_image_viewer_editor_lifecycle_tests PRIVATE /WX)
    endif()
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ClipQtOffscreenTest.cmake")
clip_shot_import_offscreen_platform(clip_image_viewer_editor_lifecycle_tests)
clip_shot_offscreen_qpa_environment(CLIP_IMAGE_VIEWER_EDITOR_LIFECYCLE_ENVIRONMENT)
add_test(NAME clip_image_viewer_editor_lifecycle_tests
    COMMAND clip_image_viewer_editor_lifecycle_tests)
add_test(NAME clip_image_viewer_editor_reopen_tests
    COMMAND clip_image_viewer_editor_lifecycle_tests --reopen-only)
set_tests_properties(clip_image_viewer_editor_lifecycle_tests
    clip_image_viewer_editor_reopen_tests PROPERTIES
    ENVIRONMENT "${CLIP_IMAGE_VIEWER_EDITOR_LIFECYCLE_ENVIRONMENT}"
    LABELS "unit"
    TIMEOUT 60
)
if(WIN32)
    set_tests_properties(clip_image_viewer_editor_lifecycle_tests
        clip_image_viewer_editor_reopen_tests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
endif()

add_executable(clip_image_viewer_generated_icon_pack_tests
    generated_icon_pack_tests.cpp
)
target_link_libraries(clip_image_viewer_generated_icon_pack_tests
    PRIVATE
        clip_image_viewer_lib
)
target_compile_features(clip_image_viewer_generated_icon_pack_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(clip_image_viewer_generated_icon_pack_tests PRIVATE
        /FS /utf-8 /permissive- /Zc:__cplusplus /W4
    )
endif()
add_test(
    NAME clip_image_viewer_generated_icon_pack_tests
    COMMAND clip_image_viewer_generated_icon_pack_tests
)
if(WIN32)
    set_tests_properties(clip_image_viewer_generated_icon_pack_tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
else()
    set_tests_properties(clip_image_viewer_generated_icon_pack_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

add_test(
    NAME clip_image_viewer_edit_mode_performance
    COMMAND $<TARGET_FILE:clip_image_viewer>
        --edit-performance-test
        --edit-performance-output
        $<TARGET_FILE_DIR:clip_image_viewer>/edit-mode-performance.json
        ${CMAKE_SOURCE_DIR}/test-images/preview.png
)
set_tests_properties(clip_image_viewer_edit_mode_performance PROPERTIES
    LABELS "performance;gui"
    TIMEOUT 130
)

if(WIN32)
    add_executable(clip_image_viewer_rhi_tests
        rhi_resize_tests.cpp
        ../resources/resources.qrc
    )
    target_link_libraries(clip_image_viewer_rhi_tests
        PRIVATE clip_image_viewer_lib Qt6::Test
    )
    target_compile_features(clip_image_viewer_rhi_tests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(clip_image_viewer_rhi_tests PRIVATE
            /FS /utf-8 /permissive- /Zc:__cplusplus /W4
        )
        if(CLIP_IMAGE_VIEWER_STRICT_COMPILE)
            target_compile_options(clip_image_viewer_rhi_tests PRIVATE /WX)
        endif()
    endif()
    add_test(NAME clip_image_viewer_rhi_tests
             COMMAND clip_image_viewer_rhi_tests --backend=d3d11)
    add_test(NAME clip_image_viewer_rhi_opengl_tests
             COMMAND clip_image_viewer_rhi_tests --backend=opengl --allow-unavailable)
    add_test(NAME clip_image_viewer_rhi_vulkan_tests
             COMMAND clip_image_viewer_rhi_tests --backend=vulkan --allow-unavailable)
    set_tests_properties(
        clip_image_viewer_rhi_tests
        clip_image_viewer_rhi_opengl_tests
        clip_image_viewer_rhi_vulkan_tests
        PROPERTIES
            ENVIRONMENT "QT_PLUGIN_PATH=${Qt6_DIR}/../../../plugins"
    )
    set_tests_properties(
        clip_image_viewer_rhi_opengl_tests
        clip_image_viewer_rhi_vulkan_tests
        PROPERTIES SKIP_RETURN_CODE 77
    )
endif()

add_custom_command(TARGET clip_image_viewer_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:clip_image_edit_worker>
        $<TARGET_FILE_DIR:clip_image_viewer_tests>
)
