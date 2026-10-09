#include "clip_canvas_display_item.h"
#include "clip_canvas_render_geometry.h"
#include "clip_canvas_renderer.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QRegion>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

constexpr int kSurfaceWidth = 1920;
constexpr int kSurfaceHeight = 1080;
constexpr int kWarmupIterations = 3;
constexpr int kMeasuredIterations = 20;

std::vector<ClipCanvasSceneItem> makeRectangles(int count, ClipFillStyle fillStyle) {
    std::vector<ClipCanvasSceneItem> items;
    items.reserve(count);
    const int columns = static_cast<int>(std::ceil(std::sqrt(count * 16.0 / 9.0)));
    const int rows = (count + columns - 1) / columns;
    const double cellWidth = static_cast<double>(kSurfaceWidth) / columns;
    const double cellHeight = static_cast<double>(kSurfaceHeight) / rows;
    for (int index = 0; index < count; ++index) {
        const int column = index % columns;
        const int row = index / columns;
        ClipSceneDisplayItem view{};
        view.kind = CLIP_SCENE_DISPLAY_ITEM_DRAW_RECT;
        view.element_id = ClipElementId{
            static_cast<std::uint32_t>(index),
            1,
        };
        view.center_x = -kSurfaceWidth / 2.0 + (column + 0.5) * cellWidth;
        view.center_y = -kSurfaceHeight / 2.0 + (row + 0.5) * cellHeight;
        view.width = std::max(2.0, cellWidth * 0.8);
        view.height = std::max(2.0, cellHeight * 0.8);
        view.rotation = index % 7 == 0 ? 0.08 : 0.0;
        view.fill = ClipColorRgba8{40, 130, 210, 190};
        view.fill_style = fillStyle;
        view.stroke = ClipColorRgba8{20, 40, 60, 255};
        view.stroke_width = 1.5;
        if (index % 5 == 0) {
            view.corner_radii = ClipCornerRadii{3.0, 3.0, 3.0, 3.0};
        }
        items.emplace_back(view);
    }
    return items;
}

double percentile(std::vector<double> values, double fraction) {
    std::sort(values.begin(), values.end());
    const std::size_t index = std::min(
        values.size() - 1, static_cast<std::size_t>(std::ceil(values.size() * fraction) - 1));
    return values[index];
}

void renderOnce(QImage& image, const std::vector<ClipCanvasSceneItem>& items,
                const SceneDisplayInfo& displayInfo, const QRegion& region,
                const std::vector<std::uint32_t>& candidates) {
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRegion(region);
    clip_canvas_renderer::renderSceneItems(clip_canvas_renderer::SceneRenderRequest{
        &painter,
        &displayInfo,
        items.data(),
        static_cast<std::uint32_t>(items.size()),
        region,
        candidates.empty() ? nullptr : candidates.data(),
        static_cast<std::uint32_t>(candidates.size()),
    });
}

void benchmarkRender(int count, ClipFillStyle fillStyle, std::string_view name,
                     const QRegion& region) {
    std::vector<ClipCanvasSceneItem> items = makeRectangles(count, fillStyle);
    SceneDisplayInfo displayInfo{};
    displayInfo.item_count = static_cast<std::uint32_t>(items.size());
    displayInfo.surface_width = kSurfaceWidth;
    displayInfo.surface_height = kSurfaceHeight;
    displayInfo.camera_zoom = 1.0;
    QImage image(kSurfaceWidth, kSurfaceHeight, QImage::Format_ARGB32_Premultiplied);
    std::vector<std::uint32_t> candidates;
    if (region.boundingRect().size() != image.size()) {
        for (std::uint32_t index = 0; index < items.size(); ++index) {
            const QRectF bounds =
                clip_canvas_render_geometry::sceneItemBounds(displayInfo, items[index]);
            if (region.intersects(bounds.toAlignedRect())) {
                candidates.push_back(index);
            }
        }
    }
    for (int iteration = 0; iteration < kWarmupIterations; ++iteration) {
        renderOnce(image, items, displayInfo, region, candidates);
    }

    std::vector<double> milliseconds;
    milliseconds.reserve(kMeasuredIterations);
    for (int iteration = 0; iteration < kMeasuredIterations; ++iteration) {
        QElapsedTimer timer;
        timer.start();
        renderOnce(image, items, displayInfo, region, candidates);
        milliseconds.push_back(timer.nsecsElapsed() / 1'000'000.0);
    }
    std::cout << "render count=" << count << " fill=" << name
              << " region=" << (region.boundingRect().size() == image.size() ? "full" : "64x64")
              << " median_ms=" << percentile(milliseconds, 0.5)
              << " p95_ms=" << percentile(milliseconds, 0.95) << '\n';
}

void benchmarkMiddleReplacement() {
    std::vector<ClipCanvasSceneItem> items = makeRectangles(10'000, CLIP_FILL_STYLE_SOLID);
    ClipCanvasSceneItem replacement = items[items.size() / 2];
    QElapsedTimer timer;
    timer.start();
    for (int iteration = 0; iteration < 10'000; ++iteration) {
        replacement.center_x += 0.001;
        items[items.size() / 2] = replacement;
    }
    std::cout << "patch count=10000 replacements=10000 total_ms="
              << timer.nsecsElapsed() / 1'000'000.0 << '\n';
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    std::cout << "abi scene_bytes=" << sizeof(ClipSceneDisplayItem)
              << " overlay_bytes=" << sizeof(ClipOverlayDisplayItem) << '\n';
    const QRegion full(QRect(0, 0, kSurfaceWidth, kSurfaceHeight));
    const QRegion partial(QRect(kSurfaceWidth / 2, kSurfaceHeight / 2, 64, 64));
    for (int count : {1'000, 10'000}) {
        benchmarkRender(count, CLIP_FILL_STYLE_SOLID, "solid", full);
        benchmarkRender(count, CLIP_FILL_STYLE_LINE, "line", full);
        benchmarkRender(count, CLIP_FILL_STYLE_CROSS_LINE, "cross", full);
        benchmarkRender(count, CLIP_FILL_STYLE_SOLID, "solid", partial);
        benchmarkRender(count, CLIP_FILL_STYLE_LINE, "line", partial);
        benchmarkRender(count, CLIP_FILL_STYLE_CROSS_LINE, "cross", partial);
    }
    benchmarkMiddleReplacement();
    return 0;
}
