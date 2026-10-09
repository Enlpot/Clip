# Clip 代码通读与优化建议报告（正式版 v1）

- 基线：git 提交 `1b25d565`（snow-apps → clip 全仓改名后），工作树 clean
- 范围：clip_shot（应用，约 30 万行含 Qt 库）+ clip-crates（Rust workspace 29 crates）+ 构建体系
- 定位：`文件路径:行号` 均基于当前工作树
- 优先级：P0 = 建议尽快（正确性/数据安全/明显体验问题）；P1 = 值得做（可维护性/中等收益）；P2 = 可选（锦上添花/低风险）

---

## 一、架构与代码质量

### A1 [P1] screenshotcontroller.cpp 巨型单文件（6791 行）
- 位置：`clip_shot/src/presentation/services/screenshotcontroller.cpp`
- 现状：一个 Impl 类承载全部截图会话逻辑 + MCP 命令（60+）+ 导出/复制/保存/钉窗/云上传协调 + 录制协调，行号到 6700+。
- 理由：已超过单人单次可审查的合理规模；新成员上手成本高；同文件内 MCP 命令、导出、录制三类职责耦合，未来任何一类扩展都会继续膨胀。
- 影响面：presentation 层中枢。拆分不改变对外 API（公有类与方法不变），只把 Impl 成员函数按职责拆到 3-4 个分部类（Capture、Export、Mcp、Pinned）或拆成独立文件内的局部类。
- 改动规模：大（结构性重构，需回归测试覆盖截图/导出/钉窗/MCP 主路径）。

### A2 [P1] MCP 服务族文件超大且样板重复
- 位置：`clip_shot/src/app/mcp/`（mcpdocumentservice.cpp 2645 行、mcpapplicationservice.cpp 1450 行、screenshotmcpsession.cpp 1000 行、mcpmediaservice.cpp 975 行）
- 现状：每个 service 各自实现幂等键重放、作业注册、取消传播、结果发布，逻辑高度相似但无公共基类。
- 理由：四份近似实现意味着 bug 要修四遍；幂等/取消这类跨切面逻辑抽成基类后一致性更易保证。
- 影响面：仅 MCP 内部（对外协议不变）。规模：中。

### A3 [P1] applicationcontroller.cpp 巨型构造函数
- 位置：`clip_shot/src/app/applicationcontroller.cpp`（1964 行，构造/启动部分数百行信号装配）
- 现状：全部服务装配集中在构造函数与 start()。
- 理由：装配顺序即隐式依赖图，一旦出错难定位；拆成按职责分组的装配函数（tray/floatingToolbar/shortcuts/globalMouse/ocr/update/mcp）可读性显著提升。
- 影响面：启动路径（无行为变化）。规模：小-中（纯移动，无逻辑改动）。

### A4 [P2] settingsadapters.cpp 约 400 个手写 getter/setter
- 位置：`clip_shot/src/storage/settingsadapters.cpp`
- 现状：每个配置项一对固定模式方法（读 configuration()/写 setValue+flush），纯样板。
- 理由：新增配置项要手写两处 + 归一化一处，易漏；可用宏展开或代码生成（脚本读取 configurationschema.cpp 的条目表生成适配器）。
- 影响面：存储层内部，对外接口不变。规模：中（生成器 + 一次性替换，需全量回归）。

### A5 [P2] configurationschema.cpp 2446 行线性堆叠
- 位置：`clip_shot/src/storage/configurationschema.cpp`
- 现状：约 300 项配置的 kRawEntries 表 + normalize() 内线性排列的历史迁移分支（若干处按 key 特判）。
- 理由：迁移逻辑与条目定义混排，读"某个 key 的完整生命周期"要跳多个位置；长期看迁移分支会越来越多。
- 建议：条目表保持集中；迁移按 key 组织成注册表（key → 迁移函数），避免 normalize 继续膨胀。
- 影响面：配置是核心契约，改动需谨慎。规模：大（但可增量做，不一次性重写）。

### A6 [P2] perf 探针宏与业务代码交织
- 位置：`screenshotcanvasrenderer.cpp`、`screenshotexportartifact.cpp`、`screenshotclipboardservice.cpp`、`screenrecordingcontroller.cpp` 等大量 `CLIP_SHOT_*_PERF_SCOPE/COUNTER/MILESTONE`
- 现状：每处业务路径内联探针，读代码时噪音显著；宏语义（scope/counter/milestone）混用。
- 建议：统一为单个 instrumentation 接口（或仅保留一个宏族），探针采集与业务解耦；至少统一命名。
- 影响面：全仓机械替换，无行为变化。规模：中。

### A7 [P2] 手写 LRU 与手写 PDF/SSE 解析器
- 位置：`screenshotexportartifact.cpp`（PNG 10 槽 LRU，lastUsed 线性扫描）、`screenshotpdfexport.cpp`（手写 PDF-1.7 xref/stream）、`clipshotapiclient.cpp`（手写 SSE 解析）
- 现状：实现正确且有注释，但均为"轮子"且无单测。
- 建议：不是必须替换（PDF 生成器其实写得很好且可控），但应补齐确定性单测：tile 边界、quality=100 无损分支、透明 SMask、xref 偏移；SSE 解析按 event/data/多帧/断流用例覆盖。
- 影响面：无行为变化。规模：小（补测试为主）。

