# Clip Canvas spotlight benchmark

`clip-canvas-spotlight-benchmark` measures the dedicated spotlight decoration renderer. CSV output
records p50/p95/p99 timing, processed and culled cutouts, early exits, fast paths, output
checksums, and environment metadata.

## Build and run

```powershell
cmake -S clip_draw_engine_qt -B clip_draw_engine_qt/build-perf `
  -DCLIP_DRAW_ENGINE_QT_BUILD_DEMO=OFF `
  -DCLIP_DRAW_ENGINE_QT_BUILD_TESTS=ON `
  -DCLIP_DRAW_ENGINE_QT_BUILD_BENCHMARKS=ON
cmake --build clip_draw_engine_qt/build-perf --config Release `
  --target clip-canvas-spotlight-benchmark

$env:QT_QPA_PLATFORM = 'offscreen'
clip_draw_engine_qt/build-perf/Release/clip-canvas-spotlight-benchmark.exe --list
clip_draw_engine_qt/build-perf/Release/clip-canvas-spotlight-benchmark.exe `
  --warmup 10 --iterations 300 `
  --csv clip_draw_engine_qt/build-perf/spotlight-results.csv
```

The stable scenario matrix covers 1920x1080 and 3840x2160, DPR 1, 1.25, and 2, and 1, 16, and
128 rotated cutouts. Additional scenarios cover fragmented and bounded exposure, opacity and color
preview bursts, zero visible cutouts, fractional geometry, small geometry changes, camera changes,
and render-area changes.

Functional or diagnostics mismatches return a nonzero exit code; timing values never do.
