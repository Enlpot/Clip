# Clip 代码通读与优化建议（工作草稿，持续更新）

> 状态：通读进行中。此文件为草稿，最终交付为正式 Markdown + 飞书文档。
> 基线：git 提交 `1b25d565`（rename snow-apps → clip 之后），工作树 clean。

## 0. 通读覆盖记录

| 阶段 | 范围 | 状态 |
|---|---|---|
| P1.1 | app/（main、applicationcontroller、singleinstance、restart、featureavailability、translationservice、MCP 全家族 6 服务、updateconfirmation） | ✅ 逐行完成 |
| P1.2 | storage/（applicationstorage、configurationstore、configurationschema、settingsadapters、capturehistoryrepository、pinnedwindowrepository、directorychange、usagetracker、编解码） | ✅ 逐行完成 |
| P1.3 | network/（clipshotapiclient、协议头） | ✅ 逐行完成 |
| P1.4 | capture/（workflow、directcapture、scrolling 管线）、overlay/（renderer、window、inputhandler、coordinator、canvaspresenter、uihost、shadowrenderer 签名）、services/screenshotcontroller 全文、core/screenshotgeometry 全文、selection/（model、editworkflow、exportworkflow） | ✅ 核心逐行 + UI 签名 |
| P1.5 | services/（exportartifact、clipboardservice、exportservice、exportcoordinator、cloudupload、pdfexport 逐行；imagefileservice/historyservice API 概览） | ✅ 核心完成 |
| P1.6 | ocr/（recognitionsessioncontroller 全文；ocrrecognitionservice/ocrassets/ocrcontroller 核心签名） | ✅ 核心完成 |
| P2 | 4 个 Qt 库（clip_image、clip_image_viewer、ant_design_qt、clip_draw_engine_qt） | ⬜ |
| P3 | clip-crates 29 个 Rust crate | ⬜ |
| P4 | 构建体系（CMake/vcpkg/scripts/CI） | ⬜ |
| P5 | 测试、文档、i18n | ⬜ |

## 1. 已收集优化候选（按四维）

### 架构与代码质量

- **A1 [P1] screenshotcontroller.cpp 6791 行单文件**：`clip_shot/src/presentation/services/screenshotcontroller.cpp`。Impl 内含全部截图会话逻辑 + MCP 命令 + 导出/复制/保存/钉窗/云上传协调，超大型文件，可维护性差。建议按职责拆分（capture/export/mcp/edit 各自 Impl 分部）。影响面：presentation 层核心，拆分不改变对外 API。改动规模：大（结构性重构，需小心）。
- **A2 [P1] MCP 家族文件超大**：`mcpdocumentservice.cpp` 2645 行、`mcpapplicationservice.cpp` 1450、`screenshotmcpsession.cpp` 1000、`mcpmediaservice.cpp` 975。且各 service 幂等重放/作业注册样板重复，可提取公共基类/模板。影响面：仅 MCP 内部。规模：中。
- **A3 [P1] applicationcontroller.cpp 1964 行巨型构造函数**：信号装配数百行集中在构造。可拆装配函数/子装配器。影响面：启动逻辑。规模：小-中（纯移动）。
- **A4 [P2] settingsadapters.cpp ~400 个手写 getter/setter**（`src/storage/settingsadapters.cpp`）：完全同构的样板代码，可用宏/模板/代码生成。风险：手工维护易漏新配置项。规模：中。
- **A5 [P2] configurationschema.cpp 2446 行手写 300+ 配置条目 + normalize 迁移**：schema 与迁移逻辑线性堆叠。可考虑 schema DSL/生成 + 迁移注册表。规模：大，需谨慎（配置是核心契约）。
- **A6 [P2] overlay 输入/渲染/滚动各处 CLIP_SHOT_CAPTURE_PERF_* 宏与业务交织**：perf 探针内联在生产代码路径，读起来噪音大。建议统一 instrumentation 通道（如 trace 接口）或彻底隔离。规模：中。
- **A7 [P2] 手动 LRU 实现**：`screenshotexportartifact.cpp` 中 pngEncodings/fileEncodings 的 LRU 用 lastUsed 序号 + 线性扫描实现。可替换为更清晰结构或接受现状（10 槽固定小）。规模：小。

### 性能

