set(
    CLIP_IMAGE_VIEWER_ADQT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../ant_design_qt"
    CACHE PATH
    "Path to the ant_design_qt monorepo checkout."
)

function(clip_image_viewer_add_ant_design_qt)
    if(TARGET adqt::widgets)
        return()
    endif()

    if(NOT EXISTS "${CLIP_IMAGE_VIEWER_ADQT_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "Unable to locate ant_design_qt at '${CLIP_IMAGE_VIEWER_ADQT_DIR}'. "
            "Set CLIP_IMAGE_VIEWER_ADQT_DIR to its repository root."
        )
    endif()

    add_subdirectory("${CLIP_IMAGE_VIEWER_ADQT_DIR}"
                     "${CMAKE_CURRENT_BINARY_DIR}/ant_design_qt")
endfunction()
