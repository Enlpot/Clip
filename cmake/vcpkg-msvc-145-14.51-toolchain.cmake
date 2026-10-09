if(NOT WIN32)
    message(FATAL_ERROR "The Clip Apps MSVC toolchain is Windows-only.")
endif()

if(DEFINED _VCPKG_ROOT_DIR)
    set(_clip_vcpkg_root "${_VCPKG_ROOT_DIR}")
else()
    get_filename_component(_clip_vcpkg_root
        "${CMAKE_CURRENT_LIST_DIR}/../.tools/vcpkg" ABSOLUTE)
endif()
set(_clip_vcpkg_windows_toolchain
    "${_clip_vcpkg_root}/scripts/toolchains/windows.cmake")
if(NOT EXISTS "${_clip_vcpkg_windows_toolchain}")
    message(FATAL_ERROR
        "The vcpkg Windows toolchain was not found: ${_clip_vcpkg_windows_toolchain}")
endif()

# A chainloaded toolchain replaces vcpkg's platform toolchain. Load its Windows
# defaults explicitly so the selected triplet still controls /MT versus /MD,
# optimization flags, and variables forwarded to non-CMake build systems.
include("${_clip_vcpkg_windows_toolchain}")
include("${CMAKE_CURRENT_LIST_DIR}/ClipWindowsArchitecture.cmake")
clip_windows_normalize_architecture(_clip_msvc_target_arch "${VCPKG_TARGET_ARCHITECTURE}")
if(CLIP_WINDOWS_HOST_ARCHITECTURE)
    set(_clip_msvc_host_arch "${CLIP_WINDOWS_HOST_ARCHITECTURE}")
elseif(NOT "$ENV{CLIP_MSVC_HOST_ARCHITECTURE}" STREQUAL "")
    set(_clip_msvc_host_arch "$ENV{CLIP_MSVC_HOST_ARCHITECTURE}")
elseif(NOT "$ENV{PROCESSOR_ARCHITEW6432}" STREQUAL "")
    set(_clip_msvc_host_arch "$ENV{PROCESSOR_ARCHITEW6432}")
