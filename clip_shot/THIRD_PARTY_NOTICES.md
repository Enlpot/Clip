# Clip Third-Party Notices

Clip incorporates third-party software and assets under their respective
licenses. The release build generates a complete, versioned notice bundle from
the resolved Rust dependency graph, installed vcpkg packages, the audited
static Qt build, and repository-owned attribution files.

Installed releases place that bundle under:

```text
share/clip-shot/licenses/third-party/
```

`INDEX.md` in that directory records every collected package and source
license file. The bundle includes the Ant Design Icons MIT notice from
`ant_design_qt/THIRD_PARTY_NOTICES.md`.

The Qt 6.12 LTS toolchain is pinned by `scripts/qt-toolchain.json`. The Qt notice
bundle preserves the source archive version and digest, license texts, and
upstream licensing metadata for Qt Base, Qt SVG, Qt Tools, and Qt Translations.
Qt Translations uses `licenseRule.json`; the other modules use REUSE metadata.

Screen color restoration uses nalgebra (Apache-2.0) for fixed-size matrix
inversion and validation. Its license and resolved dependencies are included
in the generated Rust dependency notice bundle.

The OCR worker uses `ort` and `ort-sys` 2.0.0-rc.13 (MIT OR Apache-2.0), pinned to
revision `90018ee581fb1bb1baf8e82eba8e679728ff27dc` of
https://github.com/mg-chao/ort with a native diagnostic-decoding and logger-category
fix. Cargo retrieves their source and original license files from that fork.
The release collector includes the selected Rust FFI and static OCR-worker
dependency graphs, including these Git dependencies.

The Windows asset manifest pins immutable OCR runtime 1.0.9. This raw-pixel
worker enables static ONNX Runtime, DirectML, and crash diagnostics. Its notice
collection follows that dependency closure; RapidOCR's CLI, encoded-image
decoding, HTTP/TLS model downloads, and YAML features are disabled.

RapidOCR's optional HTTP features use Reqwest with Rustls (Apache-2.0 OR ISC OR
MIT). They require Rustls 0.23.45 or a compatible newer release to address
GHSA-2mjx-qc3c-rqvc. Builds enabling those features include Rustls and its
resolved dependency licenses in the generated notice bundle.

The GPL-3.0-only `clip-shot-updater` sidecar is implemented in Rust and is
distributed as part of Clip. Its resolved normal and build dependency
graph—including Tokio, Reqwest with native platform TLS, Serde, RSA/SHA-256,
SemVer, ZIP/Deflate, and Microsoft windows-rs—is collected into the same
versioned notice bundle from its independently locked Cargo package.

The optional PP-OCRv4 and PP-OCRv5 detector/recognizer models and dictionaries
are redistributed from RapidAI/RapidOCR's ModelScope release `v3.9.2`, whose
model card declares Apache License 2.0:
https://www.modelscope.cn/models/RapidAI/RapidOCR/files?Revision=v3.9.2.
Clip downloads the matching mobile/server bundles from
https://www.modelscope.cn/models/mgchao/ClipShotOCR and pins their sizes and
SHA-256 hashes in `packaging/clip-shot-ocr-asset-manifest.json`.

Local crash diagnostics on Windows and macOS use Crashpad (Apache-2.0), pinned by the vcpkg baseline
and the repository's Crashpad overlay. The client, helper, Chromium base code,
and their bundled notices are included in the vcpkg license collection. The
Qt-independent OCR bridge and shared Rust panic hook are Apache-2.0 code under
`clip-crates/`.

In-process barcode recognition and Smart Erase reconstruction use OpenCV
(Apache-2.0), pinned by the vcpkg
baseline and built as static libraries with only the `wechat_qrcode` (contrib)
and `barcode` (objdetect) module closure; the dnn module embeds OpenCV's
bundled protobuf (BSD-3-Clause). Both licenses are included in the vcpkg
license collection. Smart Erase uses the existing `core` and `imgproc` modules
for deterministic multiscale patch reconstruction; it adds no model downloads or
additional OpenCV modules. The WeChat QR detection and super-resolution models are
redistributed from https://github.com/WeChatCV/opencv_3rdparty at revision
`a8b69ccc738421293254aec5ddb38bd523503252`, pinned by SHA-512 in
`cmake/FetchClipShotQrModels.cmake`.

Selected-text acquisition and its C bridge (`clip-selected-text` and
`clip-selected-text-c`) are Apache-2.0 code under `clip-crates/`. They use the
existing Microsoft windows-rs dependency (MIT OR Apache-2.0) for Windows UI
Automation, native edit controls, and clipboard interoperability. On macOS they
use accessibility-sys (MIT OR Apache-2.0), core-foundation (MIT OR Apache-2.0),
and the existing objc2 framework bindings (Zlib OR Apache-2.0 OR MIT) for
Accessibility and pasteboard interoperability. Their resolved notices are
included in the generated Rust dependency bundle. Apple system frameworks are
provided by macOS and are not redistributed.
Full includes these components in its unified Rust FFI archive; Mini's archive
does not enable the selected-text dependency.

