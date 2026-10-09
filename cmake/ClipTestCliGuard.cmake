# Test CLI dialog guards.
#
# Test executables must never block on modal failure dialogs: Qt fatal
# messages (Q_ASSERT/qFatal), MSVC debug-CRT reports, and Windows hard-error
# popups are converted into stderr diagnostics and a non-zero exit code so
# ctest and CI runners observe plain command-line output.
#
#   clip_test_cli_guard_qt     - Qt message handler plus native layers
#                                (links Qt6::Core)
#   clip_test_cli_guard_native - native Windows/MSVC layers only, for test
#                                executables that do not link Qt
#
# The guards register through global constructors, so linking the matching
# object library into a test target is all that is required. Typical usage in
# a tests-only scope:
#
#     clip_link_test_cli_guard(qt)
#
# links the guard to every target created afterwards in that directory scope.

function(clip_add_native_test_cli_guard)
    if(TARGET clip_test_cli_guard_native)
        return()
    endif()
    add_library(clip_test_cli_guard_native OBJECT
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test-support/clip_test_cli_guard_native.cpp"
    )
    target_include_directories(clip_test_cli_guard_native PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test-support"
    )
    set_target_properties(clip_test_cli_guard_native PROPERTIES
        AUTOMOC OFF
        AUTORCC OFF
        AUTOUIC OFF
    )
endfunction()

function(clip_add_qt_test_cli_guard)
    if(TARGET clip_test_cli_guard_qt)
        return()
    endif()
    add_library(clip_test_cli_guard_qt OBJECT
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test-support/clip_test_cli_guard_qt.cpp"
    )
    target_include_directories(clip_test_cli_guard_qt PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test-support"
    )
    target_link_libraries(clip_test_cli_guard_qt PUBLIC Qt6::Core)
    set_target_properties(clip_test_cli_guard_qt PROPERTIES
        AUTOMOC OFF
        AUTORCC OFF
        AUTOUIC OFF
    )
endfunction()

# Links the guard to every target created afterwards in the calling scope.
# Accepts the variant name "qt" or "native".
macro(clip_link_test_cli_guard variant)
    if("${variant}" STREQUAL "qt")
        clip_add_qt_test_cli_guard()
        link_libraries(clip_test_cli_guard_qt)
    elseif("${variant}" STREQUAL "native")
        clip_add_native_test_cli_guard()
        link_libraries(clip_test_cli_guard_native)
    else()
        message(FATAL_ERROR "clip_link_test_cli_guard: unknown guard variant '${variant}'")
    endif()
endmacro()
