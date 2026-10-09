# Instantiate a second product from the same source lists and build settings.
# Edition-sensitive libraries are recompiled; codecs, Qt, canvas and native FFI
# remain shared. No full-edition static library may leak into the Mini graph.
include("${CMAKE_CURRENT_LIST_DIR}/ClipShotMiniBuildContract.cmake")
add_library(clip_shot_edition_full INTERFACE)
target_compile_definitions(clip_shot_edition_full INTERFACE CLIP_SHOT_EDITION_MINI=0)
foreach(_feature IN ITEMS QR_RECOGNITION TABLE_RECOGNITION IMAGE_CONVERSION
        LATEX_RECOGNITION TEXT_TRANSLATION API_CONFIGURATION EXTENDED_FEATURES)
    target_compile_definitions(clip_shot_edition_full INTERFACE CLIP_SHOT_ENABLE_${_feature}=1)
endforeach()

set(_clip_mini_libraries
    clip_shot_cloud_upload clip_shot_storage clip_shot_main_window_skin clip_shot_settings_catalog clip_shot_settings_search
    clip_shot_settings clip_shot_global_mouse clip_shot_login_item
    clip_shot_administrator clip_shot_permissions clip_shot_updates
    clip_shot_diagnostics clip_shot_crash_bridge)

if(NOT CLIP_APPS_BUILD_CLIP_SHOT_MINI)
    foreach(_original IN LISTS _clip_mini_libraries)
        target_link_libraries(${_original} PUBLIC clip_shot_edition_full)
    endforeach()
    target_link_libraries(clip_shot PRIVATE clip_shot_edition_full)
    target_sources(clip_shot PRIVATE include/clip_shot/app/edition.h)
    return()
endif()
add_library(clip_shot_edition_mini INTERFACE)
target_compile_definitions(clip_shot_edition_mini INTERFACE CLIP_SHOT_EDITION_MINI=1)
foreach(_feature IN ITEMS QR_RECOGNITION TABLE_RECOGNITION IMAGE_CONVERSION
        LATEX_RECOGNITION TEXT_TRANSLATION API_CONFIGURATION EXTENDED_FEATURES)
    target_compile_definitions(clip_shot_edition_mini INTERFACE CLIP_SHOT_ENABLE_${_feature}=0)
endforeach()

function(_clip_mini_rewrite out value)
    set(_index 0)
    foreach(_original IN LISTS _clip_mini_libraries)
        # Qt resource object targets have names such as settings_resources_1.
        # Only rewrite a complete target name, including in generator expressions.
        string(REGEX REPLACE "${_original}($|[>;])"
            "__CLIP_MINI_TARGET_${_index}__\\1" value "${value}")
        math(EXPR _index "${_index} + 1")
    endforeach()
    set(_index 0)
    foreach(_original IN LISTS _clip_mini_libraries)
        string(REPLACE "__CLIP_MINI_TARGET_${_index}__" "${_original}_mini" value "${value}")
        math(EXPR _index "${_index} + 1")
    endforeach()
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

