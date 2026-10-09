#include "clip_shot/presentation/screenshotcursorimagesource.h"
#include "clip_shot/presentation/screenshotoverlaycanvaspresenter.h"

#include "clip_shot/presentation/screenshotcanvastoolstyles.h"
#include "clip_shot/presentation/screenshotcanvasrenderer.h"
#include "clip_draw_engine_qt/clip_canvas_widget.h"
#include "clip_shot/presentation/screenshotdisplaysession.h"
#include "clip_shot/presentation/screenshotgeometry.h"
#include "clip_shot/presentation/screenshotoverlaywindow.h"
#include "clip_draw_engine_qt/clip_canvas_cursor.h"
#include "clip_shot/presentation/components/icons/clipshoticons.h"
#include "icon_renderer.h"

#include "clip_shot/presentation/screenshotimagesource.h"
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace {
void updateOverlayCursorsForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                           bool selecting, bool dragging);

} // namespace

ScreenshotOverlayCanvasPresenter::ScreenshotOverlayCanvasPresenter(OverlayFactory ensureOverlay)
    : m_ensureOverlay(std::move(ensureOverlay)) {}

void ScreenshotOverlayCanvasPresenter::clearOverlayCanvas(ScreenshotOverlayWindow* overlay) const {
    if (overlay == nullptr || overlay->canvas() == nullptr) {
        return;
    }

    overlay->resetScreenshotRendering();
    overlay->canvas()->cancelActiveTextEditing();
    overlay->canvas()->clearRenderState();
    overlay->canvas()->clearCursorForLayer(ClipCanvasCursorLayer::Host);
}

namespace {
enum class DisplayModelApplyMode : std::uint8_t {
    PrepareOnly,
    ApplyCapturedImage,
};

void configureCanvasBackground(ScreenshotOverlayWindow& overlay, bool clearCanvasBackground) {
    overlay.setCanvasClearBackgroundEnabled(clearCanvasBackground);
}

void applyDisplayModelsToDisplaySession(
    const ScreenshotOverlayCanvasPresenter::OverlayFactory& ensureOverlay,
    ScreenshotDisplaySession& displaySession, DisplayModelApplyMode mode) {
    const bool applyCapturedImage = mode == DisplayModelApplyMode::ApplyCapturedImage;
    const bool clearCanvasBackground = applyCapturedImage;

    if (!ensureOverlay) {
        return;
    }

    displaySession.forEachMutableActiveDisplay([&](qsizetype index, CapturedDisplayModel& display) {
        ScreenshotOverlayWindow* overlay = displaySession.overlayAt(index);
        if (overlay == nullptr) {
            overlay = displaySession.ensureOverlayAt(index, ensureOverlay);
        }
        if (overlay == nullptr) {
            return;
        }

        ClipCanvasWidget* canvas = overlay->canvas();
        if (canvas == nullptr) {
            return;
        }
        configureCanvasBackground(*overlay, clearCanvasBackground);

        const ScreenshotDisplayViewportGeometry viewport =
            ScreenshotGeometryMapper::displayViewportGeometry(display);
        if (!viewport.valid) {
            return;
        }

        if (applyCapturedImage) {
            if (displaySession.hasImageSources()) {
                QList<ScreenshotImageLayer> layers;
                displaySession.forEachImageSource(
                    [&](qsizetype, const CapturedDisplayModel& source) {
                        layers.append(
                            screenshotDisplayImageLayers(source, displaySession.cursorVisible));
                    });
                overlay->setScreenshotImageSource(
                    ScreenshotImageSource::fromLayers(std::move(layers)));
            } else {
                overlay->setScreenshotImageSource(
                    screenshotDisplayImageSource(display, displaySession.cursorVisible));
            }
        }
        canvas->setViewportCamera(viewport.canvasCenter.x(), viewport.canvasCenter.y(),
                                  viewport.canvasToLogicalScale);
        if (display.screen != nullptr && overlay->screen() != display.screen) {
            overlay->setScreen(display.screen);
        }
        overlay->setCaptureGeometry(viewport.logicalRect);
        if (applyCapturedImage) {
            overlay->update();
        }
    });
}
} // namespace

