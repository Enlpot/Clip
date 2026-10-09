#pragma once

#include "clip_canvas_runtime_access.h"
#include "clip_draw_engine.h"

#include <QList>

#include <cstdint>
#include <vector>

class ClipCanvasRuntime;

namespace clip_canvas_runtime {

class ClientRegistry final {
  public:
    using DetachedClients = QList<Client*>;

    void registerClient(Client* client);
    void unregisterClient(Client* client);

    DetachedClients detachForRuntimeReplacement();
    void attachAfterRuntimeReplacement(const DetachedClients& clients, ClipRuntime runtime);
    void detachAndReleaseClients(ClipCanvasRuntime& owner);
    void abandonClientsWithoutDetach();
    void clearRenderState();
    void resetDocumentRetainedState();
    void smartEraseChanged();
    bool resetEditingState();
    void syncChangedViewports(const std::vector<std::uint64_t>& changedViewportIds);

  private:
    QList<Client*> m_clients;
};

} // namespace clip_canvas_runtime
