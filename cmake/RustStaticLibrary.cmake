include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/ClipWindowsArchitecture.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ClipRustLibclang.cmake")

# A Rust staticlib includes its dependencies and runtime. Consumers must select
# one archive for the final host rather than relying on link order to avoid
# extracting overlapping objects from two feature variants.
function(clip_add_rust_bundle_selector target default_bundle)
    if(NOT TARGET ${target})
        add_library(${target} INTERFACE)
        target_link_libraries(${target} INTERFACE
            "$<IF:$<BOOL:$<TARGET_PROPERTY:CLIP_RUST_BUNDLE_OVERRIDE>>,$<TARGET_PROPERTY:CLIP_RUST_BUNDLE_OVERRIDE>,${default_bundle}>")
    endif()
endfunction()

function(_clip_rust_target_reaches output root dependency)
    set(_pending ${root})
    set(_visited)
    while(_pending)
        list(POP_FRONT _pending _target)
        if(_target STREQUAL dependency)
            set(${output} TRUE PARENT_SCOPE)
            return()
        endif()
        if(_target IN_LIST _visited)
            continue()
        endif()
        list(APPEND _visited ${_target})
        get_target_property(_alias ${_target} ALIASED_TARGET)
        if(_alias)
            list(APPEND _pending ${_alias})
        endif()
        foreach(_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_items ${_target} ${_property})
            foreach(_item IN LISTS _items)
                if(TARGET "${_item}")
                    list(APPEND _pending "${_item}")
                elseif(_item MATCHES "\\$<")
                    # Unwrap LINK_ONLY/TARGET_OBJECTS while preserving aliases.
                    string(REGEX MATCHALL "[A-Za-z0-9_.:+-]+" _references "${_item}")
                    foreach(_reference IN LISTS _references)
                        string(REGEX REPLACE "(^|[^:]):([^:])" "\\1;\\2"
                            _names "${_reference}")
                        foreach(_name IN LISTS _names)
                            if(TARGET "${_name}")
                                list(APPEND _pending "${_name}")
                            endif()
                        endforeach()
                    endforeach()
                endif()
            endforeach()
        endforeach()
    endwhile()
    set(${output} FALSE PARENT_SCOPE)
endfunction()

