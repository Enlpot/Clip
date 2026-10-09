#pragma once

#include <stdint.h>
#include <stddef.h>

#define CLIP_ARROW_PATH_UTF8_CAPACITY 4096
#define CLIP_ARROWHEAD_PRIMITIVE_CAPACITY 16
#define CLIP_ARROWHEAD_PRIMITIVE_POINT_CAPACITY 8
#define CLIP_FONT_FAMILY_UTF8_CAPACITY 128

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClipRuntimeImpl ClipRuntimeImpl;
typedef ClipRuntimeImpl* ClipRuntime;

typedef struct ClipViewportImpl ClipViewportImpl;
typedef ClipViewportImpl* ClipViewport;
typedef struct ClipPatchHandleImpl ClipPatchHandleImpl;
typedef ClipPatchHandleImpl* ClipPatchHandle;
typedef struct ClipChangedViewportListImpl ClipChangedViewportListImpl;
typedef ClipChangedViewportListImpl* ClipChangedViewportList;

typedef struct ClipSerializedBytesImpl ClipSerializedBytesImpl;
typedef ClipSerializedBytesImpl* ClipSerializedBytes;
typedef struct ClipRuntimeConfig ClipRuntimeConfig;

typedef enum ClipError {
    CLIP_OK = 0,
    CLIP_ERROR_INVALID_ARGUMENT = 1,
    CLIP_ERROR_NOT_FOUND = 2,
    CLIP_ERROR_INVALID_STATE = 3,
    CLIP_ERROR_BUFFER_TOO_SMALL = 4,
    CLIP_ERROR_STALE_REVISION = 5,
    CLIP_ERROR_UNSUPPORTED = 6,
    CLIP_ERROR_INTERNAL = 7
} ClipError;

ClipError clip_runtime_serialize_document_session(ClipRuntime runtime, uint8_t* buffer,
                                                  size_t buffer_capacity, size_t* out_size);
// Serializes once into an independent snapshot owned by the returned handle.
// Release it with clip_serialized_bytes_destroy after copying/consuming its data.
ClipError clip_runtime_serialize_document_session_bytes(ClipRuntime runtime,
                                                        ClipSerializedBytes* out_bytes);
// The read-only data remains valid until the handle is destroyed, even if the
// originating runtime changes or is destroyed.
ClipError clip_serialized_bytes_data(ClipSerializedBytes bytes, const uint8_t** out_data,
                                     size_t* out_size);
void clip_serialized_bytes_destroy(ClipSerializedBytes bytes);
ClipError clip_runtime_serialize_selected_draw_template(ClipRuntime runtime, uint8_t* buffer,
                                                        size_t buffer_capacity, size_t* out_size);
ClipError clip_runtime_serialize_selected_element_ids(ClipRuntime runtime, uint8_t* buffer,
                                                      size_t buffer_capacity, size_t* out_size);
ClipError clip_runtime_create_from_document_session_with_config(const uint8_t* bytes, size_t size,
                                                                const ClipRuntimeConfig* config,
                                                                ClipRuntime* out_runtime);
ClipError clip_runtime_serialize_document_history(ClipRuntime runtime, uint8_t* buffer,
                                                  size_t buffer_capacity, size_t* out_size);
ClipError clip_runtime_create_from_document_history_with_config(const uint8_t* bytes, size_t size,
                                                                const ClipRuntimeConfig* config,
                                                                ClipRuntime* out_runtime);

typedef enum ClipZoomFocus {
    CLIP_ZOOM_FOCUS_POINTER = 0,
    CLIP_ZOOM_FOCUS_CENTER = 1
} ClipZoomFocus;

typedef struct ClipAutoFilterBounds {
    double left, top, right, bottom;
} ClipAutoFilterBounds;
typedef struct ClipAutoFilterRegion {
    uint64_t id;
    ClipAutoFilterBounds bounds;
    uint8_t category[128];
} ClipAutoFilterRegion;
typedef enum ClipActiveTool {
    CLIP_ACTIVE_TOOL_SELECT = 0,
    CLIP_ACTIVE_TOOL_SHAPE = 1,
    CLIP_ACTIVE_TOOL_ARROW = 2,
    CLIP_ACTIVE_TOOL_TEXT = 3,
    CLIP_ACTIVE_TOOL_SERIAL_NUMBER = 4,
    CLIP_ACTIVE_TOOL_LINE = 5,
    CLIP_ACTIVE_TOOL_FREE_DRAW = 6,
    CLIP_ACTIVE_TOOL_RECTANGLE_HIGHLIGHT = 7,
    CLIP_ACTIVE_TOOL_ERASER = 8,
    CLIP_ACTIVE_TOOL_RECTANGLE_FILTER = 9,
    CLIP_ACTIVE_TOOL_WATERMARK = 10,
    CLIP_ACTIVE_TOOL_PEN_HIGHLIGHT = 11,
    CLIP_ACTIVE_TOOL_PEN_FILTER = 12,
    CLIP_ACTIVE_TOOL_SPOTLIGHT = 13,
    CLIP_ACTIVE_TOOL_AUTO_FILTER = 14,
    CLIP_ACTIVE_TOOL_RECTANGLE_ERASER = 15,
    CLIP_ACTIVE_TOOL_BRUSH_ERASER = 16,
    CLIP_ACTIVE_TOOL_DISTANCE = 17
} ClipActiveTool;
#define CLIP_ACTIVE_TOOL_FILTER CLIP_ACTIVE_TOOL_RECTANGLE_FILTER
#define CLIP_ACTIVE_TOOL_HIGHLIGHT CLIP_ACTIVE_TOOL_RECTANGLE_HIGHLIGHT

typedef enum ClipStyleToolbarSource {
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE = 0,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE = 1,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_ARROW = 2,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_ARROW = 3,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_TEXT = 4,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_TEXT = 5,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_SERIAL_NUMBER = 6,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_SERIAL_NUMBER = 7,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_LINE = 8,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_LINE = 9,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_FREE_DRAW = 10,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_FREE_DRAW = 11,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_HIGHLIGHT = 12,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE_HIGHLIGHT = 13,
    CLIP_STYLE_TOOLBAR_SOURCE_ERASER = 14,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_FILTER = 15,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE_FILTER = 16,
    CLIP_STYLE_TOOLBAR_SOURCE_WATERMARK = 17,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_PEN_HIGHLIGHT = 18,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_PEN_HIGHLIGHT = 19,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_PEN_FILTER = 20,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_PEN_FILTER = 21,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_SPOTLIGHT = 22,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_SPOTLIGHT = 23,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_ERASER = 24,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_BRUSH_ERASER = 25,
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_DISTANCE = 26,
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_DISTANCE = 27
} ClipStyleToolbarSource;
#define CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_FILTER CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_FILTER
#define CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_FILTER                                                  \
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE_FILTER
#define CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_HIGHLIGHT                                                \
    CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_HIGHLIGHT
#define CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_HIGHLIGHT                                               \
    CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE_HIGHLIGHT

typedef enum ClipFilterType {
    CLIP_FILTER_TYPE_MOSAIC = 0,
    CLIP_FILTER_TYPE_GAUSSIAN_BLUR = 1,
    CLIP_FILTER_TYPE_GRAYSCALE = 2,
    CLIP_FILTER_TYPE_INVERSION = 3,
    CLIP_FILTER_TYPE_EMBOSS = 4,
    CLIP_FILTER_TYPE_SMART_ERASE = 5,
    CLIP_FILTER_TYPE_BRIGHTNESS = 6,
    CLIP_FILTER_TYPE_RESTORE_BACKGROUND = 7
} ClipFilterType;

typedef struct ClipBrushEraserStyle {
    double stroke_width;
} ClipBrushEraserStyle;

#define CLIP_BRUSH_ERASER_STYLE_PROPERTY_STROKE_WIDTH (1u << 0)

typedef struct ClipFilterStyle {
    ClipFilterType filter_type;
    double strength;
    double opacity;
    double stroke_width;
} ClipFilterStyle;

#define CLIP_TEXT_STYLE_MIXED_COLOR (1u << 0)
#define CLIP_TEXT_STYLE_MIXED_FONT_SIZE (1u << 1)
#define CLIP_TEXT_STYLE_MIXED_FONT_FAMILY (1u << 2)
#define CLIP_TEXT_STYLE_MIXED_FILL (1u << 3)
#define CLIP_TEXT_STYLE_MIXED_FILL_STYLE (1u << 4)
#define CLIP_TEXT_STYLE_MIXED_STROKE (1u << 5)
#define CLIP_TEXT_STYLE_MIXED_STROKE_WIDTH (1u << 6)
#define CLIP_TEXT_STYLE_MIXED_CORNER_RADII (1u << 7)
#define CLIP_TEXT_STYLE_MIXED_HORIZONTAL_ALIGN (1u << 8)
#define CLIP_TEXT_STYLE_MIXED_VERTICAL_ALIGN (1u << 9)
#define CLIP_TEXT_STYLE_MIXED_OPACITY (1u << 10)
#define CLIP_TEXT_STYLE_ALL_PROPERTIES ((1u << 11) - 1u)

