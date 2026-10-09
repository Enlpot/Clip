#include "clip_shot/presentation/selectedtexttranslationcontroller.h"

#include "clip_selected_text.h"

namespace clip_shot::presentation {
namespace {
// Existing platforms retain the established clipboard-only path and its shorter budget.
// macOS keeps the FFI defaults so Accessibility runs before guarded clipboard fallback.
#if !defined(Q_OS_MACOS)
constexpr uint32_t kClipboardCaptureTimeoutMs = 800;
#endif

SelectedTextStatus errorStatus(uint32_t kind) {
    switch (kind) {
    case CLIP_SELECTED_TEXT_ERROR_BUSY:
        return SelectedTextStatus::Busy;
    case CLIP_SELECTED_TEXT_ERROR_TIMED_OUT:
        return SelectedTextStatus::TimedOut;
    case CLIP_SELECTED_TEXT_ERROR_UNSUPPORTED_PLATFORM:
        return SelectedTextStatus::Unsupported;
    default:
        return SelectedTextStatus::Failed;
    }
}

class NativeSelectedTextCaptureBackend final : public SelectedTextCaptureBackend {
  public:
    ~NativeSelectedTextCaptureBackend() override {
        cancel();
    }

    SelectedTextCaptureResult start() override {
        if (!m_service) {
            ClipSelectedTextService* service = nullptr;
            const auto error = clip_selected_text_service_create(&service, nullptr);
            m_service.reset(service);
            if (error != 0) {
                return {errorStatus(error), {}};
            }
        }
        ClipSelectedTextOptions options{};
        if (!clip_selected_text_options_init(&options)) {
            return {SelectedTextStatus::Failed, {}};
        }
#if !defined(Q_OS_MACOS)
        options.strategy = CLIP_SELECTED_TEXT_STRATEGY_CLIPBOARD;
        options.timeout_ms = kClipboardCaptureTimeoutMs;
#endif
        ClipSelectedTextRequest* request = nullptr;
        const auto error = clip_selected_text_start(m_service.get(), &options, &request, nullptr);
        m_request.reset(request);
        return {error == 0 ? SelectedTextStatus::Pending : errorStatus(error), {}};
    }

    SelectedTextCaptureResult poll() override {
        const auto status = clip_selected_text_request_poll(m_request.get());
        if (status == CLIP_SELECTED_TEXT_PENDING) {
            return {};
        }
        ClipSelectedTextResult* rawResult = nullptr;
        const auto outcome = clip_selected_text_request_result(m_request.get(), &rawResult);
        const std::unique_ptr<ClipSelectedTextResult, decltype(&clip_selected_text_result_destroy)>
            result(rawResult, clip_selected_text_result_destroy);
        m_request.reset();
        switch (outcome) {
        case CLIP_SELECTED_TEXT_SELECTED: {
            ClipSelectedTextBytes text{};
            if (clip_selected_text_result_text(result.get(), &text)) {
                return {SelectedTextStatus::Selected,
                        QString::fromUtf8(reinterpret_cast<const char*>(text.data),
                                          static_cast<qsizetype>(text.length))};
            }
            break;
        }
        case CLIP_SELECTED_TEXT_NO_SELECTION:
            return {SelectedTextStatus::NoSelection, {}};
        case CLIP_SELECTED_TEXT_UNSUPPORTED:
            return {SelectedTextStatus::Unsupported, {}};
        default:
            ClipSelectedTextError error{};
            if (clip_selected_text_result_error(result.get(), &error)) {
                return {errorStatus(error.kind), {}};
            }
            break;
        }
        return {SelectedTextStatus::Failed, {}};
    }

    void cancel() override {
        if (m_request) {
            clip_selected_text_request_cancel(m_request.get());
            m_request.reset();
        }
    }

  private:
    std::unique_ptr<ClipSelectedTextService, decltype(&clip_selected_text_service_destroy)>
        m_service{nullptr, clip_selected_text_service_destroy};
    std::unique_ptr<ClipSelectedTextRequest, decltype(&clip_selected_text_request_destroy)>
        m_request{nullptr, clip_selected_text_request_destroy};
};
} // namespace

SelectedTextTranslationController::SelectedTextTranslationController(QObject* parent)
    : SelectedTextTranslationController(std::make_unique<NativeSelectedTextCaptureBackend>(),
                                        parent) {}

SelectedTextTranslationController::SelectedTextTranslationController(
    std::unique_ptr<SelectedTextCaptureBackend> backend, QObject* parent)
    : QObject(parent), m_backend(std::move(backend)) {
    m_pollTimer.setInterval(20);
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() { acceptResult(m_backend->poll()); });
}

SelectedTextTranslationController::~SelectedTextTranslationController() {
    shutdown();
}

void SelectedTextTranslationController::capture() {
    if (m_shutdown || m_pending) {
        return;
    }
    m_pending = true;
    m_pollTimer.start();
    acceptResult(m_backend->start());
}

void SelectedTextTranslationController::shutdown() {
    if (m_shutdown) {
        return;
    }
    m_shutdown = true;
    cancel();
}

void SelectedTextTranslationController::cancel() {
    m_pending = false;
    m_pollTimer.stop();
    m_backend->cancel();
}

void SelectedTextTranslationController::acceptResult(const SelectedTextCaptureResult& result) {
    if (m_shutdown || !m_pending || result.status == SelectedTextStatus::Pending) {
        return;
    }
    m_pollTimer.stop();
    m_pending = false;
    // Every completed capture hands off, even with empty text; the destination opens the
    // translation page and warns there when nothing was retrieved.
    emit textReady(result.text);
}
} // namespace clip_shot::presentation
