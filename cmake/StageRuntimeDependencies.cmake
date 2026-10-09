cmake_minimum_required(VERSION 3.25)

if(CLIP_RUNTIME_ROOT_LIBRARY)
    list(APPEND CLIP_RUNTIME_ROOT_LIBRARIES "${CLIP_RUNTIME_ROOT_LIBRARY}")
endif()
if(NOT CLIP_RUNTIME_ROOT_LIBRARIES)
    message(FATAL_ERROR "A runtime root library is required to resolve runtime dependencies.")
endif()
if(NOT CLIP_RUNTIME_DESTINATION AND NOT CLIP_RUNTIME_MANIFEST)
    message(FATAL_ERROR "A runtime destination or manifest is required.")
endif()

set(_clip_runtime_search_directories)
foreach(_clip_runtime_root IN LISTS CLIP_RUNTIME_ROOT_LIBRARIES)
    if(NOT EXISTS "${_clip_runtime_root}")
        message(FATAL_ERROR "Runtime root library does not exist: ${_clip_runtime_root}")
    endif()
    get_filename_component(_clip_runtime_root_directory "${_clip_runtime_root}" DIRECTORY)
    list(APPEND _clip_runtime_search_directories "${_clip_runtime_root_directory}")
endforeach()

if(DEFINED CLIP_RUNTIME_SEARCH_DIRECTORY AND
   IS_DIRECTORY "${CLIP_RUNTIME_SEARCH_DIRECTORY}")
    list(APPEND _clip_runtime_search_directories
        "${CLIP_RUNTIME_SEARCH_DIRECTORY}")
endif()
list(REMOVE_DUPLICATES _clip_runtime_search_directories)

file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES ${CLIP_RUNTIME_ROOT_LIBRARIES}
    DIRECTORIES ${_clip_runtime_search_directories}
    RESOLVED_DEPENDENCIES_VAR _clip_runtime_resolved_dependencies
    UNRESOLVED_DEPENDENCIES_VAR _clip_runtime_unresolved_dependencies
    CONFLICTING_DEPENDENCIES_PREFIX _clip_runtime_conflicts
    PRE_EXCLUDE_REGEXES
        "api-ms-.*"
        "ext-ms-.*"
        "msvcp.*\\.dll"
        "ucrtbased\\.dll"
        "vcruntime.*\\.dll"
    POST_EXCLUDE_REGEXES
        ".*[\\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/][Ss][Yy][Ss][Tt][Ee][Mm]32[\\\\/].*"
)

if(_clip_runtime_unresolved_dependencies)
    list(JOIN _clip_runtime_unresolved_dependencies ", " _clip_runtime_unresolved_text)
    message(FATAL_ERROR
        "Unable to resolve runtime dependencies for ${CLIP_RUNTIME_ROOT_LIBRARIES}: "
        "${_clip_runtime_unresolved_text}")
endif()
if(_clip_runtime_conflicts_FILENAMES)
    list(JOIN _clip_runtime_conflicts_FILENAMES ", " _clip_runtime_conflicts_text)
    message(FATAL_ERROR
        "Conflicting runtime dependencies for ${CLIP_RUNTIME_ROOT_LIBRARIES}: "
        "${_clip_runtime_conflicts_text}")
endif()

set(_clip_runtime_files
    ${CLIP_RUNTIME_ROOT_LIBRARIES}
    ${_clip_runtime_resolved_dependencies}
)
list(REMOVE_DUPLICATES _clip_runtime_files)
if(CLIP_RUNTIME_MANIFEST)
    file(WRITE "${CLIP_RUNTIME_MANIFEST}" "set(CLIP_RUNTIME_FILES\n")
    foreach(_clip_runtime_file IN LISTS _clip_runtime_files)
        file(APPEND "${CLIP_RUNTIME_MANIFEST}" "    [==[${_clip_runtime_file}]==]\n")
    endforeach()
    file(APPEND "${CLIP_RUNTIME_MANIFEST}" ")\n")
endif()
if(NOT CLIP_RUNTIME_DESTINATION)
    return()
endif()
file(MAKE_DIRECTORY "${CLIP_RUNTIME_DESTINATION}")
foreach(_clip_runtime_file IN LISTS _clip_runtime_files)
    get_filename_component(_clip_runtime_filename "${_clip_runtime_file}" NAME)
    set(_clip_runtime_destination_file
        "${CLIP_RUNTIME_DESTINATION}/${_clip_runtime_filename}")
    if(NOT _clip_runtime_file STREQUAL _clip_runtime_destination_file)
        file(COPY_FILE
            "${_clip_runtime_file}"
            "${_clip_runtime_destination_file}"
            ONLY_IF_DIFFERENT
        )
    endif()
endforeach()
