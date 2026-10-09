#pragma once

#include "clip_canvas_runtime_clients.h"
#include "clip_canvas_smart_erase.h"
#include "clip_canvas_ffi_handles.h"
#include "clip_draw_engine.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <QByteArray>
#include <vector>

class ClipCanvasRuntime;

namespace clip_canvas_runtime {

enum class OwnerDestructionPolicy {
    DetachClients,
    AbandonClientsAndDestroyAsync,
};

class RuntimeSession final {
  public:
    RuntimeSession();
    explicit RuntimeSession(const ClipCanvasRuntimeConfig& config);
    ~RuntimeSession();

    bool isValid() const;
    bool reset();
    bool cloneDocumentSessionFrom(const RuntimeSession& source);
    QByteArray serializeDocumentSession() const;
    QByteArray serializeSelectedDrawTemplate() const;
    bool restoreDocumentSession(const QByteArray& payload);
    QByteArray serializeDocumentHistory() const;
    bool restoreDocumentHistory(const QByteArray& payload);
    bool restoreDocumentHistoryPreservingEditorStyles(const QByteArray& payload);
    bool clearDocumentPreservingViewports();
    void clearRenderState();
    bool setQuickSelectionDisabledTools(const QSet<ClipCanvasTool>& tools);
    void destroyAsync(ClipCanvasRuntime& owner);
    void destroyForOwnerDestruction(ClipCanvasRuntime& owner, OwnerDestructionPolicy policy);

    ClipRuntime handle() const;
    clip_canvas_smart_erase::Coordinator& smartErase() {
        return m_smartErase;
    }
    void registerClient(Client* client);
    void unregisterClient(Client* client);
    void syncChangedViewports(ClipChangedViewportList changedViewports);
    void syncChangedViewportIds(const std::vector<std::uint64_t>& changedViewportIds);

  private:
    bool replaceRuntime(ScopedRuntimeHandle replacement);
    void destroyRuntimeAsync();

    ClipCanvasRuntimeConfig m_config;
    ScopedRuntimeHandle m_runtime;
    ClientRegistry m_clients;
    clip_canvas_smart_erase::Coordinator m_smartErase;
};

} // namespace clip_canvas_runtime