- **P1-1 [P1] 剪贴板 commit 在 UI 线程同步重试阻塞**：`screenshotclipboardservice.cpp` ClipboardCommitOperation::runAttempt 直接同步调用 OpenClipboard/SetClipboardData，最多 5 次/300ms 退避，期间 UI 线程被占用（OpenClipboard 在剪贴板被占用时可能等待）。建议 attempt 放工作线程或至少限时。影响面：剪贴板被占用时的 UI 卡顿。规模：小-中。
- **P1-2 [P2] capturehistoryrepository/pinnedwindowrepository 全量 JSON 重写**：每次 publish 重写整个 index.json / manifest。千级记录内可接受，万级+会放大 I/O。规模：中（需 benchmark 决定）。
- **P1-3 [P2] 图像转换/翻译请求的 prompt 模板每次拼装**：`clipshotapiclient.cpp` 巨型 prompt 每请求 .arg 重拼。可静态化。规模：小。
- **P1-4 [P2] 硬编码参数**：`clipshotapiclient.cpp` kMaximumSide=2880、webp q75、latex 672x192；`screenshotcanvasrenderer` 各种 tile/rect 阈值。建议集中配置 + 注释。规模：小。

### 功能完整性（待补：需与 UI/文档对照）

- 待 OCR/录屏/钉窗/设置读完再评估。

### 规范 / 工程化

- **S1 [P1] 读失败即删整条截图记录**：`capturehistoryrepository.cpp` 中读取/解码失败自动标记删除（自愈设计），但瞬时 IO 错误（文件锁/杀软）可能导致数据永久丢失。建议：失败重试 + 隔离坏记录（保留原始文件改名 .corrupt）+ 用户可见提示。影响面：截图历史数据可靠性。
- **S2 [P2] OCR zip 内 exe 名仍为 snow-ocr-process**（构建产物复发性问题）：StageClipShotOcrAssets.cmake 的 zip 内文件名未改，`-Clean` 全量重建会复发。建议 manifest 校验改为内容哈希校验 + 输出重命名兜底。
- **S3 [P2] x264/stamp/overlay-ports 构建修补是环境耦合**：这些修复目前靠本地 .tools 预置 + 环境变量，换机器/CI 会复现问题。建议把 overlay ports 与 vcpkg 基线提升到仓库级（提交到 vcpkg.json overrides 或 fork 基线）。规模：中。
- **S4 [P2] SSE 手写解析无单测**：`clipshotapiclient.cpp` 流解析（event/data 分行）无覆盖。建议补解析单测。规模：小。
- **S5 [P2] 手写 PDF 生成器无单测**：`screenshotpdfexport.cpp` 手工 xref/stream 偏移，tile 边界/透明/Quality100 分支建议补 golden 单测。规模：小。
- **P1-5 [P2] export 文档序列化在 UI 线程同步**：`screenshotexportservice.cpp` requestSelectionResult* 中 serializeDocumentSession 于 UI 线程执行；大文档/慢操作时可移 worker。规模：小-中（需测量）。

## 2. 已读关键结论（供报告引用）

- 启动装配链：main → DiagnosticsService(crashpad) → ApplicationStorage → single-instance(QLocalServer) → reconcileStartupMode → ApplicationController.start()
- MCP 架构：QLocalServer + token + ACL(SDDL) + 大端帧协议 + 优先级队列 + 幂等键 + 资源预算（文档 512MB 源/256MB 历史/64MB 缓存/64MB blob、4GB 磁盘配额）
- 存储架构：schema 驱动配置（QSaveFile 原子写 + 损坏保留 30 天 + FutureVersion 只读）；index.json + records/{uuid} 历史（严格校验 + revision 乐观并发 + 单 worker 命令队列）；manifest + pins/{id} 钉窗（双线程 debounce flush + resident demote + 生命周期协调）
- 导出架构：ScreenshotExportArtifact（source 多态 + 订阅者 + PNG 10 槽 LRU + FileSource 内存限 spill 磁盘 + 取消传播）；ScreenshotExportCoordinator（全局队列 + Foreground/Background 优先级）
- 截图管线：DisplaySession + CanvasRender（损伤差分 + 路径栅格缓存） + InputHandler 状态机 + 滚动截图双线程（Producer/Stitch）
- 剪贴板：Windows 原生 DIB(24bpp, alpha 合成, 64 行批处理) + PNG 注册格式 + 原子多格式发布 + 回滚 + 5 次重试

## 3. 下一步

- P1.5 剩余：exportservice、coordinator、cloudupload、pdf、imagefile、historyservice、clipboardcontent
- P1.6 OCR、P1.7 录屏、P1.8 钉窗、P1.9 设置主题翻译快捷键、P1.10 平台诊断更新
- P2 Qt 库、P3 Rust、P4 构建、P5 测试文档
- 最终：正式报告（四维 + 优先级 + 文件:行 + 影响面 + 规模）+ 飞书文档 + 置信度
