#include "screenshotoverlayframepresenter.h"

#include "../capture/screenshotcaptureperfinstrumentation.h"

#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QList>
#include <QtNumeric>
#include <QWidget>

#if defined(Q_OS_WIN) || defined(_WIN32)
#include <qt_windows.h>
#endif

namespace {
[[maybe_unused]] constexpr auto kRevealStrategyEnvironment = "CLIP_SHOT_CAPTURE_REVEAL_STRATEGY";

class ShowPaintSuppression final : public QObject {
  public:
    ShowPaintSuppression(QWidget& window, bool suppressNativeRedraw,
                         bool filterUpdateRequests = false) {
        if (!suppressNativeRedraw && !filterUpdateRequests) {
            return;
        }

        {
            CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.update_filter_install");
            CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.update_filter_install_requests", 1);
            window.installEventFilter(this);
        }
        m_filteredWindow = &window;

#if defined(Q_OS_WIN) || defined(_WIN32)
        if (!suppressNativeRedraw || QGuiApplication::platformName() != QStringLiteral("windows")) {
            return;
        }

        const HWND hwnd = reinterpret_cast<HWND>(window.winId());
        if (hwnd == nullptr) {
            return;
        }

        {
            CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.redraw_suppress");
            CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.redraw_suppress_requests", 1);
            static_cast<void>(SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0));
        }
        m_hwnd = hwnd;
#else
        Q_UNUSED(window);
#endif
    }

    ~ShowPaintSuppression() {
        restore();
    }

    ShowPaintSuppression(const ShowPaintSuppression&) = delete;
    ShowPaintSuppression& operator=(const ShowPaintSuppression&) = delete;

    void restoreNativeRedraw() {
#if defined(Q_OS_WIN) || defined(_WIN32)
        if (m_hwnd != nullptr) {
            const HWND hwnd = m_hwnd;
            m_hwnd = nullptr;
            CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.redraw_restore");
            CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.redraw_restore_requests", 1);
            static_cast<void>(SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0));
        }
#endif
    }

    void discardDeferredUpdateRequests() {
        if (m_filteredWindow == nullptr) {
            return;
        }

        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.update_filter_drain");
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.update_filter_drain_requests", 1);
        QCoreApplication::sendPostedEvents(m_filteredWindow->window(), QEvent::UpdateRequest);
    }

    void restore() {
        restoreNativeRedraw();

        if (m_filteredWindow != nullptr) {
            QWidget* window = m_filteredWindow;
            m_filteredWindow = nullptr;
            {
                CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.update_filter_remove");
                CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.update_filter_remove_requests",
                                               1);
                window->removeEventFilter(this);
            }
        }
    }

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_filteredWindow && event != nullptr &&
            event->type() == QEvent::UpdateRequest) {
            CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.update_requests_suppressed", 1);
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

  private:
    QWidget* m_filteredWindow = nullptr;
#if defined(Q_OS_WIN) || defined(_WIN32)
    HWND m_hwnd = nullptr;
#endif
};

ScreenshotOverlayRevealStrategy configuredRevealStrategy() {
#if defined(Q_OS_WIN) || defined(_WIN32)
    constexpr auto defaultStrategy = ScreenshotOverlayRevealStrategy::NativeInvalidateSuppressed;
#else
    constexpr auto defaultStrategy = ScreenshotOverlayRevealStrategy::SingleRepaint;
#endif
#if defined(CLIP_SHOT_CAPTURE_PERF_INSTRUMENTATION) || defined(CLIP_SHOT_BENCH_INTERNALS)
    return ScreenshotOverlayFramePresenter::strategyForName(qgetenv(kRevealStrategyEnvironment),
                                                            defaultStrategy);
#else
    return defaultStrategy;
#endif
}

bool shouldConcealFirstPaint(const QWidget& window) {
    return !window.isVisible() && window.isWindow() &&
           QGuiApplication::platformName() == QStringLiteral("windows");
}

void recordStrategy(ScreenshotOverlayRevealStrategy strategy) {
    switch (strategy) {
    case ScreenshotOverlayRevealStrategy::SingleRepaint:
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.strategy.single_repaint", 1);
        break;
    case ScreenshotOverlayRevealStrategy::PostedUpdate:
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.strategy.posted_update", 1);
        break;
    case ScreenshotOverlayRevealStrategy::NativeUpdate:
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.strategy.native_update", 1);
        break;
    case ScreenshotOverlayRevealStrategy::NativeInvalidate:
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.strategy.native_invalidate", 1);
        break;
    case ScreenshotOverlayRevealStrategy::NativeInvalidateSuppressed:
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.strategy.native_invalidate_suppressed",
                                       1);
        break;
    }
}

