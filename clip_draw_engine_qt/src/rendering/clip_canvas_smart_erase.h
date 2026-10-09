#pragma once

#include "clip_canvas_display_item.h"
#include "clip_draw_engine_qt/clip_canvas_smart_erase.h"
#include "clip_canvas_display_cache.h"

#include <QPainterPath>
#include <QSize>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

class QPainter;

namespace clip_canvas_smart_erase {

struct Result {
    QImage original;
    QImage filled;
    QRectF canvasRect;
    bool success = false;
};

// Private reconstruction controls for quality tests and performance experiments.
// Application callers use reconstruct(), which always uses the selected defaults.
struct ReconstructionOptions {
    int coarsePasses = 5;
    int intermediatePasses = 3;
    int finePasses = 2;
    bool earlyRejection = true;
    bool parallelVoting = true;
    bool cropContext = true;

    static ReconstructionOptions reference() {
        ReconstructionOptions options;
        options.cropContext = false;
        return options;
    }
};
struct LevelDiagnostics {
    QSize size;
    int maskedPixels = 0;
    int passes = 0;
    double searchMs = 0;
    double votingMs = 0;
};
struct ReconstructionDiagnostics {
    QSize workingSize;
    QSize croppedSize;
    int maskedPixels = 0;
    enum class Path { Empty, Surface, Periodic, Patches } path = Path::Empty;
    double preparationMs = 0;
    double fastPathsMs = 0;
    double guidePyramidMs = 0;
    std::vector<LevelDiagnostics> levels;
    // Optional synchronous progress observer; preserved when measurements reset.
    std::function<void(const LevelDiagnostics&)> levelStarted;
};

Result reconstructWithOptions(const ClipCanvasSceneItem& item,
                              const QList<ClipCanvasBaseImageSource>& sources,
                              const std::atomic_bool& cancelled,
                              const ReconstructionOptions& options,
                              ReconstructionDiagnostics* diagnostics = nullptr);

QPainterPath path(const ClipCanvasSceneItem& item);
QByteArray geometryKey(const ClipCanvasSceneItem& item);
Result reconstruct(const ClipCanvasSceneItem& item, const QList<ClipCanvasBaseImageSource>& sources,
                   const std::atomic_bool& cancelled);
bool hasItems(const ClipCanvasSmartEraseSnapshot& snapshot);
void applySnapshot(std::vector<ClipCanvasSceneItem>& items,
                   const ClipCanvasSmartEraseSnapshot& snapshot);
void paint(QPainter& painter, const SceneDisplayInfo& info, const ClipCanvasSceneItem& item,
           const ClipCanvasSmartEraseSnapshot& snapshot);

class Coordinator {
  public:
    using Compute =
        std::function<Result(const ClipCanvasSceneItem&, const QList<ClipCanvasBaseImageSource>&,
                             const std::atomic_bool&)>;
    explicit Coordinator(std::function<void()> repaint, Compute compute = reconstruct);
    ~Coordinator();
    void setSources(const void* owner, const QList<ClipCanvasBaseImageSource>& sources,
                    bool notify = true);
    void removeSources(const void* owner);
    void sync(ClipRuntime runtime);
    void syncItems(std::vector<ClipCanvasSceneItem> items);
    void reset();
    // Current reconstruction results remain available for rendering and export.
    void clearCache();
    ClipCanvasSmartEraseSnapshot snapshot() const;
    void restoreSnapshot(const ClipCanvasSmartEraseSnapshot& snapshot);

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace clip_canvas_smart_erase
