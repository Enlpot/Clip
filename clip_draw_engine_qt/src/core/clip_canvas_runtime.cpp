#include "clip_draw_engine_qt/clip_canvas_runtime.h"

#include "clip_canvas_export.h"
#include "clip_canvas_runtime_access.h"
#include "clip_canvas_runtime_session.h"
#include "clip_canvas_runtime_thread_affinity.h"
#include "clip_canvas_ffi_handles.h"
#include "clip_canvas_viewport.h"
#include "clip_canvas_type_conversions.h"

#include <memory>
#include <QJsonDocument>
#include <limits>
#include <cmath>

struct ClipCanvasRuntime::Impl {
    Impl(ClipCanvasRuntime& owner, const ClipCanvasRuntimeConfig& config);
    ~Impl();

    bool isOwnerThread() const;
    bool isValid() const;
    bool reset();
    bool cloneDocumentSessionFrom(const Impl& source);
    QByteArray serializeDocumentSession() const;
    QByteArray serializeSelectedDrawTemplate() const;
    bool restoreDocumentSession(const QByteArray& payload);
    QByteArray serializeDocumentHistory() const;
    bool restoreDocumentHistory(const QByteArray& payload);
    bool restoreDocumentHistoryPreservingEditorStyles(const QByteArray& payload);
    bool clearDocumentPreservingViewports();
    void clearRenderState();
    bool setQuickSelectionDisabledTools(const QSet<ClipCanvasTool>& tools);
    void destroyAsync();
    QImage renderToImage(const QRectF& virtualSelectionRect, const QSize& outputSize,
                         const QList<CanvasExportSource>& sources);

    clip_canvas_smart_erase::Coordinator& smartErase() {
        return session.smartErase();
    }
    ClipRuntime handle() const;
    void registerClient(clip_canvas_runtime::Client* client);
    void unregisterClient(clip_canvas_runtime::Client* client);
    void syncChangedViewports(ClipChangedViewportList changedViewports);
    void syncChangedViewportIds(const std::vector<std::uint64_t>& changedViewportIds);
    std::function<void()> documentChanged;
    quint64 observedRevision = 0;
    void observeDocument() {
        if (!documentChanged)
            return;
        const auto revision = clip_runtime_document_revision(handle());
        if (revision != observedRevision) {
            observedRevision = revision;
            documentChanged();
        }
    }

  private:
    bool hasThreadAccess(const char* operation) const;

    ClipCanvasRuntime& owner;
    clip_canvas_runtime::ThreadAffinity threadAffinity;
    clip_canvas_runtime::RuntimeSession session;
};

ClipCanvasRuntime::Impl::Impl(ClipCanvasRuntime& owner, const ClipCanvasRuntimeConfig& config)
    : owner(owner), session(config) {}

ClipCanvasRuntime::Impl::~Impl() {
    session.destroyForOwnerDestruction(
        owner, threadAffinity.hasDestructionAccess("~ClipCanvasRuntime")
                   ? clip_canvas_runtime::OwnerDestructionPolicy::DetachClients
                   : clip_canvas_runtime::OwnerDestructionPolicy::AbandonClientsAndDestroyAsync);
}

bool ClipCanvasRuntime::Impl::isOwnerThread() const {
    return threadAffinity.isOwnerThread();
}

bool ClipCanvasRuntime::Impl::isValid() const {
    if (!hasThreadAccess("isValid")) {
        return false;
    }
    return session.isValid();
}

bool ClipCanvasRuntime::Impl::reset() {
    if (!hasThreadAccess("reset")) {
        return false;
    }

    return session.reset();
}

bool ClipCanvasRuntime::Impl::cloneDocumentSessionFrom(const Impl& source) {
    if (!hasThreadAccess("cloneDocumentSessionFrom") ||
        !source.hasThreadAccess("cloneDocumentSessionFrom(source)")) {
        return false;
    }
    return session.cloneDocumentSessionFrom(source.session);
}

QByteArray ClipCanvasRuntime::Impl::serializeDocumentSession() const {
    if (!hasThreadAccess("serializeDocumentSession")) {
        return {};
    }
    return session.serializeDocumentSession();
}