void enableWidgetTreeUpdates(QWidget& window) {
    window.setUpdatesEnabled(true);
    const QList<QWidget*> children = window.findChildren<QWidget*>();
    for (QWidget* child : children) {
        child->setUpdatesEnabled(true);
    }
}

} // namespace

ScreenshotOverlayFramePresenter::ScreenshotOverlayFramePresenter(QWidget& window)
    : m_window(window), m_strategy(configuredRevealStrategy()) {}

void ScreenshotOverlayFramePresenter::warmPresentationSurface() {
#if defined(Q_OS_WIN) || defined(_WIN32)
    if (QGuiApplication::platformName() != QStringLiteral("windows") || m_window.isVisible()) {
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.surface_warm_skipped", 1);
        return;
    }

    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.surface_warm");
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.surface_warm_requests", 1);
    recordStrategy(m_strategy);

    const ScreenshotOverlayRevealPlan plan = planFor(m_strategy);
    {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.surface_warm.opacity_conceal");
        m_window.setWindowOpacity(0.0);
    }
    ShowPaintSuppression paintSuppression(m_window, plan.suppressShowPaint);
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.surface_warm.redraw_suppressed");
    {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.surface_warm.show");
        m_window.show();
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.surface_warm.show_returned");
    paintSuppression.restoreNativeRedraw();
    {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.surface_warm.commit");
        commitPreparedSurface(plan);
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.surface_warm.commit_done");
    paintSuppression.discardDeferredUpdateRequests();
    paintSuppression.restore();
    m_window.setUpdatesEnabled(false);
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.surface_warmed");
#endif
}

void ScreenshotOverlayFramePresenter::presentPreparedFrame(bool deferFirstPaint) {
    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.sync_reveal");
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.windows_shown", 1);
    recordStrategy(m_strategy);

    if (deferFirstPaint) {
        presentFramePaced();
        return;
    }

    const ScreenshotOverlayRevealPlan plan = planFor(m_strategy);
    const bool alreadyVisible = m_window.isVisible();
    const bool concealFirstPaint = shouldConcealFirstPaint(m_window);
    const qreal previousOpacity = m_window.windowOpacity();
    const bool restoreWarmedOpacity = alreadyVisible && qFuzzyIsNull(previousOpacity);
    if (concealFirstPaint) {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.opacity_conceal");
        m_window.setWindowOpacity(0.0);
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.concealed");

    ShowPaintSuppression paintSuppression(m_window, concealFirstPaint && plan.suppressShowPaint);
    if (alreadyVisible) {
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.show_skipped_warmed", 1);
        enableWidgetTreeUpdates(m_window);
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.redraw_suppressed");
    if (!alreadyVisible) {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.show");
        m_window.show();
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.show_returned");
    paintSuppression.restoreNativeRedraw();
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.redraw_restored");

    {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.surface_commit");
        if (alreadyVisible) {
            // Re-enabling Qt updates queues the complete image and selection paint.
            // Commit it before revealing the warmed backing store; native redraw
            // alone does not deliver the pending Qt UpdateRequest.
            sendPostedUpdate();
        } else {
            commitPreparedSurface(plan);
        }
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.surface_commit_done");
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.redraw_done");
    paintSuppression.discardDeferredUpdateRequests();
    paintSuppression.restore();
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.update_filter_removed");

    if (concealFirstPaint) {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.opacity_restore");
        m_window.setWindowOpacity(previousOpacity);
    } else if (restoreWarmedOpacity) {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.opacity_restore");
        m_window.setWindowOpacity(1.0);
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.opacity_restored");
}

void ScreenshotOverlayFramePresenter::presentFramePaced() {
    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.frame_paced_reveal");
    const bool alreadyVisible = m_window.isVisible();
    if (alreadyVisible) {
        // A warmed surface re-enables widget updates; the queued UpdateRequest
        // publishes the prepared frame at the next event-loop pass.
        enableWidgetTreeUpdates(m_window);
        if (qFuzzyIsNull(m_window.windowOpacity())) {
            CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.opacity_restore");
            m_window.setWindowOpacity(1.0);
        }
        CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.frame_paced_warmed");
        return;
    }
    // A fresh translucent layered surface stays invisible until its first
    // paint, and the freshly captured frame mirrors the desktop beneath it,
    // so showing without a synchronous commit costs at most a transparent
    // frame while the queued first paint interleaves with drag input.
    {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.show");
        m_window.show();
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.frame_paced_shown");
}

#if defined(CLIP_SHOT_BENCH_INTERNALS)
void ScreenshotOverlayFramePresenter::setStrategyForTesting(
    ScreenshotOverlayRevealStrategy strategy) {
    m_strategy = strategy;
}
#endif

ScreenshotOverlayRevealStrategy
ScreenshotOverlayFramePresenter::strategyForName(const QByteArray& name,
                                                 ScreenshotOverlayRevealStrategy fallback) {
    const QByteArray normalized = name.trimmed().toLower();
    if (normalized == QByteArrayLiteral("single-repaint") ||
        normalized == QByteArrayLiteral("single_repaint") ||
        normalized == QByteArrayLiteral("repaint")) {
        return ScreenshotOverlayRevealStrategy::SingleRepaint;
    }
    if (normalized == QByteArrayLiteral("posted-update") ||
        normalized == QByteArrayLiteral("posted_update")) {
        return ScreenshotOverlayRevealStrategy::PostedUpdate;
    }
    if (normalized == QByteArrayLiteral("native-update") ||
        normalized == QByteArrayLiteral("native_update")) {
        return ScreenshotOverlayRevealStrategy::NativeUpdate;
    }
    if (normalized == QByteArrayLiteral("native-invalidate") ||
        normalized == QByteArrayLiteral("native_invalidate")) {
        return ScreenshotOverlayRevealStrategy::NativeInvalidate;
    }
    if (normalized == QByteArrayLiteral("native-invalidate-suppressed") ||
        normalized == QByteArrayLiteral("native_invalidate_suppressed")) {
        return ScreenshotOverlayRevealStrategy::NativeInvalidateSuppressed;
    }
    return fallback;
}

ScreenshotOverlayRevealPlan
ScreenshotOverlayFramePresenter::planFor(ScreenshotOverlayRevealStrategy strategy) {
    ScreenshotOverlayRevealPlan plan;
    switch (strategy) {
    case ScreenshotOverlayRevealStrategy::SingleRepaint:
        plan.repaint = true;
        break;
    case ScreenshotOverlayRevealStrategy::PostedUpdate:
        plan.sendPostedUpdate = true;
        break;
    case ScreenshotOverlayRevealStrategy::NativeUpdate:
        plan.nativeUpdate = true;
        break;
    case ScreenshotOverlayRevealStrategy::NativeInvalidate:
        plan.nativeInvalidate = true;
        break;
    case ScreenshotOverlayRevealStrategy::NativeInvalidateSuppressed:
        plan.suppressShowPaint = true;
        plan.nativeInvalidate = true;
        break;
    }
    return plan;
}

void ScreenshotOverlayFramePresenter::commitPreparedSurface(
    const ScreenshotOverlayRevealPlan& plan) {
    if (plan.repaint) {
        repaintSurface();
    }
    if (plan.sendPostedUpdate) {
        sendPostedUpdate();
    }
    if (plan.nativeUpdate) {
        updateNativeSurface(false);
    }
    if (plan.nativeInvalidate) {
        updateNativeSurface(true);
    }
}

void ScreenshotOverlayFramePresenter::repaintSurface() {
    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.commit.repaint");
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.surface_commit_requests", 1);
    m_window.repaint();
}

void ScreenshotOverlayFramePresenter::sendPostedUpdate() {
    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.commit.posted_update");
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.surface_commit_requests", 1);
    QCoreApplication::sendPostedEvents(m_window.window(), QEvent::UpdateRequest);
}

void ScreenshotOverlayFramePresenter::updateNativeSurface(bool invalidate) {
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.surface_commit_requests", 1);
#if defined(Q_OS_WIN) || defined(_WIN32)
    if (QGuiApplication::platformName() != QStringLiteral("windows")) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(m_window.winId());
    if (hwnd == nullptr) {
        return;
    }

    if (invalidate) {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.commit.native_invalidate");
        static_cast<void>(
            RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN));
        return;
    }

    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.commit.native_update");
    static_cast<void>(RedrawWindow(hwnd, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN));
#else
    Q_UNUSED(invalidate);
#endif
}
