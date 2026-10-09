add_executable(clip_image_benchmark codec_benchmark.cpp)
target_link_libraries(clip_image_benchmark PRIVATE clip_image::clip_image)
target_compile_features(clip_image_benchmark PRIVATE cxx_std_20)

add_executable(clip_image_memory_performance_benchmark
    ../tests/memory_performance_benchmark.cpp)
target_link_libraries(clip_image_memory_performance_benchmark PRIVATE clip_image::clip_image)
target_compile_features(clip_image_memory_performance_benchmark PRIVATE cxx_std_20)
if(WIN32)
    target_link_libraries(clip_image_memory_performance_benchmark PRIVATE psapi)
endif()
if(NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
    target_link_libraries(clip_image_memory_performance_benchmark PRIVATE PNG::PNG)
endif()
