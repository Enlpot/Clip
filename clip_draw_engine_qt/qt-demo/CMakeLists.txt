qt_add_executable(clip-draw-engine-qt-demo
    main.cpp
    demo_window.cpp
    demo_window.h
)

target_link_libraries(clip-draw-engine-qt-demo PRIVATE
    clip_draw_engine_qt_canvas
)

set_target_properties(clip-draw-engine-qt-demo PROPERTIES
    WIN32_EXECUTABLE TRUE
)