function(_clip_mini_copy_build_properties original target)
    foreach(_property IN ITEMS INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS
            COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS COMPILE_FEATURES
            INTERFACE_COMPILE_FEATURES LINK_OPTIONS INTERFACE_LINK_OPTIONS
            PRECOMPILE_HEADERS CXX_STANDARD CXX_STANDARD_REQUIRED CXX_EXTENSIONS
            MSVC_RUNTIME_LIBRARY UNITY_BUILD UNITY_BUILD_BATCH_SIZE
            INTERPROCEDURAL_OPTIMIZATION_RELEASE STATIC_LIBRARY_OPTIONS
            CLIP_RELEASE_OPTIMIZATION_POLICY CLIP_RELEASE_SIZE_SOURCES)
        get_target_property(_value ${original} ${_property})
        if(NOT _value STREQUAL "_value-NOTFOUND")
            if(_property STREQUAL "CLIP_RELEASE_SIZE_SOURCES")
                clip_shot_mini_filter_sources(_value ${_value})
            endif()
            _clip_mini_rewrite(_value "${_value}")
            set_property(TARGET ${target} PROPERTY ${_property} "${_value}")
        endif()
    endforeach()
    foreach(_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(_value ${original} ${_property})
        if(NOT _value STREQUAL "_value-NOTFOUND")
            foreach(_excluded IN LISTS CLIP_SHOT_MINI_EXCLUDED_LINK_TARGETS)
                list(FILTER _value EXCLUDE REGEX "(^|[:;])${_excluded}($|[>;])")
            endforeach()
            _clip_mini_rewrite(_value "${_value}")
            set_property(TARGET ${target} PROPERTY ${_property} "${_value}")
        endif()
    endforeach()
endfunction()

# Share only edition-neutral resources. Full translations retain their original
# owner; Mini embeds its own catalogs without Full-only feature contexts.
function(_clip_mini_share_resources target)
    get_target_property(_sources ${target} SOURCES)
    set(_resources ${_sources})
    list(FILTER _resources INCLUDE REGEX "/qrc_[^/]+\\.cpp$")
    list(FILTER _resources EXCLUDE REGEX "/qrc_clip_shot_translations\\.cpp$")
    if(_resources)
        list(REMOVE_ITEM _sources ${_resources})
        set_property(TARGET ${target} PROPERTY SOURCES "${_sources}")
        add_library(${target}_shared_resources OBJECT ${_resources})
        set_target_properties(${target}_shared_resources PROPERTIES AUTOMOC OFF AUTORCC OFF)
        target_link_libraries(${target}_shared_resources PRIVATE Qt6::Core)
        target_link_libraries(${target} PRIVATE ${target}_shared_resources)
    endif()
endfunction()

foreach(_original IN LISTS _clip_mini_libraries)
    _clip_mini_share_resources(${_original})
    get_target_property(_sources ${_original} SOURCES)
    clip_shot_mini_filter_sources(_sources ${_sources})
    add_library(${_original}_mini STATIC ${_sources})
    _clip_mini_copy_build_properties(${_original} ${_original}_mini)
    target_link_libraries(${_original}_mini PUBLIC clip_shot_edition_mini)
endforeach()
target_compile_definitions(clip_shot_crash_bridge_mini PRIVATE
    CLIP_DIAGNOSTICS_PRODUCT="Clip Mini")

_clip_mini_share_resources(clip_shot)
get_target_property(_sources clip_shot SOURCES)
list(FILTER _sources EXCLUDE REGEX "/packaging/macos/[^/]+\\.lproj/InfoPlist\\.strings$")
list(FILTER _sources EXCLUDE REGEX "windows-app-resource\\.rc$|_plugin_import\\.cpp$")
clip_shot_mini_filter_sources(_sources ${_sources})
qt_add_executable(clip_shot_mini MANUAL_FINALIZATION ${_sources})
_clip_mini_copy_build_properties(clip_shot clip_shot_mini)
target_link_libraries(clip_shot_mini PRIVATE clip_shot_edition_mini)
if(CLIP_SHOT_BUILD_TESTS)
    # Full's generated plugin-import source is deliberately excluded above.
    clip_shot_import_offscreen_platform(clip_shot_mini)
endif()
clip_shot_add_translations(clip_shot_mini MINI)
set_target_properties(clip_shot_mini PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/clip_shot_mini"
    MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.clipshot.clip_shot_mini
    MACOSX_BUNDLE_BUNDLE_NAME "Clip Mini"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CLIP_SHOT_VERSION_NUMERIC}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${CLIP_SHOT_VERSION_NUMERIC}")
if(MSVC AND CLIP_SHOT_RELEASE_STATIC AND NOT CLIP_SHOT_QT_STATIC)
    set_property(TARGET clip_shot_mini PROPERTY qt_no_entrypoint TRUE)
endif()
if(WIN32)
    set(CLIP_SHOT_PRODUCT_NAME "Clip Mini")
    set(CLIP_SHOT_EXECUTABLE_NAME clip_shot_mini)
    configure_file(resources/windows-app-resource.rc.in
        "${CMAKE_CURRENT_BINARY_DIR}/generated/windows-mini-app-resource.rc" @ONLY)
    target_sources(clip_shot_mini PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated/windows-mini-app-resource.rc")
    set(CLIP_SHOT_PRODUCT_NAME "Clip")
    set(CLIP_SHOT_EXECUTABLE_NAME clip_shot)
endif()

function(_clip_add_mini_helpers)
    # Cargo features and build-script identity change these executables. Separate
    # output roots avoid races and accidentally staging the other edition.
    set(CLIP_RUST_CARGO_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo-mini")
    if(NOT APPLE)
        clip_add_rust_executable(clip-shot-mini-updater-binary
            PACKAGE clip-shot-updater MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust/clip-shot-updater"
            OUTPUT_NAME clip-shot-updater PRODUCTION_PROFILE release-size
            FEATURES mini REPRODUCIBLE SIZE_OPTIMIZED
            ENVIRONMENT
                "CLIP_SHOT_UPDATE_KEYS_PATH=${CMAKE_CURRENT_SOURCE_DIR}/resources/update-trusted-keys.json"
                "CLIP_SHOT_VERSION=${CLIP_SHOT_VERSION}"
                "CLIP_SHOT_ICON_PATH=${CMAKE_CURRENT_SOURCE_DIR}/resources/app-icon.ico")
    endif()
    if(CLIP_SHOT_ENABLE_MCP)
        clip_add_rust_executable(clip_shot_mini_mcp
            PACKAGE clip-shot-mcp MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust/clip-shot-mcp"
            OUTPUT_NAME clip-shot-mcp PRODUCTION_PROFILE release-size
            FEATURES mini REPRODUCIBLE SIZE_OPTIMIZED)
    endif()
endfunction()
_clip_add_mini_helpers()
if(WIN32)
    set(_mini_bindir bin)
    set(_mini_libdir lib)
    set(_mini_datadir share)
else()
    set(_mini_bindir "clip_shot_mini.app/Contents/MacOS")
    set(_mini_libdir "clip_shot_mini.app/Contents/Frameworks")
    set(_mini_datadir "clip_shot_mini.app/Contents/Resources")
endif()
install(TARGETS clip_shot_mini BUNDLE DESTINATION . COMPONENT ClipShotMini
    RUNTIME DESTINATION "${_mini_bindir}" COMPONENT ClipShotMini)
foreach(_helper IN ITEMS clip-shot-mini-updater-binary clip_shot_mini_mcp)
    if(TARGET ${_helper})
        if(_helper STREQUAL clip_shot_mini_mcp)
            set(_name clip-shot-mini-mcp)
        else()
            set(_name clip-shot-mini-updater)
        endif()
        if(WIN32)
            string(APPEND _name .exe)
        endif()
        add_dependencies(clip_shot_mini ${_helper}_build)
        add_custom_command(TARGET clip_shot_mini POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${_helper}>"
                "$<TARGET_FILE_DIR:clip_shot_mini>/${_name}" VERBATIM)
        install(PROGRAMS "$<TARGET_FILE:${_helper}>" DESTINATION "${_mini_bindir}"
            RENAME "${_name}" COMPONENT ClipShotMini)
    endif()
endforeach()
if(APPLE)
    add_custom_command(TARGET clip_shot_mini POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DCLIP_HANDLER_SOURCE=${CLIP_CRASHPAD_HANDLER}"
            "-DCLIP_HANDLER_DIRECTORY=$<TARGET_FILE_DIR:clip_shot_mini>"
            "-DCLIP_HANDLER_ZLIB=${_clip_handler_zlib}"
            -P "${CMAKE_CURRENT_LIST_DIR}/StageClipShotCrashHandler.cmake" VERBATIM)
else()
    add_custom_command(TARGET clip_shot_mini POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CLIP_CRASHPAD_HANDLER}"
            "$<TARGET_FILE_DIR:clip_shot_mini>/crashpad_handler.exe" VERBATIM)
endif()
install(PROGRAMS "${CLIP_CRASHPAD_HANDLER}" DESTINATION "${_mini_bindir}" COMPONENT ClipShotMini)
if(NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
    if(APPLE)
        # Remove the library left in MacOS by earlier Mini install rules.
        install(CODE [[
            file(REMOVE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/clip_shot_mini.app/Contents/MacOS/$<TARGET_FILE_NAME:clip_shot_image_codec_backend>")
        ]] COMPONENT ClipShotMini)
    endif()
    install(TARGETS clip_shot_image_codec_backend
        RUNTIME_DEPENDENCY_SET clip_shot_mini_image_runtime_dependencies
        LIBRARY DESTINATION "${_mini_libdir}" COMPONENT ClipShotMini
        RUNTIME DESTINATION "${_mini_bindir}" COMPONENT ClipShotMini)
    if(WIN32)
        install(RUNTIME_DEPENDENCY_SET clip_shot_mini_image_runtime_dependencies
            DIRECTORIES "$<TARGET_FILE_DIR:clip_shot_image_codec_backend>"
                "${_CLIP_SHOT_IMAGE_RUNTIME_SEARCH_DIRECTORY}"
            PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*" "msvcp.*\\.dll"
                "ucrtbased\\.dll" "vcruntime.*\\.dll"
            POST_EXCLUDE_REGEXES ".*[\\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/][Ss][Yy][Ss][Tt][Ee][Mm]32[\\\\/].*"
            RUNTIME DESTINATION "${_mini_bindir}" COMPONENT ClipShotMini)
    endif()
endif()
if(WIN32)
    if(NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
        _clip_shot_stage_image_runtime(clip_shot_mini)
    endif()
    if(EXISTS "${CLIP_SHOT_OCR_ASSET_MANIFEST}")
        add_custom_command(TARGET clip_shot_mini POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:clip_shot_mini>/assets/ocr"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CLIP_SHOT_OCR_ASSET_MANIFEST}"
                "$<TARGET_FILE_DIR:clip_shot_mini>/assets/ocr/asset-manifest.json" VERBATIM)
        install(FILES "${CLIP_SHOT_OCR_ASSET_MANIFEST}"
            DESTINATION "${_mini_bindir}/assets/ocr" RENAME asset-manifest.json COMPONENT ClipShotMini)
    endif()
    if(NOT CLIP_SHOT_RELEASE_STATIC)
        add_custom_command(TARGET clip_shot_mini POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_RUNTIME_DLLS:clip_shot_mini>
                "$<TARGET_FILE_DIR:clip_shot_mini>" COMMAND_EXPAND_LISTS VERBATIM)
        if(CLIP_SHOT_FFMPEG_RUNTIME_FILES)
            install(FILES ${CLIP_SHOT_FFMPEG_RUNTIME_FILES} DESTINATION "${_mini_bindir}"
                COMPONENT ClipShotMini)
        endif()
    endif()
    clip_shot_stage_ffmpeg_runtime(clip_shot_mini)
endif()
if(EXISTS "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}")
    add_custom_command(TARGET clip_shot_mini POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:clip_shot_mini>/audios"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}"
            "$<TARGET_FILE_DIR:clip_shot_mini>/audios/camera_shutter.mp3" VERBATIM)
    install(FILES "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}" DESTINATION "${_mini_bindir}/audios"
        RENAME camera_shutter.mp3 COMPONENT ClipShotMini)
endif()
install(FILES LICENSE COPYRIGHT THIRD_PARTY_NOTICES.md
    DESTINATION "${_mini_datadir}/clip-shot-mini/licenses"
    COMPONENT ClipShotMini)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../ant_design_qt/THIRD_PARTY_NOTICES.md"
    DESTINATION "${_mini_datadir}/clip-shot-mini/licenses/third-party/project-notices/ant-design-icons"
    COMPONENT ClipShotMini)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/third-party-licenses/third-party/"
    DESTINATION "${_mini_datadir}/clip-shot-mini/licenses/third-party" OPTIONAL COMPONENT ClipShotMini)
foreach(_project IN ITEMS clip_image ant_design_qt clip-crates clip_draw_engine_qt clip_rust_ffi)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../${_project}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/../${_project}/COPYRIGHT"
        DESTINATION "${_mini_datadir}/clip-shot-mini/licenses/components/${_project}"
        COMPONENT ClipShotMini)
endforeach()
if(APPLE)
    include("${CMAKE_CURRENT_LIST_DIR}/ClipShotMiniMacOS.cmake")
elseif(NOT CLIP_SHOT_QT_STATIC)
    qt_generate_deploy_app_script(TARGET clip_shot_mini OUTPUT_SCRIPT _mini_qt_deploy
        NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT "${_mini_qt_deploy}" COMPONENT ClipShotMini)
endif()
if(CLIP_SHOT_QT_STATIC)
    qt_import_plugins(clip_shot_mini NO_DEFAULT INCLUDE ${_clip_shot_static_qt_plugins})
endif()
qt_finalize_executable(clip_shot_mini)

# Full's fallback defaults also remain valid when Mini is switched off.
foreach(_original IN LISTS _clip_mini_libraries)
    target_link_libraries(${_original} PUBLIC clip_shot_edition_full)
endforeach()
target_link_libraries(clip_shot PRIVATE clip_shot_edition_full)
target_sources(clip_shot PRIVATE include/clip_shot/app/edition.h)

if(CLIP_SHOT_BUILD_TESTS)
    if(WIN32)
        find_program(_clip_launch_test_powershell NAMES pwsh powershell REQUIRED)
        add_test(NAME clip-shot-launcher-tests
            COMMAND "${_clip_launch_test_powershell}" -NoProfile -File
                "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-run-clip-shot.ps1")
        set_tests_properties(clip-shot-launcher-tests PROPERTIES LABELS "unit;windows" TIMEOUT 15)
        if(CLIP_SHOT_ENABLE_MCP)
            add_test(NAME clip-shot-mini-startup-tests
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/mini_startup_tests.py"
                    --application "$<TARGET_FILE:clip_shot_mini>")
            set_tests_properties(clip-shot-mini-startup-tests PROPERTIES LABELS "unit;windows"
                TIMEOUT 45)
        endif()
    endif()
    add_test(NAME clip-shot-mini-build-contract-tests
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/mini_build_contract_tests.py")
    set_tests_properties(clip-shot-mini-build-contract-tests PROPERTIES LABELS unit TIMEOUT 60)
    add_executable(clip-shot-mini-edition-tests tests/mini_edition_tests.cpp)
    target_include_directories(clip-shot-mini-edition-tests PRIVATE include)
    target_link_libraries(clip-shot-mini-edition-tests PRIVATE
        clip_shot_settings_search_mini clip_shot_storage_mini clip_shot_edition_mini Qt6::Core)
    add_executable(clip-shot-mini-settings-reset-tests tests/mini_settings_reset_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_include_directories(clip-shot-mini-settings-reset-tests PRIVATE include)
    target_link_libraries(clip-shot-mini-settings-reset-tests PRIVATE
        clip_shot_settings_mini clip_shot_edition_mini clip_shot_image_codec
        clip_shot_clipboard_placement Qt6::Widgets)
    if(APPLE)
        target_link_libraries(clip-shot-mini-settings-reset-tests PRIVATE clip_shot_macos_clipboard)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-mini-settings-reset-tests)

    add_executable(clip-shot-mini-local-shortcut-reset-tests tests/local_shortcut_reset_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    _clip_mini_copy_build_properties(clip-shot-local-shortcut-reset-tests
        clip-shot-mini-local-shortcut-reset-tests)
    target_link_libraries(clip-shot-mini-local-shortcut-reset-tests PRIVATE clip_shot_edition_mini)
    clip_shot_import_offscreen_platform(clip-shot-mini-local-shortcut-reset-tests)
    add_test(NAME clip-shot-mini-local-shortcut-reset-tests
        COMMAND clip-shot-mini-local-shortcut-reset-tests)
    set_tests_properties(clip-shot-mini-local-shortcut-reset-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    get_target_property(_cloud_test_sources clip-shot-cloud-upload-tests SOURCES)
    list(FILTER _cloud_test_sources EXCLUDE REGEX "/qrc_[^/]+\\.cpp$")
    add_executable(clip-shot-mini-cloud-upload-tests ${_cloud_test_sources})
    _clip_mini_copy_build_properties(clip-shot-cloud-upload-tests clip-shot-mini-cloud-upload-tests)
    target_link_libraries(clip-shot-mini-cloud-upload-tests PRIVATE clip_shot_edition_mini)
    clip_shot_add_translations(clip-shot-mini-cloud-upload-tests MINI)
    clip_shot_import_offscreen_platform(clip-shot-mini-cloud-upload-tests)
    add_test(NAME clip-shot-mini-cloud-upload-tests COMMAND clip-shot-mini-cloud-upload-tests -platform offscreen)
    set_tests_properties(clip-shot-mini-cloud-upload-tests PROPERTIES LABELS "unit" TIMEOUT 90
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    # The Mini regression also restores legacy pins and exercises palette
    # controls, so use the complete pinned-window fixture rather than only the
    # recognition-session sources.
    get_target_property(_mini_test_sources clip-shot-pinned-window-tests SOURCES)
    list(FILTER _mini_test_sources EXCLUDE REGEX "(^tests/|/qrc_[^/]+\\.cpp$)")
    clip_shot_mini_filter_sources(_mini_test_sources ${_mini_test_sources})
    add_executable(clip-shot-mini-recognition-tests ${_mini_test_sources}
        tests/mini_recognition_tests.cpp)
    _clip_mini_copy_build_properties(clip-shot-pinned-window-tests
        clip-shot-mini-recognition-tests)
    target_link_libraries(clip-shot-mini-recognition-tests PRIVATE clip_shot_edition_mini)
    clip_shot_add_translations(clip-shot-mini-recognition-tests MINI)
    clip_shot_import_offscreen_platform(clip-shot-mini-recognition-tests)

    foreach(_edition IN ITEMS full mini)
        add_executable(clip-shot-${_edition}-mcp-edition-tests tests/mcp_edition_tests.cpp)
        target_include_directories(clip-shot-${_edition}-mcp-edition-tests PRIVATE include)
        target_link_libraries(clip-shot-${_edition}-mcp-edition-tests PRIVATE
            clip_shot_edition_${_edition} Qt6::Core)
    endforeach()
    get_target_property(_branding_sources clip-shot-about-page-tests SOURCES)
    list(REMOVE_ITEM _branding_sources tests/about_page_tests.cpp)
    foreach(_edition IN ITEMS full mini)
        set(_suffix "")
        set(_branding_qm_directory "${CMAKE_CURRENT_BINARY_DIR}")
        set(_branding_translation_target clip_shot_release_translations)
        if(_edition STREQUAL mini)
            set(_suffix _mini)
            set(_branding_qm_directory "${_clip_shot_mini_qm_dir}")
            set(_branding_translation_target clip_shot_mini_release_translations)
        endif()
        add_executable(clip-shot-${_edition}-branding-tests ${_branding_sources}
            tests/mini_branding_tests.cpp src/app/updateconfirmationdialog.cpp)
        target_include_directories(clip-shot-${_edition}-branding-tests PRIVATE include)
        target_compile_definitions(clip-shot-${_edition}-branding-tests PRIVATE
            CLIP_SHOT_TEST_TRANSLATIONS_DIR="${_branding_qm_directory}")
        target_link_libraries(clip-shot-${_edition}-branding-tests PRIVATE
            clip_shot_edition_${_edition} clip_shot_settings${_suffix}
            clip_shot_updates${_suffix} clip_shot_image_codec clip_shot_clipboard_placement Qt6::Widgets)
        if(APPLE)
            target_link_libraries(clip-shot-${_edition}-branding-tests PRIVATE
                clip_shot_macos_clipboard "-framework AppKit")
        endif()
        clip_shot_import_offscreen_platform(clip-shot-${_edition}-branding-tests)
        add_dependencies(clip-shot-${_edition}-branding-tests ${_branding_translation_target})
    endforeach()
    foreach(_test IN ITEMS clip-shot-mini-edition-tests clip-shot-mini-recognition-tests
            clip-shot-mini-cloud-upload-tests clip-shot-mini-settings-reset-tests clip-shot-mini-local-shortcut-reset-tests
            clip-shot-mini-mcp-edition-tests
            clip-shot-mini-branding-tests)
        clip_shot_assert_mini_build_contract(${_test})
    endforeach()
    foreach(_test IN ITEMS clip-shot-mini-edition-tests clip-shot-mini-recognition-tests
            clip-shot-mini-settings-reset-tests
            clip-shot-full-mcp-edition-tests clip-shot-mini-mcp-edition-tests
            clip-shot-full-branding-tests clip-shot-mini-branding-tests)
        add_test(NAME ${_test} COMMAND ${_test})
        set_tests_properties(${_test} PROPERTIES LABELS unit TIMEOUT 60
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endforeach()
    clip_shot_offscreen_qpa_environment(_mini_qpa_environment)
    set_tests_properties(clip-shot-mini-recognition-tests
        clip-shot-mini-settings-reset-tests
        clip-shot-full-branding-tests clip-shot-mini-branding-tests PROPERTIES
        ENVIRONMENT "${_mini_qpa_environment}")
    if(Python3_Interpreter_FOUND AND CLIP_SHOT_ENABLE_MCP)
        add_test(NAME clip-shot-mini-mcp-capabilities-tests
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_mcp_capabilities.py" --mini)
        set_tests_properties(clip-shot-mini-mcp-capabilities-tests PROPERTIES LABELS unit TIMEOUT 30)
    endif()
endif()

# Keep this after helpers, resources and finalization so transitive link and
# build dependencies are audited as well as the application's copied sources.
clip_shot_assert_mini_build_contract(clip_shot_mini)