void ScreenshotOverlayCanvasPresenter::prepareDisplayModels(
    ScreenshotDisplaySession& displaySession) const {
    applyDisplayModelsToDisplaySession(m_ensureOverlay, displaySession,
                                       DisplayModelApplyMode::PrepareOnly);
}

void ScreenshotOverlayCanvasPresenter::applyDisplayModels(
    ScreenshotDisplaySession& displaySession) const {
    applyDisplayModelsToDisplaySession(m_ensureOverlay, displaySession,
                                       DisplayModelApplyMode::ApplyCapturedImage);
}

namespace {
void raiseOverlayWindow(ScreenshotOverlayWindow& overlay) {
    overlay.raise();
}

void activateAndFocusOverlayWindow(ScreenshotOverlayWindow& overlay) {
    overlay.activateWindow();

    if (overlay.canvas() != nullptr) {
        overlay.canvas()->setFocus(Qt::OtherFocusReason);
    }
    overlay.commitInitialSelectionCursor();
}

void activateOverlayWindow(ScreenshotOverlayWindow& overlay) {
    raiseOverlayWindow(overlay);
    activateAndFocusOverlayWindow(overlay);
}

void scheduleOverlayActivation(ScreenshotOverlayWindow* overlay) {
    if (overlay == nullptr) {
        return;
    }

    QTimer::singleShot(0, overlay, [overlay]() {
        if (overlay == nullptr || !overlay->isVisible()) {
            return;
        }

        activateAndFocusOverlayWindow(*overlay);
        QTimer::singleShot(0, overlay, [overlay]() {
            if (overlay != nullptr && overlay->isVisible()) {
                overlay->commitInitialSelectionCursor();
            }
        });
    });
}

constexpr qreal kFramePacedFallbackRefreshRate = 60.0;

int framePacedActivationDelayMs(const QScreen* screen) {
    const qreal rate = screen != nullptr ? screen->refreshRate() : 0.0;
    return std::max(1, static_cast<int>(std::ceil(
                           1000.0 / (rate > 0.0 ? rate : kFramePacedFallbackRefreshRate))));
}

void commitInitialSelectionCursorOn(ScreenshotOverlayWindow* overlay) {
    if (overlay != nullptr) {
        overlay->commitInitialSelectionCursor();
    }
}

void scheduleFramePacedOverlayActivation(ScreenshotOverlayWindow* overlay) {
    if (overlay == nullptr) {
        return;
    }
    // Foreground and focus negotiation is slow on Windows and is not needed
    // while a global-mouse drag runs on hook input. Keep the cursor sprite and
    // the raise() above immediate, but run activation one frame after the
    // reveal so the first paced drag updates are not queued behind it.
    QTimer::singleShot(framePacedActivationDelayMs(overlay->screen()), overlay, [overlay]() {
        if (overlay == nullptr || !overlay->isVisible()) {
            return;
        }

        activateAndFocusOverlayWindow(*overlay);
    });
}

void showOverlayWindow(ScreenshotOverlayWindow& overlay, bool deferFirstPaint) {
    overlay.showPreparedFrame(deferFirstPaint);
}

struct ActiveOverlayEntry {
    const CapturedDisplayModel* display = nullptr;
    ScreenshotOverlayWindow* overlay = nullptr;
};

QVector<ActiveOverlayEntry> activeOverlayEntries(const ScreenshotDisplaySession& displaySession) {
    QVector<ActiveOverlayEntry> entries;
    entries.reserve(static_cast<int>(displaySession.size()));
    displaySession.forEachActiveOverlay(
        [&](qsizetype, const CapturedDisplayModel& display, ScreenshotOverlayWindow* overlay) {
            entries.push_back(ActiveOverlayEntry{&display, overlay});
        });
    return entries;
}

qsizetype overlayEntryIndexAtPosition(const QVector<ActiveOverlayEntry>& entries,
                                      const QPoint& cursorPosition) {
    for (qsizetype index = 0; index < entries.size(); ++index) {
        const CapturedDisplayModel* display = entries.at(index).display;
        if (display != nullptr && display->logicalRect.contains(cursorPosition, false)) {
            return index;
        }
    }

    return -1;
}

qsizetype preferredOverlayEntryIndex(const QVector<ActiveOverlayEntry>& entries,
                                     const QPoint& cursorPosition) {
    const qsizetype cursorIndex = overlayEntryIndexAtPosition(entries, cursorPosition);
    return cursorIndex >= 0 || entries.isEmpty() ? cursorIndex : 0;
}

void showCapturedImageOverlayNow(ScreenshotOverlayWindow& overlay, bool framePaced) {
    showOverlayWindow(overlay, framePaced);
    raiseOverlayWindow(overlay);
    if (framePaced) {
        commitInitialSelectionCursorOn(&overlay);
        scheduleFramePacedOverlayActivation(&overlay);
        return;
    }
    scheduleOverlayActivation(&overlay);
}

void showCapturedImageOverlayDeferred(ScreenshotOverlayWindow* overlay, bool framePaced) {
    if (overlay == nullptr) {
        return;
    }

    QTimer::singleShot(0, overlay, [overlay, framePaced]() {
        if (overlay == nullptr) {
            return;
        }

        showCapturedImageOverlayNow(*overlay, framePaced);
    });
}

void showCapturedImageOverlaysForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                                bool framePaced) {
    const QVector<ActiveOverlayEntry> entries = activeOverlayEntries(displaySession);
    const qsizetype preferredIndex =
        preferredOverlayEntryIndex(entries, displaySession.logicalCursorPosition());

    if (preferredIndex >= 0 && preferredIndex < entries.size()) {
        showCapturedImageOverlayNow(*entries.at(preferredIndex).overlay, framePaced);
    }

    for (qsizetype index = 0; index < entries.size(); ++index) {
        if (index == preferredIndex) {
            continue;
        }
        showCapturedImageOverlayDeferred(entries.at(index).overlay, framePaced);
    }
}

void showOverlayWindowsForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                         ScreenshotOverlayShowMode mode) {
    if (mode == ScreenshotOverlayShowMode::WarmSurface) {
        displaySession.forEachActiveOverlay(
            [](qsizetype, const CapturedDisplayModel&, ScreenshotOverlayWindow* overlay) {
                overlay->warmPresentationSurface();
            });
        return;
    }
    if (mode == ScreenshotOverlayShowMode::CapturedImage ||
        mode == ScreenshotOverlayShowMode::CapturedImageFramePaced) {
        showCapturedImageOverlaysForDisplaySession(
            displaySession, mode == ScreenshotOverlayShowMode::CapturedImageFramePaced);
        return;
    }

    displaySession.forEachActiveOverlay(
        [mode](qsizetype, const CapturedDisplayModel&, ScreenshotOverlayWindow* overlay) {
            const bool wasVisible = overlay->isVisible();

            if (!wasVisible) {
                showOverlayWindow(*overlay, false);
            }

            if (mode == ScreenshotOverlayShowMode::PreparedPreview) {
                activateOverlayWindow(*overlay);
            }
        });
}
} // namespace

void ScreenshotOverlayCanvasPresenter::showOverlayWindows(
    const ScreenshotDisplaySession& displaySession, ScreenshotOverlayShowMode mode) const {
    showOverlayWindowsForDisplaySession(displaySession, mode);
}