# Run after all executables have been declared. The override must be explicit
# on each final host: compatible interface properties are inferred too late to
# select that host's transitive link libraries.
function(clip_select_rust_bundle_consumers dependency bundle)
    set(_directories ${CMAKE_SOURCE_DIR})
    while(_directories)
        list(POP_FRONT _directories _directory)
        get_property(_children DIRECTORY "${_directory}" PROPERTY SUBDIRECTORIES)
        list(APPEND _directories ${_children})
        get_property(_targets DIRECTORY "${_directory}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_target IN LISTS _targets)
            get_target_property(_type ${_target} TYPE)
            if(NOT _type STREQUAL "EXECUTABLE")
                continue()
            endif()
            _clip_rust_target_reaches(_uses_bundle ${_target} ${dependency})
            if(_uses_bundle)
                set_property(TARGET ${_target} PROPERTY CLIP_RUST_BUNDLE_OVERRIDE ${bundle})
            endif()
        endforeach()
    endwhile()
endfunction()

# Resolve the target OS before compiler family: GCC on Linux is not a MinGW
# cross compiler.
function(clip_detect_rust_target out_var)
    if(WIN32)
        clip_configure_windows_architecture()
        if(MSVC)
            if(CLIP_WINDOWS_ARCHITECTURE STREQUAL "arm64")
                set(_windows_target aarch64-pc-windows-msvc)
            else()
                set(_windows_target x86_64-pc-windows-msvc)
            endif()
        elseif(CLIP_WINDOWS_ARCHITECTURE STREQUAL "x64")
            set(_windows_target x86_64-pc-windows-gnu)
        else()
            message(FATAL_ERROR "Windows ARM64 requires the MSVC Rust target")
        endif()
    endif()
    if(DEFINED CLIP_RUST_TARGET AND NOT CLIP_RUST_TARGET STREQUAL "")
        if(WIN32 AND NOT CLIP_RUST_TARGET STREQUAL _windows_target)
            message(FATAL_ERROR
                "CLIP_RUST_TARGET=${CLIP_RUST_TARGET} disagrees with the ${CLIP_WINDOWS_ARCHITECTURE} C++ compiler; use ${_windows_target}")
        endif()
        set(${out_var} "${CLIP_RUST_TARGET}" PARENT_SCOPE)
        return()
    endif()
    if(WIN32)
        set(_target "${_windows_target}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
            set(_target aarch64-unknown-linux-gnu)
        else()
            set(_target x86_64-unknown-linux-gnu)
        endif()
    else()
        message(FATAL_ERROR "Unsupported Rust host: ${CMAKE_SYSTEM_NAME}")
    endif()
    set(${out_var} "${_target}" PARENT_SCOPE)
endfunction()

function(clip_add_rust_static_libraries batch_name)
    set(options STRIP_MSVC_DIRECTIVES)
    set(oneValueArgs MANIFEST_DIR)
    set(multiValueArgs TARGETS PACKAGES OUTPUT_NAMES FEATURES)
    cmake_parse_arguments(CLIP_RUST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CLIP_RUST_MANIFEST_DIR)
        message(FATAL_ERROR "clip_add_rust_static_libraries requires MANIFEST_DIR")
    endif()
    foreach(_list_name IN ITEMS TARGETS PACKAGES OUTPUT_NAMES)
        list(LENGTH CLIP_RUST_${_list_name} _list_length)
        if(_list_length EQUAL 0)
            message(FATAL_ERROR
                "clip_add_rust_static_libraries requires a non-empty ${_list_name} list")
        endif()
        if(DEFINED _rust_library_count AND NOT _list_length EQUAL _rust_library_count)
            message(FATAL_ERROR
                "TARGETS, PACKAGES, and OUTPUT_NAMES must have the same length")
        endif()
        set(_rust_library_count ${_list_length})
    endforeach()

    find_program(CARGO_EXECUTABLE NAMES cargo REQUIRED)
    if(NOT DEFINED CLIP_RUST_CARGO_TARGET_DIR OR CLIP_RUST_CARGO_TARGET_DIR STREQUAL "")
        set(CLIP_RUST_CARGO_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo" CACHE PATH
            "Cargo target directory for the active CMake build tree.")
    endif()
    if(NOT DEFINED CLIP_RUST_RELEASE_PROFILE OR CLIP_RUST_RELEASE_PROFILE STREQUAL "")
        set(CLIP_RUST_RELEASE_PROFILE "release" CACHE STRING
            "Cargo profile used for non-Debug Rust builds.")
    endif()
    if(NOT DEFINED CLIP_VCPKG_ROOT OR CLIP_VCPKG_ROOT STREQUAL "")
        set(CLIP_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
    endif()
    if(NOT DEFINED CLIP_FFMPEG_ROOT OR CLIP_FFMPEG_ROOT STREQUAL "")
        if(DEFINED ENV{FFMPEG_DIR} AND NOT "$ENV{FFMPEG_DIR}" STREQUAL "")
            set(CLIP_FFMPEG_ROOT "$ENV{FFMPEG_DIR}")
        elseif(DEFINED VCPKG_INSTALLED_DIR AND NOT VCPKG_INSTALLED_DIR STREQUAL "")
            set(CLIP_FFMPEG_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
        else()
            set(CLIP_FFMPEG_ROOT "${CLIP_VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
        endif()
    endif()
    clip_detect_rust_target(CLIP_RUST_TARGET)

    execute_process(
        COMMAND "${CARGO_EXECUTABLE}" metadata --format-version 1
            --filter-platform "${CLIP_RUST_TARGET}" --no-deps
        WORKING_DIRECTORY "${CLIP_RUST_MANIFEST_DIR}"
        RESULT_VARIABLE _metadata_result
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(NOT _metadata_result EQUAL 0)
        message(FATAL_ERROR
            "Rust target '${CLIP_RUST_TARGET}' is not installed or usable. "
            "Run scripts/bootstrap.ps1.")
    endif()

    if(CMAKE_CONFIGURATION_TYPES)
        set(_profile "$<IF:$<CONFIG:Debug>,debug,${CLIP_RUST_RELEASE_PROFILE}>")
        set(_cargo_profile "$<IF:$<CONFIG:Debug>,dev,${CLIP_RUST_RELEASE_PROFILE}>")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_profile debug)
        set(_cargo_profile dev)
    else()
        set(_profile "${CLIP_RUST_RELEASE_PROFILE}")
        set(_cargo_profile "${CLIP_RUST_RELEASE_PROFILE}")
    endif()

    if(CLIP_RUST_TARGET MATCHES "msvc$")
        set(_lib_prefix "")
        set(_lib_suffix ".lib")
    else()
        set(_lib_prefix "lib")
        set(_lib_suffix ".a")
    endif()

    set(_static_libraries)
    set(_debug_libraries)
    set(_release_libraries)
    set(_cargo_package_args)
    math(EXPR _rust_library_last "${_rust_library_count} - 1")
    foreach(_index RANGE ${_rust_library_last})
        list(GET CLIP_RUST_PACKAGES ${_index} _package)
        list(GET CLIP_RUST_OUTPUT_NAMES ${_index} _output_name)
        string(REPLACE "-" "_" _output_stem "${_output_name}")
        list(APPEND _cargo_package_args --package "${_package}")
        list(APPEND _static_libraries
            "${CLIP_RUST_CARGO_TARGET_DIR}/${CLIP_RUST_TARGET}/${_profile}/${_lib_prefix}${_output_stem}${_lib_suffix}")
        list(APPEND _debug_libraries
            "${CLIP_RUST_CARGO_TARGET_DIR}/${CLIP_RUST_TARGET}/debug/${_lib_prefix}${_output_stem}${_lib_suffix}")
        list(APPEND _release_libraries
            "${CLIP_RUST_CARGO_TARGET_DIR}/${CLIP_RUST_TARGET}/${CLIP_RUST_RELEASE_PROFILE}/${_lib_prefix}${_output_stem}${_lib_suffix}")
    endforeach()

    clip_resolve_rust_libclang_directory(_libclang_dir)

    set(_clip_rust_static_crt FALSE)
    if(CLIP_APPS_RELEASE_STATIC OR CLIP_SHOT_RELEASE_STATIC OR
       (MSVC AND CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "^MultiThreaded" AND
        NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL"))
        set(_clip_rust_static_crt TRUE)
    endif()

    set(_vcpkg_dynamic 1)
    if(CLIP_APPS_RELEASE_STATIC OR CLIP_SHOT_RELEASE_STATIC)
        set(_vcpkg_dynamic 0)
    endif()
    set(_cargo_environment
        "VCPKG_ROOT=${CLIP_VCPKG_ROOT}"
        "VCPKGRS_TRIPLET=${VCPKG_TARGET_TRIPLET}"
        "VCPKGRS_DYNAMIC=${_vcpkg_dynamic}"
        "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
        "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
    )
    clip_windows_rust_environment(_windows_cargo_environment "${CLIP_RUST_TARGET}")
    list(APPEND _cargo_environment ${_windows_cargo_environment})
    if(_libclang_dir)
        list(APPEND _cargo_environment "LIBCLANG_PATH=${_libclang_dir}")
    endif()

    if(MSVC AND (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE STREQUAL "Debug" OR
                 CLIP_APPS_RELEASE_STATIC OR CLIP_SHOT_RELEASE_STATIC))
        if(_clip_rust_static_crt)
            set(_rust_debug_runtime "/MTd /D_DEBUG")
            set(_rust_release_runtime "/MT")
        else()
            set(_rust_debug_runtime "/MDd /D_DEBUG")
            set(_rust_release_runtime "/MD")
        endif()
        if(CMAKE_CONFIGURATION_TYPES)
            set(_rust_cxxflags
                "$<IF:$<CONFIG:Debug>,$ENV{CXXFLAGS} ${_rust_debug_runtime},$ENV{CXXFLAGS} ${_rust_release_runtime}>")
            set(_rust_cflags
                "$<IF:$<CONFIG:Debug>,$ENV{CFLAGS} ${_rust_debug_runtime},$ENV{CFLAGS} ${_rust_release_runtime}>")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_rust_cxxflags "$ENV{CXXFLAGS} ${_rust_debug_runtime}")
            set(_rust_cflags "$ENV{CFLAGS} ${_rust_debug_runtime}")
        else()
            set(_rust_cxxflags "$ENV{CXXFLAGS} ${_rust_release_runtime}")
            set(_rust_cflags "$ENV{CFLAGS} ${_rust_release_runtime}")
        endif()
        list(INSERT _cargo_environment 0
            "CXXFLAGS=${_rust_cxxflags}"
            "CFLAGS=${_rust_cflags}"
        )
        if(_clip_rust_static_crt)
            list(INSERT _cargo_environment 0
                "RUSTFLAGS=$ENV{RUSTFLAGS} -Dwarnings -C target-feature=+crt-static")
        endif()
    endif()

    set(_cargo_feature_args)
    if(CLIP_RUST_FEATURES)
        list(JOIN CLIP_RUST_FEATURES "," _cargo_features)
        list(APPEND _cargo_feature_args --features "${_cargo_features}")
    endif()
    set(_cargo_commands
        COMMAND "${CMAKE_COMMAND}" -E env
            ${_cargo_environment}
            "${CARGO_EXECUTABLE}" build --locked
            ${_cargo_package_args}
            ${_cargo_feature_args}
            --target "${CLIP_RUST_TARGET}"
            --profile "${_cargo_profile}"
    )
    if(CLIP_RUST_STRIP_MSVC_DIRECTIVES AND CLIP_RUST_TARGET MATCHES "gnu$")
        find_program(RUST_ARCHIVE_OBJCOPY NAMES llvm-objcopy objcopy REQUIRED)
        foreach(_static_library IN LISTS _static_libraries)
            list(APPEND _cargo_commands
                COMMAND "${RUST_ARCHIVE_OBJCOPY}" --remove-section=.drectve "${_static_library}")
        endforeach()
    endif()

    add_custom_target("${batch_name}_build"
        ${_cargo_commands}
        BYPRODUCTS ${_static_libraries}
        WORKING_DIRECTORY "${CLIP_RUST_MANIFEST_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
    )

    foreach(_index RANGE ${_rust_library_last})
        list(GET CLIP_RUST_TARGETS ${_index} _target_name)
        list(GET _static_libraries ${_index} _static_library)
        list(GET _debug_libraries ${_index} _debug_library)
        list(GET _release_libraries ${_index} _release_library)
        add_library("${_target_name}" STATIC IMPORTED GLOBAL)
        if(CMAKE_CONFIGURATION_TYPES)
            set_target_properties("${_target_name}" PROPERTIES
                IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
                IMPORTED_LOCATION_DEBUG "${_debug_library}"
                IMPORTED_LOCATION_RELEASE "${_release_library}"
                IMPORTED_LOCATION_RELWITHDEBINFO "${_release_library}"
                IMPORTED_LOCATION_MINSIZEREL "${_release_library}"
            )
        else()
            set_target_properties("${_target_name}" PROPERTIES
                IMPORTED_LOCATION "${_static_library}")
        endif()
        add_dependencies("${_target_name}" "${batch_name}_build")
    endforeach()
endfunction()

function(clip_add_rust_static_library target_name)
    set(options STRIP_MSVC_DIRECTIVES)
    set(oneValueArgs PACKAGE MANIFEST_DIR OUTPUT_NAME)
    cmake_parse_arguments(CLIP_RUST "${options}" "${oneValueArgs}" "FEATURES" ${ARGN})

    foreach(_required IN ITEMS PACKAGE MANIFEST_DIR)
        if(NOT CLIP_RUST_${_required})
            message(FATAL_ERROR "clip_add_rust_static_library requires ${_required}")
        endif()
    endforeach()
    if(NOT CLIP_RUST_OUTPUT_NAME)
        set(CLIP_RUST_OUTPUT_NAME "${CLIP_RUST_PACKAGE}")
    endif()
    set(_strip_option)
    if(CLIP_RUST_STRIP_MSVC_DIRECTIVES)
        set(_strip_option STRIP_MSVC_DIRECTIVES)
    endif()
    clip_add_rust_static_libraries("${target_name}_rust"
        MANIFEST_DIR "${CLIP_RUST_MANIFEST_DIR}"
        TARGETS "${target_name}"
        PACKAGES "${CLIP_RUST_PACKAGE}"
        OUTPUT_NAMES "${CLIP_RUST_OUTPUT_NAME}"
        FEATURES ${CLIP_RUST_FEATURES}
        ${_strip_option}
    )
endfunction()

function(clip_add_rust_executable target_name)
    set(options PERFORMANCE_PROFILE NO_DEFAULT_FEATURES REPRODUCIBLE SIZE_OPTIMIZED)
    set(oneValueArgs PACKAGE MANIFEST_DIR OUTPUT_NAME PRODUCTION_PROFILE)
    set(multiValueArgs FEATURES ENVIRONMENT)
    cmake_parse_arguments(CLIP_RUST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    foreach(_required IN ITEMS PACKAGE MANIFEST_DIR)
        if(NOT CLIP_RUST_${_required})
            message(FATAL_ERROR "clip_add_rust_executable requires ${_required}")
        endif()
    endforeach()
    if(NOT CLIP_RUST_OUTPUT_NAME)
        set(CLIP_RUST_OUTPUT_NAME "${CLIP_RUST_PACKAGE}")
    endif()
    find_program(CARGO_EXECUTABLE NAMES cargo REQUIRED)
    if(NOT DEFINED CLIP_RUST_CARGO_TARGET_DIR OR CLIP_RUST_CARGO_TARGET_DIR STREQUAL "")
        set(CLIP_RUST_CARGO_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo" CACHE PATH
            "Cargo target directory for the active CMake build tree.")
    endif()
    if(NOT DEFINED CLIP_RUST_RELEASE_PROFILE OR CLIP_RUST_RELEASE_PROFILE STREQUAL "")
        set(CLIP_RUST_RELEASE_PROFILE "release" CACHE STRING
            "Cargo profile used for non-Debug Rust builds.")
    endif()
    set(_selected_release_profile "${CLIP_RUST_RELEASE_PROFILE}")
    # The fast preset is explicitly intended to avoid expensive full LTO, even for
    # executables which use a dedicated production profile in release packages.
    if(CLIP_RUST_PRODUCTION_PROFILE AND NOT CLIP_RUST_RELEASE_PROFILE STREQUAL "release-fast")
        set(_selected_release_profile "${CLIP_RUST_PRODUCTION_PROFILE}")
    endif()
    clip_detect_rust_target(_rust_target)
    if(CLIP_RUST_PERFORMANCE_PROFILE)
        # The executable is performance-critical and independent of the CMake
        # configuration, so always build it with the release profile.
        set(_profile "${_selected_release_profile}")
        set(_cargo_profile "${_selected_release_profile}")
    elseif(CMAKE_CONFIGURATION_TYPES)
        set(_profile "$<IF:$<CONFIG:Debug>,debug,${_selected_release_profile}>")
        set(_cargo_profile "$<IF:$<CONFIG:Debug>,dev,${_selected_release_profile}>")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_profile debug)
        set(_cargo_profile dev)
    else()
        set(_profile "${_selected_release_profile}")
        set(_cargo_profile "${_selected_release_profile}")
    endif()
    if(WIN32)
        set(_binary_suffix ".exe")
    else()
        set(_binary_suffix)
    endif()
    set(_binary_path
        "${CLIP_RUST_CARGO_TARGET_DIR}/${_rust_target}/${_profile}/${CLIP_RUST_OUTPUT_NAME}${_binary_suffix}")
    set(_debug_binary_path
        "${CLIP_RUST_CARGO_TARGET_DIR}/${_rust_target}/debug/${CLIP_RUST_OUTPUT_NAME}${_binary_suffix}")
    set(_release_binary_path
        "${CLIP_RUST_CARGO_TARGET_DIR}/${_rust_target}/${_selected_release_profile}/${CLIP_RUST_OUTPUT_NAME}${_binary_suffix}")
    set(_cargo_environment
        "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}")
    clip_windows_rust_environment(_windows_cargo_environment "${_rust_target}")
    list(APPEND _cargo_environment ${_windows_cargo_environment})
    if(WIN32 AND (CLIP_LIBCLANG_BIN_DIR OR DEFINED ENV{LIBCLANG_PATH}))
        clip_resolve_rust_libclang_directory(_libclang_dir)
        list(APPEND _cargo_environment "LIBCLANG_PATH=${_libclang_dir}")
    endif()
    if(DEFINED VCPKG_ROOT)
        list(APPEND _cargo_environment "VCPKG_ROOT=${VCPKG_ROOT}")
    elseif(DEFINED ENV{VCPKG_ROOT})
        list(APPEND _cargo_environment "VCPKG_ROOT=$ENV{VCPKG_ROOT}")
    endif()
    if(DEFINED VCPKG_TARGET_TRIPLET)
        list(APPEND _cargo_environment "VCPKGRS_TRIPLET=${VCPKG_TARGET_TRIPLET}")
    endif()
    if(CLIP_SHOT_RELEASE_STATIC OR CLIP_APPS_RELEASE_STATIC)
        list(APPEND _cargo_environment "VCPKGRS_DYNAMIC=0")
    else()
        list(APPEND _cargo_environment "VCPKGRS_DYNAMIC=1")
    endif()
    if(CLIP_RUST_ENVIRONMENT)
        list(APPEND _cargo_environment ${CLIP_RUST_ENVIRONMENT})
    endif()
    if(MSVC)
        set(_rust_release_link_flags)
        if(CLIP_RUST_REPRODUCIBLE)
            set(_rust_release_link_flags " -C link-arg=/Brepro")
        endif()
        if(CLIP_RUST_SIZE_OPTIMIZED)
            string(APPEND _rust_release_link_flags
                " -C link-arg=/OPT:REF -C link-arg=/OPT:ICF -C link-arg=/INCREMENTAL:NO -C link-arg=/DEBUG:FULL")
        endif()
        set(_rust_link_flags "${_rust_release_link_flags}")
        # Size-tuned link flags belong to the production profile. A multi-config
        # Debug build must retain Cargo's ordinary development-link behavior.
        if(CLIP_RUST_SIZE_OPTIMIZED AND NOT CLIP_RUST_PERFORMANCE_PROFILE)
            if(CMAKE_CONFIGURATION_TYPES)
                set(_rust_link_flags "$<IF:$<CONFIG:Debug>,,${_rust_release_link_flags}>")
            elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(_rust_link_flags)
            endif()
        endif()
        set(_clip_rust_static_crt FALSE)
        if(CLIP_APPS_RELEASE_STATIC OR CLIP_SHOT_RELEASE_STATIC OR
           (CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "^MultiThreaded" AND
            NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL"))
            set(_clip_rust_static_crt TRUE)
        endif()
        # Select the CRT flavor from the CMake configuration. In a multi-config
        # generator, _cargo_profile is itself a generator expression and cannot
        # be compared with STREQUAL at configure time. Performance executables
        # deliberately retain their release CRT even in a Debug configuration.
        if(_clip_rust_static_crt)
            set(_rust_debug_runtime "/MTd /D_DEBUG")
            set(_rust_release_runtime "/MT")
        else()
            set(_rust_debug_runtime "/MDd /D_DEBUG")
            set(_rust_release_runtime "/MD")
        endif()
        if(CLIP_RUST_PERFORMANCE_PROFILE)
            set(_rust_c_runtime "${_rust_release_runtime}")
        elseif(CMAKE_CONFIGURATION_TYPES)
            set(_rust_c_runtime
                "$<IF:$<CONFIG:Debug>,${_rust_debug_runtime},${_rust_release_runtime}>")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_rust_c_runtime "${_rust_debug_runtime}")
        else()
            set(_rust_c_runtime "${_rust_release_runtime}")
        endif()
        list(APPEND _cargo_environment
            "CXXFLAGS=$ENV{CXXFLAGS} ${_rust_c_runtime}"
            "CFLAGS=$ENV{CFLAGS} ${_rust_c_runtime}"
        )
        if(_clip_rust_static_crt)
            list(INSERT _cargo_environment 0
                "RUSTFLAGS=$ENV{RUSTFLAGS} -Dwarnings -C target-feature=+crt-static${_rust_link_flags}")
        elseif(CLIP_RUST_REPRODUCIBLE OR CLIP_RUST_SIZE_OPTIMIZED)
            list(INSERT _cargo_environment 0
                "RUSTFLAGS=$ENV{RUSTFLAGS} -Dwarnings${_rust_link_flags}")
        endif()
    endif()
    set(_cargo_feature_args)
    if(CLIP_RUST_NO_DEFAULT_FEATURES)
        list(APPEND _cargo_feature_args --no-default-features)
    endif()
    if(CLIP_RUST_FEATURES)
        list(JOIN CLIP_RUST_FEATURES "," _cargo_features)
        list(APPEND _cargo_feature_args --features "${_cargo_features}")
    endif()
    add_custom_target("${target_name}_build"
        COMMAND "${CMAKE_COMMAND}" -E env
            ${_cargo_environment}
            "${CARGO_EXECUTABLE}" build --locked
            --package "${CLIP_RUST_PACKAGE}"
            --target "${_rust_target}"
            --profile "${_cargo_profile}"
            ${_cargo_feature_args}
        BYPRODUCTS "${_binary_path}"
        WORKING_DIRECTORY "${CLIP_RUST_MANIFEST_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    add_executable("${target_name}" IMPORTED GLOBAL)
    if(CLIP_RUST_PERFORMANCE_PROFILE)
        set_target_properties("${target_name}" PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
            IMPORTED_LOCATION "${_release_binary_path}"
            IMPORTED_LOCATION_DEBUG "${_release_binary_path}"
            IMPORTED_LOCATION_RELEASE "${_release_binary_path}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${_release_binary_path}"
            IMPORTED_LOCATION_MINSIZEREL "${_release_binary_path}"
        )
    elseif(CMAKE_CONFIGURATION_TYPES)
        set_target_properties("${target_name}" PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
            IMPORTED_LOCATION_DEBUG "${_debug_binary_path}"
            IMPORTED_LOCATION_RELEASE "${_release_binary_path}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${_release_binary_path}"
            IMPORTED_LOCATION_MINSIZEREL "${_release_binary_path}"
        )
    else()
        set_target_properties("${target_name}" PROPERTIES IMPORTED_LOCATION "${_binary_path}")
    endif()
    add_dependencies("${target_name}" "${target_name}_build")
    set_property(TARGET "${target_name}" PROPERTY
        CLIP_RUST_PROFILE_DIRECTORY
        "${CLIP_RUST_CARGO_TARGET_DIR}/${_rust_target}/${_selected_release_profile}")
endfunction()