QByteArray ClipCanvasRuntime::Impl::serializeSelectedDrawTemplate() const {
    if (!hasThreadAccess("serializeSelectedDrawTemplate")) {
        return {};
    }
    return session.serializeSelectedDrawTemplate();
}

bool ClipCanvasRuntime::Impl::restoreDocumentSession(const QByteArray& payload) {
    return hasThreadAccess("restoreDocumentSession") && session.restoreDocumentSession(payload);
}

QByteArray ClipCanvasRuntime::Impl::serializeDocumentHistory() const {
    if (!hasThreadAccess("serializeDocumentHistory")) {
        return {};
    }
    return session.serializeDocumentHistory();
}

bool ClipCanvasRuntime::Impl::restoreDocumentHistory(const QByteArray& payload) {
    return hasThreadAccess("restoreDocumentHistory") && session.restoreDocumentHistory(payload);
}

bool ClipCanvasRuntime::Impl::restoreDocumentHistoryPreservingEditorStyles(
    const QByteArray& payload) {
    return hasThreadAccess("restoreDocumentHistoryPreservingEditorStyles") &&
           session.restoreDocumentHistoryPreservingEditorStyles(payload);
}

bool ClipCanvasRuntime::Impl::clearDocumentPreservingViewports() {
    if (!hasThreadAccess("clearDocumentPreservingViewports")) {
        return false;
    }
    return session.clearDocumentPreservingViewports();
}

bool ClipCanvasRuntime::Impl::setQuickSelectionDisabledTools(const QSet<ClipCanvasTool>& tools) {
    return hasThreadAccess("setQuickSelectionDisabledTools") &&
           session.setQuickSelectionDisabledTools(tools);
}

void ClipCanvasRuntime::Impl::destroyAsync() {
    if (!hasThreadAccess("destroyAsync")) {
        return;
    }

    session.destroyAsync(owner);
}

ClipRuntime ClipCanvasRuntime::Impl::handle() const {
    if (!hasThreadAccess("handle")) {
        return nullptr;
    }
    return session.handle();
}

bool ClipCanvasRuntime::Impl::hasThreadAccess(const char* operation) const {
    return threadAffinity.hasAccess(operation);
}

void ClipCanvasRuntime::Impl::registerClient(clip_canvas_runtime::Client* client) {
    if (!hasThreadAccess("registerClient")) {
        return;
    }
    session.registerClient(client);
}

void ClipCanvasRuntime::Impl::unregisterClient(clip_canvas_runtime::Client* client) {
    if (!hasThreadAccess("unregisterClient")) {
        return;
    }
    session.unregisterClient(client);
}

void ClipCanvasRuntime::Impl::syncChangedViewports(ClipChangedViewportList changedViewports) {
    if (!hasThreadAccess("syncChangedViewports")) {
        return;
    }
    session.syncChangedViewports(changedViewports);
    observeDocument();
}

void ClipCanvasRuntime::Impl::syncChangedViewportIds(
    const std::vector<std::uint64_t>& changedViewportIds) {
    if (!hasThreadAccess("syncChangedViewportIds")) {
        return;
    }
    session.syncChangedViewportIds(changedViewportIds);
    observeDocument();
}

QImage ClipCanvasRuntime::Impl::renderToImage(const QRectF& virtualSelectionRect,
                                              const QSize& outputSize,
                                              const QList<CanvasExportSource>& sources) {
    if (!hasThreadAccess("renderToImage")) {
        return {};
    }
    return clip_canvas_export::renderToImage(session.handle(), virtualSelectionRect, outputSize,
                                             sources, session.smartErase().snapshot());
}

ClipCanvasRuntime::ClipCanvasRuntime() : ClipCanvasRuntime(ClipCanvasRuntimeConfig{}) {}

ClipCanvasRuntime::ClipCanvasRuntime(const ClipCanvasRuntimeConfig& config)
    : m_impl(std::make_unique<Impl>(*this, config)) {}

ClipCanvasRuntime::~ClipCanvasRuntime() = default;

bool ClipCanvasRuntime::isOwnerThread() const {
    return m_impl->isOwnerThread();
}

