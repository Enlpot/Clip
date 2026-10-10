# Clip

Clip 是一款面向 Windows 的截图与屏幕录制工具，集成了截图、长图拼接、屏幕录制、OCR 文字识别、图片编辑、钉窗（窗口置顶）、云上传与文本翻译等能力，追求开箱即用的体验与较高的工程可靠性。

本仓库源自 [snow-apps](https://github.com/mg-chao/snow-apps)（原作者：mg-chao），在保留原作者版权与许可的前提下进行持续维护与优化。当前版本仅支持 Windows x64，已移除 macOS 与 Windows ARM64 支持。

## 主要特性

- **智能截图**：区域/窗口/全屏截图、滚动截屏与长图拼接、像素级十字光标与放大镜
- **屏幕录制**：区域录屏、麦克风/系统声音采集、光标与按键特效、视频导出（H.264 等）
- **OCR 文字识别**：离线 OCR（DirectML 加速）、截图取词、批量识别
- **图片编辑**：标注、马赛克、高亮、文字等，内置绘图引擎
- **钉窗**：将截图或窗口置顶显示，支持调整大小与透明度
- **云能力**：截图上传、自定义 AI 模型配置、文本翻译
- **其它**：快捷键管理、多显示器 DPI 适配、启动项管理、主题与多语言（简体中文/繁体中文/英文）

## 系统要求

- Windows 10/11（x64）
- 构建工具链：Visual Studio 2026 Build Tools（MSVC 14.51）、CMake 4.2+、Rust 1.97.1
- 依赖由仓库管理：vcpkg 依赖与 Qt 6.12.0 均安装在仓库内 `.tools/` 下，不污染系统全局环境

## 构建

首次构建前先安装依赖并校验工具链（每台机器一次）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap.ps1
```

配置并构建（默认构建全部目标）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset clip-shot-msvc-release
```

常用预设：`clip-shot-msvc-release`（发布版）、`clip-shot-msvc-fast`（快速验证）、`windows-msvc-debug`（调试）、`windows-msvc-performance`（性能基准，仅用于 Release 基准测试）。

构建产物位于 `build/clip-shot-msvc-release/clip_shot/Release/clip_shot.exe`。

从构建树直接运行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run-clip-shot.ps1
```

## 测试

按仓库规范，改动后只运行相关测试集，不要全量运行：

```powershell
ctest --preset test-windows-msvc-debug -R <测试名正则>
```

Rust crate 单独测试：

```powershell
cd clip-crates
cargo test -p <crate>
```

## 安装

发布版安装包由 CI 构建（NSIS 安装器）。正式发布渠道与升级说明见 [docs/clip-shot-releases.md](docs/clip-shot-releases.md)。

## 许可证

本仓库为多许可证结构，详见 [LICENSE.md](LICENSE.md)：

| 范围 | 许可证 |
| --- | --- |
| `clip_shot/` | GPL-3.0-or-later |
| `clip_image/`、`clip_image_viewer/` | GPL-3.0-or-later |
| `ant_design_qt/`、`clip-crates/`、`clip_draw_engine_qt/`、`clip_rust_ffi/` | Apache-2.0 |
| 根构建文件、`.github/`、`docs/`、`scripts/`、共享 `cmake/` 模块 | GPL-3.0-or-later |

版权归原作者 mg-chao 所有。第三方材料保留其上游许可证，详见各项目的 `THIRD_PARTY_NOTICES.md`。