elseif(NOT "$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "")
    set(_clip_msvc_host_arch "$ENV{PROCESSOR_ARCHITECTURE}")
else()
    set(_clip_msvc_host_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()
clip_windows_normalize_architecture(_clip_msvc_host_arch "${_clip_msvc_host_arch}")
set(_clip_msvc_component Microsoft.VisualStudio.Component.VC.Tools.x86.x64)
if(_clip_msvc_target_arch STREQUAL "arm64")
    set(_clip_msvc_component Microsoft.VisualStudio.Component.VC.Tools.ARM64)
endif()

set(_clip_msvc_145_root "$ENV{VCToolsInstallDir}")
if(_clip_msvc_145_root STREQUAL "")
    find_program(_clip_vswhere vswhere.exe
        PATHS
            "$ENV{SystemDrive}/Program Files (x86)/Microsoft Visual Studio/Installer"
            "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer"
    )
    if(_clip_vswhere)
        execute_process(
            COMMAND "${_clip_vswhere}" -latest -products *
                -requires "${_clip_msvc_component}"
                -property installationPath
            OUTPUT_VARIABLE _clip_vs_install
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        file(GLOB _clip_msvc_candidates
            "${_clip_vs_install}/VC/Tools/MSVC/14.51.*")
        list(SORT _clip_msvc_candidates COMPARE NATURAL ORDER DESCENDING)
        list(LENGTH _clip_msvc_candidates _clip_msvc_count)
        if(_clip_msvc_count GREATER 0)
            list(GET _clip_msvc_candidates 0 _clip_msvc_145_root)
        endif()
    endif()
endif()
if(_clip_msvc_145_root STREQUAL "")
    message(FATAL_ERROR
        "MSVC 14.51 was not found. Set VCToolsInstallDir or install ${_clip_msvc_component}.")
endif()
string(REGEX REPLACE "[/\\]+$" "" _clip_msvc_145_root "${_clip_msvc_145_root}")
file(TO_CMAKE_PATH "${_clip_msvc_145_root}" _clip_msvc_145_root)
set(_clip_msvc_145_bin
    "${_clip_msvc_145_root}/bin/Host${_clip_msvc_host_arch}/${_clip_msvc_target_arch}")
foreach(_clip_msvc_tool IN ITEMS cl.exe link.exe lib.exe)
    if(NOT EXISTS "${_clip_msvc_145_bin}/${_clip_msvc_tool}")
        message(FATAL_ERROR "The selected MSVC host/target tools are missing: ${_clip_msvc_145_bin}/${_clip_msvc_tool}")
    endif()
endforeach()
if(_clip_msvc_target_arch STREQUAL "arm64" AND
   NOT EXISTS "${_clip_msvc_145_bin}/armasm64.exe")
    message(FATAL_ERROR "Windows ARM64 requires ${_clip_msvc_145_bin}/armasm64.exe")
endif()
set(CMAKE_C_COMPILER "${_clip_msvc_145_bin}/cl.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_clip_msvc_145_bin}/cl.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_LINKER "${_clip_msvc_145_bin}/link.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_AR "${_clip_msvc_145_bin}/lib.exe" CACHE FILEPATH "" FORCE)
set(_clip_windows_sdk_root "$ENV{WindowsSdkDir}")
string(REGEX REPLACE "[/\\]+$" "" _clip_windows_sdk_root "${_clip_windows_sdk_root}")
set(_clip_windows_sdk_version "$ENV{WindowsSDKVersion}")
string(REGEX REPLACE "[/\\]+$" "" _clip_windows_sdk_version "${_clip_windows_sdk_version}")
if(_clip_windows_sdk_root STREQUAL "")
    set(_clip_windows_kits_root "$ENV{SystemDrive}/Program Files (x86)/Windows Kits/10")
    file(GLOB _clip_windows_sdk_includes "${_clip_windows_kits_root}/Include/*")
    list(SORT _clip_windows_sdk_includes COMPARE NATURAL ORDER DESCENDING)
    list(LENGTH _clip_windows_sdk_includes _clip_sdk_count)
    if(_clip_sdk_count GREATER 0)
        list(GET _clip_windows_sdk_includes 0 _clip_windows_sdk_include)
        get_filename_component(_clip_windows_sdk_version "${_clip_windows_sdk_include}" NAME)
        set(_clip_windows_sdk_root "${_clip_windows_kits_root}")
    endif()
else()
    set(_clip_windows_sdk_include "${_clip_windows_sdk_root}/Include/${_clip_windows_sdk_version}")
endif()
if(_clip_windows_sdk_root STREQUAL "" OR NOT IS_DIRECTORY "${_clip_windows_sdk_include}")
    message(FATAL_ERROR "Windows 10 SDK was not found. Set WindowsSdkDir/WindowsSDKVersion or install a Windows SDK.")
endif()
set(_clip_sdk_tool_arch "${_clip_msvc_host_arch}")
if(_clip_sdk_tool_arch STREQUAL "arm64" AND
   NOT EXISTS "${_clip_windows_sdk_root}/bin/${_clip_windows_sdk_version}/arm64/rc.exe")
    # Windows ARM64 can run the SDK's x64 host utilities under emulation.
    set(_clip_sdk_tool_arch x64)
endif()
set(CMAKE_MT "${_clip_windows_sdk_root}/bin/${_clip_windows_sdk_version}/${_clip_sdk_tool_arch}/mt.exe"
    CACHE FILEPATH "" FORCE)
set(CMAKE_RC_COMPILER "${_clip_windows_sdk_root}/bin/${_clip_windows_sdk_version}/${_clip_sdk_tool_arch}/rc.exe"
    CACHE FILEPATH "" FORCE)
foreach(_clip_sdk_tool IN ITEMS CMAKE_MT CMAKE_RC_COMPILER)
    if(NOT EXISTS "${${_clip_sdk_tool}}")
        message(FATAL_ERROR "The selected Windows SDK host tool is missing: ${${_clip_sdk_tool}}")
    endif()
endforeach()
set(_clip_include_directories
    "${_clip_msvc_145_root}/include"
    "${_clip_windows_sdk_include}/ucrt"
    "${_clip_windows_sdk_include}/shared"
    "${_clip_windows_sdk_include}/um"
    "${_clip_windows_sdk_include}/winrt"
    "${_clip_windows_sdk_include}/cppwinrt")
set(_clip_library_directories
    "${_clip_msvc_145_root}/lib/${_clip_msvc_target_arch}"
    "${_clip_windows_sdk_root}/Lib/${_clip_windows_sdk_version}/um/${_clip_msvc_target_arch}"
    "${_clip_windows_sdk_root}/Lib/${_clip_windows_sdk_version}/ucrt/${_clip_msvc_target_arch}")
foreach(_clip_library_directory IN LISTS _clip_library_directories)
    if(NOT IS_DIRECTORY "${_clip_library_directory}")
        message(FATAL_ERROR "The selected Windows target library directory is missing: ${_clip_library_directory}")
    endif()
endforeach()
set(_clip_include_flags "")
set(_clip_rc_include_flags "")
set(_clip_linker_paths "")
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${_clip_include_directories}" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${_clip_include_directories}" CACHE STRING "" FORCE)
set(CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES "${_clip_include_directories}" CACHE STRING "" FORCE)
link_directories(${_clip_library_directories})

# x265 removes <FLAGS> from CMAKE_RC_COMPILE_OBJECT because it does not want
# C/C++ flags or target defines passed to its resource compiler. Keep the
# compiler and Windows SDK include paths in the rule itself so winresrc.h is
# still discoverable after that project-level customization.
set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <DEFINES> <INCLUDES> ${_clip_rc_include_flags} <FLAGS> /fo <OBJECT> <SOURCE>"
    CACHE STRING "Windows resource compiler rule" FORCE)

foreach(_clip_language IN ITEMS C CXX)
    string(FIND "${CMAKE_${_clip_language}_FLAGS}"
        "${_clip_msvc_145_root}/include" _clip_has_msvc_include)
    if(_clip_has_msvc_include EQUAL -1)
        set(CMAKE_${_clip_language}_FLAGS
            "${CMAKE_${_clip_language}_FLAGS} ${_clip_include_flags}"
            CACHE STRING "" FORCE)
    endif()
endforeach()

string(FIND "${CMAKE_RC_FLAGS}" "${_clip_msvc_145_root}/include"
    _clip_has_rc_msvc_include)
if(_clip_has_rc_msvc_include EQUAL -1)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} ${_clip_rc_include_flags}"
        CACHE STRING "" FORCE)