bool ClipCanvasRuntime::isValid() const {
    return m_impl->isValid();
}

bool ClipCanvasRuntime::reset() {
    return m_impl->reset();
}

bool ClipCanvasRuntime::cloneDocumentSessionFrom(const ClipCanvasRuntime& source) {
    return m_impl->cloneDocumentSessionFrom(*source.m_impl);
}

QByteArray ClipCanvasRuntime::serializeDocumentSession() const {
    return m_impl->serializeDocumentSession();
}

QByteArray ClipCanvasRuntime::serializeSelectedDrawTemplate() const {
    return m_impl->serializeSelectedDrawTemplate();
}

QJsonArray ClipCanvasRuntime::selectedElementIds() const {
    if (!isOwnerThread() || !isValid())
        return {};
    std::size_t size = 0;
    if (clip_runtime_serialize_selected_element_ids(m_impl->handle(), nullptr, 0, &size) !=
            CLIP_OK ||
        size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {};
    QByteArray bytes(static_cast<qsizetype>(size), Qt::Uninitialized);
    std::size_t written = 0;
    if (clip_runtime_serialize_selected_element_ids(m_impl->handle(),
                                                    reinterpret_cast<std::uint8_t*>(bytes.data()),
                                                    size, &written) != CLIP_OK ||
        written != size)
        return {};
    return QJsonDocument::fromJson(bytes).array();
}

bool ClipCanvasRuntime::restoreDocumentSession(const QByteArray& payload) {
    return m_impl->restoreDocumentSession(payload);
}

QByteArray ClipCanvasRuntime::serializeDocumentHistory() const {
    return m_impl->serializeDocumentHistory();
}

bool ClipCanvasRuntime::restoreDocumentHistory(const QByteArray& payload) {
    return m_impl->restoreDocumentHistory(payload);
}

bool ClipCanvasRuntime::restoreDocumentHistoryPreservingEditorStyles(const QByteArray& payload) {
    return m_impl->restoreDocumentHistoryPreservingEditorStyles(payload);
}

bool ClipCanvasRuntime::clearDocumentPreservingViewports() {
    return m_impl->clearDocumentPreservingViewports();
}

bool ClipCanvasRuntime::setQuickSelectionDisabledTools(const QSet<ClipCanvasTool>& tools) {
    return m_impl->setQuickSelectionDisabledTools(tools);
}

QByteArray ClipCanvasRuntime::applyAnnotationTransaction(const QByteArray& payload) {
    if (!isOwnerThread() || payload.isEmpty() || payload.size() > 1024 * 1024)
        return {};
    ClipChangedViewportList changed = nullptr;
    uint8_t* json = nullptr;
    size_t size = 0;
    const auto result = clip_runtime_apply_annotation_json(
        m_impl->handle(), reinterpret_cast<const uint8_t*>(payload.constData()),
        static_cast<size_t>(payload.size()), &json, &size, &changed);
    const QByteArray response(result == CLIP_OK ? reinterpret_cast<const char*>(json) : nullptr,
                              result == CLIP_OK ? static_cast<qsizetype>(size) : 0);
    clip_annotation_result_destroy(json, size);
    if (result == CLIP_OK)
        m_impl->syncChangedViewports(changed);
    clip_changed_viewports_destroy(changed);
    return response;
}

bool ClipCanvasRuntime::undo() {
    if (!isOwnerThread() || !canUndo())
        return false;
    ClipChangedViewportList changed = nullptr;
    const bool ok = clip_runtime_undo_ex(m_impl->handle(), &changed) == CLIP_OK;
    if (ok)
        m_impl->syncChangedViewports(changed);
    clip_changed_viewports_destroy(changed);
    return ok;
}
bool ClipCanvasRuntime::redo() {
    if (!isOwnerThread() || !canRedo())
        return false;
    ClipChangedViewportList changed = nullptr;
    const bool ok = clip_runtime_redo_ex(m_impl->handle(), &changed) == CLIP_OK;
    if (ok)
        m_impl->syncChangedViewports(changed);
    clip_changed_viewports_destroy(changed);
    return ok;
}
bool ClipCanvasRuntime::canUndo() const {
    ClipHistoryState state{};
    return isOwnerThread() && clip_runtime_get_history_state(m_impl->handle(), &state) == CLIP_OK &&
           state.can_undo != 0;
}
bool ClipCanvasRuntime::canRedo() const {
    ClipHistoryState state{};
    return isOwnerThread() && clip_runtime_get_history_state(m_impl->handle(), &state) == CLIP_OK &&
           state.can_redo != 0;
}
quint64 ClipCanvasRuntime::documentRevision() const {
    return isOwnerThread() ? clip_runtime_document_revision(m_impl->handle()) : 0;
}
bool ClipCanvasRuntime::hasDocumentContent() const {
    return !isOwnerThread() || clip_runtime_has_document_content(m_impl->handle()) != 0;
}
void ClipCanvasRuntime::setDocumentChangedHandler(std::function<void()> handler) {
    if (isOwnerThread()) {
        if (handler)
            m_impl->observedRevision = documentRevision();
        m_impl->documentChanged = std::move(handler);
    }
}

void ClipCanvasRuntime::destroyAsync() {
    m_impl->destroyAsync();
}

void ClipCanvasRuntime::Impl::clearRenderState() {
    if (hasThreadAccess("clearRenderState")) {
        session.clearRenderState();
    }
}

void ClipCanvasRuntime::clearRenderState() {
    m_impl->clearRenderState();
}

QImage ClipCanvasRuntime::renderToImage(const QRectF& virtualSelectionRect, const QSize& outputSize,
                                        const QList<CanvasExportSource>& sources) {
    return m_impl->renderToImage(virtualSelectionRect, outputSize, sources);
}

void clip_canvas_runtime::Access::registerClient(ClipCanvasRuntime& runtime, Client& client) {
    runtime.m_impl->registerClient(&client);
}

ClipRuntime clip_canvas_runtime::Access::handle(const ClipCanvasRuntime& runtime) {
    return runtime.m_impl->handle();
}

void clip_canvas_runtime::Access::unregisterClient(ClipCanvasRuntime& runtime, Client& client) {
    runtime.m_impl->unregisterClient(&client);
}

void clip_canvas_runtime::Access::syncChangedViewports(ClipCanvasRuntime& runtime,
                                                       ClipChangedViewportList changedViewports) {
    runtime.m_impl->syncChangedViewports(changedViewports);
}

void clip_canvas_runtime::Access::syncChangedViewportIds(
    ClipCanvasRuntime& runtime, const std::vector<std::uint64_t>& changedViewportIds) {
    runtime.m_impl->syncChangedViewportIds(changedViewportIds);
}

clip_canvas_smart_erase::Coordinator&
clip_canvas_runtime::Access::smartErase(ClipCanvasRuntime& runtime) {
    return runtime.m_impl->smartErase();
}
void ClipCanvasRuntime::setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources) {
    if (!isOwnerThread())
        return;
    m_impl->smartErase().setSources(this, sources);
    m_impl->smartErase().sync(m_impl->handle());
}
ClipCanvasSmartEraseSnapshot ClipCanvasRuntime::smartEraseSnapshot() const {
    return isOwnerThread() ? m_impl->smartErase().snapshot() : ClipCanvasSmartEraseSnapshot{};
}
void ClipCanvasRuntime::restoreSmartEraseSnapshot(const ClipCanvasSmartEraseSnapshot& snapshot) {
    if (isOwnerThread())
        m_impl->smartErase().restoreSnapshot(snapshot);
}