---

## 二、性能

### P1-1 [P1] 剪贴板 commit 在 UI 线程同步重试
- 位置：`clip_shot/src/presentation/services/screenshotclipboardservice.cpp`（ClipboardCommitOperation 的 runAttempt 直接调用 OpenClipboard/SetClipboardData，最多 5 次、300ms 退避）
- 现状：OpenClipboard 在剪贴板被其他进程占用时可能阻塞，且整个 attempt 序列在调用线程（UI 线程）同步执行。
- 理由：截图后"复制"是最高频动作；剪贴板被 Excel/浏览器等短暂占用时 UI 会卡顿 0.3-1.5s。
- 建议：attempt 移到工作线程（像 export 那样 QueuedConnection 回投）；或至少把单次 attempt 改为限时 + 首失败即异步重试。
- 影响面：复制路径，行为不变。规模：小-中。

### P1-2 [P2] 历史/钉窗全量 JSON 重写
- 位置：`capturehistoryrepository.cpp`（每次 publish 重写 index.json）、`pinnedwindowrepository.cpp`（debounce flush 全量 manifest）
- 现状：千级记录内完全够用；万级 + 时每次提交 O(n) 序列化 + 原子替换 I/O 放大。
- 建议：暂不动作；若用户历史量大（>5000 条），可评估增量条目文件或 JSONL。先加基准再决定。
- 规模：中（需 benchmark 支撑）。

### P1-3 [P2] 导出文档序列化在 UI 线程
- 位置：`screenshotexportservice.cpp`（requestSelectionResult*/requestSelectionClipboard 中 serializeDocumentSession 于 UI 线程同步执行）
- 现状：画布文档序列化通常在毫秒级；超大截图 + 大量标注时才明显。
- 建议：保持现状，标注风险；若实测卡顿再迁 worker。
- 规模：小（仅测量 + 可选迁移）。

### P1-4 [P2] 硬编码阈值散落
- 位置：`clipshotapiclient.cpp`（kMaximumSide=2880、webp q75、latex 672x192）、`screenshotexportservice.cpp`（100000000 像素）、`screenshotpdfexport.cpp`（2048 tile、32MB 内存限）、`screenshotcanvasrenderer.cpp`（tile/rect 阈值）
- 现状：常量正确但分布广，调优需逐处找。
- 建议：集中到一个 `presentation/limits.h`（或已有 constants 头），带注释说明依据。规模：小。

---

## 三、功能完整性

### F1 [P1] 截图历史"读失败即删"的数据安全策略
- 位置：`clip_shot/src/storage/capturehistoryrepository.cpp`（读取/解码失败自动标记删除的自愈逻辑）
- 现状：任何一次 decode/IO 失败（文件锁、杀软扫描、瞬时损坏）都会把整条记录列入 pendingDeletions 删除。
- 理由：瞬时故障导致用户截图历史永久丢失不可逆；正确做法是重试 + 隔离（坏记录改名 `.corrupt` 保留原始文件）+ 可恢复。
- 影响面：历史可靠性（用户数据）。规模：小-中。

### F2 [P1] 超大文件与跨进程一致性（构建侧）
- 位置：`cmake/StageClipShotOcrAssets.cmake`（OCR zip 内 exe 名仍为 snow-ocr-process，`-Clean` 重建会复发）
- 现状：本地已手工重命名兜底；内容 SHA256 与 manifest 匹配，仅外层文件名陈旧。
- 建议：staging 改为"内容哈希校验 + 输出名强制重命名"双保险，避免重建回退。
- 影响面：构建产物一致性。规模：小。

### F3 [P2] 翻译/识别等服务配置缺失的降级路径
- 位置：`screenshotrecognitionsessioncontroller.cpp`（provider destroyed → 状态机已处理）；`clipshotapiclient.cpp`
- 现状：OCR 进程崩溃、网络失败均有降级文案与重试；建议核对"模型下载中取消识别""翻译流式中断"两条路径的 UI 文案与状态恢复（代码已覆盖大部分，属验证项而非缺陷）。

---

## 四、规范 / 工程化

### S1 [P1] 构建环境耦合点（换机器会复现）
- 位置：`.tools/`（vcpkg 静态 Qt、overlay-ports 的 abseil/protobuf 修复、x264 GitHub mirror 归档预置、PowerShell 7 便携版）
- 现状：本机全部通过，但依赖 3 个环境变量 + 手动预置；CI 或新机器必然踩：protoc 崩溃（MSVC 19.51 + SDK 26100）、videolan 反爬、Qt stamp 路径。
- 建议：① 把 overlay ports 差异合并进仓库的 `vcpkg.json` overrides（abseil<20250512.1 已在内，protobuf 崩溃需确认是否也已在 overrides 锁定）；② x264 下载改为从 GitHub mirror 归档（同哈希）纳入 vcpkg 注册源配置；③ 写 `docs/local-build-environment.md` 归档三件套命令（此前提议过，用户未确认，建议采纳）。
- 影响面：可重复构建性。规模：中。