namespace {
void updateOverlayStateForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                         const ScreenshotSelectionVisualState& selectionState,
                                         bool intelligentSelecting, bool manualSelecting,
                                         bool dragging) {
    const QRectF& selection = selectionState.bounds;
    const bool hasSelection = selection.isValid() && !selection.isEmpty();
    const ScreenshotHalfOpenRect selectionRect =
        hasSelection ? ScreenshotHalfOpenRect::fromRectF(selection) : ScreenshotHalfOpenRect();
    const auto renderSpec =
        screenshotSelectionRenderSpec(displaySession, selectionRect.toAlignedQRect());
    const qreal pixelScale = renderSpec.isValid() ? renderSpec.scale : 1.0;
    displaySession.forEachActiveOverlay([&](qsizetype, const CapturedDisplayModel& display,
                                            ScreenshotOverlayWindow* overlay) {
        ClipCanvasWidget* canvas = overlay->canvas();
        if (canvas == nullptr) {
            return;
        }
        static_cast<void>(canvas->setDistanceCreationPixelScale(QSizeF(pixelScale, pixelScale)));
        const ScreenshotHalfOpenRect displayRect =
            ScreenshotHalfOpenRect::fromRectF(ScreenshotGeometryMapper::displayCanvasRect(display));
        const bool selectionIntersectsDisplay =
            hasSelection && selectionRect.intersects(displayRect);
        overlay->setScreenshotMaskVisible(true);
        if (selectionIntersectsDisplay) {
            overlay->setScreenshotSelectionState(selectionState);
        } else {
            ScreenshotSelectionVisualState draftOnly;
            draftOnly.draftPath = selectionState.draftPath;
            draftOnly.draftVertices = selectionState.draftVertices;
            draftOnly.subtracting = selectionState.subtracting;
            draftOnly.dangerColor = selectionState.dangerColor;
            overlay->setScreenshotSelectionState(draftOnly);
        }
    });
    updateOverlayCursorsForDisplaySession(displaySession, intelligentSelecting || manualSelecting,
                                          dragging || selectionState.hoveredEffectHandle !=
                                                          ScreenshotSelectionEffectHandle::None);
}
} // namespace

void ScreenshotOverlayCanvasPresenter::updateOverlayState(
    const ScreenshotDisplaySession& displaySession,
    const ScreenshotSelectionVisualState& selectionState, bool intelligentSelecting,
    bool manualSelecting, bool dragging) const {
    updateOverlayStateForDisplaySession(displaySession, selectionState, intelligentSelecting,
                                        manualSelecting, dragging);
}

namespace {
void updateOverlayCursorsForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                           bool selecting, bool dragging) {
    // Apply each overlay's final cursor directly. Clearing a canvas only to
    // reapply the crosshair on the next loop resolves the cursor to the arrow
    // in between, and Windows applies every cursor-shape change to the native
    // sprite, so the streamed selection updates made the cursor flicker.
    displaySession.forEachDisplayWithOverlay(
        [&](qsizetype, const CapturedDisplayModel& display, ScreenshotOverlayWindow* overlay) {
            ClipCanvasWidget* canvas = overlay == nullptr ? nullptr : overlay->canvas();
            if (canvas == nullptr) {
                return;
            }
            if (selecting && display.active) {
                canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::CrossCursor));
            } else if (!dragging) {
                canvas->clearCursorForLayer(ClipCanvasCursorLayer::Host);
            }
        });
}
} // namespace

void ScreenshotOverlayCanvasPresenter::updateOverlayCursors(
    const ScreenshotDisplaySession& displaySession, bool selecting, bool dragging) const {
    updateOverlayCursorsForDisplaySession(displaySession, selecting, dragging);
}