struct ClipCanvasRuntimeEditor::Impl {
    ClipCanvasRuntime& runtime;
    ClipCanvasViewport viewport;
    bool succeeded = true;
    Impl(ClipCanvasRuntime& value, ClipCanvasTool tool) : runtime(value) {
        if (!runtime.isOwnerThread() ||
            !viewport.create(clip_canvas_runtime::Access::handle(runtime),
                             clip_canvas_viewport::defaultEngineConfig())) {
            succeeded = false;
            return;
        }
        if (tool != ClipCanvasTool::Select)
            mutate([tool](ClipRuntime r, ClipViewport v, ClipChangedViewportList* changed) {
                return clip_viewport_set_active_tool_ex(r, v, clip_canvas_types::toEngineTool(tool),
                                                        changed);
            });
    }
    template <class F> bool mutate(F function) {
        if (!runtime.isOwnerThread() || !viewport.isValid())
            return succeeded = false;
        ScopedChangedViewportList changed;
        const bool ok = function(viewport.runtime(), viewport.get(), changed.outParam()) == CLIP_OK;
        if (ok)
            clip_canvas_runtime::Access::syncChangedViewports(runtime, changed.get());
        succeeded = succeeded && ok;
        return ok;
    }
};
ClipCanvasRuntimeEditor::ClipCanvasRuntimeEditor(ClipCanvasRuntime& runtime, ClipCanvasTool tool)
    : m_impl(std::make_unique<Impl>(runtime, tool)) {}
