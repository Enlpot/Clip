# Keep production-kit acceptance independent of QtTest and unrelated UI tests.
# General and focused modes share these declarations and their runtime contracts.
function(clip_shot_add_diagnostics_crash_tests)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-bin")
    if(WIN32 OR APPLE)
        add_executable(clip-shot-diagnostics-crash-tests tests/diagnostics_crash_tests.cpp)
        clip_add_rust_static_library(clip_diagnostics_test
            PACKAGE clip-diagnostics-test
            MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
            OUTPUT_NAME clip_diagnostics_test
            STRIP_MSVC_DIRECTIVES)
        target_link_libraries(clip-shot-diagnostics-crash-tests PRIVATE clip_diagnostics_test)
        target_link_libraries(clip-shot-diagnostics-crash-tests PRIVATE clip_shot_diagnostics clip_shot_crash_bridge)
        if(APPLE)
            set_property(TARGET clip-shot-diagnostics-crash-tests PROPERTY
                CLIP_RUST_BUNDLE_OVERRIDE clip_diagnostics_test)
        endif()
        target_compile_definitions(clip-shot-diagnostics-crash-tests PRIVATE
            CLIP_TEST_CRASHPAD_HANDLER="${CLIP_CRASHPAD_HANDLER}")
        add_test(NAME clip-shot-diagnostics-crash-tests COMMAND clip-shot-diagnostics-crash-tests)
        set_tests_properties(clip-shot-diagnostics-crash-tests PROPERTIES LABELS "unit;crash" TIMEOUT 180)
        if(APPLE)
            set_property(TEST clip-shot-diagnostics-crash-tests APPEND PROPERTY LABELS macos)
        else()
            set_property(TEST clip-shot-diagnostics-crash-tests APPEND PROPERTY LABELS windows)
        endif()
    endif()
endfunction()