void ScreenshotOverlayCanvasPresenter::updateGuideLines(
    const ScreenshotDisplaySession& displaySession, ScreenshotOverlayWindow* owner,
    const QPointF& localPosition, bool visible, const QColor& cursorColor,
    const QColor& monitorCenterColor, const QColor& selectionCenterColor) const {
    const bool guideLinesEnabled = (cursorColor.isValid() && cursorColor.alpha() > 0) ||
                                   (monitorCenterColor.isValid() && monitorCenterColor.alpha() > 0);
    ScreenshotOverlayWindow* nextOwner = visible && guideLinesEnabled ? owner : nullptr;
    if (nextOwner != nullptr && m_guideLineOwner != nextOwner) {
        bool activeOwner = false;
        displaySession.forEachActiveOverlay(
            [&](qsizetype, const CapturedDisplayModel&, ScreenshotOverlayWindow* overlay) {
                activeOwner = activeOwner || overlay == nextOwner;
            });
        if (!activeOwner) {
            nextOwner = nullptr;
        }
    }

    if (m_guideLineOwner != nextOwner && m_guideLineOwner != nullptr) {
        m_guideLineOwner->clearScreenshotGuideLines();
    }
    m_guideLineOwner = nextOwner;
    if (nextOwner != nullptr) {
        nextOwner->setScreenshotGuideLines(localPosition, cursorColor, monitorCenterColor);
    }
    const bool selectionGuideVisible =
        visible && selectionCenterColor.isValid() && selectionCenterColor.alpha() > 0;
    const bool monitorGuideVisible =
        visible && monitorCenterColor.isValid() && monitorCenterColor.alpha() > 0;
    displaySession.forEachActiveOverlay([&](qsizetype, const CapturedDisplayModel&,
                                            ScreenshotOverlayWindow* overlay) {
        if (overlay == nullptr) {
            return;
        }
        overlay->setSelectionCenterGuideLineColor(selectionGuideVisible ? selectionCenterColor
                                                                        : QColor(Qt::transparent));
        ClipCanvasWidget* canvas = overlay->canvas();
        if (canvas == nullptr) {
            return;
        }
        ClipCanvasSnapGuideTargets targets;
        const bool ownsMonitorGuide = monitorGuideVisible && overlay == nextOwner;
        if (selectionGuideVisible || ownsMonitorGuide) {
            const QTransform canvasToView = canvas->canvasToViewTransform();
            if (!canvasToView.isInvertible()) {
                canvas->setCanvasSnapGuideTargets({});
                return;
            }
            const QRectF viewport(canvas->rect());
            if (selectionGuideVisible) {
                const QRectF selection = overlay->screenshotSelection();
                if (selection.isValid() && !selection.isEmpty()) {
                    const QPointF center = canvasToView.map(selection.center());
                    if (center.x() >= viewport.left() && center.x() < viewport.right()) {
                        targets.verticalXs.append(selection.center().x());
                    }
                    if (center.y() >= viewport.top() && center.y() < viewport.bottom()) {
                        targets.horizontalYs.append(selection.center().y());
                    }
                }
            }
            if (ownsMonitorGuide) {
                const QPointF center = canvasToView.inverted().map(viewport.center());
                targets.verticalXs.append(center.x());
                targets.horizontalYs.append(center.y());
            }
        }
        canvas->setCanvasSnapGuideTargets(targets);
    });
}

void ScreenshotOverlayCanvasPresenter::updateGuideLinesAtGlobalPosition(
    const ScreenshotDisplaySession& displaySession, const QPoint& globalPosition, bool visible,
    const QColor& cursorColor, const QColor& monitorCenterColor,
    const QColor& selectionCenterColor) const {
    if (!visible) {
        clearGuideLines(displaySession);
        return;
    }

    ScreenshotOverlayWindow* owner = nullptr;
    displaySession.forEachActiveOverlay(
        [&](qsizetype, const CapturedDisplayModel& display, ScreenshotOverlayWindow* overlay) {
            if (owner == nullptr && display.logicalRect.contains(globalPosition, false)) {
                owner = overlay;
            }
        });
    const QPointF localPosition =
        owner != nullptr ? owner->canvasLocalPosition(globalPosition) : QPointF();
    updateGuideLines(displaySession, owner, localPosition, true, cursorColor, monitorCenterColor,
                     selectionCenterColor);
}

void ScreenshotOverlayCanvasPresenter::clearGuideLines(
    const ScreenshotDisplaySession& displaySession) const {
    const QPointer<ScreenshotOverlayWindow> previousOwner = m_guideLineOwner;
    m_guideLineOwner = nullptr;
    if (previousOwner != nullptr) {
        previousOwner->clearScreenshotGuideLines();
    }
    displaySession.forEachOverlay([previousOwner](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay != previousOwner) {
            overlay->clearScreenshotGuideLines();
        }
        if (overlay != nullptr && overlay->canvas() != nullptr) {
            overlay->canvas()->setCanvasSnapGuideTargets({});
        }
    });
}

