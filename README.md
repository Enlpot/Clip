# Clip

A screenshot, annotation, OCR and screen recording tool for Windows and macOS.

---

## Origin and license

**Clip is a derivative work of [Snow Shot](https://github.com/mg-chao/snow-apps)
by mg-chao**, which is licensed under the **GNU General Public License v3.0 or later**.
This repository is therefore also distributed under **GPL-3.0-or-later**, in
accordance with the terms of that license.

- Original copyright (c) mg-chao — retained in `snow_shot/COPYRIGHT` and
  `snow_image/COPYRIGHT`.
- Every modified file keeps its original copyright and license notices.
- See [LICENSE.md](LICENSE.md) for the multi-license scope of each directory and
  the third-party material policy.

## Status

Derived from **Snow Shot 1.1.8**. Current changes on top of upstream:

| Change | Note |
| --- | --- |
| Rebranded to **Clip** | Display name, icons, tray icons, public links |
| Removed **Snow Image Viewer** | The `snow_image/` library is kept — `ant_design_qt` depends on it |
| **MCP integration disabled** | Build switch off; settings entries removed |
| **Automatic updates disabled** | Update mode defaults to manual |
| **Cloud API left configurable** | No upstream endpoint is filled in by default |

## Features

Inherited from upstream and left intact: region / full-screen / window / scrolling
screenshots, 17 annotation tools with templates, OCR (text, table, barcode, QR) with
editable results, translation, screen recording (MP4 / GIF / APNG / WebP), pin to
screen with groups, capture history, and PDF / image export.

## Build

The toolchain is heavy: Visual Studio 2026 (MSVC 14.51), CMake 4.2+, Rust 1.97.1,
and a vcpkg-managed Qt 6.11.1.

```powershell
.\scripts\bootstrap.ps1
.\scripts\build.ps1 -Preset windows-msvc-debug -Target snow_shot
.\scripts\run-snow-shot.ps1
```

See [AGENTS.md](AGENTS.md) for the full module layout, presets, and development rules.

## Open Source Licenses

| Project | License |
| --- | --- |
| `ant_design_qt/` | [Apache License 2.0](ant_design_qt/LICENSE) |
| `snow-crates/` | [Apache License 2.0](snow-crates/LICENSE) |
| `snow_draw_engine_qt/` | [Apache License 2.0](snow_draw_engine_qt/LICENSE) |
| `snow_rust_ffi/` | [Apache License 2.0](snow_rust_ffi/LICENSE) |
| `snow_image/` | [GNU GPL v3.0 or later](snow_image/COPYRIGHT) |
| `snow_shot/` | [GNU GPL v3.0 or later](snow_shot/COPYRIGHT) |

Synchronized and bundled third-party materials retain their upstream licenses.
See [Ant Design Qt third-party notices](ant_design_qt/THIRD_PARTY_NOTICES.md)
and [Snow Shot third-party notices](snow_shot/THIRD_PARTY_NOTICES.md).