#define CLIP_SERIAL_NUMBER_STYLE_MIXED_NUMBER (1u << 0)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_COLOR (1u << 1)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_FILL (1u << 2)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_FILL_STYLE (1u << 3)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_FONT_SIZE (1u << 4)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_FONT_FAMILY (1u << 5)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_STROKE_WIDTH (1u << 6)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_STROKE_STYLE (1u << 7)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_OPACITY (1u << 8)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_TYPE (1u << 9)
#define CLIP_SERIAL_NUMBER_STYLE_MIXED_NUMERIC_TYPE (1u << 10)

typedef enum ClipSerialNumberNumericType {
    CLIP_SERIAL_NUMBER_NUMERIC_TYPE_ARABIC = 0,
    CLIP_SERIAL_NUMBER_NUMERIC_TYPE_ROMAN = 1,
    CLIP_SERIAL_NUMBER_NUMERIC_TYPE_LOWERCASE_LETTERS = 2,
    CLIP_SERIAL_NUMBER_NUMERIC_TYPE_UPPERCASE_LETTERS = 3,
    CLIP_SERIAL_NUMBER_NUMERIC_TYPE_CHINESE = 4
} ClipSerialNumberNumericType;

typedef enum ClipSerialNumberType {
    CLIP_SERIAL_NUMBER_TYPE_OUTLINED_CIRCLE = 0,
    CLIP_SERIAL_NUMBER_TYPE_SOLID_CIRCLE = 1,
    CLIP_SERIAL_NUMBER_TYPE_OUTLINED_SQUARE = 2,
    CLIP_SERIAL_NUMBER_TYPE_SOLID_SQUARE = 3,
    CLIP_SERIAL_NUMBER_TYPE_CIRCLE = 4
} ClipSerialNumberType;

#ifdef __cplusplus
static_assert(sizeof(ClipSerialNumberType) == sizeof(uint32_t),
              "ClipSerialNumberType must remain a four-byte C ABI enum");
#endif

typedef enum ClipPointerEventType {
    CLIP_POINTER_EVENT_DOWN = 0,
    CLIP_POINTER_EVENT_MOVE = 1,
    CLIP_POINTER_EVENT_UP = 2,
    CLIP_POINTER_EVENT_CANCEL = 3,
    CLIP_POINTER_EVENT_ENTER = 4,
    CLIP_POINTER_EVENT_LEAVE = 5,
    CLIP_POINTER_EVENT_DOUBLE_CLICK = 6
} ClipPointerEventType;

typedef enum ClipPointerDevice {
    CLIP_POINTER_DEVICE_MOUSE = 0,
    CLIP_POINTER_DEVICE_TOUCH = 1,
    CLIP_POINTER_DEVICE_PEN = 2,
    CLIP_POINTER_DEVICE_UNKNOWN = 3
} ClipPointerDevice;

typedef enum ClipPointerButton {
    CLIP_POINTER_BUTTON_NONE = 0,
    CLIP_POINTER_BUTTON_PRIMARY = 1,
    CLIP_POINTER_BUTTON_SECONDARY = 2,
    CLIP_POINTER_BUTTON_MIDDLE = 3
} ClipPointerButton;

typedef enum ClipInputEventKind {
    CLIP_INPUT_EVENT_POINTER = 0,
    CLIP_INPUT_EVENT_WHEEL = 1,
    CLIP_INPUT_EVENT_KEY = 2,
    CLIP_INPUT_EVENT_FOCUS_LOST = 3
} ClipInputEventKind;

typedef enum ClipWheelDeltaKind {
    CLIP_WHEEL_DELTA_PIXEL = 0,
    CLIP_WHEEL_DELTA_ANGLE = 1
} ClipWheelDeltaKind;

typedef enum ClipKeyEventType { CLIP_KEY_EVENT_DOWN = 0, CLIP_KEY_EVENT_UP = 1 } ClipKeyEventType;

typedef enum ClipKeyCode {
    CLIP_KEY_CODE_UNKNOWN = 0,
    CLIP_KEY_CODE_SPACE = 1,
    CLIP_KEY_CODE_ESCAPE = 2,
    CLIP_KEY_CODE_ARROW_UP = 3,
    CLIP_KEY_CODE_ARROW_DOWN = 4,
    CLIP_KEY_CODE_ARROW_LEFT = 5,
    CLIP_KEY_CODE_ARROW_RIGHT = 6,
    CLIP_KEY_CODE_CHARACTER = 7,
    CLIP_KEY_CODE_BACKSPACE = 8,
    CLIP_KEY_CODE_DELETE = 9
} ClipKeyCode;

typedef enum ClipPointerCaptureCommandKind {
    CLIP_POINTER_CAPTURE_NO_CHANGE = 0,
    CLIP_POINTER_CAPTURE_CAPTURE = 1,
    CLIP_POINTER_CAPTURE_RELEASE = 2
} ClipPointerCaptureCommandKind;

typedef enum ClipCursorCommandKind {
    CLIP_CURSOR_NO_CHANGE = 0,
    CLIP_CURSOR_SET = 1
} ClipCursorCommandKind;

typedef enum ClipCursorStyle {
    CLIP_CURSOR_STYLE_DEFAULT = 0,
    CLIP_CURSOR_STYLE_CROSSHAIR = 1,
    CLIP_CURSOR_STYLE_GRAB = 2,
    CLIP_CURSOR_STYLE_GRABBING = 3,
    CLIP_CURSOR_STYLE_MOVE = 4,
    CLIP_CURSOR_STYLE_RESIZE_HORIZONTAL = 5,
    CLIP_CURSOR_STYLE_RESIZE_VERTICAL = 6,
    CLIP_CURSOR_STYLE_RESIZE_NWSE = 7,
    CLIP_CURSOR_STYLE_RESIZE_NESW = 8,
    CLIP_CURSOR_STYLE_NOT_ALLOWED = 9,
    CLIP_CURSOR_STYLE_TEXT = 10,
    CLIP_CURSOR_STYLE_CORNER_RADIUS = 11,
    CLIP_CURSOR_STYLE_HIDDEN = 12,
    CLIP_CURSOR_STYLE_STROKE = 13,
    CLIP_CURSOR_STYLE_ERASER = 14
} ClipCursorStyle;

typedef enum ClipSceneDisplayItemKind {
    CLIP_SCENE_DISPLAY_ITEM_UNKNOWN = 0,
    CLIP_SCENE_DISPLAY_ITEM_DRAW_RECT = 1,
    CLIP_SCENE_DISPLAY_ITEM_STROKE = 2,
    CLIP_SCENE_DISPLAY_ITEM_TEXT = 3,
    CLIP_SCENE_DISPLAY_ITEM_IMAGE = 4,
    CLIP_SCENE_DISPLAY_ITEM_ARROW = 5,
    CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER = 6,
    CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER_CONNECTOR = 7,
    CLIP_SCENE_DISPLAY_ITEM_FILTER = 8
} ClipSceneDisplayItemKind;

typedef enum ClipBlendMode {
    CLIP_BLEND_MODE_NORMAL = 0,
    CLIP_BLEND_MODE_MULTIPLY = 1
} ClipBlendMode;

typedef enum ClipOverlayDisplayItemKind {
    CLIP_OVERLAY_DISPLAY_ITEM_DRAW_RECT = 1,
    CLIP_OVERLAY_DISPLAY_ITEM_SNAP_GUIDE = 2,
    CLIP_OVERLAY_DISPLAY_ITEM_FOCUS_CONNECTION = 3,
    CLIP_OVERLAY_DISPLAY_ITEM_PEN_FILTER_CONTOUR = 4
} ClipOverlayDisplayItemKind;

typedef enum ClipArrowhead {
    CLIP_ARROWHEAD_NONE = 0,
    CLIP_ARROWHEAD_ARROW = 1,
    CLIP_ARROWHEAD_BAR = 2,
    CLIP_ARROWHEAD_DOT = 3,
    CLIP_ARROWHEAD_CIRCLE = 4,
    CLIP_ARROWHEAD_CIRCLE_OUTLINE = 5,
    CLIP_ARROWHEAD_TRIANGLE = 6,
    CLIP_ARROWHEAD_TRIANGLE_OUTLINE = 7,
    CLIP_ARROWHEAD_DIAMOND = 8,
    CLIP_ARROWHEAD_DIAMOND_OUTLINE = 9,
    CLIP_ARROWHEAD_CROWFOOT_ONE = 10,
    CLIP_ARROWHEAD_CROWFOOT_MANY = 11,
    CLIP_ARROWHEAD_CROWFOOT_ONE_OR_MANY = 12,
    CLIP_ARROWHEAD_SQUARE = 13,
    CLIP_ARROWHEAD_INVERTED_TRIANGLE = 14,
    CLIP_ARROWHEAD_INDENTED_TRIANGLE = 15
} ClipArrowhead;

typedef enum ClipArrowShaftType {
    CLIP_ARROW_SHAFT_TYPE_PLAIN = 0,
    CLIP_ARROW_SHAFT_TYPE_TAPERED = 1,
} ClipArrowShaftType;

typedef enum ClipArrowType {
    CLIP_ARROW_TYPE_STRAIGHT = 0,
    CLIP_ARROW_TYPE_CURVE = 1,
    CLIP_ARROW_TYPE_ELBOW = 2
} ClipArrowType;