void ScreenshotOverlayCanvasPresenter::setOverlayCursor(
    ScreenshotOverlayWindow* overlay, ScreenshotSelectionDragMode dragMode) const {
    if (overlay == nullptr) {
        return;
    }

    ClipCanvasWidget* canvas = overlay->canvas();
    if (canvas == nullptr) {
        return;
    }

    switch (dragMode) {
    case ScreenshotSelectionDragMode::Marquee:
        canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::CrossCursor));
        break;
    case ScreenshotSelectionDragMode::All:
        canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::SizeAllCursor));
        break;
    case ScreenshotSelectionDragMode::TopLeft:
    case ScreenshotSelectionDragMode::BottomRight:
        canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::SizeFDiagCursor));
        break;
    case ScreenshotSelectionDragMode::TopRight:
    case ScreenshotSelectionDragMode::BottomLeft:
        canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::SizeBDiagCursor));
        break;
    case ScreenshotSelectionDragMode::Top:
    case ScreenshotSelectionDragMode::Bottom:
        canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::SizeVerCursor));
        break;
    case ScreenshotSelectionDragMode::Right:
    case ScreenshotSelectionDragMode::Left:
        canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, QCursor(Qt::SizeHorCursor));
        break;
    case ScreenshotSelectionDragMode::None:
    default:
        canvas->clearCursorForLayer(ClipCanvasCursorLayer::Host);
        break;
    }
}

void ScreenshotOverlayCanvasPresenter::setOverlayEffectCursor(
    ScreenshotOverlayWindow* overlay, ScreenshotSelectionEffectHandle handle) {
    auto* canvas = overlay != nullptr ? overlay->canvas() : nullptr;
    if (canvas == nullptr)
        return;
    const qreal dpr = canvas->devicePixelRatioF();
    const QCursor cursor =
        handle == ScreenshotSelectionEffectHandle::Shadow
            ? adqt::icons::makeCursor(
                  clip_shot::presentation::icons::custom::cursor::SelectionShadowCursor(),
                  QSize(32, 32), QPoint(3, 3), dpr)
            : clipCanvasCornerRadiusCursor(dpr);
    canvas->setCursorForLayer(ClipCanvasCursorLayer::Host, cursor);
}

namespace {
void setCanvasInteractionEnabledForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                                  bool enabled) {
    displaySession.forEachOverlay([enabled](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay->canvas() != nullptr) {
            overlay->canvas()->setInteractionEnabled(enabled);
        }
    });
}

void setCanvasToolForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                    ClipCanvasTool tool) {
    displaySession.forEachOverlay([tool](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay->canvas() != nullptr) {
            overlay->canvas()->setCanvasTool(tool);
        }
    });
}
} // namespace

void ScreenshotOverlayCanvasPresenter::setCanvasInteractionEnabled(
    const ScreenshotDisplaySession& displaySession, bool enabled) const {
    setCanvasInteractionEnabledForDisplaySession(displaySession, enabled);
}

void ScreenshotOverlayCanvasPresenter::setCanvasTool(const ScreenshotDisplaySession& displaySession,
                                                     ClipCanvasTool tool) const {
    setCanvasToolForDisplaySession(displaySession, tool);
}

void ScreenshotOverlayCanvasPresenter::refreshCanvasCreationStyles(
    const ScreenshotDisplaySession& displaySession, const ClipCanvasStyleDefaults& defaults) const {
    displaySession.forEachOverlay([&defaults](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay != nullptr && overlay->canvas() != nullptr) {
            clip_shot::presentation::applyScreenshotCanvasToolStyles(*overlay->canvas(), defaults);
        }
    });
}

bool ScreenshotOverlayCanvasPresenter::resetEditingState(
    const ScreenshotDisplaySession& displaySession) const {
    bool reset = true;
    displaySession.forEachOverlay([&reset](qsizetype, ScreenshotOverlayWindow* overlay) {
        ClipCanvasWidget* canvas = overlay->canvas();
        if (canvas != nullptr && !canvas->resetEditingState()) {
            reset = false;
        }
    });
    return reset;
}

namespace {
bool tryCurrentRectangleStyleForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                               ClipCanvasShapeStyle* outStyle) {
    if (outStyle == nullptr) {
        return false;
    }

    bool found = false;
    displaySession.forEachActiveOverlay(
        [&](qsizetype, const CapturedDisplayModel&, ScreenshotOverlayWindow* overlay) {
            if (found || overlay->canvas() == nullptr) {
                return;
            }
            *outStyle = overlay->canvas()->canvasStyleToolbarState().shapeStyle;
            found = true;
        });
    if (found) {
        return true;
    }

    displaySession.forEachOverlay([&](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (found || overlay->canvas() == nullptr) {
            return;
        }
        *outStyle = overlay->canvas()->canvasStyleToolbarState().shapeStyle;
        found = true;
    });
    return found;
}
} // namespace

