if(NOT APPLE)
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

endif()

if(APPLE)
    set(_clip_update_source src/update/macosupdateservice.cpp)
else()
    set(_clip_update_source src/update/updateservice.cpp)
endif()

add_library(clip_shot_updates STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clip_shot/update/updateservice.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clip_shot/update/startupupdate.h"
    ${_clip_update_source}
    src/update/startupupdate.cpp
    # This file is an extraction-only inventory for stable translated Rust error messages.
    src/update/updateerrors.cpp)
target_include_directories(clip_shot_updates PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_updates PUBLIC Qt6::Core)
if(APPLE)
    target_link_libraries(clip_shot_updates PRIVATE Qt6::Network)
    target_compile_definitions(clip_shot_updates PRIVATE CLIP_SHOT_VERSION="${CLIP_SHOT_VERSION}")
endif()
target_link_libraries(clip_shot_settings PUBLIC clip_shot_updates)

target_link_libraries(clip_shot PRIVATE clip_shot_updates)
if(APPLE)
    # An incremental build may still contain the helper copied by an older configuration.
    add_custom_command(TARGET clip_shot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE_DIR:clip_shot>/clip-shot-updater")
endif()
if(NOT APPLE)
    add_dependencies(clip_shot clip-shot-updater)
    add_custom_command(TARGET clip_shot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:clip-shot-updater-binary>"
            "$<TARGET_FILE_DIR:clip_shot>/$<TARGET_FILE_NAME:clip-shot-updater-binary>")

    install(PROGRAMS "$<TARGET_FILE:clip-shot-updater-binary>"
        DESTINATION bin COMPONENT ClipShot)
endif()

option(CLIP_SHOT_BUILD_UPDATE_TESTS "Build focused update tests without unrelated test targets"
    ${CLIP_SHOT_BUILD_TESTS})
if(CLIP_SHOT_BUILD_UPDATE_TESTS)
    enable_testing()
    if(APPLE)
        add_executable(clip-shot-update-adapter-tests tests/macos_update_tests.cpp)
        target_link_libraries(clip-shot-update-adapter-tests PRIVATE Qt6::Network)
    else()
        # The macOS version-check service uses portable Qt networking. Exercise it on Windows
        # as well, without linking the Windows helper adapter into the same executable.
        add_executable(clip-shot-macos-update-tests tests/macos_update_tests.cpp
            src/update/macosupdateservice.cpp include/clip_shot/update/updateservice.h)
        target_include_directories(clip-shot-macos-update-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-macos-update-tests PRIVATE Qt6::Core Qt6::Network)
        target_compile_definitions(clip-shot-macos-update-tests PRIVATE CLIP_SHOT_VERSION="${CLIP_SHOT_VERSION}")
        add_test(NAME clip-shot-macos-update-tests COMMAND clip-shot-macos-update-tests)
        set_tests_properties(clip-shot-macos-update-tests PROPERTIES LABELS "unit" TIMEOUT 30)
        add_executable(clip-shot-update-adapter-tests tests/update_adapter_tests.cpp)
        add_executable(clip-shot-startup-update-tests tests/startup_update_tests.cpp
            src/app/singleinstancecoordinator.cpp
            include/clip_shot/app/singleinstancecoordinator.h)
        target_link_libraries(clip-shot-startup-update-tests PRIVATE
            clip_shot_updates Qt6::Core Qt6::Network)
        add_test(NAME clip-shot-startup-update-tests COMMAND clip-shot-startup-update-tests)
        set_tests_properties(clip-shot-startup-update-tests PROPERTIES LABELS "unit" TIMEOUT 30)
    endif()
    target_link_libraries(clip-shot-update-adapter-tests PRIVATE clip_shot_updates Qt6::Core)
    add_test(NAME clip-shot-update-adapter-tests COMMAND clip-shot-update-adapter-tests)
    # This process fixture launches a helper for each operation and policy case.
    # Leave headroom for debug process startup while each assertion keeps its own deadline.
    set_tests_properties(clip-shot-update-adapter-tests PROPERTIES LABELS "unit" TIMEOUT 60)
endif()