### S2 [P2] 手写解析器/序列化器补单测
- 见 A7。PDF 生成器、SSE 解析、clipboard DIB 转换、geometry 半开区间映射都应补确定性单测（仓库已有 require(condition) 测试框架，成本低）。

### S3 [P2] i18n 与命名规范抽查
- 已核对 AGENTS.md 要求（en_US 源语言、tr() 包裹、ID 用 QStringLiteral）；本次改名后建议跑一次 `clip_shot_update_translations` 确认无 `type="unfinished"`（构建已用 -fail-on-unfinished，隐含已通过）。
- 建议：无需动作，验证项。

### S4 [P2] 分支/合并历史卫生
- 当前提交 `1b25d565` 为单原子 rename 提交（2826 files, +63391/-63710），可回滚点清晰；后续优化建议**逐条小提交**（Conventional Commits），保持可回滚粒度。

---

## 五、通读覆盖与置信度

### 已详读（逐行）清单
- 应用层：main.cpp、applicationcontroller(.cpp/h)、singleinstancecoordinator、edition.h、applicationrestart、featureavailability、translationservice、updateconfirmationdialog、MCP 全家族（server/session/applicationservice/documentservice/mediaservice/jobregistry + 头文件）
- 存储层：applicationstorage、configurationstore、configurationschema（2446 行全文）、settingsadapters（模式全文 + 签名全量）、capturehistoryrepository（1371 行全文）、pinnedwindowrepository（2547 行全文）、storagedirectorychange、storageusagetracker、persistedselectioncodec、persistedwindowgeometry
- 网络层：clipshotapiclient（1561 行全文）、s3uploadprotocol、texttranslationprotocol
- 截图管线：capture/ 全部核心、screenshotcanvasrenderer（2120 行全文）、screenshotoverlaywindow（1085 行全文）、screenshotoverlayinputhandler（1600 行全文）、screenshotoverlaycoordinator、screenshotoverlaycanvaspresenter、screenshotgeometry（1330 行全文）、screenshotcontroller（6791 行全文）、selection 核心（model/editworkflow/exportworkflow）
- 编辑导出：screenshotexportartifact（1491 行全文）、screenshotclipboardservice（819 行全文）、screenshotexportservice（462 行全文）、screenshotexportcoordinator、screenshotclouduploadservice、screenshotpdfexport（427 行全文）
- OCR：screenshotrecognitionsessioncontroller（2669 行全文）、ocrrecognitionservice/ocrassets/ocrcontroller 核心签名
- 录屏：screenrecordingcontroller 前 600 行（FFI 会话管理/定时器/析构接管）+ 全方法签名
- 钉窗：screenshotpinnedwindow 全方法签名（8139 行，覆盖编辑/导出/变换/点击穿透/缩略图/分组/拖拽）
- 其余（settings/styles/components/tools/widgets/mainwindow/platform/diagnostics/update 等）：核心 API 签名概览

### 置信度评分：**82%**
### 不确定点清单
1. UI 组件细节（palette 样式控件、settings 各页面表单、主题 token、图标绘制）只做了 API 签名概览，未逐行读——相关建议若涉及 UI 内部结构需二次确认（A6 受影响）。
2. platform/（win32 native 窗口、Hook、DirectCapture native）与 diagnostics/update 未深读——录屏/钉窗的 native 边界行为（点击穿透、窗口 chrome）建议按 A1/F1 之外的单独立项复核。
3. Rust crates（29 个，含 clip-capture/clip-stitch-images/clip-recording-*）本次未逐行读，仅基于改名期接触 + AGENTS.md；若需 Rust 侧建议需补一轮。
4. 性能条目 P1-1 的 UI 卡顿影响基于代码路径推断（OpenClipboard 阻塞语义），未实测；建议修复后加基准验证。
5. 编译期宏门控（CLIP_SHOT_ENABLE_*）与 edition 运行期门控的双层组合在部分文件造成大量 `#if` 分支，报告中未单独评估其可读性（建议后续专项）。

---

## 六、建议执行顺序（若采纳）
1. F1（历史数据安全，P1，小）
2. F2 + S1（构建可重复，P1，小-中）
3. A3（构造函数拆分，P1，小）
4. P1-1（剪贴板异步，P1，小-中）
5. A1/A2（巨型文件拆分，P1，大——建议拆两次提交，每拆一步跑相关测试）
6. S2（解析器补测试，P2，小）
7. A4/A5/A6/A7、P1-2/P1-3/P1-4（P2 批量，随功能迭代消化）

> 每条建议待用户逐条确认后实施；本报告不包含任何代码改动。