typedef enum ClipArrowheadPrimitiveKind {
    CLIP_ARROWHEAD_PRIMITIVE_NONE = 0,
    CLIP_ARROWHEAD_PRIMITIVE_LINE = 1,
    CLIP_ARROWHEAD_PRIMITIVE_POLYGON = 2,
    CLIP_ARROWHEAD_PRIMITIVE_CIRCLE = 3
} ClipArrowheadPrimitiveKind;

typedef enum ClipArrowheadFillMode {
    CLIP_ARROWHEAD_FILL_STROKE = 0,
    CLIP_ARROWHEAD_FILL_BACKGROUND = 1
} ClipArrowheadFillMode;

typedef enum ClipArrowheadDashMode {
    CLIP_ARROWHEAD_DASH_INHERIT = 0,
    CLIP_ARROWHEAD_DASH_SOLID = 1,
    CLIP_ARROWHEAD_DASH_DOTTED_CAP = 2
} ClipArrowheadDashMode;

typedef enum ClipArrowPathCommandKind {
    CLIP_ARROW_PATH_COMMAND_NONE = 0,
    CLIP_ARROW_PATH_COMMAND_MOVE_TO = 1,
    CLIP_ARROW_PATH_COMMAND_LINE_TO = 2,
    CLIP_ARROW_PATH_COMMAND_QUAD_TO = 3,
    CLIP_ARROW_PATH_COMMAND_CUBIC_TO = 4
} ClipArrowPathCommandKind;

typedef enum ClipTextHorizontalAlign {
    CLIP_TEXT_HORIZONTAL_ALIGN_LEFT = 0,
    CLIP_TEXT_HORIZONTAL_ALIGN_CENTER = 1,
    CLIP_TEXT_HORIZONTAL_ALIGN_RIGHT = 2
} ClipTextHorizontalAlign;

typedef enum ClipTextVerticalAlign {
    CLIP_TEXT_VERTICAL_ALIGN_TOP = 0,
    CLIP_TEXT_VERTICAL_ALIGN_CENTER = 1,
    CLIP_TEXT_VERTICAL_ALIGN_BOTTOM = 2
} ClipTextVerticalAlign;

typedef enum ClipFillStyle {
    CLIP_FILL_STYLE_LINE = 0,
    CLIP_FILL_STYLE_CROSS_LINE = 1,
    CLIP_FILL_STYLE_SOLID = 2
} ClipFillStyle;

typedef enum ClipStrokeStyle {
    CLIP_STROKE_STYLE_SOLID = 0,
    CLIP_STROKE_STYLE_DASHED = 1,
    CLIP_STROKE_STYLE_DOTTED = 2
} ClipStrokeStyle;

typedef enum ClipDisplayRectShape {
    CLIP_DISPLAY_RECT_SHAPE_RECTANGLE = 0,
    CLIP_DISPLAY_RECT_SHAPE_ELLIPSE = 1,
    CLIP_DISPLAY_RECT_SHAPE_DIAMOND = 2
} ClipDisplayRectShape;

typedef enum ClipOverlayRectKind {
    CLIP_OVERLAY_RECT_UNSPECIFIED = 0,
    CLIP_OVERLAY_RECT_SELECTION_MARQUEE = 1,
    CLIP_OVERLAY_RECT_SELECTION_CANDIDATE_FRAME = 2,
    CLIP_OVERLAY_RECT_SELECTION_FRAME = 3,
    CLIP_OVERLAY_RECT_SELECTION_MULTI_FRAME = 4,
    CLIP_OVERLAY_RECT_SELECTION_RESIZE_HANDLE = 5,
    CLIP_OVERLAY_RECT_SELECTION_ROTATION_HANDLE = 6,
    CLIP_OVERLAY_RECT_SELECTION_CORNER_RADIUS_HANDLE = 7,
    CLIP_OVERLAY_RECT_ARROW_ENDPOINT_HANDLE = 8,
    CLIP_OVERLAY_RECT_ARROW_FOCUS_HANDLE = 9,
    CLIP_OVERLAY_RECT_ARROW_SEGMENT_HANDLE = 10,
    CLIP_OVERLAY_RECT_TEXT_ACTUAL_FRAME = 11,
    CLIP_OVERLAY_RECT_TEXT_HOVER_UNDERLINE = 12,
    CLIP_OVERLAY_RECT_ERASER_CURSOR = 13,
    CLIP_OVERLAY_RECT_BINDING_HIGHLIGHT = 14
} ClipOverlayRectKind;

typedef enum ClipSnapGuideKind {
    CLIP_SNAP_GUIDE_POINT = 0,
    CLIP_SNAP_GUIDE_GAP = 1
} ClipSnapGuideKind;

typedef enum ClipSnapGuideAxis {
    CLIP_SNAP_GUIDE_HORIZONTAL = 0,
    CLIP_SNAP_GUIDE_VERTICAL = 1
} ClipSnapGuideAxis;

typedef struct ClipColorRgba8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} ClipColorRgba8;

typedef struct ClipCornerRadii {
    double top_left;
    double top_right;
    double bottom_right;
    double bottom_left;
} ClipCornerRadii;

typedef struct ClipWatermarkTemplateApplicationTime {
    int32_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t reserved0[3];
} ClipWatermarkTemplateApplicationTime;

typedef struct ClipWatermarkConfig {
    ClipColorRgba8 color;
    uint32_t text_utf8_len;
    char text_utf8[256];
    double font_size;
    uint32_t font_family_utf8_len;
    char font_family_utf8[128];
    double angle;
    double gap;
    double opacity;
    uint32_t template_value_utf8_len;
    char template_value_utf8[1024];
    uint8_t has_template_application_time;
    uint8_t reserved0[3];
    ClipWatermarkTemplateApplicationTime template_application_time;
} ClipWatermarkConfig;

typedef struct ClipSpotlightConfig {
    ClipColorRgba8 color;
    double opacity;
} ClipSpotlightConfig;

typedef enum ClipHighlightShape {
    CLIP_HIGHLIGHT_SHAPE_RECTANGLE = 0,
    CLIP_HIGHLIGHT_SHAPE_ELLIPSE = 1
} ClipHighlightShape;

typedef enum ClipRectangleShape {
    CLIP_RECTANGLE_SHAPE_RECTANGLE = 0,
    CLIP_RECTANGLE_SHAPE_ELLIPSE = 1,
    CLIP_RECTANGLE_SHAPE_DIAMOND = 2
} ClipRectangleShape;

typedef struct ClipShapeStyle {
    ClipColorRgba8 fill;
    ClipColorRgba8 stroke;
    double stroke_width;
    ClipCornerRadii corner_radii;
    ClipArrowhead start_arrowhead;
    ClipArrowhead end_arrowhead;
    ClipStrokeStyle stroke_style;
    ClipArrowType arrow_type;
    ClipArrowShaftType arrow_shaft_type;
    double arrow_ratio;
    ClipFillStyle fill_style;
    double opacity;
    ClipHighlightShape highlight_shape;
    ClipRectangleShape shape;
} ClipShapeStyle;

typedef enum ClipShapeKind {
    CLIP_SHAPE_KIND_RECTANGLE = 0,
    CLIP_SHAPE_KIND_ARROW = 1,
    CLIP_SHAPE_KIND_LINE = 2,
    CLIP_SHAPE_KIND_FREE_DRAW = 3,
    CLIP_SHAPE_KIND_RECTANGLE_HIGHLIGHT = 4,
    CLIP_SHAPE_KIND_PEN_HIGHLIGHT = 5,
    CLIP_SHAPE_KIND_SPOTLIGHT = 6
} ClipShapeKind;
#define CLIP_SHAPE_KIND_HIGHLIGHT CLIP_SHAPE_KIND_RECTANGLE_HIGHLIGHT

#define CLIP_SHAPE_STYLE_PROPERTY_FILL (1u << 0)
#define CLIP_SHAPE_STYLE_PROPERTY_FILL_STYLE (1u << 1)
#define CLIP_SHAPE_STYLE_PROPERTY_STROKE (1u << 2)
#define CLIP_SHAPE_STYLE_PROPERTY_STROKE_WIDTH (1u << 3)
#define CLIP_SHAPE_STYLE_PROPERTY_CORNER_RADII (1u << 4)
#define CLIP_SHAPE_STYLE_PROPERTY_START_ARROWHEAD (1u << 5)
#define CLIP_SHAPE_STYLE_PROPERTY_END_ARROWHEAD (1u << 6)
#define CLIP_SHAPE_STYLE_PROPERTY_STROKE_STYLE (1u << 7)
#define CLIP_SHAPE_STYLE_PROPERTY_ARROW_RATIO (1u << 13)
#define CLIP_SHAPE_STYLE_PROPERTY_ARROW_SHAFT_TYPE (1u << 12)
#define CLIP_SHAPE_STYLE_PROPERTY_ARROW_TYPE (1u << 8)
#define CLIP_SHAPE_STYLE_PROPERTY_OPACITY (1u << 9)
#define CLIP_SHAPE_STYLE_PROPERTY_HIGHLIGHT_SHAPE (1u << 10)
#define CLIP_SHAPE_STYLE_PROPERTY_SHAPE (1u << 11)
#define CLIP_SHAPE_STYLE_PROPERTY_LINE                                                             \
    (CLIP_SHAPE_STYLE_PROPERTY_FILL | CLIP_SHAPE_STYLE_PROPERTY_FILL_STYLE |                       \
     CLIP_SHAPE_STYLE_PROPERTY_STROKE | CLIP_SHAPE_STYLE_PROPERTY_STROKE_WIDTH |                   \
     CLIP_SHAPE_STYLE_PROPERTY_ARROW_RATIO | CLIP_SHAPE_STYLE_PROPERTY_ARROW_SHAFT_TYPE |          \
     CLIP_SHAPE_STYLE_PROPERTY_STROKE_STYLE | CLIP_SHAPE_STYLE_PROPERTY_ARROW_TYPE |               \
     CLIP_SHAPE_STYLE_PROPERTY_OPACITY)
