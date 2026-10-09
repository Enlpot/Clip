# Editable feature catalogs are merged only in the build tree. Each edition
# shares its lrelease rule while retaining the same runtime resource names.
set(QT_I18N_SOURCE_LANGUAGE en_US)
include("${CMAKE_CURRENT_LIST_DIR}/ClipShotTranslationRelease.cmake")
set(_clip_shot_catalog_dir "${CMAKE_CURRENT_SOURCE_DIR}/i18n")
set(_clip_shot_catalog_tool "${CMAKE_CURRENT_SOURCE_DIR}/scripts/translation_catalogs.py")
set(_clip_shot_merged_dir "${CMAKE_CURRENT_BINARY_DIR}/i18n/merged")
set(_clip_shot_update_dir "${CMAKE_CURRENT_BINARY_DIR}/i18n/update")
file(GLOB_RECURSE TS_FILES CONFIGURE_DEPENDS "${_clip_shot_catalog_dir}/*.ts")
list(SORT TS_FILES)
set(_clip_shot_merged_ts)
set(_clip_shot_update_ts)
foreach(_locale IN ITEMS en_US zh_CN zh_TW)
    list(APPEND _clip_shot_merged_ts "${_clip_shot_merged_dir}/clip_shot_${_locale}.ts")
    list(APPEND _clip_shot_update_ts "${_clip_shot_update_dir}/clip_shot_${_locale}.ts")
endforeach()

# Qt reads the TS language during configuration when merging its own catalogs.
# Seed real catalogs rather than allowing Qt to create empty generated inputs.
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${_clip_shot_catalog_tool}" merge
        --catalog-dir "${_clip_shot_catalog_dir}" --output-dir "${_clip_shot_merged_dir}"
    COMMAND_ERROR_IS_FATAL ANY
)
add_custom_command(
    OUTPUT ${_clip_shot_merged_ts}
    COMMAND "${Python3_EXECUTABLE}" "${_clip_shot_catalog_tool}" merge
        --catalog-dir "${_clip_shot_catalog_dir}" --output-dir "${_clip_shot_merged_dir}"
    DEPENDS ${TS_FILES} "${_clip_shot_catalog_dir}/modules.json" "${_clip_shot_catalog_tool}"
    COMMENT "Merging Clip feature translation catalogs"
    VERBATIM
)
clip_shot_release_translation_catalogs(clip_shot_release_translations
    "${_clip_shot_merged_dir}" "${CMAKE_CURRENT_BINARY_DIR}" _clip_shot_qm_files)

if(CLIP_APPS_BUILD_CLIP_SHOT_MINI)
    set(_clip_shot_mini_merged_dir "${CMAKE_CURRENT_BINARY_DIR}/i18n/mini-merged")
    set(_clip_shot_mini_qm_dir "${CMAKE_CURRENT_BINARY_DIR}/i18n/mini")
    set(_clip_shot_mini_merged_ts)
    foreach(_locale IN ITEMS en_US zh_CN zh_TW)
        list(APPEND _clip_shot_mini_merged_ts
            "${_clip_shot_mini_merged_dir}/clip_shot_${_locale}.ts")
    endforeach()
    set(_clip_shot_mini_catalog_arguments --exclude-module translation)
    foreach(_context IN ITEMS ClipShotApiClient CustomAiModelsSettingsWidget
            TextTranslationSettingsWidget ScreenshotQrController
            ScreenshotImageConversionController ScreenshotImageConversionView
            ScreenshotTableEditor ScreenshotRecognitionFileExport)
        list(APPEND _clip_shot_mini_catalog_arguments --exclude-context "${_context}")
    endforeach()
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${_clip_shot_catalog_tool}" merge
            --catalog-dir "${_clip_shot_catalog_dir}" --output-dir "${_clip_shot_mini_merged_dir}"
            ${_clip_shot_mini_catalog_arguments}
        COMMAND_ERROR_IS_FATAL ANY)
    add_custom_command(
        OUTPUT ${_clip_shot_mini_merged_ts}
        COMMAND "${Python3_EXECUTABLE}" "${_clip_shot_catalog_tool}" merge
            --catalog-dir "${_clip_shot_catalog_dir}" --output-dir "${_clip_shot_mini_merged_dir}"
            ${_clip_shot_mini_catalog_arguments}
        DEPENDS ${TS_FILES} "${_clip_shot_catalog_dir}/modules.json" "${_clip_shot_catalog_tool}"
        COMMENT "Merging Mini translation catalogs without excluded feature contexts"
        VERBATIM)
    clip_shot_release_translation_catalogs(clip_shot_mini_release_translations
        "${_clip_shot_mini_merged_dir}" "${_clip_shot_mini_qm_dir}" _clip_shot_mini_qm_files)
endif()

function(clip_shot_add_translations target)
    if("MINI" IN_LIST ARGN)
        set(_qm_files ${_clip_shot_mini_qm_files})
        set(_qm_base "${_clip_shot_mini_qm_dir}")
        set(_release_target clip_shot_mini_release_translations)
    else()
        set(_qm_files ${_clip_shot_qm_files})
        set(_qm_base "${CMAKE_CURRENT_BINARY_DIR}")
        set(_release_target clip_shot_release_translations)
    endif()
    qt_add_resources(${target} "${target}_translations"
        PREFIX "/i18n"
        BASE "${_qm_base}"
        FILES ${_qm_files}
    )
    add_dependencies(${target} ${_release_target})

    # Test targets consume the complete catalogs but must never extract their
    # partial source lists back into application translations.
    if(NOT target STREQUAL "clip_shot")
        return()
    endif()

    add_custom_target(clip_shot_prepare_translation_update
        COMMAND "${Python3_EXECUTABLE}" "${_clip_shot_catalog_tool}" merge
            --catalog-dir "${_clip_shot_catalog_dir}" --output-dir "${_clip_shot_update_dir}"
        VERBATIM
    )
    qt_add_lupdate(
        SOURCE_TARGETS
            clip_shot_administrator
            clip_shot
            clip_shot_storage
            clip_shot_main_window_skin
            clip_shot_settings_catalog
            clip_shot_settings_search
            clip_shot_settings
            clip_shot_permissions
            clip_shot_global_mouse
            clip_shot_translation
            clip_shot_cloud_upload
            clip_shot_print
            clip_shot_diagnostics
            clip_shot_updates
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/update/updateservice.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/nativeprint.cpp"
        TS_FILES ${_clip_shot_update_ts}
        LUPDATE_TARGET clip_shot_update_translations
        OPTIONS -no-obsolete -locations none
    )
    add_dependencies(clip_shot_update_translations clip_shot_prepare_translation_update)
    add_custom_command(TARGET clip_shot_update_translations POST_BUILD
        COMMAND "${Python3_EXECUTABLE}" "${_clip_shot_catalog_tool}" split
            --catalog-dir "${_clip_shot_catalog_dir}" --input-dir "${_clip_shot_update_dir}"
        COMMENT "Updating Clip feature translation catalogs"
        VERBATIM
    )
endfunction()
