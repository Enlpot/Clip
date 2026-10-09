# The GPL updater is a standalone Rust package inside the Clip license boundary. The main
# application links only the Qt process/protocol adapter below.
set(_clip_shot_updater_manifest_dir
    "${CMAKE_CURRENT_SOURCE_DIR}/rust/clip-shot-updater")
clip_add_rust_executable(clip-shot-updater-binary
    PACKAGE clip-shot-updater
    MANIFEST_DIR "${_clip_shot_updater_manifest_dir}"
    OUTPUT_NAME clip-shot-updater
    PRODUCTION_PROFILE release-size
    REPRODUCIBLE
    SIZE_OPTIMIZED
    ENVIRONMENT
        "CLIP_SHOT_UPDATE_KEYS_PATH=${CMAKE_CURRENT_SOURCE_DIR}/resources/update-trusted-keys.json"
        "CLIP_SHOT_VERSION=${CLIP_SHOT_VERSION}"
        "CLIP_SHOT_ICON_PATH=${CMAKE_CURRENT_SOURCE_DIR}/resources/app-icon.ico"
)
# Imported executable targets do not create build-system targets. Keep the
# public clip-shot-updater target used by scripts and release automation as a
# real proxy for the Cargo build while the imported target supplies its path.
# Also materialize the traditional build-tree output when this target is built
# directly, rather than only as a dependency of clip_shot.
add_custom_target(clip-shot-updater
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:clip_shot>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:clip-shot-updater-binary>"
        "$<TARGET_FILE_DIR:clip_shot>/$<TARGET_FILE_NAME:clip-shot-updater-binary>"
    DEPENDS clip-shot-updater-binary_build
    VERBATIM)

set(_clip_update_source src/update/updateservice.cpp)

add_library(clip_shot_updates STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clip_shot/update/updateservice.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clip_shot/update/startupupdate.h"
    ${_clip_update_source}
    src/update/startupupdate.cpp
    # This file is an extraction-only inventory for stable translated Rust error messages.
    src/update/updateerrors.cpp)
target_include_directories(clip_shot_updates PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_updates PUBLIC Qt6::Core)
target_link_libraries(clip_shot_settings PUBLIC clip_shot_updates)

target_link_libraries(clip_shot PRIVATE clip_shot_updates)
add_dependencies(clip_shot clip-shot-updater)
add_custom_command(TARGET clip_shot POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:clip-shot-updater-binary>"
        "$<TARGET_FILE_DIR:clip_shot>/$<TARGET_FILE_NAME:clip-shot-updater-binary>")

install(PROGRAMS "$<TARGET_FILE:clip-shot-updater-binary>"
    DESTINATION bin COMPONENT ClipShot)

option(CLIP_SHOT_BUILD_UPDATE_TESTS "Build focused update tests without unrelated test targets"
    ${CLIP_SHOT_BUILD_TESTS})
if(CLIP_SHOT_BUILD_UPDATE_TESTS)
    enable_testing()
    add_executable(clip-shot-update-adapter-tests tests/update_adapter_tests.cpp)
    add_executable(clip-shot-startup-update-tests tests/startup_update_tests.cpp
        src/app/singleinstancecoordinator.cpp
        include/clip_shot/app/singleinstancecoordinator.h)
    target_link_libraries(clip-shot-startup-update-tests PRIVATE
        clip_shot_updates Qt6::Core Qt6::Network)
    add_test(NAME clip-shot-startup-update-tests COMMAND clip-shot-startup-update-tests)
    set_tests_properties(clip-shot-startup-update-tests PROPERTIES LABELS "unit" TIMEOUT 30)
    target_link_libraries(clip-shot-update-adapter-tests PRIVATE clip_shot_updates Qt6::Core)
    add_test(NAME clip-shot-update-adapter-tests COMMAND clip-shot-update-adapter-tests)
    # This process fixture launches a helper for each operation and policy case.
    # Leave headroom for debug process startup while each assertion keeps its own deadline.
    set_tests_properties(clip-shot-update-adapter-tests PROPERTIES LABELS "unit" TIMEOUT 60)
endif()
