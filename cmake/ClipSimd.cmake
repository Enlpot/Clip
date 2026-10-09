include_guard(GLOBAL)

# Keep runtime-dispatched AVX2 kernels separate from baseline translation units.
# On macOS, CMAKE_SYSTEM_PROCESSOR can describe the host during a cross build.
function(clip_configure_avx2_sources)
    set(_clip_avx2_options)
    if(MSVC)
        if(CLIP_WINDOWS_ARCHITECTURE STREQUAL "x64")
            set(_clip_avx2_options "/arch:AVX2")
        endif()
    elseif(APPLE AND CMAKE_OSX_ARCHITECTURES)
        if("x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
            list(LENGTH CMAKE_OSX_ARCHITECTURES _clip_architecture_count)
            if(_clip_architecture_count EQUAL 1)
                set(_clip_avx2_options "-mavx2")
            else()
                # Apply the option only to the x64 slice of a universal build.
                set(_clip_avx2_options "-Xarch_x86_64" "-mavx2")
            endif()
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|x86|i.86)$")
        set(_clip_avx2_options "-mavx2")
    endif()

    if(_clip_avx2_options)
        set_property(SOURCE ${ARGN} APPEND PROPERTY COMPILE_OPTIONS ${_clip_avx2_options})
        set_source_files_properties(${ARGN} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Clang cannot load a baseline-CPU PCH in an AVX2 translation unit.
            set_source_files_properties(${ARGN} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
        endif()
    endif()
endfunction()