#define CLIP_SHAPE_STYLE_PROPERTY_FREE_DRAW                                                        \
    (CLIP_SHAPE_STYLE_PROPERTY_FILL | CLIP_SHAPE_STYLE_PROPERTY_FILL_STYLE |                       \
     CLIP_SHAPE_STYLE_PROPERTY_STROKE | CLIP_SHAPE_STYLE_PROPERTY_STROKE_WIDTH |                   \
     CLIP_SHAPE_STYLE_PROPERTY_STROKE_STYLE | CLIP_SHAPE_STYLE_PROPERTY_OPACITY)

typedef struct ClipRectangleShapeStyle {
    ClipColorRgba8 fill;
    ClipFillStyle fill_style;
    ClipColorRgba8 stroke;
    double stroke_width;
    ClipStrokeStyle stroke_style;
    ClipCornerRadii corner_radii;
} ClipRectangleShapeStyle;

typedef struct ClipArrowStyle {
    ClipColorRgba8 stroke;
    double stroke_width;
    ClipArrowhead start_arrowhead;
    ClipArrowhead end_arrowhead;
    ClipStrokeStyle stroke_style;
    ClipArrowType arrow_type;
    ClipArrowShaftType arrow_shaft_type;
    double arrow_ratio;
    uint8_t reserved0[4];
} ClipArrowStyle;

typedef struct ClipTextStyle {
    ClipColorRgba8 color;
    double font_size;
    ClipColorRgba8 fill;
    ClipFillStyle fill_style;
    ClipColorRgba8 stroke;
    double stroke_width;
    ClipCornerRadii corner_radii;
    ClipTextHorizontalAlign horizontal_align;
    ClipTextVerticalAlign vertical_align;
    double opacity;
    uint8_t reserved0[4];
    uint32_t font_family_utf8_len;
    uint8_t font_family_truncated;
    uint8_t reserved1[3];
    char font_family_utf8[CLIP_FONT_FAMILY_UTF8_CAPACITY];
} ClipTextStyle;

typedef struct ClipSerialNumberStyle {
    int64_t number;
    ClipColorRgba8 color;
    ClipColorRgba8 fill;
    ClipFillStyle fill_style;
    double font_size;
    double stroke_width;
    ClipStrokeStyle stroke_style;
    double opacity;
    ClipSerialNumberType serial_number_type;
    uint32_t font_family_utf8_len;
    uint8_t font_family_truncated;
    uint8_t reserved1[3];
    char font_family_utf8[CLIP_FONT_FAMILY_UTF8_CAPACITY];
    ClipSerialNumberNumericType numeric_type;
} ClipSerialNumberStyle;

typedef enum ClipDistanceUnit {
    CLIP_DISTANCE_UNIT_PX = 0,
    CLIP_DISTANCE_UNIT_CM = 1,
    CLIP_DISTANCE_UNIT_M = 2,
    CLIP_DISTANCE_UNIT_KM = 3,
    CLIP_DISTANCE_UNIT_MM = 4
} ClipDistanceUnit;

typedef struct ClipDistanceStyle {
    ClipColorRgba8 stroke;
    double stroke_width;
    double factor;
    ClipDistanceUnit unit;
    uint32_t decimal_places;
    double endpoint_scale;
    ClipArrowhead endpoint_style;
} ClipDistanceStyle;

#define CLIP_DISTANCE_STYLE_PROPERTY_STROKE (1u << 0)
#define CLIP_DISTANCE_STYLE_PROPERTY_STROKE_WIDTH (1u << 1)
#define CLIP_DISTANCE_STYLE_PROPERTY_FACTOR (1u << 2)
#define CLIP_DISTANCE_STYLE_PROPERTY_UNIT (1u << 3)
#define CLIP_DISTANCE_STYLE_PROPERTY_DECIMAL_PLACES (1u << 4)
#define CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_SCALE (1u << 5)
#define CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_STYLE (1u << 6)
#define CLIP_DISTANCE_STYLE_PROPERTY_ALL ((1u << 7) - 1u)
#define CLIP_DISTANCE_STYLE_MIXED_STROKE CLIP_DISTANCE_STYLE_PROPERTY_STROKE
#define CLIP_DISTANCE_STYLE_MIXED_STROKE_WIDTH CLIP_DISTANCE_STYLE_PROPERTY_STROKE_WIDTH
#define CLIP_DISTANCE_STYLE_MIXED_FACTOR CLIP_DISTANCE_STYLE_PROPERTY_FACTOR
#define CLIP_DISTANCE_STYLE_MIXED_UNIT CLIP_DISTANCE_STYLE_PROPERTY_UNIT
#define CLIP_DISTANCE_STYLE_MIXED_DECIMAL_PLACES CLIP_DISTANCE_STYLE_PROPERTY_DECIMAL_PLACES
#define CLIP_DISTANCE_STYLE_MIXED_ENDPOINT_SCALE CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_SCALE
#define CLIP_DISTANCE_STYLE_MIXED_ENDPOINT_STYLE CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_STYLE

typedef struct ClipStyleToolbarState {
    ClipStyleToolbarSource source;
    uint32_t selected_element_count;
    ClipShapeStyle shape_style;
    ClipTextStyle text_style;
    ClipSerialNumberStyle serial_number_style;
    uint32_t text_style_mixed;
    uint32_t serial_number_style_mixed;
    uint32_t shape_style_mixed;
    ClipFilterStyle filter_style;
    uint32_t filter_style_mixed;
    ClipBrushEraserStyle brush_eraser_style;
    ClipDistanceStyle distance_style;
    uint32_t distance_style_mixed;
} ClipStyleToolbarState;

typedef struct ClipStyleDefaults {
    ClipShapeStyle rectangle;
    ClipShapeStyle arrow;
    ClipShapeStyle line;
    ClipShapeStyle free_draw;
    ClipShapeStyle rectangle_highlight;
    ClipShapeStyle pen_highlight;
    ClipFilterStyle rectangle_filter;
    ClipFilterStyle pen_filter;
    ClipTextStyle text;
    ClipSerialNumberStyle serial_number;
    ClipWatermarkConfig watermark;
    ClipSpotlightConfig spotlight;
    ClipBrushEraserStyle brush_eraser;
    ClipRectangleShape spotlight_shape;
    ClipDistanceStyle distance;
} ClipStyleDefaults;

struct ClipRuntimeConfig {
    const ClipStyleDefaults* style_defaults;
};

typedef struct ClipSerialNumberToolbarState {
    uint8_t visible;
    uint8_t can_decrease;
    uint8_t can_increase;
    uint8_t can_create_text;
    uint8_t reserved0[4];
    double left;
    double top;
    double width;
    double height;
} ClipSerialNumberToolbarState;

typedef struct ClipHistoryState {
    uint8_t can_undo;
    uint8_t can_redo;
    uint8_t reserved0[6];
} ClipHistoryState;

typedef struct ClipModifiers {
    uint8_t ctrl;
    uint8_t shift;
    uint8_t alt;
    uint8_t meta;
} ClipModifiers;

typedef struct ClipSnapConfig {
    uint8_t enabled;
    uint8_t enable_point_snaps;
    uint8_t enable_gap_snaps;
    uint8_t show_guides;
    uint8_t show_gap_size;
    uint8_t reserved0[3];
    double distance;
    ClipColorRgba8 line_color;
    double line_width;
    double marker_size;
    double gap_dash_length;
    double gap_dash_gap;
} ClipSnapConfig;

typedef struct ClipGridConfig {
    uint8_t enabled;
    uint8_t reserved0[7];
    double size;
} ClipGridConfig;

typedef struct ClipEngineConfig {
    double min_zoom;
    double max_zoom;
    ClipZoomFocus zoom_focus;
    double wheel_zoom_sensitivity;
    ClipColorRgba8 clear_color;
    ClipSnapConfig snap;
    ClipGridConfig grid;
    uint8_t enable_pointer_capture;
    uint8_t reserved[7];
} ClipEngineConfig;

#define CLIP_TEXT_UTF8_CAPACITY 1024

typedef struct ClipElementId {
    uint32_t index;
    uint32_t generation;
} ClipElementId;

typedef struct ClipTextLayoutSize {
    /* Exact host-renderer measured text width. */
    double width;
    /* Exact host-renderer measured text height. */
    double height;
    /* Measured painted ink width (widest line); 0 means "not measured" and
       consumers fall back to width. */
    double content_width;
    /* Measured painted ink height; 0 means "not measured" and consumers fall
       back to height. */
    double content_height;
} ClipTextLayoutSize;

typedef struct ClipTextLayoutOverride {
    /* Text element that this exact host-renderer layout applies to. */
    ClipElementId id;
    ClipTextLayoutSize size;
} ClipTextLayoutOverride;

