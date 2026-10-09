cmake_minimum_required(VERSION 3.25)

project(clip_image VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipPixelBuffer.cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CLIP_IMAGE_BUILD_TESTS "Build clip_image tests." ON)
option(CLIP_IMAGE_BUILD_SHARED "Build the shared clip_image library." ON)
option(CLIP_IMAGE_BUILD_STATIC "Build the static clip_image library." ON)
option(CLIP_IMAGE_STRICT_COMPILE "Treat clip_image warnings as errors." ON)
option(CLIP_IMAGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer." OFF)
option(CLIP_IMAGE_BUILD_FUZZERS "Build libFuzzer codec entry points." OFF)
option(CLIP_IMAGE_BUILD_BENCHMARKS "Build the command-line codec benchmark." OFF)
option(CLIP_IMAGE_ENABLE_INSTALL "Install Clip Image as a standalone package." ON)
option(CLIP_IMAGE_CODEC_PNG "Enable the libpng codec." OFF)
option(CLIP_IMAGE_CODEC_JPEG "Enable the libjpeg-turbo codec." OFF)
option(CLIP_IMAGE_CODEC_GIF "Enable the giflib codec." OFF)
option(CLIP_IMAGE_CODEC_ICON "Enable ICO and CUR container codecs." OFF)
option(CLIP_IMAGE_CODEC_XBITMAP "Enable XBM and XPM codecs." OFF)
option(CLIP_IMAGE_CODEC_SVG "Enable SVG and SVGZ codecs." OFF)
option(CLIP_IMAGE_CODEC_HEIF "Enable HEIF, HEIC, HIF, and AVIF codecs." OFF)
option(CLIP_IMAGE_CODEC_JXL "Enable JPEG XL codecs." OFF)
option(CLIP_IMAGE_CODEC_EXR "Enable OpenEXR codecs." OFF)
option(CLIP_IMAGE_CODEC_WEBP "Enable WebP codecs." OFF)
option(CLIP_IMAGE_LINK_PNG_DEPENDENCIES
    "Link the PNG dependency into the static Clip Image target."
    ON
)
option(CLIP_IMAGE_LINK_JPEG_DEPENDENCIES
    "Link JPEG dependencies into the static Clip Image target."
    ON
)
set(CLIP_IMAGE_PROFILE "full" CACHE STRING "Codec profile: full or redistributable.")
set_property(CACHE CLIP_IMAGE_PROFILE PROPERTY STRINGS full redistributable)
set(CLIP_IMAGE_DEFAULT_LINKAGE "static" CACHE STRING "Default clip_image target linkage.")
set_property(CACHE CLIP_IMAGE_DEFAULT_LINKAGE PROPERTY STRINGS static shared)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ProjectWarnings.cmake")
if(CLIP_IMAGE_STRICT_COMPILE)
    clip_enable_strict_warnings()
endif()
clip_enable_clang_tidy()

if(NOT CLIP_IMAGE_BUILD_STATIC AND NOT CLIP_IMAGE_BUILD_SHARED)
    message(FATAL_ERROR "At least one of CLIP_IMAGE_BUILD_STATIC or CLIP_IMAGE_BUILD_SHARED must be ON.")
endif()
if(NOT CLIP_IMAGE_PROFILE STREQUAL "full" AND
   NOT CLIP_IMAGE_PROFILE STREQUAL "redistributable")
    message(FATAL_ERROR "CLIP_IMAGE_PROFILE must be full or redistributable.")
endif()

set(CLIP_IMAGE_PUBLIC_HEADERS
    include/clip/image/codec.h
    include/clip/image/document.h
    include/clip/image/export.h
    include/clip/image/format.h
    include/clip/image/image.h
    include/clip/image/io.h
    include/clip/image/processing.h
    include/clip/image/raster.h
    include/clip/image/raster_conversion.h
    include/clip/image/result.h
    include/clip/image/resource_estimate.h
    include/clip/image/resource_plan.h
    include/clip/image/service.h
    include/clip/image/version.h
)

set(CLIP_IMAGE_SOURCES
    src/codec_registry.cpp
    src/codec_registry.h
    src/compression_backend.cpp
    src/codecs/bmp_codec.cpp
    src/codecs/bmp_codec.h
    src/codecs/pnm_codec.cpp
    src/codecs/pnm_codec.h
    src/format.cpp
    src/exif_orientation.cpp
    src/exif_orientation.h
    src/image.cpp
    src/io.cpp
    src/planar_raster_io.cpp
    src/planar_raster_io.h
    src/processing.cpp
    src/raster.cpp
    src/raster_buffer.cpp
    src/raster_conversion.cpp
    src/resource_estimate.cpp
    src/resource_plan.cpp
    src/service.cpp
)

set(CLIP_IMAGE_PRIVATE_LIBRARIES)
set(CLIP_IMAGE_PRIVATE_DEFINITIONS)
set(CLIP_IMAGE_PRIVATE_INCLUDE_DIRS)

set(CLIP_IMAGE_LIBYUV_AVAILABLE OFF)
find_package(libyuv CONFIG QUIET)
if(TARGET yuv)
    set(CLIP_IMAGE_LIBYUV_AVAILABLE ON)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES yuv)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_LIBYUV=1)
