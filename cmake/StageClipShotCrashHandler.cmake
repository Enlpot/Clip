# Build-tree helpers need their native dependencies beside them. Installation
# starts from the original helper and uses the normal bundle deployment/signing.
file(MAKE_DIRECTORY "${CLIP_HANDLER_DIRECTORY}")
set(_handler "${CLIP_HANDLER_DIRECTORY}/crashpad_handler")
file(COPY_FILE "${CLIP_HANDLER_SOURCE}" "${_handler}" ONLY_IF_DIFFERENT)
file(CHMOD "${_handler}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
if(NOT "${CLIP_HANDLER_ZLIB}" STREQUAL "")
    file(COPY "${CLIP_HANDLER_ZLIB}" DESTINATION "${CLIP_HANDLER_DIRECTORY}" FOLLOW_SYMLINK_CHAIN)
    execute_process(COMMAND /usr/bin/install_name_tool -add_rpath "@loader_path" "${_handler}"
        COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND /usr/bin/codesign --force --sign - "${_handler}"
        COMMAND_ERROR_IS_FATAL ANY)
endif()