bool ScreenshotOverlayCanvasPresenter::tryCurrentRectangleStyle(
    const ScreenshotDisplaySession& displaySession, ClipCanvasShapeStyle* outStyle) const {
    return tryCurrentRectangleStyleForDisplaySession(displaySession, outStyle);
}

namespace {
ClipCanvasShapeStyle
currentRectangleStyleForDisplaySession(const ScreenshotDisplaySession& displaySession) {
    ClipCanvasShapeStyle style{};
    if (tryCurrentRectangleStyleForDisplaySession(displaySession, &style)) {
        return style;
    }
    return {};
}
} // namespace

ClipCanvasShapeStyle ScreenshotOverlayCanvasPresenter::currentRectangleStyle(
    const ScreenshotDisplaySession& displaySession) const {
    return currentRectangleStyleForDisplaySession(displaySession);
}

void ScreenshotOverlayCanvasPresenter::setShapeStylePatch(
    const ScreenshotDisplaySession& displaySession, const ClipCanvasShapeStyle& style,
    quint32 properties, ClipCanvasShapeKind kind) const {
    displaySession.forEachOverlay(
        [&style, properties, kind](qsizetype, ScreenshotOverlayWindow* overlay) {
            if (overlay != nullptr && overlay->canvas() != nullptr) {
                overlay->canvas()->setCanvasShapeStylePatch(style, properties, kind);
            }
        });
}

void ScreenshotOverlayCanvasPresenter::setFilterStyle(
    const ScreenshotDisplaySession& displaySession, const ClipCanvasFilterStyle& style,
    quint32 properties) const {
    displaySession.forEachOverlay(
        [&style, properties](qsizetype, ScreenshotOverlayWindow* overlay) {
            if (overlay != nullptr && overlay->canvas() != nullptr) {
                overlay->canvas()->setCanvasFilterStyle(style, properties);
            }
        });
}

void ScreenshotOverlayCanvasPresenter::setWatermarkConfig(
    const ScreenshotDisplaySession& displaySession, const ClipCanvasWatermarkConfig& config) const {
    displaySession.forEachOverlay([&config](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay != nullptr && overlay->canvas() != nullptr) {
            overlay->canvas()->setCanvasWatermarkConfig(config);
        }
    });
}

void ScreenshotOverlayCanvasPresenter::previewWatermarkConfig(
    const ScreenshotDisplaySession& displaySession, const ClipCanvasWatermarkConfig& config) const {
    displaySession.forEachOverlay([&config](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay != nullptr && overlay->canvas() != nullptr) {
            overlay->canvas()->previewCanvasWatermarkConfig(config);
        }
    });
}

void ScreenshotOverlayCanvasPresenter::setSpotlightConfig(
    ScreenshotDisplaySession& displaySession, const ClipCanvasSpotlightConfig& config) const {
    displaySession.forEachOverlay([&config](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay != nullptr && overlay->canvas() != nullptr) {
            overlay->canvas()->setCanvasSpotlightConfig(config);
        }
    });
}

void ScreenshotOverlayCanvasPresenter::previewSpotlightConfig(
    ScreenshotDisplaySession& displaySession, const ClipCanvasSpotlightConfig& config) const {
    displaySession.forEachOverlay([&config](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay != nullptr && overlay->canvas() != nullptr) {
            overlay->canvas()->previewCanvasSpotlightConfig(config);
        }
    });
}

void ScreenshotOverlayCanvasPresenter::setTextStyle(const ScreenshotDisplaySession& displaySession,
                                                    const ClipCanvasTextStyle& style,
                                                    quint32 properties) const {
    displaySession.forEachOverlay(
        [&style, properties](qsizetype, ScreenshotOverlayWindow* overlay) {
            if (overlay != nullptr && overlay->canvas() != nullptr) {
                static_cast<void>(overlay->canvas()->setCanvasTextStyle(style, properties));
            }
        });
}