endif()

if(CLIP_IMAGE_ENABLE_SANITIZERS)
    add_library(clip_image_sanitizers INTERFACE)
    if(MSVC)
        target_compile_options(clip_image_sanitizers INTERFACE /fsanitize=address)
        target_link_options(clip_image_sanitizers INTERFACE /INFERASANLIBS)
    else()
        target_compile_options(clip_image_sanitizers INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer
        )
        target_link_options(clip_image_sanitizers INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer
        )
    endif()
endif()

if(CLIP_IMAGE_CODEC_PNG)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)
    include(CheckCXXSourceCompiles)
    set(_clip_image_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES ZLIB::ZLIB)
    check_cxx_source_compiles(
        "#include <zlib.h>\n#ifndef ZLIBNG_VERSION\n#error zlib-ng required\n#endif\nint main() { return 0; }"
        CLIP_IMAGE_HAS_ZLIB_NG_COMPAT)
    set(CMAKE_REQUIRED_LIBRARIES "${_clip_image_required_libraries}")
    unset(_clip_image_required_libraries)
    if(NOT CLIP_IMAGE_HAS_ZLIB_NG_COMPAT)
        message(FATAL_ERROR
            "PNG support requires zlib-ng compatibility mode. Configure through the repository vcpkg overlay.")
    endif()
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/png_codec.cpp src/codecs/png_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES PNG::PNG ZLIB::ZLIB)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_PNG=1)
endif()

if(CLIP_IMAGE_CODEC_JPEG)
    find_package(libjpeg-turbo 3.1 CONFIG REQUIRED)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/jpeg_codec.cpp src/codecs/jpeg_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES
        $<IF:$<TARGET_EXISTS:libjpeg-turbo::jpeg>,libjpeg-turbo::jpeg,libjpeg-turbo::jpeg-static>
        $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
    )
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_JPEG=1)
endif()

if(CLIP_IMAGE_CODEC_GIF)
    find_package(GIF REQUIRED)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/gif_codec.cpp src/codecs/gif_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES GIF::GIF)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_GIF=1)
endif()

if(CLIP_IMAGE_CODEC_ICON)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/icon_codec.cpp src/codecs/icon_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_ICON=1)
endif()

if(CLIP_IMAGE_CODEC_XBITMAP)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/xbitmap_codec.cpp src/codecs/xbitmap_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_XBITMAP=1)
endif()

if(CLIP_IMAGE_CODEC_SVG)
    find_package(lunasvg CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/svg_codec.cpp src/codecs/svg_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES lunasvg::lunasvg ZLIB::ZLIB)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_SVG=1)
endif()

if(CLIP_IMAGE_CODEC_HEIF)
    find_package(libheif CONFIG REQUIRED)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/heif_codec.cpp src/codecs/heif_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES heif)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_HEIF=1)
endif()

