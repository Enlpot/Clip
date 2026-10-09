if(NOT DEFINED CLIP_MACOS_ICON_OUTPUT OR CLIP_MACOS_ICON_OUTPUT STREQUAL "")
    message(FATAL_ERROR "CLIP_MACOS_ICON_OUTPUT is required")
endif()
if(NOT DEFINED CLIP_MACOS_ICON_ARTWORK OR CLIP_MACOS_ICON_ARTWORK STREQUAL "")
    message(FATAL_ERROR "CLIP_MACOS_ICON_ARTWORK is required")
endif()

find_program(CLIP_SIPS_EXECUTABLE NAMES sips REQUIRED)
find_program(CLIP_ICONUTIL_EXECUTABLE NAMES iconutil REQUIRED)
find_program(CLIP_BASE64_EXECUTABLE NAMES base64 REQUIRED)

get_filename_component(_clip_icon_output_dir "${CLIP_MACOS_ICON_OUTPUT}" DIRECTORY)
set(_clip_iconset "${_clip_icon_output_dir}/clip-shot.iconset")
set(_clip_resolved_icon_source "${_clip_icon_output_dir}/clip-shot-macos-icon.svg")
file(REMOVE_RECURSE "${_clip_iconset}")
file(MAKE_DIRECTORY "${_clip_iconset}")

execute_process(
    COMMAND "${CLIP_BASE64_EXECUTABLE}" -i "${CLIP_MACOS_ICON_ARTWORK}"
    OUTPUT_VARIABLE _clip_icon_artwork_base64
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _clip_base64_result)
if(NOT _clip_base64_result EQUAL 0)
    message(FATAL_ERROR "base64 failed to encode ${CLIP_MACOS_ICON_ARTWORK}")
endif()
string(CONCAT _clip_icon_source
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1024\" height=\"1024\" "
    "viewBox=\"0 0 1024 1024\">\n"
    "  <image x=\"100\" y=\"100\" width=\"824\" height=\"824\" "
    "href=\"data:image/svg+xml;base64,${_clip_icon_artwork_base64}\"/>\n"
    "</svg>\n")
file(WRITE "${_clip_resolved_icon_source}" "${_clip_icon_source}")

set(_clip_icon_rasters
    "16|icon_16x16.png"
    "32|icon_16x16@2x.png"
    "32|icon_32x32.png"
    "64|icon_32x32@2x.png"
    "128|icon_128x128.png"
    "256|icon_128x128@2x.png"
    "256|icon_256x256.png"
    "512|icon_256x256@2x.png"
    "512|icon_512x512.png"
    "1024|icon_512x512@2x.png")

foreach(_clip_icon_raster IN LISTS _clip_icon_rasters)
    string(REPLACE "|" ";" _clip_icon_fields "${_clip_icon_raster}")
    list(GET _clip_icon_fields 0 _clip_icon_size)
    list(GET _clip_icon_fields 1 _clip_icon_name)
    execute_process(
        COMMAND "${CLIP_SIPS_EXECUTABLE}" -s format png -z
            "${_clip_icon_size}" "${_clip_icon_size}"
            "${_clip_resolved_icon_source}"
            --out "${_clip_iconset}/${_clip_icon_name}"
        OUTPUT_QUIET
        RESULT_VARIABLE _clip_sips_result)
    if(NOT _clip_sips_result EQUAL 0)
        message(FATAL_ERROR "sips failed to render ${_clip_icon_name}")
    endif()
endforeach()

execute_process(
    COMMAND "${CLIP_ICONUTIL_EXECUTABLE}" -c icns "${_clip_iconset}"
        -o "${CLIP_MACOS_ICON_OUTPUT}"
    RESULT_VARIABLE _clip_iconutil_result)
if(NOT _clip_iconutil_result EQUAL 0)
    message(FATAL_ERROR "iconutil failed to generate ${CLIP_MACOS_ICON_OUTPUT}")
endif()

file(REMOVE_RECURSE "${_clip_iconset}")
file(REMOVE "${_clip_resolved_icon_source}")
