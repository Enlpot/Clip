# Full-only leaf implementations must never enter Mini through a copied source
# list or a transitive static/object library. Shared image codecs and recording
# dependencies remain available because Mini still saves images and records.
include_guard(GLOBAL)

set(CLIP_SHOT_MINI_EXCLUDED_SOURCE_NAMES
    selectedtexttranslationcoordinator selectedtexttranslationcontroller
    standalonetranslationwindow translationpagewidget translationpagecontroller
    translationservice translationlanguages clipshotapiclient
    texttranslationsettingswidget customaimodelssettingswidget
    screenshottranslationsettingsdialog
    screenshotqrcontroller screenshotqrrecognitionservice
    screenshotimageconversioncontroller screenshotimageconversionview
    screenshottabledocument screenshottableeditor screenshotrecognitionfileexport)
list(JOIN CLIP_SHOT_MINI_EXCLUDED_SOURCE_NAMES "|" _clip_mini_source_names)
set(CLIP_SHOT_MINI_EXCLUDED_SOURCE_PATTERN
    "(^|[/,:])(${_clip_mini_source_names})\\.(cpp|h|mm)($|>)")
set(CLIP_SHOT_MINI_EXCLUDED_LINK_TARGETS
    clip_shot_translation clip_selected_text_c clip_shot_full_rust_ffi_bundle
    opencv_wechat_qrcode opencv_objdetect opencv_dnn)
set(CLIP_SHOT_MINI_EXCLUDED_TARGETS
    ${CLIP_SHOT_MINI_EXCLUDED_LINK_TARGETS}
    clip_shot clip_shot_edition_full
    clip_shot_storage clip_shot_main_window_skin clip_shot_settings_catalog clip_shot_settings_search
    clip_shot_settings clip_shot_global_mouse clip_shot_login_item
    clip_shot_administrator clip_shot_permissions clip_shot_updates
    clip_shot_diagnostics clip_shot_crash_bridge
    clip-shot-updater-binary clip-shot-updater-binary_build
    clip_shot_release_translations)
if(WIN32)
    # Windows Mini acquires its trusted text OCR worker on demand. macOS Mini
    # bundles the local worker but acquires every selected model on demand.
    list(APPEND CLIP_SHOT_MINI_EXCLUDED_TARGETS
        clip_ocr_process clip_ocr_process_build clip_ocr_diagnostics_bridge
        onnxruntime::onnxruntime)
endif()

function(clip_shot_mini_filter_sources output)
    set(_sources ${ARGN})
    list(FILTER _sources EXCLUDE REGEX "${CLIP_SHOT_MINI_EXCLUDED_SOURCE_PATTERN}")
    list(FILTER _sources EXCLUDE REGEX "qrc_clip_shot_translations\\.cpp$|/clip_shot_translations\\.qrc$")
    # Qt also attaches generated .qm inputs as header-only target sources. Do
    # not copy these, or building Mini still invokes Full's lrelease commands.
    list(FILTER _sources EXCLUDE REGEX "(^|/)clip_shot_(en_US|zh_CN|zh_TW)\\.qm$")
    set(${output} "${_sources}" PARENT_SCOPE)
endfunction()

# Inspect target references even inside LINK_ONLY, TARGET_OBJECTS and conditional
# generator expressions. Reject all configurations, including currently inactive
# branches, so switching a preset cannot silently restore an excluded feature.
function(clip_shot_assert_mini_build_contract root)
    set(_pending ${root})
    set(_visited)
    while(_pending)
        list(POP_FRONT _pending _target)
        if(_target IN_LIST _visited)
            continue()
        endif()
        list(APPEND _visited "${_target}")
        get_target_property(_alias ${_target} ALIASED_TARGET)
        if(_alias)
            list(APPEND _pending "${_alias}")
        endif()
        if(_target IN_LIST CLIP_SHOT_MINI_EXCLUDED_TARGETS)
            message(FATAL_ERROR "Mini build includes Full-only target: ${_target} (from ${root})")
        endif()
        foreach(_property IN ITEMS SOURCES INTERFACE_SOURCES LINK_LIBRARIES
                INTERFACE_LINK_LIBRARIES MANUALLY_ADDED_DEPENDENCIES)
            get_target_property(_items ${_target} ${_property})
            if(NOT _items)
                continue()
            endif()
            if(_property MATCHES "SOURCES$")
                foreach(_source IN LISTS _items)
                    string(REPLACE "\\" "/" _source "${_source}")
                    if(_source MATCHES "${CLIP_SHOT_MINI_EXCLUDED_SOURCE_PATTERN}" OR
                            _source MATCHES "qrc_clip_shot_translations\\.cpp$|/clip_shot_translations\\.qrc$" OR
                            (_source MATCHES "(^|/)clip_shot_(en_US|zh_CN|zh_TW)\\.qm$" AND
                                NOT _source MATCHES "(^|/)i18n/mini/clip_shot_[^/]+\\.qm$"))
                        message(FATAL_ERROR "Mini build includes Full-only source: ${_source} (${_target})")
                    endif()
                endforeach()
            endif()
            set(_expressions)
            foreach(_item IN LISTS _items)
                if(TARGET "${_item}")
                    list(APPEND _pending "${_item}")
                elseif(_property MATCHES "SOURCES$")
                    string(REGEX MATCHALL "\\$<TARGET_OBJECTS:[^>]+>" _objects "${_item}")
                    list(APPEND _expressions ${_objects})
                elseif(_item MATCHES "\\$<")
                    list(APPEND _expressions "${_item}")
                endif()
            endforeach()
            string(REGEX MATCHALL "[A-Za-z0-9_.:+-]+" _references "${_expressions}")
            foreach(_reference IN LISTS _references)
                # ':' is also a generator-expression separator. Keep namespace
                # aliases such as Qt6::Core intact while unwrapping expressions.
                string(REGEX REPLACE "(^|[^:]):([^:])" "\\1;\\2" _references_in_item "${_reference}")
                foreach(_dependency IN LISTS _references_in_item)
                    if(TARGET "${_dependency}")
                        list(APPEND _pending "${_dependency}")
                    endif()
                endforeach()
            endforeach()
        endforeach()
    endwhile()
endfunction()