macOS application QoS uses the repository's Apache-2.0 `clip-core` scheduling
policy and the existing Rayon worker pools (MIT OR Apache-2.0). Native pthread
and dispatch APIs are provided by macOS and are not redistributed. The resolved
Rayon notices remain included in the generated Rust dependency bundle.

Smart selection (`clip-ui-selector` and `clip-ui-selector-c`) uses the same
accessibility-sys, core-foundation, and core-foundation-sys dependencies
(MIT OR Apache-2.0) for macOS Accessibility and Quartz window snapshots.
ApplicationServices, CoreFoundation, and CoreGraphics are system frameworks and
are not redistributed. Windows selection continues to use Microsoft windows-rs.

Mouse and keyboard effect rendering and its C bridge (`clip-recording-effects`
and `clip-recording-effects-c`) are Apache-2.0 code under `clip-crates/`. They
reuse crossbeam-channel (MIT OR Apache-2.0) and Microsoft windows-rs (MIT OR
Apache-2.0) for bounded input observation and native font rendering. Their
resolved licenses are included in the generated Rust dependency bundle.

Shared recording source metadata and export settings reuse bincode 1.3.3 (MIT)
for versioned serialization. Its original notice is included in the resolved
Rust dependency bundle. Deferred HDR source encoding uses the existing x265
(GPL-2.0-or-later) dependency with both 8-bit and Main10 APIs; it adds no separate
codec library or license.

GPU screen recording uses the repository's Apache-2.0 `clip-d3d11` crate and
Microsoft windows-rs (MIT OR Apache-2.0). The restricted FFmpeg 9.0 build enables
native H.264 surfaces using AMD AMF headers 1.5.2 (MIT), NVIDIA nv-codec-headers
13.0.19.0 (MIT), and Intel oneVPL dispatcher 2.17.0 (MIT). The headers and
dispatcher notices are collected from the resolved vcpkg packages, including
the full copyright notices. Vendor display drivers supply the hardware codec
implementations; those drivers are not distributed with Clip.

The standalone macOS media libraries use objc2 framework bindings, block2,
and dispatch2 (MIT), and CoreText/CoreGraphics system font rendering. Apple's
ScreenCaptureKit, CoreVideo, Metal, CoreAudio, and VideoToolbox frameworks are
provided by macOS and are not redistributed. The macOS FFmpeg profile includes
x264 (GPL-2.0-or-later), x265 (GPL-2.0-or-later), WebP (BSD-3-Clause), and zlib-ng
(Zlib). The resulting FFmpeg binaries are GPL builds. Capture-only C libraries
do not link FFmpeg. Each architecture's resolved license bundle is generated
with `scripts/collect-third-party-licenses.ps1 -StandaloneMedia`.

Recording GIF regression tests use the `gif` crate 0.14.2 (MIT OR Apache-2.0)
as an independent decoder. It is a development dependency; the adaptive palette
encoder adds no production dependencies. The crate's original notices are in
the Cargo registry source used by the test build.

The generated bundle is authoritative for a particular binary because its
contents are produced from that build environment. Dependency licenses and
copyright notices remain the property of their respective owners.

## STranslate OCR Layout Analysis

The Smart Merge implementation in `src/presentation/ocr/screenshotocrlayout.cpp`
and its layout test fixtures are adapted from STranslate's `OcrLayoutAnalyzer.cs`
and `OcrLayoutAnalyzerTests.cs` (https://github.com/STranslate/STranslate).
Clip adapts the algorithm to Qt and retains unsupported OCR geometry as
original translation units. The following license applies to the adapted code:

The MIT License (MIT)

Copyright (c) 2022 zggsong zggsong@foxmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Excalidraw arrow icons

Arrow type and arrowhead SVG assets are adapted from
https://github.com/excalidraw/excalidraw, packages/excalidraw/components/icons.tsx.
Start arrowheads are horizontally mirrored for their endpoint.

MIT License

Copyright (c) 2020 Excalidraw

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## PixiJS Filters emboss effect

The Clip Draw Engine emboss filter is a CPU adaptation of the diagonal sampling formula in the
PixiJS Filters emboss shader:
https://github.com/pixijs/filters/tree/main/src/emboss.

The MIT License

Copyright (c) 2013-2025 Mathew Groves, Chad Engler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Auto Filter uses the repository's Apache-2.0 `visual-region-detector` and
`clip-visual-region-detector-c` crates with the pure Rust backend. Its image
boundary uses the existing `image` dependency (MIT OR Apache-2.0); JPEG support
adds `zune-core` and `zune-jpeg` (MIT OR Apache-2.0 OR Zlib). Their resolved
licenses are included in the Rust dependency notice bundle. OpenCV is not
required by this feature.