if(CLIP_IMAGE_CODEC_JXL)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(JXL QUIET IMPORTED_TARGET libjxl)
        pkg_check_modules(JXL_THREADS QUIET IMPORTED_TARGET libjxl_threads)
    endif()
    if(NOT WIN32 AND TARGET PkgConfig::JXL AND TARGET PkgConfig::JXL_THREADS)
        list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES PkgConfig::JXL PkgConfig::JXL_THREADS)
    else()
        find_package(hwy CONFIG REQUIRED)
        find_package(lcms2 CONFIG REQUIRED)
        find_package(unofficial-brotli CONFIG REQUIRED)
        find_path(CLIP_IMAGE_JXL_INCLUDE_DIR NAMES jxl/decode.h REQUIRED)
        find_library(CLIP_IMAGE_JXL_LIBRARY NAMES jxl REQUIRED)
        find_library(CLIP_IMAGE_JXL_CMS_LIBRARY NAMES jxl_cms REQUIRED)
        find_library(CLIP_IMAGE_JXL_THREADS_LIBRARY NAMES jxl_threads REQUIRED)
        if(NOT TARGET clip_image_dependencies::jxl_cms)
            add_library(clip_image_dependencies::jxl_cms UNKNOWN IMPORTED)
            set_target_properties(clip_image_dependencies::jxl_cms PROPERTIES
                IMPORTED_LOCATION "${CLIP_IMAGE_JXL_CMS_LIBRARY}"
                INTERFACE_COMPILE_DEFINITIONS JXL_CMS_STATIC_DEFINE
                INTERFACE_LINK_LIBRARIES lcms2::lcms2
            )
        endif()
        if(NOT TARGET clip_image_dependencies::jxl)
            add_library(clip_image_dependencies::jxl UNKNOWN IMPORTED)
            set_target_properties(clip_image_dependencies::jxl PROPERTIES
                IMPORTED_LOCATION "${CLIP_IMAGE_JXL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CLIP_IMAGE_JXL_INCLUDE_DIR}"
                INTERFACE_COMPILE_DEFINITIONS JXL_STATIC_DEFINE
                INTERFACE_LINK_LIBRARIES
                    "hwy::hwy;unofficial::brotli::brotlienc;unofficial::brotli::brotlidec;clip_image_dependencies::jxl_cms"
            )
        endif()
        if(NOT TARGET clip_image_dependencies::jxl_threads)
            add_library(clip_image_dependencies::jxl_threads UNKNOWN IMPORTED)
            set_target_properties(clip_image_dependencies::jxl_threads PROPERTIES
                IMPORTED_LOCATION "${CLIP_IMAGE_JXL_THREADS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CLIP_IMAGE_JXL_INCLUDE_DIR}"
                INTERFACE_COMPILE_DEFINITIONS JXL_THREADS_STATIC_DEFINE
            )
        endif()
        list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES
            clip_image_dependencies::jxl
            clip_image_dependencies::jxl_threads
        )
    endif()
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/jxl_codec.cpp src/codecs/jxl_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_JXL=1)
endif()

if(CLIP_IMAGE_CODEC_EXR)
    find_package(OpenEXR CONFIG REQUIRED)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/exr_codec.cpp src/codecs/exr_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES OpenEXR::OpenEXR)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_EXR=1)
endif()

if(CLIP_IMAGE_CODEC_WEBP)
    find_package(WebP CONFIG REQUIRED)
    list(APPEND CLIP_IMAGE_SOURCES src/codecs/webp_codec.cpp src/codecs/webp_codec.h)
    list(APPEND CLIP_IMAGE_PRIVATE_LIBRARIES WebP::webp WebP::webpdemux WebP::libwebpmux)
    list(APPEND CLIP_IMAGE_PRIVATE_DEFINITIONS CLIP_IMAGE_HAS_WEBP=1)
endif()

