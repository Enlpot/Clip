# Clip

Windows 与 macOS 上的截图、标注、文字识别与录屏工具。

---

## 来源与许可

**Clip 衍生自 [Snow Shot](https://github.com/mg-chao/snow-apps)（作者 mg-chao）**，该项目采用
**GNU 通用公共许可证第 3 版或之后版本（GPL-3.0-or-later）**。依据该许可证的要求，本仓库同样以
**GPL-3.0-or-later** 发布。

- 原始版权归 mg-chao 所有，保留在 `snow_shot/COPYRIGHT` 与 `snow_image/COPYRIGHT` 中。
- 所有被修改的文件均保留原有的版权与许可声明。
- 各目录的许可范围与第三方材料政策见 [LICENSE.md](LICENSE.md)。

## 当前状态

基于 **Snow Shot 1.1.8** 派生。相对上游已做的改动：

| 改动 | 说明 |
| --- | --- |
| 更名为 **Clip** | 显示名、应用图标、托盘图标、资源前缀与公开链接 |
| 移除 **Snow Image Viewer** | `snow_image/` 库予以保留，`ant_design_qt` 依赖它 |
| **关闭 MCP 集成** | 构建开关关闭，设置项已移除，桥接程序不再构建打包 |
| **关闭自动更新** | 更新模式默认改为手动 |
| **云端接口留空可配** | 不再默认填入上游地址，交由用户自行配置 |

## 功能

继承上游并保持完整：区域 / 全屏 / 窗口 / 滚动长截图，17 种标注工具并支持模板，
文字识别（文本、表格、条码、二维码）且结果可编辑，翻译，录屏（MP4 / GIF / APNG / WebP），
贴屏与窗口分组，截图历史，以及图片与 PDF 导出。

## 构建

工具链要求较重：Visual Studio 2026（MSVC 14.51）、CMake 4.2 以上、Rust 1.97.1，
以及由 vcpkg 管理的 Qt 6.11.1。

```powershell
.\scripts\bootstrap.ps1
.\scripts\build.ps1 -Preset windows-msvc-debug -Target snow_shot
.\scripts\run-snow-shot.ps1
```

模块划分、构建预设与开发规范见 [AGENTS.md](AGENTS.md)。

## 开源许可

| 目录 | 许可证 |
| --- | --- |
| `ant_design_qt/` | [Apache License 2.0](ant_design_qt/LICENSE) |
| `snow-crates/` | [Apache License 2.0](snow-crates/LICENSE) |
| `snow_draw_engine_qt/` | [Apache License 2.0](snow_draw_engine_qt/LICENSE) |
| `snow_rust_ffi/` | [Apache License 2.0](snow_rust_ffi/LICENSE) |
| `snow_image/` | [GNU GPL v3.0 或之后版本](snow_image/COPYRIGHT) |
| `snow_shot/` | [GNU GPL v3.0 或之后版本](snow_shot/COPYRIGHT) |

同步引入与打包的第三方材料沿用其上游许可。另见
[Ant Design Qt 第三方声明](ant_design_qt/THIRD_PARTY_NOTICES.md) 与
[Snow Shot 第三方声明](snow_shot/THIRD_PARTY_NOTICES.md)。