typedef struct ClipTextCommitDraft {
    /* Existing text element to edit when has_existing_element is nonzero. Ignored for new text. */
    ClipElementId element_id;
    /* Owner of a new arrow label; generation zero means standalone text. */
    ClipElementId arrow_id;
    /* Nonzero edits element_id; zero creates a new text element. */
    uint8_t has_existing_element;
    /* Nonzero keeps auto-resize enabled after commit. */
    uint8_t auto_resize;
    /* Nonzero updates the editor default text style from style when text is non-empty. */
    uint8_t update_default_style;
    uint8_t reserved0[5];
    /* Final host-measured text center in canvas coordinates. */
    double center_x;
    double center_y;
    /* UTF-8 draft contents. May be null only when text_utf8_len is zero. */
    const char* text_utf8;
    uint32_t text_utf8_len;
    uint32_t reserved1;
    /* Exact host-renderer layout for the committed text. */
    ClipTextLayoutSize measured_layout;
    /* Full draft style to persist for the committed text. */
    ClipTextStyle style;
} ClipTextCommitDraft;

typedef struct ClipActiveTextDraftPresentation {
    ClipElementId element_id;
    ClipElementId arrow_id;
    uint8_t has_existing_element;
    uint8_t auto_resize;
    uint8_t reserved0[6];
    double center_x;
    double center_y;
    double width;
    double height;
    double rotation;
    /* Painted ink box of the draft preview; 0 means "not measured". */
    double content_width;
    double content_height;
    const char* text_utf8;
    uint32_t text_utf8_len;
    uint32_t reserved1;
    ClipTextStyle style;
} ClipActiveTextDraftPresentation;

typedef struct ClipTextElementInfo {
    ClipElementId id;
    ClipElementId arrow_id;
    double arrow_width;
    double center_x;
    double center_y;
    double width;
    double height;
    double content_width;
    double content_height;
    double rotation;
    double font_size;
    uint32_t text_utf8_len;
    uint8_t text_truncated;
    uint8_t auto_resize;
    /* Nonzero asks the host to measure natural unwrapped width; zero asks for
       wrapped height using width. */
    uint8_t measure_natural_width;
    uint8_t reserved0[1];
    char text_utf8[CLIP_TEXT_UTF8_CAPACITY];
    uint32_t font_family_utf8_len;
    uint8_t font_family_truncated;
    uint8_t reserved1[3];
    char font_family_utf8[CLIP_FONT_FAMILY_UTF8_CAPACITY];
} ClipTextElementInfo;

typedef struct ClipArrowTextLayoutRequest {
    ClipTextElementInfo info;
    ClipTextStyle style;
    uint64_t key;
    double max_width;
} ClipArrowTextLayoutRequest;

typedef struct ClipArrowTextLayoutResult {
    ClipElementId text_id;
    uint64_t key;
    ClipTextLayoutSize size;
} ClipArrowTextLayoutResult;

typedef struct ClipArrowTextLayoutMetrics {
    ClipElementId text_id;
    uint64_t key;
    ClipTextLayoutSize size;
    /* Natural layout width; size.width must be min(natural_width, max_width).
       Zero means
     * unknown and restricts reuse to the original constraint. */
    double natural_width;
} ClipArrowTextLayoutMetrics;

/* Call before requesting replacement layouts after host font or DPI changes. */
ClipError clip_viewport_invalidate_arrow_text_layouts(ClipRuntime runtime, ClipViewport viewport);

/* Pending host measurement for the empty label attached by an active serial
   number drag; measure the empty draft for this font and apply the result. */
typedef struct ClipSerialLabelLayoutRequest {
    ClipElementId text_id;
    double font_size;
    uint32_t font_family_utf8_len;
    uint8_t font_family_truncated;
    uint8_t reserved0[3];
    char font_family_utf8[CLIP_FONT_FAMILY_UTF8_CAPACITY];
} ClipSerialLabelLayoutRequest;

uint32_t clip_runtime_arrow_text_count(ClipRuntime runtime);
ClipError clip_viewport_get_arrow_text_layout_requests(ClipRuntime runtime, ClipViewport viewport,
                                                       ClipArrowTextLayoutRequest* out_items,
                                                       uint32_t capacity, uint32_t* out_count);