endif()

foreach(_clip_linker_kind IN ITEMS EXE SHARED MODULE)
    string(FIND "${CMAKE_${_clip_linker_kind}_LINKER_FLAGS}"
        "${_clip_msvc_145_root}/lib/${_clip_msvc_target_arch}" _clip_has_msvc_libpath)
    if(_clip_has_msvc_libpath EQUAL -1)
        set(CMAKE_${_clip_linker_kind}_LINKER_FLAGS
            "${CMAKE_${_clip_linker_kind}_LINKER_FLAGS} ${_clip_linker_paths}"
            CACHE STRING "" FORCE)
    endif()
endforeach()

unset(_clip_vcpkg_root)
unset(_clip_vcpkg_windows_toolchain)
unset(_clip_windows_sdk_root)
unset(_clip_windows_sdk_include)
unset(_clip_msvc_libpath)
unset(_clip_sdk_libpath)
unset(_clip_include_flags)
unset(_clip_rc_include_flags)
unset(_clip_linker_paths)
unset(_clip_language)
unset(_clip_has_msvc_include)
unset(_clip_has_rc_msvc_include)
unset(_clip_linker_kind)
unset(_clip_has_msvc_libpath)
unset(_clip_msvc_145_root)
unset(_clip_msvc_145_bin)
unset(_clip_vswhere)
unset(_clip_vs_install)
unset(_clip_msvc_candidates)
unset(_clip_msvc_count)
unset(_clip_windows_kits_root)
unset(_clip_windows_sdk_includes)
unset(_clip_windows_sdk_version)
unset(_clip_windows_sdk_include)
unset(_clip_sdk_count)
unset(_clip_msvc_host_arch)
unset(_clip_msvc_target_arch)
unset(_clip_msvc_component)
unset(_clip_msvc_tool)
unset(_clip_library_directory)
unset(_clip_sdk_tool_arch)
unset(_clip_sdk_tool)