function(clip_shot_add_ocr_runtime_tests)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-bin")
    add_executable(clip-shot-ocr-recognition-service-tests
        tests/screenshot_ocr_recognition_service_tests.cpp
        tests/screenshot_ocr_process_lifecycle_tests.cpp
        include/clip_shot/presentation/screenshotocrpresentation.h
        include/clip_shot/presentation/screenshotocrrecognitionservice.h
        include/clip_shot/presentation/screenshotocrassets.h
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrrecognitionservice.cpp
        src/presentation/ocr/screenshotocrassets.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp
    )
    target_include_directories(clip-shot-ocr-recognition-service-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_DRAW_ENGINE_SOURCE_DIR}/include"
    )
    target_compile_definitions(clip-shot-ocr-recognition-service-tests PRIVATE
        CLIP_TEST_OCR_EXECUTABLE="$<TARGET_FILE:clip_ocr_process>")
    if(APPLE AND CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
        # The test host compiles the same asset parser and consumes the worker's
        # generated manifest, so its runtime linkage contract must match the app.
        target_compile_definitions(clip-shot-ocr-recognition-service-tests PRIVATE
            CLIP_SHOT_OCR_STATIC_ONNXRUNTIME=1)
    endif()
    if(CLIP_SHOT_BUILD_BENCHMARKS AND CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_definitions(clip-shot-ocr-recognition-service-tests PRIVATE CLIP_TEST_OCR_PERFORMANCE=1)
    endif()
    target_link_libraries(clip-shot-ocr-recognition-service-tests PRIVATE
        clip_shot_diagnostics
        clip_draw_engine_qt_canvas
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        MINIZIP::minizip-ng
    )
    if(WIN32)
        target_link_libraries(clip-shot-ocr-recognition-service-tests PRIVATE
            bcrypt
            ntdll
        )
        target_compile_definitions(clip-shot-ocr-recognition-service-tests PRIVATE
            CLIP_TEST_CRASHPAD_HANDLER="${CLIP_CRASHPAD_HANDLER}")
        add_test(NAME clip-shot-ocr-crash-tests COMMAND clip-shot-ocr-recognition-service-tests --native-crash)
        set_tests_properties(clip-shot-ocr-crash-tests PROPERTIES TIMEOUT 60 LABELS "unit;windows")
    endif()
    add_test(
        NAME clip-shot-ocr-cpu-recognition-service-tests
        COMMAND clip-shot-ocr-recognition-service-tests
    )
    add_test(
        NAME clip-shot-ocr-directml-recognition-service-tests
        COMMAND clip-shot-ocr-recognition-service-tests --directml
    )
    set_tests_properties(clip-shot-ocr-cpu-recognition-service-tests PROPERTIES
        TIMEOUT 120
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-ocr-directml-recognition-service-tests PROPERTIES
        TIMEOUT 120
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(NAME clip-shot-ocr-storage-relocation-tests
        COMMAND clip-shot-ocr-recognition-service-tests --storage-relocation-only)
    set_tests_properties(clip-shot-ocr-storage-relocation-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-ocr-process-lifecycle-tests
        COMMAND clip-shot-ocr-recognition-service-tests --process-lifecycle)
    add_executable(clip-shot-ocr-assets-tests
        tests/screenshot_ocr_assets_tests.cpp
        include/clip_shot/presentation/screenshotocrassets.h
        src/presentation/ocr/screenshotocrassets.cpp)
    target_include_directories(clip-shot-ocr-assets-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-ocr-assets-tests PRIVATE Qt6::Core Qt6::Network MINIZIP::minizip-ng)
    if(APPLE)
        target_link_libraries(clip-shot-ocr-assets-tests PRIVATE clip_shot_rust_ffi_selector)
    endif()
    add_test(NAME clip-shot-ocr-assets-tests COMMAND clip-shot-ocr-assets-tests)
    set_tests_properties(clip-shot-ocr-assets-tests PROPERTIES TIMEOUT 60 LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(WIN32 AND CLIP_WINDOWS_ARCHITECTURE STREQUAL "x64")
        add_executable(clip-shot-ocr-assets-arm64-contract-tests
            tests/screenshot_ocr_assets_arm64_tests.cpp
            tests/screenshot_ocr_assets_arm64_fixture.cpp
            include/clip_shot/presentation/screenshotocrassets.h)
        target_include_directories(clip-shot-ocr-assets-arm64-contract-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-ocr-assets-arm64-contract-tests PRIVATE
            Qt6::Core Qt6::Network MINIZIP::minizip-ng)
        add_test(NAME clip-shot-ocr-assets-arm64-contract-tests
            COMMAND clip-shot-ocr-assets-arm64-contract-tests)
        set_tests_properties(clip-shot-ocr-assets-arm64-contract-tests PROPERTIES
            TIMEOUT 60 LABELS "unit;windows"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    add_test(NAME clip-shot-ocr-managed-runtime-tests
        COMMAND clip-shot-ocr-recognition-service-tests --managed-runtime-only)
    add_test(NAME clip-shot-ocr-managed-directml-runtime-tests
        COMMAND clip-shot-ocr-recognition-service-tests --managed-runtime-only --directml)
    set_tests_properties(clip-shot-ocr-managed-runtime-tests
        clip-shot-ocr-managed-directml-runtime-tests PROPERTIES
        TIMEOUT 60 LABELS "unit;windows"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(APPLE)
        set_tests_properties(clip-shot-ocr-managed-runtime-tests PROPERTIES LABELS "unit")
        set_tests_properties(clip-shot-ocr-directml-recognition-service-tests PROPERTIES LABELS "unit;windows")
    endif()
    set_tests_properties(clip-shot-ocr-process-lifecycle-tests PROPERTIES TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_dependencies(clip-shot-ocr-recognition-service-tests clip_ocr_process)
    if(NOT CLIP_SHOT_BUILD_TESTS)
        clip_add_qt_test_cli_guard()
        foreach(_runtime_test IN ITEMS clip-shot-ocr-recognition-service-tests
                clip-shot-ocr-assets-tests clip-shot-ocr-assets-arm64-contract-tests)
            if(TARGET ${_runtime_test})
                target_link_libraries(${_runtime_test} PRIVATE clip_test_cli_guard_qt)
            endif()
        endforeach()
    endif()
endfunction()

if(CLIP_SHOT_BUILD_RUNTIME_TESTS AND NOT CLIP_SHOT_BUILD_TESTS)
    enable_testing()
    clip_shot_add_diagnostics_crash_tests()
    clip_shot_add_ocr_runtime_tests()
endif()