ClipError clip_viewport_apply_arrow_text_layouts_ex(ClipRuntime runtime, ClipViewport viewport,
                                                    const ClipArrowTextLayoutResult* layouts,
                                                    uint32_t count,
                                                    ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_apply_arrow_text_layout_metrics_ex(
    ClipRuntime runtime, ClipViewport viewport, const ClipArrowTextLayoutMetrics* layouts,
    uint32_t count, ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_get_serial_label_layout_request(ClipRuntime runtime, ClipViewport viewport,
                                                        ClipSerialLabelLayoutRequest* out_request,
                                                        uint8_t* out_has_request);
ClipError
clip_viewport_apply_serial_label_layout_ex(ClipRuntime runtime, ClipViewport viewport,
                                           ClipElementId text_id, ClipTextLayoutSize layout,
                                           ClipChangedViewportList* out_changed_viewports);

typedef struct ClipPointerEvent {
    uint32_t pointer_id;
    ClipPointerEventType event_type;
    ClipPointerDevice device;
    double position_x;
    double position_y;
    ClipPointerButton button;
    uint8_t buttons;
    uint8_t reserved0[3];
    ClipModifiers modifiers;
} ClipPointerEvent;

typedef struct ClipWheelEvent {
    double position_x;
    double position_y;
    double delta_x;
    double delta_y;
    ClipWheelDeltaKind delta_kind;
    ClipModifiers modifiers;
} ClipWheelEvent;

typedef struct ClipKeyEvent {
    ClipKeyEventType event_type;
    ClipKeyCode key_code;
    uint32_t codepoint;
    ClipModifiers modifiers;
    uint8_t repeat;
    uint8_t reserved0[3];
} ClipKeyEvent;

typedef struct ClipInputEvent {
    ClipInputEventKind kind;
    ClipPointerEvent pointer;
    ClipWheelEvent wheel;
    ClipKeyEvent key;
} ClipInputEvent;

typedef struct ClipInteractionOutput {
    uint8_t consumed;
    uint8_t reserved0[3];
    ClipPointerCaptureCommandKind capture_kind;
    uint32_t capture_pointer_id;
    ClipCursorCommandKind cursor_kind;
    ClipCursorStyle cursor_style;
} ClipInteractionOutput;

typedef struct ClipPatchCursor {
    uint64_t scene_revision;
    uint64_t decoration_revision;
    uint64_t overlay_revision;
} ClipPatchCursor;

typedef struct ClipPatchOp {
    uint32_t start;
    uint32_t delete_count;
    uint32_t insert_offset;
    uint32_t insert_count;
} ClipPatchOp;

typedef struct ClipPenFilterGeometryPatch {
    ClipElementId element_id;
    uint64_t expected_geometry_revision;
    uint64_t resulting_geometry_revision;
    uint32_t retain_prefix_count;
    uint32_t append_offset;
    uint32_t append_count;
    double old_changed_min_x;
    double old_changed_min_y;
    double old_changed_max_x;
    double old_changed_max_y;
    double new_changed_min_x;
    double new_changed_min_y;
    double new_changed_max_x;
    double new_changed_max_y;
    uint8_t full_reset;
    uint8_t element_removed;
    uint8_t reserved0[6];
} ClipPenFilterGeometryPatch;

typedef struct ClipPathGeometryPatch {
    ClipElementId element_id;
    uint64_t expected_geometry_revision;
    uint64_t resulting_geometry_revision;
    uint32_t range_offset;
    uint32_t range_count;
    double old_changed_min_x;
    double old_changed_min_y;
    double old_changed_max_x;
    double old_changed_max_y;
    double new_changed_min_x;
    double new_changed_min_y;
    double new_changed_max_x;
    double new_changed_max_y;
    uint8_t closed;
    uint8_t full_reset;
    uint8_t element_removed;
    uint8_t reserved0[5];
} ClipPathGeometryPatch;

typedef struct ClipPathChunkRange {
    uint32_t start;
    uint32_t delete_count;
    uint32_t insert_chunk_offset;
    uint32_t insert_chunk_count;
} ClipPathChunkRange;

typedef struct ClipPathChunk {
    uint64_t stable_id;
    uint32_t command_start;
    uint32_t command_offset;
    uint32_t command_count;
    uint32_t reserved0;
    double start_x;
    double start_y;
    double min_x;
    double min_y;
    double max_x;
    double max_y;
    double cumulative_start_length;
} ClipPathChunk;

#define CLIP_WATERMARK_TEXT_CAPACITY 256
#define CLIP_WATERMARK_FONT_FAMILY_CAPACITY 128

typedef struct ClipPatchInfo {
    uint64_t scene_base_revision;
    uint64_t scene_revision;
    uint64_t decoration_base_revision;
    uint64_t decoration_revision;
    uint64_t overlay_base_revision;
    uint64_t overlay_revision;
    uint8_t scene_reset;
    uint8_t decoration_reset;
    uint8_t overlay_reset;
    uint8_t reserved0[5];
    uint32_t scene_op_count;
    uint32_t overlay_op_count;
    uint32_t spotlight_op_count;
    uint32_t scene_item_count;
    uint32_t overlay_item_count;
    uint32_t spotlight_item_count;
    uint32_t scene_dirty_rect_count;
    uint32_t decoration_dirty_rect_count;
    uint32_t overlay_dirty_rect_count;
    uint32_t surface_width;
    uint32_t surface_height;
    double camera_center_x;
    double camera_center_y;
    double camera_zoom;
    ClipColorRgba8 clear_color;
    ClipColorRgba8 watermark_color;
    uint16_t watermark_text_len;
    uint16_t watermark_font_family_len;
    uint8_t watermark_text[CLIP_WATERMARK_TEXT_CAPACITY];
    double watermark_font_size;
    uint8_t watermark_font_family[CLIP_WATERMARK_FONT_FAMILY_CAPACITY];
    double watermark_angle;
    double watermark_gap;
    double watermark_opacity;
    ClipColorRgba8 spotlight_color;
    double spotlight_opacity;
    uint8_t spotlight_active;
    uint8_t reserved1[7];
} ClipPatchInfo;

typedef struct ClipDirtyRect {
    double min_x;
    double min_y;
    double max_x;
    double max_y;
} ClipDirtyRect;

typedef struct ClipSpotlightCutout {
    double center_x;
    double center_y;
    double width;
    double height;
    double rotation;
    uint8_t shape;
    uint8_t reserved[7];
} ClipSpotlightCutout;

#define CLIP_ARROW_POINT_CAPACITY 64

typedef struct ClipArrowPoint {
    double x;
    double y;
} ClipArrowPoint;

typedef struct ClipArrowheadPrimitive {
    ClipArrowheadPrimitiveKind kind;
    ClipArrowheadFillMode fill_mode;
    ClipArrowheadDashMode dash_mode;
    uint32_t point_count;
    ClipArrowPoint points[CLIP_ARROWHEAD_PRIMITIVE_POINT_CAPACITY];
    ClipArrowPoint center;
    double diameter;
} ClipArrowheadPrimitive;

typedef struct ClipArrowPathCommand {
    ClipArrowPathCommandKind kind;
    uint32_t reserved0;
    ClipArrowPoint point;
    ClipArrowPoint control1;
    ClipArrowPoint control2;
} ClipArrowPathCommand;

/*
 * Compact borrowed display view. Pointer fields are valid only while the
 * ClipPatchHandle that returned this item remains alive. A pointer may be
 * null only when its corresponding count or UTF-8 length is zero.
 */
/* Filter runs use stable uncropped identities; start/count index the resulting scene.
   Replacement flag 0 retains the plan; 1 replaces it (including an empty plan). */
typedef struct ClipSceneRenderRun {
    ClipElementId source_pass;
    ClipElementId effect_run;
    uint32_t start;
    uint32_t count;
} ClipSceneRenderRun;

typedef struct ClipFilterRenderSpec {
    uint32_t filter_type;
    uint32_t render_phase;
    double strength;
    double mosaic_block_size;
    double blur_sigma;
    double sampling_radius;
} ClipFilterRenderSpec;

ClipFilterRenderSpec clip_filter_render_spec_resolve(uint32_t filter_type, double strength);

typedef struct ClipTextPaintOutset {
    double x;
    double y;
} ClipTextPaintOutset;

/* Conservative document-space ink bounds of a text item: the aligned content
   box expanded by the fill padding and the text stroke halo. Dirty regions and
   visibility culling must consume this instead of reconstructing the
   arithmetic. */
typedef struct ClipTextPaintBounds {
    double min_x;
    double min_y;
    double max_x;
    double max_y;
} ClipTextPaintBounds;

typedef struct ClipSerialTextConnection {
    double start_x;
    double start_y;
    double end_x;
    double end_y;
    double baseline_start_x;
    double baseline_start_y;
    double baseline_end_x;
    double baseline_end_y;
    uint8_t has_baseline;
    uint8_t reserved[7];
} ClipSerialTextConnection;

typedef struct ClipSceneDisplayItem {
    ClipSceneDisplayItemKind kind;
    ClipBlendMode blend_mode;
    ClipElementId element_id;
    double center_x;
    double center_y;
    double width;
    double height;
    double rotation;
    ClipColorRgba8 fill;
    ClipColorRgba8 stroke;
    ClipColorRgba8 text_color;
    double stroke_width;
    ClipCornerRadii corner_radii;
    uint32_t arrow_point_count;
    ClipArrowType arrow_type;
    ClipArrowShaftType arrow_shaft_type;
    double arrow_ratio;
    uint8_t is_free_draw;
    uint8_t reserved1[2];
    ClipArrowhead arrow_start_head;
    ClipArrowhead arrow_end_head;
    ClipStrokeStyle arrow_stroke_style;
    uint32_t bound_text_element_index;
    const ClipArrowPoint* arrow_points;
    uint32_t arrow_path_command_count;
    uint32_t arrowhead_primitive_count;
    const ClipArrowPathCommand* arrow_path_commands;
    const ClipArrowheadPrimitive* arrowhead_primitives;
    double font_size;
    double opacity;
    /* Text items: painted ink box (widest line x laid-out height), resolved to
       at least the item size. Serial connectors anchor to this box aligned
       inside the item rectangle, never to the wrap rectangle. */
    double content_width;
    double content_height;
    int64_t serial_number;
    uint32_t text_utf8_len;
    ClipTextHorizontalAlign text_horizontal_align;
    ClipTextVerticalAlign text_vertical_align;
    ClipFillStyle fill_style;
    ClipStrokeStyle stroke_style;
    uint8_t has_bound_text_element;
    uint8_t rect_shape;
    uint8_t serial_number_type;
    uint8_t reserved2[1];
    uint32_t bound_text_element_generation;
    /* Canvas-space label exclusion rectangle, used only by arrows. */
    double arrow_text_bounds[4];
    ClipFilterRenderSpec filter;
    const char* text_utf8;
    uint32_t font_family_utf8_len;
    const char* font_family_utf8;
} ClipSceneDisplayItem;

/* Conservative ink (fill padding plus text stroke halo) for dirty regions. */
ClipTextPaintOutset clip_scene_text_paint_outset(const ClipSceneDisplayItem* item);
/* Fill-pill padding only. The host text painter must consume this instead of
   measuring its own padding so the painted edge matches the dirty regions. */
ClipTextPaintOutset clip_scene_text_fill_outset(const ClipSceneDisplayItem* item);
/* Conservative document-space ink bounds (aligned content box + fill padding +
   stroke halo) for dirty regions and culling. */
ClipTextPaintBounds clip_scene_text_paint_bounds(const ClipSceneDisplayItem* item);
uint8_t clip_scene_resolve_serial_text_connection(const ClipSceneDisplayItem* serial,
                                                  const ClipSceneDisplayItem* text,
                                                  ClipSerialTextConnection* out_connection);

/* Pointer fields follow the same ClipPatchHandle lifetime as scene items. */
/* The visitor borrows each item for the duration of the call and must not reenter runtime. */
ClipError clip_runtime_visit_smart_erase(ClipRuntime runtime,
                                         void (*visitor)(void* context,
                                                         const ClipSceneDisplayItem* item),
                                         void* context);

typedef struct ClipOverlayDisplayItem {
    ClipOverlayDisplayItemKind kind;
    ClipOverlayRectKind rect_kind;
    ClipSnapGuideKind snap_guide_kind;
    ClipSnapGuideAxis snap_guide_axis;
    uint8_t snap_marker_count;
    uint8_t snap_has_label;
    uint16_t reserved0;
    double center_x;
    double center_y;
    double width;
    double height;
    double rotation;
    ClipColorRgba8 fill;
    ClipColorRgba8 stroke;
    double stroke_width;
    ClipCornerRadii corner_radii;
    uint32_t arrow_point_count;
    ClipArrowType arrow_type;
    ClipArrowShaftType arrow_shaft_type;
    double arrow_ratio;
    uint8_t reserved1[3];
    ClipArrowhead arrow_start_head;
    ClipArrowhead arrow_end_head;
    ClipStrokeStyle arrow_stroke_style;
    ClipFillStyle fill_style;
    const ClipArrowPoint* arrow_points;
    uint32_t arrow_path_command_count;
    uint32_t arrowhead_primitive_count;
    const ClipArrowPathCommand* arrow_path_commands;
    const ClipArrowheadPrimitive* arrowhead_primitives;
    double snap_start_x;
    double snap_start_y;
    double snap_end_x;
    double snap_end_y;
    double snap_marker0_x;
    double snap_marker0_y;
    double snap_marker1_x;
    double snap_marker1_y;
    double snap_label;
    ClipColorRgba8 snap_color;
    double snap_line_width;
    double snap_marker_size;
    double snap_gap_dash_length;
    double snap_gap_dash_gap;
} ClipOverlayDisplayItem;

ClipError clip_runtime_create(ClipRuntime* out_runtime);

ClipError clip_runtime_create_with_config(const ClipRuntimeConfig* config,
                                          ClipRuntime* out_runtime);

ClipError clip_runtime_style_defaults_default(ClipStyleDefaults* out_defaults);

ClipError clip_runtime_clone_document_session_with_config(ClipRuntime source,
                                                          const ClipRuntimeConfig* config,
                                                          ClipRuntime* out_runtime);

void clip_runtime_destroy(ClipRuntime runtime);

/* Nestable presentation scope: mutations and host text metrics remain queryable,
 * but patches and changed-viewport lists are published only at the outermost end.
 * Balance every successful begin, even after a failed mutation. This does not
 * group document history or roll back mutations. Destroy the returned list. */
ClipError clip_runtime_begin_presentation_update(ClipRuntime runtime);
ClipError clip_runtime_end_presentation_update_ex(ClipRuntime runtime,
                                                  ClipChangedViewportList* out_changed_viewports);

ClipError
clip_runtime_set_quick_selection_disabled_tools_ex(ClipRuntime runtime, uint64_t tools,
                                                   ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_create(ClipRuntime runtime, const ClipEngineConfig* config,
                               ClipViewport* out_viewport);

void clip_viewport_destroy(ClipRuntime runtime, ClipViewport viewport);

ClipError clip_viewport_set_surface_size(ClipRuntime runtime, ClipViewport viewport, uint32_t width,
                                         uint32_t height);

ClipError clip_viewport_set_camera(ClipRuntime runtime, ClipViewport viewport, double center_x,
                                   double center_y, double zoom);

ClipError clip_viewport_get_id(ClipViewport viewport, uint64_t* out_id);

ClipError clip_viewport_set_active_tool_ex(ClipRuntime runtime, ClipViewport viewport,
                                           ClipActiveTool tool,
                                           ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_get_active_tool(ClipRuntime runtime, ClipViewport viewport,
                                        ClipActiveTool* out_tool);

ClipError clip_viewport_get_snap_config(ClipRuntime runtime, ClipViewport viewport,
                                        ClipSnapConfig* out_config);

ClipError clip_viewport_set_snap_config_ex(ClipRuntime runtime, ClipViewport viewport,
                                           const ClipSnapConfig* config,
                                           ClipChangedViewportList* out_changed_viewports);

/* Transient canvas-coordinate targets; each axis accepts at most two positions. */
ClipError clip_viewport_set_snap_guide_targets_ex(ClipRuntime runtime, ClipViewport viewport,
                                                  const double* vertical_xs, size_t vertical_count,
                                                  const double* horizontal_ys,
                                                  size_t horizontal_count,
                                                  ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_get_grid_config(ClipRuntime runtime, ClipViewport viewport,
                                        ClipGridConfig* out_config);

ClipError clip_viewport_set_grid_config_ex(ClipRuntime runtime, ClipViewport viewport,
                                           const ClipGridConfig* config,
                                           ClipChangedViewportList* out_changed_viewports);

ClipError clip_runtime_get_auto_filter_regions(ClipRuntime runtime, uint64_t* generation,
                                               uint8_t* identified, ClipAutoFilterBounds* bounds,
                                               ClipAutoFilterRegion* regions, size_t capacity,
                                               size_t* count);
ClipError clip_viewport_set_auto_filter_regions(ClipRuntime runtime, ClipViewport viewport,
                                                const ClipAutoFilterBounds* bounds,
                                                const ClipAutoFilterRegion* regions, size_t count,
                                                ClipChangedViewportList* changed);
ClipError clip_viewport_fill_auto_filter_category(ClipRuntime runtime, ClipViewport viewport,
                                                  const uint8_t* category, size_t length,
                                                  ClipChangedViewportList* changed);

ClipError clip_runtime_get_history_state(ClipRuntime runtime, ClipHistoryState* out_state);

/* Apply one validated versioned annotation batch as one history entry. Output bytes are
   owned by
 * the caller and released with clip_annotation_result_destroy, even on empty result. */
ClipError clip_runtime_apply_annotation_json(ClipRuntime runtime, const uint8_t* bytes, size_t size,
                                             uint8_t** out_json, size_t* out_size,
                                             ClipChangedViewportList* out_changed);
void clip_annotation_result_destroy(uint8_t* bytes, size_t size);
uint64_t clip_runtime_document_revision(ClipRuntime runtime);
// Returns nonzero for invalid handles, elements (including hidden ones), or watermark content.
uint8_t clip_runtime_has_document_content(ClipRuntime runtime);

ClipError
clip_runtime_clear_document_preserving_viewports(ClipRuntime runtime,
                                                 ClipChangedViewportList* out_changed_viewports);

ClipError clip_runtime_restore_document_history_preserving_editor_styles(
    ClipRuntime runtime, const uint8_t* bytes, size_t size,
    ClipChangedViewportList* out_changed_viewports);

ClipError clip_runtime_undo_ex(ClipRuntime runtime, ClipChangedViewportList* out_changed_viewports);

ClipError clip_runtime_redo_ex(ClipRuntime runtime, ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_get_style_toolbar_state(ClipRuntime runtime, ClipViewport viewport,
                                                ClipStyleToolbarState* out_state);

ClipError clip_viewport_get_serial_number_toolbar_state(ClipRuntime runtime, ClipViewport viewport,
                                                        ClipSerialNumberToolbarState* out_state);

ClipError clip_viewport_set_distance_style_patch_ex(ClipRuntime runtime, ClipViewport viewport,
                                                    const ClipDistanceStyle* style,
                                                    uint32_t properties,
                                                    ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_set_distance_pixel_scale_ex(ClipRuntime runtime, ClipViewport viewport,
                                                    double scale_x, double scale_y,
                                                    ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_set_shape_style_patch_ex(ClipRuntime runtime, ClipViewport viewport,
                                                 const ClipShapeStyle* style, uint32_t properties,
                                                 ClipShapeKind kind,
                                                 ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_set_filter_style_ex(ClipRuntime runtime, ClipViewport viewport,
                                            const ClipFilterStyle* style, uint32_t properties,
                                            ClipChangedViewportList* out_changed_viewports);
/* Patches creation defaults without changing the tool, selection, or document.
 * tool must be
 * RectangleFilter or PenFilter. Strength remains shared by both families. */
ClipError clip_viewport_set_brush_eraser_creation_style_ex(
    ClipRuntime runtime, ClipViewport viewport, const ClipBrushEraserStyle* style,
    uint32_t properties, ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_set_filter_creation_style_ex(
    ClipRuntime runtime, ClipViewport viewport, const ClipFilterStyle* style, uint32_t properties,
    ClipActiveTool tool, ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_get_watermark_config(ClipRuntime runtime, ClipViewport viewport,
                                             ClipWatermarkConfig* out_config);
ClipError clip_viewport_set_watermark_config_ex(ClipRuntime runtime, ClipViewport viewport,
                                                const ClipWatermarkConfig* config,
                                                ClipChangedViewportList* out_changed_viewports);

ClipError
clip_viewport_set_rectangle_shape_style_ex(ClipRuntime runtime, ClipViewport viewport,
                                           const ClipRectangleShapeStyle* style,
                                           ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_set_text_style_ex(ClipRuntime runtime, ClipViewport viewport,
                                          const ClipTextStyle* style,
                                          const ClipTextLayoutOverride* layouts,
                                          uint32_t layout_count,
                                          ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_patch_text_style_ex(ClipRuntime runtime, ClipViewport viewport,
                                            const ClipTextStyle* style, uint32_t properties,
                                            const ClipTextLayoutOverride* layouts,
                                            uint32_t layout_count,
                                            ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_set_text_creation_style_ex(ClipRuntime runtime, ClipViewport viewport,
                                                   const ClipTextStyle* style, uint32_t properties,
                                                   ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_set_serial_number_style_patch_ex(
    ClipRuntime runtime, ClipViewport viewport, const ClipSerialNumberStyle* style,
    uint32_t properties, ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_set_serial_number_style_ex(ClipRuntime runtime, ClipViewport viewport,
                                                   const ClipSerialNumberStyle* style,
                                                   ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_create_text(ClipRuntime runtime, ClipViewport viewport, double center_x,
                                    double center_y, const char* text_utf8, uint32_t text_utf8_len,
                                    double measured_width, double measured_height);

ClipError clip_viewport_hit_text(ClipRuntime runtime, ClipViewport viewport, double canvas_x,
                                 double canvas_y, ClipElementId* out_id, uint8_t* out_hit);

// Body selection only. Uses the viewport zoom and active tool scope; secondary
// selection ignores the disabled-tools policy. Misses return out_hit = 0.
ClipError clip_viewport_hit_quick_selection(ClipRuntime runtime, ClipViewport viewport,
                                            double canvas_x, double canvas_y,
                                            ClipPointerButton button, ClipElementId* out_id,
                                            uint8_t* out_hit);

ClipError clip_viewport_is_element_selected(ClipRuntime runtime, ClipViewport viewport,
                                            ClipElementId id, uint8_t* out_selected);

ClipError clip_viewport_selected_text_count(ClipRuntime runtime, ClipViewport viewport,
                                            uint32_t* out_count);

ClipError clip_viewport_get_selected_text_elements(ClipRuntime runtime, ClipViewport viewport,
                                                   ClipTextElementInfo* out_items,
                                                   uint32_t capacity, uint32_t* out_count);

ClipError clip_viewport_get_active_text_resize_measurement(ClipRuntime runtime,
                                                           ClipViewport viewport,
                                                           ClipTextElementInfo* out_info,
                                                           uint8_t* out_active);

ClipError clip_viewport_apply_active_text_resize_measurement_ex(
    ClipRuntime runtime, ClipViewport viewport, const ClipTextLayoutSize* layout,
    ClipChangedViewportList* out_changed_viewports);

ClipError
clip_viewport_set_active_text_draft_presentation_ex(ClipRuntime runtime, ClipViewport viewport,
                                                    const ClipActiveTextDraftPresentation* draft,
                                                    ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_clear_active_text_draft_presentation_ex(
    ClipRuntime runtime, ClipViewport viewport, ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_get_active_text_draft_presentation(ClipRuntime runtime,
                                                           ClipViewport viewport,
                                                           ClipTextElementInfo* out_info,
                                                           ClipTextStyle* out_style,
                                                           uint8_t* out_active);

ClipError clip_viewport_is_text_bound_to_serial_number(ClipRuntime runtime, ClipViewport viewport,
                                                       ClipElementId id, uint8_t* out_bound);

/* Resolves a point hit or a single selected arrow without creating document content. */
ClipError clip_viewport_get_arrow_text_target(ClipRuntime runtime, ClipViewport viewport,
                                              uint8_t use_point, double x, double y,
                                              ClipTextElementInfo* out_info,
                                              ClipTextStyle* out_style, uint8_t* out_found);
ClipError clip_runtime_get_text_utf8(ClipRuntime runtime, ClipElementId element, uint8_t* buffer,
                                     uint32_t capacity, uint32_t* out_length);

ClipError clip_runtime_get_text_element(ClipRuntime runtime, ClipElementId id,
                                        ClipTextElementInfo* out_info);

/* Typed draft commit path. Persists text content, exact host layout, auto-resize
   state, and full ClipTextStyle from ClipTextCommitDraft. */
ClipError
clip_viewport_commit_text_draft_payload_ex(ClipRuntime runtime, ClipViewport viewport,
                                           const ClipTextCommitDraft* draft,
                                           ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_select_element_ex(ClipRuntime runtime, ClipViewport viewport,
                                          ClipElementId id,
                                          ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_reset_editing_state_ex(ClipRuntime runtime, ClipViewport viewport,
                                               ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_delete_selected_ex(ClipRuntime runtime, ClipViewport viewport,
                                           ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_delete_all_elements_ex(ClipRuntime runtime, ClipViewport viewport,
                                               ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_duplicate_selected_ex(ClipRuntime runtime, ClipViewport viewport,
                                              double offset_x, double offset_y,
                                              ClipChangedViewportList* out_changed_viewports);
ClipError clip_viewport_insert_draw_template_ex(ClipRuntime runtime, ClipViewport viewport,
                                                const uint8_t* bytes, size_t size, double center_x,
                                                double center_y,
                                                ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_reorder_selected_ex(ClipRuntime runtime, ClipViewport viewport,
                                            uint32_t action,
                                            ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_align_selected_ex(ClipRuntime runtime, ClipViewport viewport,
                                          uint32_t alignment,
                                          ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_set_selected_opacity_ex(ClipRuntime runtime, ClipViewport viewport,
                                                double opacity,
                                                ClipChangedViewportList* out_changed_viewports);

ClipError
clip_viewport_adjust_selected_serial_numbers_ex(ClipRuntime runtime, ClipViewport viewport,
                                                int64_t delta,
                                                ClipChangedViewportList* out_changed_viewports);

ClipError
clip_viewport_create_serial_number_text_ex(ClipRuntime runtime, ClipViewport viewport,
                                           double measured_width, double measured_height,
                                           ClipElementId* out_text_id, uint8_t* out_has_text_id,
                                           ClipChangedViewportList* out_changed_viewports);

ClipError clip_viewport_take_text_edit_request(ClipRuntime runtime, ClipViewport viewport,
                                               ClipElementId* out_text_id,
                                               uint8_t* out_has_text_id);

ClipError clip_viewport_take_new_text_draft_request(ClipRuntime runtime, ClipViewport viewport,
                                                    uint8_t* out_requested);

ClipError clip_viewport_process_input_ex(ClipRuntime runtime, ClipViewport viewport,
                                         const ClipInputEvent* event,
                                         ClipInteractionOutput* out_output,
                                         ClipChangedViewportList* out_changed_viewports);

/* Processes an ordered, non-empty batch of pointer-move events and refreshes
   viewport presentation once after the final event. */
ClipError clip_viewport_process_pointer_move_batch_ex(
    ClipRuntime runtime, ClipViewport viewport, const ClipInputEvent* events, uint32_t event_count,
    ClipInteractionOutput* out_output, ClipChangedViewportList* out_changed_viewports);

void clip_changed_viewports_destroy(ClipChangedViewportList changed_viewports);

uint32_t clip_changed_viewports_count(ClipChangedViewportList changed_viewports);

ClipError clip_changed_viewports_get(ClipChangedViewportList changed_viewports, uint32_t index,
                                     uint64_t* out_viewport_id);

ClipError clip_viewport_acquire_patch(ClipRuntime runtime, ClipViewport viewport,
                                      const ClipPatchCursor* cursor, ClipPatchHandle* out_patch);

void clip_patch_destroy(ClipPatchHandle patch);

ClipError clip_patch_get_info(ClipPatchHandle patch, ClipPatchInfo* out_info);

ClipError clip_patch_get_scene_ops(ClipPatchHandle patch, const ClipPatchOp** out_ops,
                                   uint32_t* out_count);

ClipError clip_patch_get_pen_filter_geometry_ops(ClipPatchHandle patch,
                                                 const ClipPenFilterGeometryPatch** out_ops,
                                                 uint32_t* out_count);

ClipError clip_patch_get_pen_filter_geometry_points(ClipPatchHandle patch,
                                                    const ClipArrowPoint** out_points,
                                                    uint32_t* out_count);

ClipError clip_patch_get_path_geometry_ops(ClipPatchHandle patch,
                                           const ClipPathGeometryPatch** out_ops,
                                           uint32_t* out_count);

ClipError clip_patch_get_path_geometry_ranges(ClipPatchHandle patch,
                                              const ClipPathChunkRange** out_ranges,
                                              uint32_t* out_count);

ClipError clip_patch_get_path_geometry_chunks(ClipPatchHandle patch,
                                              const ClipPathChunk** out_chunks,
                                              uint32_t* out_count);

ClipError clip_patch_get_path_geometry_commands(ClipPatchHandle patch,
                                                const ClipArrowPathCommand** out_commands,
                                                uint32_t* out_count);

ClipError clip_patch_get_overlay_ops(ClipPatchHandle patch, const ClipPatchOp** out_ops,
                                     uint32_t* out_count);

ClipError clip_patch_get_spotlight_ops(ClipPatchHandle patch, const ClipPatchOp** out_ops,
                                       uint32_t* out_count);

/* Returned items and all nested pointer fields borrow from `patch`. */
ClipError clip_patch_get_scene_render_plan(ClipPatchHandle patch, uint8_t* out_replace,
                                           const ClipSceneRenderRun** out_runs,
                                           uint32_t* out_count);

ClipError clip_patch_get_scene_items(ClipPatchHandle patch, const ClipSceneDisplayItem** out_items,
                                     uint32_t* out_count);

/* Returned items and all nested pointer fields borrow from `patch`. */
ClipError clip_patch_get_overlay_items(ClipPatchHandle patch,
                                       const ClipOverlayDisplayItem** out_items,
                                       uint32_t* out_count);

/* Returned cutouts borrow from `patch`. */
ClipError clip_patch_get_spotlight_cutouts(ClipPatchHandle patch,
                                           const ClipSpotlightCutout** out_cutouts,
                                           uint32_t* out_count);

ClipError clip_patch_get_scene_dirty_rects(ClipPatchHandle patch, const ClipDirtyRect** out_rects,
                                           uint32_t* out_count);

ClipError clip_patch_get_overlay_dirty_rects(ClipPatchHandle patch, const ClipDirtyRect** out_rects,
                                             uint32_t* out_count);
ClipError clip_viewport_get_spotlight_config(ClipRuntime runtime, ClipViewport viewport,
                                             ClipSpotlightConfig* out_config);
ClipError clip_viewport_set_spotlight_config_ex(ClipRuntime runtime, ClipViewport viewport,
                                                const ClipSpotlightConfig* config,
                                                ClipChangedViewportList* out_changed_viewports);

ClipError clip_patch_get_decoration_dirty_rects(ClipPatchHandle patch,
                                                const ClipDirtyRect** out_rects,
                                                uint32_t* out_count);

// Stateful shared freehand stabilization. Input batches contain only new points.
typedef struct ClipStrokeFilter ClipStrokeFilter;
ClipError clip_stroke_filter_create(ClipArrowPoint start, double sample_spacing,
                                    double response_distance, ClipStrokeFilter** output);
void clip_stroke_filter_free(ClipStrokeFilter* filter);
// Each batch is limited to 65536 input points and 65536 resampled points of work.
// Invalid batches leave filter state unchanged.
// Output borrows from filter until its next append/free; copy before another call.
// finish recovers the exact endpoint. An empty finish-only batch is valid.
ClipError clip_stroke_filter_append(ClipStrokeFilter* filter, const ClipArrowPoint* points,
                                    size_t count, uint8_t finish, const ClipArrowPoint** output,
                                    size_t* output_count);

// Stateless curve construction; null commands queries the required count.
ClipError clip_build_catmull_rom_path(const ClipArrowPoint* vertices, size_t vertex_count,
                                      uint8_t closed, ClipArrowPathCommand* commands,
                                      size_t capacity, size_t* count);

#ifdef __cplusplus
}
#endif