void ScreenshotOverlayCanvasPresenter::setSerialNumberStyle(
    const ScreenshotDisplaySession& displaySession, const ClipCanvasSerialNumberStyle& style,
    std::optional<quint32> properties) const {
    displaySession.forEachOverlay(
        [&style, properties](qsizetype, ScreenshotOverlayWindow* overlay) {
            if (overlay != nullptr && overlay->canvas() != nullptr) {
                if (properties) {
                    static_cast<void>(overlay->canvas()->applyStyleEdit(
                        ClipCanvasSerialNumberEdit{style, *properties}));
                } else {
                    static_cast<void>(overlay->canvas()->setCanvasSerialNumberStyle(style));
                }
            }
        });
}

namespace {
void adjustSelectedSerialNumbersForDisplaySession(const ScreenshotDisplaySession& displaySession,
                                                  qint64 delta) {
    displaySession.forEachOverlay([delta](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay->canvas() != nullptr) {
            overlay->canvas()->adjustSelectedSerialNumbers(delta);
        }
    });
}
} // namespace

void ScreenshotOverlayCanvasPresenter::adjustSelectedSerialNumbers(
    const ScreenshotDisplaySession& displaySession, qint64 delta) const {
    adjustSelectedSerialNumbersForDisplaySession(displaySession, delta);
}

namespace {
void createTextForSelectedSerialNumberForDisplaySession(
    const ScreenshotDisplaySession& displaySession) {
    displaySession.forEachOverlay([](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (overlay->canvas() != nullptr) {
            overlay->canvas()->createSerialNumberText();
        }
    });
}
} // namespace

void ScreenshotOverlayCanvasPresenter::createTextForSelectedSerialNumber(
    const ScreenshotDisplaySession& displaySession) const {
    createTextForSelectedSerialNumberForDisplaySession(displaySession);
}

void ScreenshotOverlayCanvasPresenter::reorderSelectedElements(
    const ScreenshotDisplaySession& displaySession, ClipCanvasSelectionOrder order) const {
    bool handled = false;
    displaySession.forEachOverlay([order, &handled](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (!handled && overlay != nullptr && overlay->canvas() != nullptr) {
            static_cast<void>(overlay->canvas()->reorderSelected(order));
            handled = true;
        }
    });
}

void ScreenshotOverlayCanvasPresenter::alignSelectedElements(
    const ScreenshotDisplaySession& displaySession, ClipCanvasSelectionAlignment alignment) const {
    bool handled = false;
    displaySession.forEachOverlay(
        [alignment, &handled](qsizetype, ScreenshotOverlayWindow* overlay) {
            if (!handled && overlay != nullptr && overlay->canvas() != nullptr) {
                static_cast<void>(overlay->canvas()->alignSelected(alignment));
                handled = true;
            }
        });
}

void ScreenshotOverlayCanvasPresenter::setSelectedElementsOpacity(
    const ScreenshotDisplaySession& displaySession, qreal opacity) const {
    bool handled = false;
    displaySession.forEachOverlay([opacity, &handled](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (!handled && overlay != nullptr && overlay->canvas() != nullptr) {
            static_cast<void>(overlay->canvas()->setSelectedOpacity(opacity));
            handled = true;
        }
    });
}

void ScreenshotOverlayCanvasPresenter::duplicateSelectedElements(
    const ScreenshotDisplaySession& displaySession) const {
    bool handled = false;
    displaySession.forEachOverlay([&handled](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (!handled && overlay != nullptr && overlay->canvas() != nullptr) {
            static_cast<void>(overlay->canvas()->duplicateSelected());
            handled = true;
        }
    });
}

void ScreenshotOverlayCanvasPresenter::deleteSelectedElements(
    const ScreenshotDisplaySession& displaySession) const {
    bool handled = false;
    displaySession.forEachOverlay([&handled](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (!handled && overlay != nullptr && overlay->canvas() != nullptr) {
            static_cast<void>(overlay->canvas()->deleteSelected());
            handled = true;
        }
    });
}

void ScreenshotOverlayCanvasPresenter::deleteAllElements(
    const ScreenshotDisplaySession& displaySession) const {
    bool handled = false;
    displaySession.forEachOverlay([&handled](qsizetype, ScreenshotOverlayWindow* overlay) {
        if (!handled && overlay != nullptr && overlay->canvas() != nullptr) {
            static_cast<void>(overlay->canvas()->deleteAllElements());
            handled = true;
        }
    });
}