ClipCanvasRuntimeEditor::~ClipCanvasRuntimeEditor() = default;
bool ClipCanvasRuntimeEditor::isValid() const {
    return m_impl->runtime.isOwnerThread() && m_impl->viewport.isValid();
}
bool ClipCanvasRuntimeEditor::setActiveTool(ClipCanvasTool tool) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_active_tool_ex(r, v, clip_canvas_types::toEngineTool(tool),
                                                changed);
    });
}
bool ClipCanvasRuntimeEditor::succeeded() const {
    return m_impl->succeeded;
}
ClipCanvasStyleToolbarState ClipCanvasRuntimeEditor::canvasStyleToolbarState() const {
    ClipStyleToolbarState state{};
    if (isValid())
        static_cast<void>(clip_viewport_get_style_toolbar_state(m_impl->viewport.runtime(),
                                                                m_impl->viewport.get(), &state));
    return clip_canvas_types::toCanvasStyleToolbarState(state);
}
ClipCanvasWatermarkConfig ClipCanvasRuntimeEditor::canvasWatermarkConfig() const {
    ClipWatermarkConfig state{};
    if (isValid())
        static_cast<void>(clip_viewport_get_watermark_config(m_impl->viewport.runtime(),
                                                             m_impl->viewport.get(), &state));
    return clip_canvas_types::toCanvasWatermarkConfig(state);
}
ClipCanvasSpotlightConfig ClipCanvasRuntimeEditor::canvasSpotlightConfig() const {
    ClipSpotlightConfig state{};
    if (isValid())
        static_cast<void>(clip_viewport_get_spotlight_config(m_impl->viewport.runtime(),
                                                             m_impl->viewport.get(), &state));
    return clip_canvas_types::toCanvasSpotlightConfig(state);
}
bool ClipCanvasRuntimeEditor::setDistanceStyleFromToolbar(const ClipCanvasDistanceStyle& style,
                                                          quint32 properties) {
    if (!clip_canvas_types::validDistanceStyle(style))
        return false;
    const auto value = clip_canvas_types::toEngineDistanceStyle(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_distance_style_patch_ex(r, v, &value, properties, changed);
    });
}
bool ClipCanvasRuntimeEditor::setDistanceCreationPixelScale(const QSizeF& scale) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_distance_pixel_scale_ex(r, v, scale.width(), scale.height(),
                                                         changed);
    });
}
bool ClipCanvasRuntimeEditor::setShapeStyleFromToolbar(const ClipCanvasShapeStyle& style,
                                                       quint32 properties,
                                                       ClipCanvasShapeKind kind) {
    const auto value = clip_canvas_types::toEngineShapeStyle(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_shape_style_patch_ex(
            r, v, &value, properties, clip_canvas_types::toEngineShapeKind(kind), changed);
    });
}
bool ClipCanvasRuntimeEditor::setTextStyleFromToolbar(const ClipCanvasTextStyle& style,
                                                      quint32 properties) {
    const auto value = clip_canvas_types::toEngineTextStyle(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_patch_text_style_ex(r, v, &value, properties, nullptr, 0, changed);
    });
}
bool ClipCanvasRuntimeEditor::setSerialNumberStyleFromToolbar(
    const ClipCanvasSerialNumberStyle& style) {
    const auto value = clip_canvas_types::toEngineSerialNumberStyle(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_serial_number_style_ex(r, v, &value, changed);
    });
}
bool ClipCanvasRuntimeEditor::setSerialNumberStyleFromToolbar(
    const ClipCanvasSerialNumberStyle& style, quint32 properties) {
    const auto value = clip_canvas_types::toEngineSerialNumberStyle(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_serial_number_style_patch_ex(r, v, &value, properties, changed);
    });
}
bool ClipCanvasRuntimeEditor::setFilterStyleFromToolbar(const ClipCanvasFilterStyle& style,
                                                        quint32 properties) {
    const ClipFilterStyle value{static_cast<ClipFilterType>(style.type), style.strength,
                                style.opacity, style.strokeWidth};
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_filter_style_ex(r, v, &value, properties, changed);
    });
}
bool ClipCanvasRuntimeEditor::setBrushEraserCreationStyle(const ClipCanvasBrushEraserStyle& style,
                                                          quint32 properties) {
    const ClipBrushEraserStyle value{style.strokeWidth};
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_brush_eraser_creation_style_ex(r, v, &value, properties, changed);
    });
}
bool ClipCanvasRuntimeEditor::setWatermarkConfigFromToolbar(
    const ClipCanvasWatermarkConfig& style) {
    const auto value = clip_canvas_types::toEngineWatermarkConfig(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_watermark_config_ex(r, v, &value, changed);
    });
}
bool ClipCanvasRuntimeEditor::setSpotlightConfigFromToolbar(
    const ClipCanvasSpotlightConfig& style) {
    const auto value = clip_canvas_types::toEngineSpotlightConfig(style);
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_spotlight_config_ex(r, v, &value, changed);
    });
}
bool ClipCanvasRuntimeEditor::select(quint32 index, quint32 generation) {
    const ClipElementId id{index, generation};
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_select_element_ex(r, v, id, changed);
    });
}
bool ClipCanvasRuntimeEditor::deleteSelected() {
    return m_impl->mutate([](auto r, auto v, auto changed) {
        return clip_viewport_delete_selected_ex(r, v, changed);
    });
}
bool ClipCanvasRuntimeEditor::deleteAllElements() {
    return m_impl->mutate([](auto r, auto v, auto changed) {
        return clip_viewport_delete_all_elements_ex(r, v, changed);
    });
}
bool ClipCanvasRuntimeEditor::erasePath(const QList<QPointF>& points, ClipCanvasTool eraserTool) {
    if (!isValid() || points.isEmpty() || points.size() > 8192 ||
        (eraserTool != ClipCanvasTool::Eraser && eraserTool != ClipCanvasTool::RectangleEraser &&
         eraserTool != ClipCanvasTool::BrushEraser) ||
        (eraserTool == ClipCanvasTool::RectangleEraser && points.size() != 2))
        return false;
    for (const auto& point : points)
        if (!std::isfinite(point.x()) || !std::isfinite(point.y()))
            return false;
    const auto runtime = m_impl->viewport.runtime();
    const auto viewport = m_impl->viewport.get();
    ClipActiveTool previous{};
    if (clip_viewport_get_active_tool(runtime, viewport, &previous) != CLIP_OK ||
        clip_viewport_set_surface_size(runtime, viewport, 2, 2) != CLIP_OK ||
        clip_viewport_set_camera(runtime, viewport, 1, 1, 1) != CLIP_OK)
        return false;
    const auto setTool = [&](ClipActiveTool tool) {
        return m_impl->mutate([&](auto r, auto v, auto changed) {
            return clip_viewport_set_active_tool_ex(r, v, tool, changed);
        });
    };
    if (!setTool(clip_canvas_types::toEngineTool(eraserTool)))
        return false;
    const auto eventAt = [](QPointF point, ClipPointerEventType type) {
        ClipInputEvent event{};
        event.kind = CLIP_INPUT_EVENT_POINTER;
        event.pointer.pointer_id = 1;
        event.pointer.event_type = type;
        event.pointer.device = CLIP_POINTER_DEVICE_MOUSE;
        event.pointer.position_x = point.x();
        event.pointer.position_y = point.y();
        event.pointer.button = CLIP_POINTER_BUTTON_PRIMARY;
        event.pointer.buttons = type == CLIP_POINTER_EVENT_UP ? 0 : 1;
        return event;
    };
    const auto send = [&](const ClipInputEvent& event) {
        ClipInteractionOutput output{};
        return m_impl->mutate([&](auto r, auto v, auto changed) {
            return clip_viewport_process_input_ex(r, v, &event, &output, changed);
        });
    };
    bool ok = send(eventAt(points.first(), CLIP_POINTER_EVENT_DOWN));
    if (ok && points.size() > 1) {
        std::vector<ClipInputEvent> moves;
        moves.reserve(static_cast<size_t>(points.size() - 1));
        for (qsizetype index = 1; index < points.size(); ++index)
            moves.push_back(eventAt(points[index], CLIP_POINTER_EVENT_MOVE));
        ClipInteractionOutput output{};
        ok = m_impl->mutate([&](auto r, auto v, auto changed) {
            return clip_viewport_process_pointer_move_batch_ex(
                r, v, moves.data(), static_cast<uint32_t>(moves.size()), &output, changed);
        });
    }
    if (ok)
        ok = send(eventAt(points.last(), CLIP_POINTER_EVENT_UP));
    // Tool changes discard uncommitted eraser previews if an input failed.
    const bool restored = setTool(previous);
    return ok && restored;
}
bool ClipCanvasRuntimeEditor::duplicateSelected(QPointF offset) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_duplicate_selected_ex(r, v, offset.x(), offset.y(), changed);
    });
}
bool ClipCanvasRuntimeEditor::reorderSelected(ClipCanvasSelectionOrder order) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_reorder_selected_ex(r, v, static_cast<uint32_t>(order), changed);
    });
}
bool ClipCanvasRuntimeEditor::alignSelected(ClipCanvasSelectionAlignment alignment) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_align_selected_ex(r, v, static_cast<uint32_t>(alignment), changed);
    });
}
bool ClipCanvasRuntimeEditor::setSelectedOpacity(double opacity) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_selected_opacity_ex(r, v, opacity, changed);
    });
}
bool ClipCanvasRuntimeEditor::adjustSelectedSerialNumbers(qint64 delta) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_adjust_selected_serial_numbers_ex(r, v, delta, changed);
    });
}
bool ClipCanvasRuntimeEditor::createSerialNumberText() {
    ClipElementId id{};
    uint8_t hasId = 0;
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_create_serial_number_text_ex(r, v, 0, 0, &id, &hasId, changed);
    }) && hasId != 0;
}
bool ClipCanvasRuntimeEditor::insertDrawTemplate(const QByteArray& payload, QPointF center) {
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_insert_draw_template_ex(
            r, v, reinterpret_cast<const uint8_t*>(payload.constData()),
            static_cast<size_t>(payload.size()), center.x(), center.y(), changed);
    });
}
bool ClipCanvasRuntimeEditor::setAutoFilterRegions(const ClipCanvasAutoFilterRecord& record) {
    const auto boundsOf = [](QRectF value) {
        return ClipAutoFilterBounds{value.left(), value.top(), value.right(), value.bottom()};
    };
    const auto bounds = boundsOf(record.sourceBounds);
    std::vector<ClipAutoFilterRegion> regions;
    for (const auto& region : record.regions) {
        const auto category = region.category.toUtf8();
        if (category.size() > 127)
            return false;
        ClipAutoFilterRegion value{};
        value.id = region.id;
        value.bounds = boundsOf(region.bounds);
        std::memcpy(value.category, category.constData(), static_cast<size_t>(category.size()));
        regions.push_back(value);
    }
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_set_auto_filter_regions(r, v, &bounds, regions.data(), regions.size(),
                                                     changed);
    });
}
bool ClipCanvasRuntimeEditor::fillAutoFilterCategory(const QString& category) {
    const auto bytes = category.toUtf8();
    return m_impl->mutate([&](auto r, auto v, auto changed) {
        return clip_viewport_fill_auto_filter_category(
            r, v, reinterpret_cast<const uint8_t*>(bytes.constData()),
            static_cast<size_t>(bytes.size()), changed);
    });
}
