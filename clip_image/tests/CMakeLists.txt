if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ClipTestCliGuard.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ClipTestCliGuard.cmake")
    # Keep failure reports on stderr instead of modal dialogs; the native
    # guard variant adds no Qt dependency.
    clip_link_test_cli_guard(native)
endif()

add_executable(clip_image_core_tests core_tests.cpp)
target_link_libraries(clip_image_core_tests PRIVATE clip_image::clip_image)
if(NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
    # The workspace's static Qt executable supplies bundled codec symbols,
    # but this non-Qt test executable still needs explicit codec archives.
    target_link_libraries(clip_image_core_tests PRIVATE PNG::PNG)
endif()
target_compile_features(clip_image_core_tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(clip_image_core_tests PRIVATE /utf-8 /permissive- /Zc:__cplusplus /W4)
else()
    target_compile_options(clip_image_core_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME clip_image_core_tests COMMAND clip_image_core_tests)
add_test(NAME clip_image_pixel_ownership_tests COMMAND clip_image_core_tests --ownership-only)
add_test(NAME clip_image_buffer_paths_tests COMMAND clip_image_core_tests --buffer-paths-only)
add_test(NAME clip_image_processing_tests COMMAND clip_image_core_tests --processing-only)
add_test(NAME clip_image_heif_tests COMMAND clip_image_core_tests --heif-only)
add_test(NAME clip_image_jpeg_tests COMMAND clip_image_core_tests --jpeg-only)
add_test(NAME clip_image_jxl_tests COMMAND clip_image_core_tests --jxl-only)

add_executable(clip_image_raster_conversion_tests raster_conversion_tests.cpp)
target_link_libraries(clip_image_raster_conversion_tests
    PRIVATE clip_image::clip_image ${CLIP_IMAGE_PRIVATE_LIBRARIES})
target_compile_definitions(clip_image_raster_conversion_tests
    PRIVATE ${CLIP_IMAGE_PRIVATE_DEFINITIONS})
target_compile_features(clip_image_raster_conversion_tests PRIVATE cxx_std_20)
add_test(NAME clip_image_raster_conversion_tests COMMAND clip_image_raster_conversion_tests)

add_executable(clip_image_raster_export_tests raster_export_tests.cpp)
target_link_libraries(clip_image_raster_export_tests PRIVATE clip_image::clip_image)
if(NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
    target_link_libraries(clip_image_raster_export_tests PRIVATE PNG::PNG)
endif()
target_compile_features(clip_image_raster_export_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(clip_image_raster_export_tests PRIVATE
        /utf-8 /permissive- /Zc:__cplusplus /W4
    )
else()
    target_compile_options(clip_image_raster_export_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME clip_image_raster_export_tests COMMAND clip_image_raster_export_tests)

add_executable(clip_image_resource_plan_tests resource_plan_tests.cpp)
target_link_libraries(clip_image_resource_plan_tests PRIVATE clip_image::clip_image)
if(NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
    target_link_libraries(clip_image_resource_plan_tests PRIVATE PNG::PNG)
endif()
target_compile_features(clip_image_resource_plan_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(clip_image_resource_plan_tests PRIVATE
        /utf-8 /permissive- /Zc:__cplusplus /W4
    )
else()
    target_compile_options(clip_image_resource_plan_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME clip_image_resource_plan_tests COMMAND clip_image_resource_plan_tests)

if(CLIP_IMAGE_CODEC_HEIF)
    add_executable(clip_image_avif_encoding_tests avif_encoding_tests.cpp)
    target_link_libraries(clip_image_avif_encoding_tests PRIVATE clip_image::clip_image heif)
    if(NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
        target_link_libraries(clip_image_avif_encoding_tests PRIVATE PNG::PNG)
    endif()
    target_compile_features(clip_image_avif_encoding_tests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(clip_image_avif_encoding_tests PRIVATE
            /utf-8 /permissive- /Zc:__cplusplus /W4
        )
    else()
        target_compile_options(clip_image_avif_encoding_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME clip_image_avif_encoding_tests COMMAND clip_image_avif_encoding_tests)
endif()

if(MSVC)
    # Regression coverage for the test CLI dialog guard (cmake/test-support):
    # reports raised through the shared dynamic debug CRT instance that
    # /MD-built dependency DLLs use must land on stderr, never on a modal
    # dialog. The trigger DLL is deliberately built against the dynamic CRT,
    # so it must not receive the static-CRT guard objects from this scope.
    get_directory_property(_CLIP_IMAGE_TEST_GUARD_LIBS LINK_LIBRARIES)
    set_property(DIRECTORY PROPERTY LINK_LIBRARIES "")
    add_library(clip_image_test_cli_guard_trigger SHARED test_cli_guard_trigger.cpp)
    set_target_properties(clip_image_test_cli_guard_trigger PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
    set_property(DIRECTORY PROPERTY LINK_LIBRARIES ${_CLIP_IMAGE_TEST_GUARD_LIBS})
    add_executable(clip_image_test_cli_guard_tests test_cli_guard_tests.cpp)
    # Linking the trigger DLL makes the executable load the shared dynamic
    # debug CRT at process start, before the guard's global constructor runs,
    # mirroring production executables that link /MD-built vcpkg codec DLLs.
    target_link_libraries(clip_image_test_cli_guard_tests
        PRIVATE clip_image_test_cli_guard_trigger
    )
    target_compile_features(clip_image_test_cli_guard_tests PRIVATE cxx_std_20)
    target_compile_options(clip_image_test_cli_guard_tests PRIVATE
        /utf-8 /permissive- /Zc:__cplusplus /W4
    )
    add_test(NAME clip_image_test_cli_guard_tests COMMAND clip_image_test_cli_guard_tests)
endif()
