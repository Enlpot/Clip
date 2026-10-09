if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "CLIP_IMAGE_BUILD_FUZZERS requires Clang with libFuzzer.")
endif()

add_executable(clip_image_decode_fuzzer decode_fuzzer.cpp)
target_link_libraries(clip_image_decode_fuzzer PRIVATE clip_image::clip_image)
target_compile_features(clip_image_decode_fuzzer PRIVATE cxx_std_20)
target_compile_options(clip_image_decode_fuzzer PRIVATE
    -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer
)
target_link_options(clip_image_decode_fuzzer PRIVATE
    -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer
)
