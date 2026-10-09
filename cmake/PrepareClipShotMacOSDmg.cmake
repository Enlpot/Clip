# Keep the build target/executable and development bundle names stable. Only the
# distributable bundle gets the product name; renaming does not alter its seal.
if(NOT CPACK_CLIP_SHOT_BUNDLE_NAME)
    set(CPACK_CLIP_SHOT_BUNDLE_NAME clip_shot)
    set(CPACK_CLIP_SHOT_PRODUCT_NAME "Clip")
endif()
set(_clip_staged_app "${CPACK_TEMPORARY_DIRECTORY}/${CPACK_CLIP_SHOT_BUNDLE_NAME}.app")
if(NOT EXISTS "${_clip_staged_app}/Contents/Info.plist")
    message(FATAL_ERROR "The staged Clip application is missing: ${_clip_staged_app}")
endif()
file(RENAME "${_clip_staged_app}" "${CPACK_TEMPORARY_DIRECTORY}/${CPACK_CLIP_SHOT_PRODUCT_NAME}.app")