function(clip_image_configure_target target_name linkage)
    add_library(${target_name} ${linkage} ${CLIP_IMAGE_PUBLIC_HEADERS} ${CLIP_IMAGE_SOURCES})
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    target_link_libraries(${target_name} PRIVATE $<BUILD_INTERFACE:clip_pixel_buffer>)
    target_include_directories(${target_name}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CLIP_IMAGE_PRIVATE_INCLUDE_DIRS}
    )
    target_compile_definitions(${target_name}
        PRIVATE
            CLIP_IMAGE_BUILDING_LIBRARY=1
            ${CLIP_IMAGE_PRIVATE_DEFINITIONS}
            $<$<STREQUAL:${CLIP_IMAGE_PROFILE},full>:CLIP_IMAGE_FULL_PROFILE=1>
    )
    set(_clip_image_target_libraries ${CLIP_IMAGE_PRIVATE_LIBRARIES})
    if(linkage STREQUAL "STATIC" AND NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
        # A static Qt kit without system_png already supplies the bundled PNG
        # objects. Linking vcpkg's libpng as well creates duplicate png_*
        # definitions in the final executable. Keep ZLIB::ZLIB because SVG
        # gzip helpers require the zlib API directly.
        list(REMOVE_ITEM _clip_image_target_libraries PNG::PNG)
    endif()
    if(linkage STREQUAL "STATIC" AND NOT CLIP_IMAGE_LINK_JPEG_DEPENDENCIES)
        # The bundled Qt library and its headers must be selected together.
        # Compiling with vcpkg's libjpeg-62 headers and resolving those symbols
        # to Qt's libjpeg-80 fails at jpeg_create_compress (the structs differ).
        include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipQt.cmake")
        clip_find_qt(COMPONENTS Core BundledLibjpeg)
        find_path(_clip_image_qt_jpeg_include NAMES jpeglib.h
            PATHS ${Qt6Core_INCLUDE_DIRS} PATH_SUFFIXES QtJpeg
            NO_DEFAULT_PATH NO_CACHE REQUIRED)
        target_include_directories(${target_name} BEFORE PRIVATE
            "${_clip_image_qt_jpeg_include}")
        list(FILTER _clip_image_target_libraries EXCLUDE
            REGEX "TARGET_EXISTS:libjpeg-turbo::jpeg>")
        list(PREPEND _clip_image_target_libraries Qt6::BundledLibjpeg)
    endif()
    target_link_libraries(${target_name} PRIVATE ${_clip_image_target_libraries})
    unset(_clip_image_target_libraries)
    if(CLIP_IMAGE_ENABLE_SANITIZERS)
        target_link_libraries(${target_name} PUBLIC clip_image_sanitizers)
    endif()
    set_target_properties(${target_name} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if(linkage STREQUAL "SHARED")
        target_compile_definitions(${target_name} PUBLIC CLIP_IMAGE_SHARED=1)
    endif()
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /utf-8
            /permissive-
            /Zc:__cplusplus
            /EHsc
            /W4
        )
        if(linkage STREQUAL "SHARED")
            # The public C++20 API intentionally owns standard-library values. Consumers and
            # the DLL use the same MSVC runtime; cross-toolchain ABI is not promised.
            target_compile_options(${target_name} PRIVATE /wd4251)
        endif()
        if(CLIP_IMAGE_STRICT_COMPILE)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
        if(CLIP_IMAGE_STRICT_COMPILE)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

if(CLIP_IMAGE_BUILD_STATIC)
    clip_image_configure_target(clip_image_static STATIC)
    target_precompile_headers(clip_image_static PRIVATE
        <algorithm>
        <array>
        <cstdint>
        <memory>
        <optional>
        <span>
        <string>
        <vector>
    )
    clip_enable_unity_build(clip_image_static)
    # These translation units intentionally use the same private helper names
    # in separate anonymous namespaces. Keep them out of the unity source so
    # those private definitions do not collide when included into one TU.
    set_source_files_properties(
        src/raster.cpp
        src/raster_buffer.cpp
        src/codecs/heif_codec.cpp
        src/codecs/jxl_codec.cpp
        PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
    )
    add_library(clip_image::clip_image_static ALIAS clip_image_static)
endif()
if(CLIP_IMAGE_BUILD_SHARED)
    clip_image_configure_target(clip_image_shared SHARED)
    add_library(clip_image::clip_image_shared ALIAS clip_image_shared)
endif()

add_library(clip_image INTERFACE)
if(CLIP_IMAGE_DEFAULT_LINKAGE STREQUAL "shared")
    if(NOT TARGET clip_image_shared)
        message(FATAL_ERROR "CLIP_IMAGE_DEFAULT_LINKAGE=shared requires CLIP_IMAGE_BUILD_SHARED=ON.")
    endif()
    target_link_libraries(clip_image INTERFACE clip_image_shared)
else()
    if(NOT TARGET clip_image_static)
        message(FATAL_ERROR "CLIP_IMAGE_DEFAULT_LINKAGE=static requires CLIP_IMAGE_BUILD_STATIC=ON.")
    endif()
    target_link_libraries(clip_image INTERFACE clip_image_static)
endif()
add_library(clip_image::clip_image ALIAS clip_image)

if(CLIP_IMAGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CLIP_IMAGE_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()
if(CLIP_IMAGE_BUILD_BENCHMARKS)
    add_subdirectory(tools)
endif()

if(CLIP_IMAGE_ENABLE_INSTALL)
    set(CLIP_IMAGE_INSTALL_TARGETS clip_image)
    if(TARGET clip_image_sanitizers)
        list(APPEND CLIP_IMAGE_INSTALL_TARGETS clip_image_sanitizers)
    endif()
    if(TARGET clip_image_static)
        list(APPEND CLIP_IMAGE_INSTALL_TARGETS clip_image_static)
    endif()
    if(TARGET clip_image_shared)
        list(APPEND CLIP_IMAGE_INSTALL_TARGETS clip_image_shared)
    endif()

    install(TARGETS ${CLIP_IMAGE_INSTALL_TARGETS}
        EXPORT clip_imageTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT clip_imageTargets
        FILE clip_imageTargets.cmake
        NAMESPACE clip_image::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clip_image
    )

    configure_package_config_file(
        cmake/clip_imageConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/clip_imageConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clip_image
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/clip_imageConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/clip_imageConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/clip_imageConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clip_image
    )
    install(FILES LICENSE COPYRIGHT
        DESTINATION ${CMAKE_INSTALL_DATADIR}/clip-image/licenses
    )
endif()
