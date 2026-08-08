# Pipeline 页面

管理端 Pipeline 页面，用于配置 DeepStream 推理流水线（源、模型、追踪、分析、输出等）。

**GIE** 与 **Analyzer** 是与 **Pipeline** **同级的可复用模板**（不绑定某条 Pipeline）；Pipeline 通过下拉 **引用** 模板 id。本文档定义 **[主页面](#主页面)** 与各 **子页面** 规格。

**Generator 触发时机**：子页 **Save** 只持久化管理端配置，**不**调用 Generator。主页 **▶**（`POST /pipelines/{id}/start`）时，后端**先**代理调用 Generator 生成 DeepStream 配置，**成功后再**拉起 DeepStream；Generator 失败则不启动，Status → `error`。具体路径与字段名见 [pipeline_api.md](./pipeline_api.md)。

| 页面 | 说明 |
|------|------|
| **[主页面](#主页面)** | Pipeline 列表；工具栏进入 Pipeline / GIE 库 / Analyzer 库 |
| **[Pipeline 子页面](#pipeline-子页面)** | 一条流水线的 Name、Type、Probe 与 Generator（引用 GIE / Analyzer 模板） |
| **[GIE 子页面](#gie-子页面)** | GIE **模板**：模型 + `class_attrs` |
| **[Analyzer 子页面](#analyzer-子页面)** | Analyzer **模板**：参考帧标注 ROI / 拥挤 / 越线 / 方向 |

各资源唯一标识均为 **UUID**（RFC 4122），由后端生成、不可修改。列表默认展示 **Name**；API 与前端 key 以 **id** 为准。

Analyzer 画布层采用 **vue-konva + konva**；各页 UI 壳（下拉、表格、弹窗、Save/Cancel、Toast）统一用 **Element Plus**。

---

## 主页面

Pipeline 管理入口：表格只列 **Pipeline**；**GIE** / **Analyzer** 通过工具栏左侧（Refresh 旁）按钮打开**模板库弹窗**（非行内绑定）。

### 页面布局

```
+==========================================================================================================+
| Pipelines                                                                                                |
+==========================================================================================================+
| [ Add ] [ Remove ] [ Refresh ] [ GIE ] [ Analyzer ]                                  [ Search: ___ ] |
| Target: 0 selected                                                                                       |
+--+------------------+----------------------+----------+--------------------------------------------------+
|[]| Name             | Type                 | Status   | Operations                                       |
+--+------------------+----------------------+----------+--------------------------------------------------+
|[]| yolo26n-det-rtsp | DetVisRTSPPipeline   | Running  | ▶/■  ↻  ✏  🗑  📓                                 |
|[]| smoke-fire       | PresenceVideoPipeline| Stopped  | ▶/■  ↻  ✏  🗑  📓                                 |
+--+------------------+----------------------+----------+--------------------------------------------------+
|                                              < 1  2  3 >                                                 |
+==========================================================================================================+
```

| 区域 | 说明 |
|------|------|
| 页面标题 | 固定 `Pipelines` |
| 工具栏左 | **Add** / **Remove** / **Refresh** / **GIE** / **Analyzer**（左对齐；GIE / Analyzer 紧挨 Refresh） |
| 工具栏右 | **Search** |
| **Search** | 最右侧；按 Pipeline **Name** 子串过滤 |
| Target | 已勾选行数；位于工具栏与表格之间 |
| 数据表格 | 仅 Pipeline；行主键 **pipeline id** |

### 工具栏

| 元素 | 对齐 | 样式 | 可用条件 | 行为 |
|------|------|------|----------|------|
| **Add** | 左 | 蓝底白字 | 始终 | 打开 [Pipeline 子页面](#pipeline-子页面)（新建） |
| **Remove** | 左 | 红底白字 | ≥1 行勾选 | 二次确认后删除勾选 Pipeline |
| **Refresh** | 左 | 绿底白字 | 始终 | 刷新表格 Status 等；按 pipeline id 恢复勾选 |
| **GIE** | 左（Refresh 旁） | 次按钮 / 默认 | 始终 | 打开 [GIE 模板库弹窗](#gie-模板库弹窗) |
| **Analyzer** | 左（GIE 旁） | 次按钮 / 默认 | 始终 | 打开 [Analyzer 模板库弹窗](#analyzer-模板库弹窗) |
| **Search** | 右 | 文本 | 始终 | 过滤 Pipeline 表 |

未勾选时 **Remove** 为 `disabled`（浅红底白字）。

### 数据表格（Pipeline）

| 列名 | 类型 | 说明 |
|------|------|------|
| Checkbox | 多选 | 批量 Remove；绑定 **pipeline id** |
| Name | `str` | Pipeline 名；全库唯一 |
| Type | `str` | 如 `DetVisRTSPPipeline` |
| Status | `str` | 运行态（如 `Running` / `Stopped` / `Error`；实现以运行时为准） |
| Operations | 图标组 | 见下 |

**Operations**

| 图标 | 行为 |
|------|------|
| **▶ / ■** | 开始 / 停止该 Pipeline（互斥展示）；**▶** 见 [启动与 Generator](#启动与-generator) |
| **↻** | 刷新该行 Status（及运行摘要） |
| **✏** | 编辑 → [Pipeline 子页面](#pipeline-子页面) |
| **🗑** | 删除该行（二次确认） |
| **📓** | 打开该 Pipeline 日志 |

### 启动与 Generator

主页 **▶** 调用 `POST /pipelines/{id}/start`。后端顺序：

```
1. status → starting
2. 代理调用 Generator（按已保存的 Pipeline 配置 + 引用的 GIE / Analyzer 模板生成 DeepStream 配置并落盘）
3. Generator 成功 → 拉起 DeepStream → status → running（或启动失败 → error）
4. Generator 失败 → 不拉起 DeepStream → status → error；message 带失败原因
```

| 规则 | 说明 |
|------|------|
| Save ≠ Generate | Pipeline / GIE / Analyzer 子页 **Save** 只写库；**不**触发 Generator |
| 唯一触发点 | 管理端仅通过 **▶ / start** 触发 Generator（前端不直连 Generator） |
| 模板变更 | 修改 GIE / Analyzer 模板后，**下次 ▶** 时按最新模板重新 generate |
| 前端 | Start 受理后轮询 `GET .../status`；见下表与 Toast |

**Status 轮询（▶ / ■）**

| 项 | 值 |
|----|-----|
| 间隔 | **1s** |
| 超时 | **60s**（自首次轮询起累计） |
| 结束条件 | `status` 为 `running` / `error` / `stopped`（■ 场景至 `stopped`），或超时 |
| 超时行为 | 停止轮询；Toast **`Pipeline status timeout`**；行内 Status 仍以最后一次成功响应为准（可为 `starting` / `stopping`）；用户可再点 **↻** 手动刷新，或稍后重试 |

| 操作 | Toast |
|------|-------|
| **▶** 受理（进入 `starting`） | **`Pipeline starting`**（可选） |
| 最终 `running` | **`Pipeline started`** |
| 最终 `error`（含 Generator 失败） | **`{message}`** 或 **`Pipeline failed`** |
| 轮询超时 | **`Pipeline status timeout`** |
| **■** 成功 | **`Pipeline stopped`** |

---

### GIE 模板库弹窗

主页工具栏 **GIE** 打开。管理与 Pipeline **同级** 的 GIE 模板；供 Pipeline 子页 **PGIE** 下拉引用。

```
+------------------------------------------------------------------------------------------+
| GIE Templates                                                                       [x]  |
+------------------------------------------------------------------------------------------+
| [ Add ] [ Remove ]                                                                       |
+----+------------------+------------------------------------------+-----------------------+
| [] | Name             | Model      | Class Attrs                    | Pipelines              | Ops |
+----+------------------+------------+--------------------------------+------------------------+-----+
| [] | yolo26n-default  | yolo26n    | -1 - ALL: Confidence=0.25, …   | yolo26n-det-rtsp       | ✏ 🗑 |
|    |                  |            |                                | smoke-fire             |     |
| [] | person-only      | yolo26n    | 0 - person: Confidence=0.9, …  | —                      | ✏ 🗑 |
+----+------------------+------------+--------------------------------+------------------------+-----+
|                                              < 1  2  3 >                                 |
+------------------------------------------------------------------------------------------+
```

| 区域 | 说明 |
|------|------|
| **Add** / **Remove** | 列表**外**左上角；Add → [GIE 子页面](#gie-子页面)（新建）；Remove → **二次确认**后删勾选模板（被 Pipeline 引用时拒绝，实现定） |
| **Name** | 模板名；全库唯一 |
| **Model** | 只读；关联 Models 的 **name**（字段 `model_name`） |
| **Class Attrs** | 只读摘要；每行一条，格式 `class_id - label: Confidence=…, Top K=…, MinW=…, MinH=…, MaxW=…, MaxH=…` |
| **Pipelines** | 只读；引用该模板的 Pipeline **Name**，**每个占一行**（无引用为 `—`）；来自列表 API 字段 `pipelines` |
| **Operations** | **✏** 编辑 → GIE 子页面；**🗑** **二次确认**后删除该模板 |

---

### Analyzer 模板库弹窗

主页工具栏 **Analyzer** 打开。管理与 Pipeline **同级** 的 Analyzer 模板；供 Pipeline 子页 **Template** 下拉引用。

```
+----------------------------------------------------------------------------------------------------------+
| Analyzer Templates                                                                                  [x]  |
+----------------------------------------------------------------------------------------------------------+
| [ Add ] [ Remove ]                                                                                       |
+----+------------------+---------------+--------------+---------------+------------------+--------------------+-----+
| [] | Name             | ROI Filtering | Overcrowding | Line Crossing | Direction Detect | Pipelines          | Ops |
+----+------------------+---------------+--------------+---------------+------------------+--------------------+-----+
| [] | entrance_default | 2             | 1            | 1             | 0                | yolo26n-det-rtsp   | ✏ 🗑 |
|    |                  |               |              |               |                  | smoke-fire         |     |
| [] | gate_only        | 1             | 0            | 2             | 1                | —                  | ✏ 🗑 |
+----+------------------+---------------+--------------+---------------+------------------+--------------------+-----+
|                                              < 1  2  3 >                                                 |
+----------------------------------------------------------------------------------------------------------+
```

| 区域 | 说明 |
|------|------|
| **Add** / **Remove** | 列表**外**左上角；Add → [Analyzer 子页面](#analyzer-子页面)（新建）；Remove → **二次确认**后删勾选模板（被引用时策略同 GIE） |
| **Name** | 模板名；全库唯一 |
| **ROI Filtering** / **Overcrowding** / **Line Crossing** / **Direction Detect** | 只读；该模板下对应 **Type** 的标注条数（`0` 表示无） |
| **Pipelines** | 只读；引用该模板的 Pipeline **Name**，**每个占一行**（无引用为 `—`）；来自列表 API 字段 `pipelines` |
| **Operations** | **✏** 编辑 → Analyzer 子页面；**🗑** **二次确认**后删除该模板 |

---

## Pipeline 子页面

由主页 **Add** 或行内 **✏** 进入。**Save** 后写入该 Pipeline 的完整配置（Probe 侧 Drawer / Logger / Messager / Debouncer + Generator 侧 Streams / PGIE / Interval / Tracker / Analyzer）。

**PGIE**、**Analyzer Template** 下拉从模板库 **引用** GIE / Analyzer（见主页弹窗）；本子页 Analyzer 区只配 **streams / template / 各 rule 的 class_id**，几何标注在 [Analyzer 子页面](#analyzer-子页面) 维护。

### 页面布局

```
+==========================================================================================+
| Pipeline                                                                             [x] |
+==========================================================================================+
| Type   [ DetVisRTSPPipeline            ▼ ]                                               |
| Name   [ yolo26n-det-rtsp________________ ]                                              |
+==========================================================================================+
| ▼ Pipeline                                                                               |
|   Drawer                                                                                 |
|     Show Label  [ True  ▼ ]     Show Conf  [ True  ▼ ]                                   |
|     Interval    [ 50________ ]  Fade Time  [ 1_________ ]                                |
|   Logger                                                                                 |
|     Interval    [ 50________ ]                                                           |
|   Messager                                                                               |
|     Interval    [ 0_________ ]                                                           |
|   Debouncer                                                                              |
|     Length      [ 10________ ]  Threshold  [ 0.5_______ ]                                |
|     Class       [ 0,1______________ ] [+]     Mode  [ fold ▼ ]                           |
+------------------------------------------------------------------------------------------+
| ▼ Generator                                                                              |
|   Streams       [ video1, video2___________ ] [+]                                        |
|   PGIE          [ yolo26n                  ▼ ]                                           |
|   Interval      [ 50________ ]                                                           |
|   [✓] Tracker                                                                            |
|       Class     [ 0____________________ ] [+]                                            |
|   [✓] Analyzer                                                                           |
|       Streams   [ video1, video2_______ ▼ ]                                              |
|       Template  [ default                ▼ ]                                             |
|         Roi Filtering          [ -1______________ ] [+]                                  |
|         Overcrowding           [ 0_______________ ] [+]                                  |
|         Line Crossing          [ 0,2_____________ ] [+]                                  |
|         Direction Detection    [ ________________ ] [+]                                  |
+==========================================================================================+
|                                              [ Save ]    [ Cancel ]                      |
+==========================================================================================+
```

| 区域 | 说明 |
|------|------|
| **Type** | 下拉；选项来自 `GET /types`；选定后调用 `POST /schema` 拉取全量表单 schema（含 `pipeline` / `generator`）；见 [Type](#type) |
| **Name** | Pipeline 名称；非空；**全库唯一**；选 Type **前**亦可编辑 |
| **Pipeline** | 分组标题（可折叠）；Probe 侧参数；**未选 Type 时整组不可编辑** |
| **Generator** | 分组标题（可折叠）；生成 / 拓扑侧参数；**未选 Type 时整组不可编辑** |
| **Tracker / Analyzer** | 标题左、**checkbox** 右对齐；子字段**始终展开**；未勾选时子字段禁用且 Save 为 `tracker: null` / `analyzer: null`；可编辑性由 schema 的 `available` 决定 |
| 底栏 | **Save** / **Cancel**（有未保存编辑时确认丢弃）；**未选 Type 时 Save 禁用** |

---

### Type

下拉选项与 DeepStream / generator 注册表对齐；选项列表由 **`GET /pipelines/types`** 返回（仅 type 名，无能力开关）。

选定（或切换）某个 Type 后，前端立即调用 **`POST /pipelines/schema`**，入参为该 `pipeline_type`；响应为该 Type 下 **Pipeline** / **Generator** 的**全量**表单 schema（`pipeline` / `generator` 含每个区块/字段的 `available` 与 `default`；**禁止缺省字段**）。前端据此渲染控件的可编辑态与默认值，**不再**在页面内用 Type 名硬编码「是否显示 Debouncer / Streams」等规则。

| 规则 | 说明 |
|------|------|
| 未选 Type | **Type 以下**（Pipeline / Generator 分组内全部控件，含 Tracker / Analyzer）一律 **disabled / 不可编辑**；不调用 schema |
| 选中 Type | `POST /schema`；用响应填充默认值，并按各层 `available` 决定可编辑 |
| `available: false` | 对应区块或字段 **不可编辑**（禁用）；值仍可按 `default` 保留在表单态，**Save 时**：区块级 `available: false` 则该区块不落盘（或落 `null`，与现有 `debouncer` / `tracker` / `analyzer` 约定一致）；字段级 `available: false` 不提交该字段或沿用服务端忽略策略 |
| `available: true` | 可编辑；初始值取 schema 中该字段的 `default` |
| 切换 Type | 再次 `POST /schema`；用**新** schema 的 `default` **覆盖**表单中 Pipeline / Generator 相关字段（Name 不变）；覆盖前若有未保存编辑可二次确认（实现可选） |
| 出参完整性 | schema **必须**返回该 Type 下约定的全部区块与叶子字段；前端**不**对缺失键做静默默认 |

schema 字段形状与示例见 [pipeline_api.md](./pipeline_api.md) `POST /schema`。

---

### Pipeline 分组

分组及子字段是否可编辑，以当前 Type 的 schema `pipeline.params.*.available`（及嵌套字段 `available`）为准；未选 Type 时整组禁用。

#### Drawer

| 字段 | 控件 | 范围 / 选项 | 说明 |
|------|------|-------------|------|
| **Show Label** | 下拉 | `True` / `False` | → `drawer.show_label` |
| **Show Conf** | 下拉 | `True` / `False` | → `drawer.show_conf` |
| **Interval** | 整数输入 | `≥ 0` | → `drawer.interval` |
| **Fade Time** | 整数输入 | `≥ 0` | → `drawer.fade_time` |

默认值以 `POST /schema` 返回的 `default` 为准（勿在前端写死）。

#### Logger

| 字段 | 控件 | 范围 | 说明 |
|------|------|------|------|
| **Interval** | 整数输入 | `≥ 0` | → `logger.interval` |

**不**向用户暴露 `root` 等路径（后端按 Pipeline Name 自动生成）。

#### Messager

| 字段 | 控件 | 范围 | 说明 |
|------|------|------|------|
| **Interval** | 整数输入 | `≥ 0` | → `messager.interval`（若运行时暂无该字段，仍先按本子页持久化，对接时映射） |

**不**暴露 `topic` / `host` / `port`（服务端默认或全局配置）。

#### Debouncer

是否可编辑由 schema `pipeline.params.debouncer.available` 决定（不再按 Type 名硬编码）。

| 字段 | 控件 | 范围 / 选项 | 说明 |
|------|------|-------------|------|
| **Length** | 整数输入 | `≥ 1` | → `debouncer.length` |
| **Threshold** | 数字输入 | `0`–`1` | → `debouncer.threshold` |
| **Class** | 只读编辑框 + **[+]** | 见 [Class 选择弹窗](#class-选择弹窗) | → `debouncer.class_ids`；展示为逗号连接，**禁止手输** |
| **Mode** | 下拉 | `slide` / `fold` | → `debouncer.mode` |

默认值以 schema `default` 为准。

---

### Generator 分组

分组及子字段是否可编辑，以当前 Type 的 schema `generator.params.*.available` 为准；未选 Type 时整组禁用。

#### Streams

| 项 | 说明 |
|----|------|
| 展示 | 只读摘要框：已选 stream **Name**，逗号连接；右侧 **[+]** |
| **[+]** | 打开 [Streams 多选弹窗](#streams-多选弹窗)；`streams.available: false` 时禁用 |
| 数据 | 持久化为 stream **id** 列表；UI 展示 Name |
| 分辨率 | 弹窗展示 resolution / fps；多选不限制一致性 |
| 必填 | 当 `streams.available: true` 时 Save 须 **≥1**（具体以 schema / 校验为准） |

**Analyzer → Streams** 复用同一展示/选择交互，但候选集 = **本子页 Generator 已选中的 Streams**（不可选未加入 Generator 的流）。

#### PGIE

| 项 | 说明 |
|----|------|
| 控件 | 下拉；选项来自 **GIE 模板库**；展示模板 **Name**（可附模型名）；`pgie.available: false` 时禁用 |
| 值 | **GIE 模板 id** |
| 空态 | 占位 **Select PGIE**；模板在主页 **GIE** 弹窗中维护，本下拉只引用 |

#### Interval

| 字段 | 控件 | 范围 | 说明 |
|------|------|------|------|
| **Interval** | 整数输入 | `≥ 0` | → generator / PGIE `interval`（跳帧） |

#### Tracker

| 项 | 说明 |
|----|------|
| 可编辑 | 由 schema `generator.params.tracker.available` 决定；未选 Type 时禁用 |
| 分组 checkbox | 右对齐；勾选启用子字段；未勾选 → 子字段禁用且 `tracker: null` |
| **Class** | 只读编辑框 + **[+]**；与 Debouncer **共用** [Class 选择弹窗](#class-选择弹窗) |
| 展示 | `class_id` 逗号连接（如 `0,2`）；`-1`（ALL）时展示 `-1` 或文案 `ALL` |
| 落盘 | `{ "class_id": <int \| int[]> }`；与 generator `validate_tracker` 一致；须 ⊆ 当前 PGIE 启用类 |

#### Analyzer

| 项 | 说明 |
|----|------|
| 可编辑 | 由 schema `generator.params.analyzer.available` 决定；未选 Type 时禁用 |
| 分组 checkbox | 右对齐；勾选启用子字段；未勾选 → 子字段禁用且 `analyzer: null` |
| **Streams** | 下拉/多选；候选 = Generator 已选 Streams；至少选 1 条（勾选 Analyzer 时） |
| **Template** | 下拉；选项来自 **Analyzer 模板库**（主页 **Analyzer** 弹窗）；选中后展开下列 4 行 |
| 四行 rule | 每行：**规则名** + 只读 Class 框 + **[+]**（复用 Class 弹窗） |

Template 展开后：

| 行文案 | 落盘键 | 说明 |
|--------|--------|------|
| **Roi Filtering** | `roi_filtering.class_id` | |
| **Overcrowding** | `overcrowding.class_id` | |
| **Line Crossing** | `line_crossing.class_id` | |
| **Direction Detection** | `direction_detection.class_id` | |

某 rule 的 Class 为空 → 该 rule **不启用**（生成配置时跳过）；与 generator `NvdsanalyticsParser`「rule 为 null 则跳过」对齐。几何 ROI / 线 仍在 Analyzer 子页绘制，**Save Analyzer 子页** 时与本子页 class_id 合并。

#### SAHI

| 项 | 说明 |
|----|------|
| 可编辑 | 由 schema `generator.params.sahi.available` 决定 |
| 字段 | Slice Width / Height、Overlap Width/Height Ratio、Match Threshold |
| 落盘 | `sahi: { nvsahipreprocess, nvsahipostprocess }`；不可用时为 `null` |

#### Input / Output

| 项 | 说明 |
|----|------|
| 可编辑 | 分别由 `generator.params.input` / `output.available` 决定（文件/视频 Type） |
| 控件 | 文本输入，路径字符串 |
| 落盘 | `input` / `output`；不可用时为 `null`；可用时 Save 须非空 |

---

### Streams 多选弹窗

点击 Generator **Streams [+]**（或 Analyzer Streams 在「从已选中再筛」时可用同类弹窗）打开：

```
+------------------------------------------------------------------------------+
| Select Streams                                                          [x]  |
+------------------------------------------------------------------------------+
| [] | Name     | Resolution | FPS | Status  | Group                            |
+----+----------+------------+-----+---------+---------------------------------+
| [✓]| cam-01   | 1920x1080  | 25  | Online  | Entrance                        |
| [✓]| cam-02   | 1280x720   | 15  | Online  | Entrance                        |
| [ ]| cam-04   | 1920x1080  | 30  | Offline | All                              |
+------------------------------------------------------------------------------+
|                                              [ Save ]  [ Cancel ]            |
+------------------------------------------------------------------------------+
```

| 列 | 说明 |
|----|------|
| **checkbox** | 表头 = 全选/取消全选；行内多选；一次确认批量加入 |
| **Name** | Streams 页 **Name** |
| **Resolution** | 如 `1920x1080`（width×height） |
| **FPS** | 整数 |
| **Status** | `Online` / `Offline` |
| **Group** | 展示组 **Name**（关联 group id） |

| 交互 | 说明 |
|------|------|
| 多选 | 任意行可同时勾选，互不禁用 |
| 表头 checkbox | 全选 / 取消全选当前列表 |
| 数据源 | `GET .../streams`（建议 Online + Enabled）；字段见 [stream.md](./stream.md) |
| Analyzer 场景 | 列表仅含 Generator 已选 stream |

---

### Class 选择弹窗

供 **Debouncer.Class**、**Tracker.Class**、Analyzer 四条 rule 的 Class **共用**：

```
+----------------------------------------------------------+
| Select Classes                                      [x]  |
+----------------------------------------------------------+
| [✓] | Class ID | Label                                   |
+-----+----------+-----------------------------------------+
| [✓] | 0        | person     ← 表头勾选 = ALL（-1）       |
| [✓] | 1        | bicycle    ← 可编辑；取消某行则退出 ALL |
| [✓] | 2        | car                                     |
+----------------------------------------------------------+
|                              [ OK ]    [ Cancel ]        |
+----------------------------------------------------------+
```

| 项 | 说明 |
|----|------|
| **表头 checkbox** | 即 **ALL**；对应 class_id **`-1`**；勾选后行内全部显示为勾选且**可编辑**；改动任一行则退出 ALL、改为具体 id 列表 |
| 列表列 | **checkbox** / **Class ID** / **Label**；表宽撑满 |
| 候选类 | 来自当前 **PGIE** 启用集（`class_attrs` 逻辑：有 `all` → 模型全部类；无 `all` → 仅数字 key）；label 来自 Models `classes` |
| 未选 PGIE | 弹窗不可用或列表为空 |
| 回填编辑框 | 多选 → `0,2`；仅 ALL → `-1`；**禁止**用户在编辑框键盘输入 |
| **[+]** | 打开本弹窗；确认后覆盖该字段 |

与 generator：`tracker.class_id == -1` 表示全部；analyzer rule 中 `-1` 跳过「必须 ∈ pgie_class_ids」的子集校验。

---

### 数据模型（Pipeline 子页面）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo26n-det-rtsp",
  "type": "DetVisRTSPPipeline",
  "drawer": {
    "show_label": true,
    "show_conf": true,
    "interval": 50,
    "fade_time": 1
  },
  "logger": { "interval": 50 },
  "messager": { "interval": 0 },
  "debouncer": null,
  "streams": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "8f3a2b1c-1234-4cde-9abc-def012345678"
  ],
  "gie_id": "9a1b2c3d-4567-89ab-cdef-0123456789ab",
  "interval": 50,
  "tracker": { "class_id": [0] },
  "analyzer": {
    "streams": ["7c9e6679-7425-40de-944b-e07fc1f90ae7"],
    "template": "9a1b2c3d-4567-89ab-cdef-0123456789ab",
    "roi_filtering": { "class_id": [-1] },
    "overcrowding": { "class_id": [0] },
    "line_crossing": { "class_id": [0, 2] },
    "direction_detection": null
  }
}
```

| 字段 | 说明 |
|------|------|
| `debouncer` | Type 不需要时为 `null`；需要时含 `length` / `threshold` / `class_ids` / `mode` |
| `tracker` | 未勾选为 `null`；否则 `{ "class_id": int \| int[] }` |
| `analyzer` | 未勾选为 `null`；某 rule 未配 Class 则为 `null` 或不写该键 |
| `gie_id` | 引用 [GIE 模板](#gie-子页面) id |
| `streams` | Generator 流 id 列表；Analyzer.streams ⊆ 此列表 |
| `analyzer.template` | 引用 [Analyzer 模板](#analyzer-子页面) id（若勾选 Analyzer） |

---

### Toast / 校验（Pipeline 子页面）

| 规则 | 说明 |
|------|------|
| Name | 非空；全库唯一 |
| Type | 必选；未选时不可 Save |
| Streams | 当 schema 中 `generator.params.streams.available: true` 时：**≥1** |
| PGIE | 当 `generator.params.pgie.available: true` 时必选；且对应 GIE 的 model 仍为 `built` |
| SAHI | 当 `generator.params.sahi.available: true` 时必填 |
| Input / Output | 当对应 `available: true` 时路径非空 |
| Tracker / Debouncer / Analyzer Class | 对应区块 `available: true` 且勾选后不可为空；须 ⊆ PGIE 启用类（`-1` 除外） |
| Analyzer Streams | ⊆ Generator Streams；非空（当 analyzer 可用且勾选时） |
| **Save** 成功 | Toast **`Pipeline saved`** |

---

## GIE 子页面

由主页 **GIE 模板库弹窗** 的 **Add** / **✏** 进入。编辑一条 **GIE 模板**（与 Pipeline 同级、可被多条 Pipeline 引用）；**Save** 写入模板库。语义对齐 generator 的 `pgie.class_attrs`（见 `servers/generator/utils/subelement_generator/utils/pgie_parser.py`）。

### 页面布局

```
+==========================================================================================+
| GIE                                                                                  [x] |
+==========================================================================================+
| Name   [ yolo26n-default________________ ]                                               |
| Model  [ yolo26n                              ▼ ]                                        |
+==========================================================================================+
| class_attrs                                                                              |
| +--------+------+------+----------------+----------------+----------------+----------------+--------+
| | Class ID - Label | Confidence | Top K | MinW | MinH | MaxW | MaxH | Ops    |
| +------------------+------------+-------+------+------+------+------+--------+
| | -1 - ALL       ▼ | 0.25       | 300   | -1   | -1   | -1   | -1   |  🗑    |
| +------------------+------------+-------+------+------+------+------+--------+
| | 0 - person     ▼ | 0.9        | 10    | 32   | 32   | -1   | -1   |  🗑    |
| +--------+------+------+----------------+----------------+----------------+----------------+--------+
|                                        [ + ]                                             |
+==========================================================================================+
|                                              [ Save ]    [ Cancel ]                      |
+==========================================================================================+
```

| 区域 | 说明 |
|------|------|
| 顶栏 **Name** | 模板名；非空；**全库唯一** |
| 顶栏 **Model** | 下拉；选项来自 [Models 页面](./model.md)；**只展示 `name`（model_name）**，值为 **model id** |
| **class_attrs** | 可编辑表；列见下；默认一行见 [默认行](#默认行) |
| 表下 **[+]** | 追加一行；新行字段初值与默认行相同，但 **class** 为空（须用户选择） |
| **Ops** | 仅 **🗑**；删除该行（含 `all` 行，**可删**） |
| 底栏 | **Save** 持久化模板；**Cancel** / **[x]** 关闭（有未保存编辑时确认丢弃） |

---

### Model

| 项 | 说明 |
|----|------|
| 控件 | `ElSelect`；收起态显示当前模型 **Name**；未选时占位 **Select model** |
| 数据源 | `GET /ai_stream2/backend/models`（或等价列表）；与 [model_api.md](./model_api.md) 一致 |
| 展示 | 仅 **`name`**；不展示 Version / Status 等 |
| 可选范围 | 仅 **`status=built`** 的模型（未编译不可用于推理配置） |
| 值 | **model id**（UUID）；持久化与 API 以 id 为准 |
| 换模 | 切换 Model 后，若已有行的 `class` 数字 id **超出**新模型 `num_class`（或不在 `classes` 中）→ 清空该行 `class` 并 Toast；`all` 行保留 |

下拉展开示意：

```
┌──────────────────────────┐
│ yolo26n                  │
│ yolo11n-seg              │
│ yolov10x-smoke-fire      │
└──────────────────────────┘
```

---

### class_attrs 表

#### 列定义

| 列 | 控件 | 类型 / 范围 | 说明 |
|----|------|-------------|------|
| **Class ID - Label** | 下拉 | `all` \| `0 .. num_class-1` | 选项依赖当前 **Model**；未选 Model 时禁用。收起态与展开项同一文案 `class_id - label`（如 `-1 - ALL`）；展开时 ID 固定宽居中以对齐；值为 id 或 `all` |
| **Confidence** | 数字输入 | `0`–`1` | 允许小数；允许整数 `0` / `1`；对应 nvinfer `pre-cluster-threshold`（字段 `conf`） |
| **Top K** | 数字输入 | 整数 `0`–`300` | 对应 nvinfer `topk` |
| **MinW**（`detected_min_w`） | 数字输入 | 整数；**`-1` 或 ≥0** | 默认 `-1` → 落盘 **`0`** |
| **MinH**（`detected_min_h`） | 数字输入 | 整数；**`-1` 或 ≥0** | 默认 `-1` → 落盘 **`0`** |
| **MaxW**（`detected_max_w`） | 数字输入 | 整数；**`-1` 或 ≥0** | 默认 `-1` → 落盘 **`2147483647`**（`2_147_483_647`） |
| **MaxH**（`detected_max_h`） | 数字输入 | 整数；**`-1` 或 ≥0** | 默认 `-1` → 落盘 **`2147483647`** |
| **Ops** | 图标 | — | **🗑** 删除本行 |

本阶段 **不**暴露 `iou`（仅 yolo8/yolo11 有意义；后续可加）。

#### 默认行

打开空配置或新建 GIE 时，表中预置 **一行**：

| class | conf | topk | detected_min_w | detected_min_h | detected_max_w | detected_max_h |
|-------|------|------|----------------|----------------|----------------|----------------|
| `all` | `0.25` | `300` | `-1` | `-1` | `-1` | `-1` |

#### 增删

| 操作 | 行为 |
|------|------|
| **[+]** | 追加一行：`class` 未选；其余同默认行数值 |
| **🗑** | 删除该行；**`all` 亦可删**（否则无法表达「仅启用部分类」） |
| 删至 0 行 | 允许处于编辑态；**Save** 时校验失败（至少一行） |

#### 过滤语义（对齐 generator）

| `class_attrs` 内容 | 运行时含义 |
|--------------------|------------|
| 含 **`all`**（可另有数字 class 行） | **全开**；`filter-out-class-ids` 不写；数字行仅为该类覆盖阈值，`all` 覆盖其余类 |
| **无** `all`，仅数字 class（如 `0`、`2`） | **仅启用这些类**；其余类写入 `filter-out-class-ids` |
| 仅 `all` | 全开；统一阈值 |

同一表内 **`class` 值不可重复**（不可两行都是 `all` 或都是 `0`）。

#### UI → 落盘映射

UI 行在 **Save** 时转为对象（key = class；`-1` 的 detected 字段展开为默认值）：

```yaml
# 例：表中 all + 0 两行，detected 均为 -1
class_attrs:
  all:
    conf: 0.25
    topk: 300
    detected_min_w: 0
    detected_min_h: 0
    detected_max_w: 2147483647
    detected_max_h: 2147483647
  "0":
    conf: 0.9
    topk: 10
    detected_min_w: 0
    detected_min_h: 0
    detected_max_w: 2147483647
    detected_max_h: 2147483647
```

与 generator 模板字段 `pgie.class_attrs` / `PgieParser` 一致；`detected_*` 在 UI 存 `-1`、落盘写实值，避免把哨兵值传给 nvinfer。

---

### 数据模型

#### GIE 模板（页面级）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo26n-default",
  "model_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "class_attrs": [
    {
      "class": "all",
      "conf": 0.25,
      "topk": 300,
      "detected_min_w": -1,
      "detected_min_h": -1,
      "detected_max_w": -1,
      "detected_max_h": -1
    }
  ]
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | UUID | GIE **模板** id |
| `name` | string | 顶栏 **Name**；全库唯一 |
| `model_id` | UUID | 顶栏 **Model**；对应 Models **model id** |
| `class_attrs` | array | 表行列表；顺序即 UI 顺序；**Save** 时再转为 dict |

**class_attrs[] 行**

| 字段 | 类型 | 说明 |
|------|------|------|
| `class` | string \| int | `"all"` 或类别 id（`0`–`num_class-1`） |
| `conf` | number | `[0, 1]` |
| `topk` | int | `[0, 300]` |
| `detected_min_w` / `detected_min_h` | int | `-1` 或 `≥0` |
| `detected_max_w` / `detected_max_h` | int | `-1` 或 `≥0`；若均 ≥0 建议 `max ≥ min`（校验） |

---

### Toast 提示（GIE）

| 操作 | 文案 | 说明 |
|------|------|------|
| **Save** 成功 | **`GIE saved`** | |
| 换模导致 class 失效 | **`Class {id} invalid for selected model`** | 可合并为一条摘要 |
| 校验失败 | 表内联 / 弹窗，不 Toast | |
| API 4xx/5xx | **`{message}`** | |

---

### 边界与校验（GIE）

| 规则 | 说明 |
|------|------|
| Name 必填 | 非空；全库唯一 |
| Model 必填 | 未选 Model 不可 **Save** |
| Model 状态 | 仅 `built`；若已保存模型后来变为非 Built → 打开时提示并禁用 Save 直至重选 |
| 至少一行 | `class_attrs.length ≥ 1` |
| class 必填 | 每行必须已选 `class` |
| class 唯一 | 表内 `class` 不重复 |
| conf | `0 ≤ conf ≤ 1` |
| topk | 整数且 `0 ≤ topk ≤ 300` |
| detected_* | 整数；`-1` 或 `≥ 0`；非 `-1` 时 `max_* ≥ min_*`（同维度） |
| 无 `all` 时 | 数字 class 须 ∈ 当前模型类别集合 |
| 未 Save 离开 | **Cancel** / **[x]** 须确认丢弃 |

---

### 相关 API（建议）

| 方法 | 路径（示例） | 说明 |
|------|--------------|------|
| **GET** | `/ai_stream2/backend/models?status=built` | Model 下拉；展示 `name`，值 `id`；可带 `classes` / `num_class` 供 class 下拉 |
| **GET** | `/ai_stream2/backend/models/{model_id}` | 换模或打开时取类别列表 |
| **GET/POST/PUT/DELETE** | `.../gie-templates` / `.../gie-templates/{id}` | GIE 模板库 CRUD（主页弹窗 + 子页 Save） |

---

## Analyzer 子页面

由主页 **Analyzer 模板库弹窗** 的 **Add** / **✏** 进入。编辑一条 **Analyzer 模板**（与 Pipeline 同级、可被多条 Pipeline 引用）；用户在**参考图**上绘制分析区域/线/方向，**Save** 写入模板库（坐标系与 `config-width` / `config-height` 一致，见 [数据模型](#数据模型)）。

### 页面布局

```
+====================================================================================================+
| Analyzer                                                                                       [x] |
+====================================================================================================+
| Name   [ entrance_analytics________________ ]    Source [ cam-01 ▼ ] [↻]                      |
+====================================================================================================+
| Mode [ ROI Filtering ▼ ]   [▭] [⬡] [↗] [→] [✎]  |  Name          | Type              | Ops     |
|                              ^^^^^ 图形栏        | --------------- | ----------------- | ------- |
| +----------------------------------------------+ | DOOR           | ROI Filtering     | ✏ 🗑  |
| | ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  · | | ENTRANCE       | Overcrowding      | ✏ 🗑  |
| | ·  ┌──────────┐              ·  ·  ·  ·  · | | Exit           | Line Crossing     | ✏ 🗑  |
| | ·  │  DOOR    │    ────→     ·  ·  ·  ·  · | | South          | Direction Detect. | ✏ 🗑  |
| | ·  └──────────┘              ·  ·  ·  ·  · | |                 |                   |         |
| | ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  · | |                 |                   |         |
| +----------------------------------------------+ |                 |                   |         |
|   ↑ 画布（参考图 + 标注层 + 红色虚线十字准星）      +-----------------+-------------------+---------+
+====================================================================================================+
|                                              [ Save ]    [ Cancel ]                                |
+====================================================================================================+
```

| 区域 | 说明 |
|------|------|
| 顶栏 **Name** | 当前 **Analyzer 模板**名称（非单条标注 Name）；非空；**全库唯一** |
| 顶栏 **Source** | 下拉 + **↻**；分组 **File** / **Stream**；见 [Source](#source) |
| **↻ Refresh** | 紧挨 Source 下拉右侧；**File** 重载文件、**Stream** 重新截帧 |
| **Mode** | 下拉；决定当前新建标注的 **Type** 与图形栏可用项；见 [模式栏](#模式栏-mode) |
| **图形栏** | 5 个互斥 Toggle：`Rectangle` / `Polygon` / `Line+Direction` / `Direction` / `Edit` |
| **画布** | 参考图 + 矢量标注；绘制模式下鼠标位置显示**红色虚线**十字准星（横线 + 竖线，延伸至画布边缘） |
| **右侧列表** | 已保存标注；列 **Name** / **Type** / **Operations**（✏ 属性编辑 / 🗑 删除） |
| 底栏 | **Save** 持久化全部标注；**Cancel** 关闭子页（有未保存编辑时确认丢弃） |

**布局补充**

| 项 | 说明 |
|----|------|
| 画布缩放 | 图片 **等比 fit** 入画布；坐标一律按**原图像素**存储（与 `config-width` / `config-height` 一致） |
| 十字准星 | 仅在 **非 Edit** 且已选可用图形工具、且 **Source 已加载参考图** 时显示；颜色 **红**、线型 **虚线** |
| 列表 **Type** | 只读；取该条标注创建时的 **Mode** 文案（`ROI Filtering` 等） |
| 多标注 | 同一 Mode 可有多条（如多个 `roi-<label>`）；**Name**（标注名）在**同一 Analyzer 内唯一** |

---

## Source

顶栏 **Source** 为 **下拉框** + 右侧 **↻** 图标；参考图加载成功后画布可用。实现建议：`ElSelect` + `ElOptionGroup`；隐藏 `<input type="file">` 供 **File** 触发。

### 控件布局

```
Source  [ cam-01                    ▼ ]  [↻]
         ↑ 收起态：当前 File 文件名或 Stream Name；未选时为占位 Select source
```

下拉展开：

```
┌──────────────────────────┐
│ File                     │
│   Choose file...         │
├──────────────────────────┤
│ Stream                   │
│   cam-01                 │
│   cam-02                 │
│   cam-06                 │
└──────────────────────────┘
```

### File 分组

| 项 | 说明 |
|----|------|
| 菜单项 | 固定一项 **`Choose file...`**（非已选文件名列表） |
| 点击 | 打开系统文件选择框；筛选 `jpg` / `jpeg` / `png` |
| 选中后 | 下拉收起态显示**文件名**（如 `snapshot.jpg`）；画布加载该图；记录 `source_kind: file` 与本地/服务端文件引用 |
| 格式 | 仅 `jpg` / `jpeg` / `png`；其它格式拒绝并 Toast **`Unsupported image format`** |

### Stream 分组

| 项 | 说明 |
|----|------|
| 数据来源 | `GET .../streams`（或等价列表 API）；与 [Streams 页面](./stream.md) 字段一致 |
| 可见条目 | 仅 **`enabled=true` 且 `status=online`** 的流；展示 **Name**；请求与绑定用 **stream id** |
| 点击某流 | 立即对该流**截帧**（选中瞬间）；成功则画布展示截图；下拉收起态显示该流 **Name**；记录 `source_kind: stream`、`source_stream_id` |
| 空分组 | 无符合条件流时 **Stream** 组下展示灰色 **`No online streams`**（不可点） |
| 请求中 | 下拉收起态或画布区 loading；防重复点击 |

> **Online and Enabled** 对应 API：`enabled=true` 且 `status=online`。`enabled=false`（Disabled）或 `status=offline`（Offline）**不出现在列表**。

### Refresh（↻）

| 项 | 说明 |
|----|------|
| 位置 | Source 下拉**紧右侧** |
| 可用 | 已选 File 或 Stream 时可点；未选 Source 时 **disabled** |
| 请求中 | 图标 **disabled** 或 loading；防重复点击 |
| **File** | 用当前已选文件**重新读取**并刷新画布（同一文件路径 / 同一已上传 blob） |
| **Stream** | 对当前 `source_stream_id` **再次截帧**并刷新画布 |
| Tooltip | 建议 **`Refresh source`** |

### 换源与刷新时的标注

| 场景 | 行为 |
|------|------|
| 首次选 Source | 清空进行中绘制；无标注时直接换图 |
| **↻** 刷新且 **`config_width` / `config_height` 不变** | 仅换参考图；**保留**已有标注与 `points` |
| 换 File / 换 Stream / 刷新后**分辨率变化** | 二次确认：「Resolution changed. Clear annotations?」；确认则清空标注，取消则保留标注（实现自定，建议仍更新底图并 Toast 提示尺寸已变） |

---

## 模式栏（Mode）

下拉四项；切换时**同步约束图形栏**并选中该 Mode 的**默认图形**。

| Mode | 可选图形 | 默认图形 | 禁用图形 |
|------|----------|----------|----------|
| **ROI Filtering** | Rectangle、Polygon | **Rectangle** | Line+Direction、Direction |
| **Overcrowding** | Rectangle、Polygon | **Rectangle** | Line+Direction、Direction |
| **Line Crossing** | Line+Direction | **Line+Direction** | Rectangle、Polygon、Direction |
| **Direction Detection** | Direction | **Direction** | Rectangle、Polygon、Line+Direction |

| 行为 | 说明 |
|------|------|
| 切换 Mode | 禁用项 **disabled**（灰显、不可点）；若当前图形工具被禁用 → 自动切到上表**默认图形** |
| 新建标注 | 始终继承**当前 Mode** 为列表 **Type**；图形种类由当前图形工具决定 |
| Edit 模式 | **不受 Mode 限制**；可选中并编辑任意已存在标注 |

---

## 图形栏

五个图标**互斥**（同一时刻仅一项 active）；**Edit** 与其它四项互斥。

| 图标 | 名称 | 说明 |
|:----:|------|------|
| ▭ | **Rectangle** | 矩形（两点击定对角） |
| ⬡ | **Polygon** | 多边形（逐点闭合） |
| ↗ | **Line+Direction** | 线段 + 方向箭头（四点） |
| → | **Direction** | 方向向量（两点箭头） |
| ✎ | **Edit** | 几何编辑模式；见 [Edit 模式](#edit-模式) |

未加载 **Source** 参考图时，除 **Edit** 外全部 **disabled**；**Edit** 在无标注时 **disabled**。

---

## 绘制交互

通用规则：

| 规则 | 说明 |
|------|------|
| 坐标 | 点击位置映射为原图像素 `(x, y)`，四舍五入为整数 |
| 预览 | 绘制过程中实时预览图形（红色实线）；未完成时不写入列表 |
| 取消进行中绘制 | **Esc** 或切换图形工具 / Mode → 丢弃当前未完成的全部点列 |
| 右键撤销 | **右键**撤销**上一次**左键落点（不弹系统菜单）：Rectangle / Direction 撤销首点；Polygon / Line+Direction 逐点回退 |
| 完成 | 满足闭合条件后，在**完成点击坐标附近**弹出 **Popover**（非居中 Dialog）；**√** 确认后写入列表并在画布固化 |

### Rectangle

| 步骤 | 行为 |
|------|------|
| 1 | 第一次单击 → 左上角（或任意对角点） |
| 2 | 移动鼠标 → 预览矩形（随十字准星更新） |
| 3 | 第二次单击 → 对角点确定；在该点附近弹出 **Name Popover** |

**Rectangle Name Popover**（ROI Filtering / Overcrowding）：

```
  (第二次点击附近)
  +---------------------------+
  | [ DOOR__________ ]  √  ×  |
  +---------------------------+
```

| 字段 | 说明 |
|------|------|
| **Name** | 标注名；非空；**Analyzer 内唯一**；对应 nvdsanalytics 的 `<label>` |
| **√** | 确认：写入列表 + 画布；关闭弹窗 |
| **×** | 取消：不保存该次绘制 |

> Rectangle 存为 4 顶点矩形多边形（顺时针）：`(x1,y1,x2,y1,x2,y2,x1,y2)`，与 nvdsanalytics `roi-<label>` 多边形格式一致。

### Polygon

| 步骤 | 行为 |
|------|------|
| 1 | 单击添加顶点；段与段之间直线连接 |
| 2 | 移动鼠标 → 预览下一实线段（至当前鼠标点） |
| 3 | 单击**首顶点**（命中容差 **8px** 屏幕距离）→ 闭合；弹出 **Polygon 属性 Popover** |
| 最少顶点 | **3**；未闭合前 **右键**撤销上一顶点，**Esc** 清空全部 |

**Polygon 属性 Popover**：

```
+--------------------------------------------------+
| Annotation                                   [x] |
+--------------------------------------------------+
| Name              [ ENTRANCE__________ ]  √ ×      |
| Object Threshold  [ 5_________________ ]         |
| Time Threshold    [ 3000______________ ]  (ms)   |
+--------------------------------------------------+
```

| 字段 | Mode | 说明 |
|------|------|------|
| **Name** | 全部 | 标注名；**Analyzer 内唯一** |
| **Object Threshold** | **Overcrowding** | 整数 **≥1**；默认 `5`；对应 `object-threshold` |
| **Time Threshold** | **Overcrowding** | 整数 **≥1**（毫秒）；默认 `3000`；对应 `time-threshold` |
| **√ / ×** | 全部 | 确认写入 / 取消绘制 |

> **ROI Filtering** 下 Polygon 弹窗**仅 Name + √ ×**（无 Threshold 行）。**Overcrowding** 下显示全部三字段。

### Line+Direction

| 步骤 | 行为 |
|------|------|
| 1 | 第一次单击 → 线段端点 A |
| 2 | 第二次单击 → 线段端点 B；显示线段 |
| 3 | 第三次单击 → 方向向量起点 C |
| 4 | 第四次单击 → 方向向量终点 D；显示箭头 **C→D**；弹出 **Line+Direction 属性 Popover** |

顺序固定：**先线后方向**；方向箭头独立于线段（可不在线上）。

**Line+Direction 属性 Popover**（Line Crossing）：

```
+--------------------------------------------------+
| Annotation                                   [x] |
+--------------------------------------------------+
| Name    [ Exit_______________________ ]  √ ×     |
| Extend  [ ] Off   [x] On                         |
| Mode    ( ) strict  (•) balanced  ( ) loose      |
+--------------------------------------------------+
```

| 字段 | 说明 |
|------|------|
| **Name** | 标注名；**Analyzer 内唯一** |
| **Extend** | Toggle；**On** 时线段 AB **双向延长**至图像矩形边界后再参与越线计算；对应 `extended: 1` |
| **Mode** | 单选：`strict` / `balanced` / `loose`；默认 **balanced** |
| **√ / ×** | 确认 / 取消 |

> 持久化 8 整数：**方向** `(Cx,Cy,Dx,Dy)` + **线段** `(Ax,Ay,Bx,By)`，与 nvdsanalytics `line-crossing-<label>` 一致。

### Direction

| 步骤 | 行为 |
|------|------|
| 1 | 第一次单击 → 箭头起点 |
| 2 | 第二次单击 → 箭头终点；显示方向箭头；弹出 **Direction 属性 Popover** |

**Direction 属性 Popover**：

```
+--------------------------------------------------+
| Annotation                                   [x] |
+--------------------------------------------------+
| Name  [ South____________________ ]  √ ×         |
| Mode  ( ) strict  ( ) balanced  (•) loose          |
+--------------------------------------------------+
```

| 字段 | 说明 |
|------|------|
| **Name** | 标注名；**Analyzer 内唯一** |
| **Mode** | `strict` / `balanced` / `loose`；默认 **balanced** |
| **√ / ×** | 确认 / 取消 |

> 持久化 4 整数 `(x1,y1,x2,y2)` 方向向量；对应 `direction-<label>`。

---

## Edit 模式

点击图形栏 **✎ Edit** 进入；再次点击 **✎** 或其它图形工具退出。

| 操作 | 行为 |
|------|------|
| 点击图形主体（非控制点） | 选中该标注（高亮描边）；**按住左键拖动** → **平移整个图形** |
| 点击控制点 | 仅 **`click_points`（用户点击落点）** 有锚点；Rectangle 为对角两点，**不是**展开后的 4 角 |
| 拖动控制点 | **按住左键**更新对应 `click_points`；Rectangle **保持矩形**（由对角两点重算 `points` 四顶点）；实时预览 |
| 点击空白 | 取消选中 |
| 列表联动 | 选中图形时列表对应行高亮；点击列表行 → 画布选中同一条 |

Edit 模式下**不显示**十字准星；**不弹出**属性 Popover（几何变更即时生效，**Save** 时一并持久化）。

列表 **Operations ✏** 与 Edit 模式独立：打开该条标注的**属性 Popover**（字段同新建完成时），**不**自动进入 Edit 模式。

---

## 右侧列表

| 列 | 说明 |
|----|------|
| **Name** | 标注展示名 |
| **Type** | `ROI Filtering` / `Overcrowding` / `Line Crossing` / `Direction Detection` |
| **Operations** | **✏** 打开属性 Popover（可改 Name / Threshold / Extend / Mode）；**🗑** 删除（二次确认） |

| 操作 | 行为 |
|------|------|
| **✏** | 打开对应类型属性 Popover；**√** 更新列表与画布 |
| **🗑** | 确认后从列表与画布移除 |
| 行点击 | 画布选中该标注（不进入 Edit 亦可高亮） |

---

## 属性 Popover（汇总）

锚定在完成点击（或标注附近）；**Name** 行布局为 **`[ input ] √ ×`**（同一行）。

| 图形 | Mode | 弹窗字段 |
|------|------|----------|
| Rectangle | ROI Filtering | Name |
| Rectangle | Overcrowding | Name |
| Polygon | ROI Filtering | Name |
| Polygon | Overcrowding | Name、Object Threshold、Time Threshold |
| Line+Direction | Line Crossing | Name、Extend、Mode |
| Direction | Direction Detection | Name、Mode |

空 Name 时仅 **√** **disabled**。点 **√** 时若重名或 Threshold 非法 → 对应控件红框 + **Toast**；**不**关闭弹窗、**无**行内红字栏。

---

## 视觉规范

| 元素 | 样式 |
|------|------|
| 十字准星 | 红色 `#FF0000`；**虚线**；贯穿画布（**唯一**使用虚线的元素） |
| 标注描边 | 按 **Type** 分色实线；默认 **2px**；选中 **3px** 高亮 |
| ROI Filtering | 黄色 `#F5C518` |
| Overcrowding | 橙色 `#FF8C00` |
| Line Crossing | 绿色 `#22C55E` |
| Direction Detection | 红色 `#FF0000` |
| 绘制 / Edit 锚点 | 仅 **`click_points`** 实心圆 **radius 4**（Type 色）；Edit 下可拖；Rectangle 拖对角点仍保持矩形 |
| 方向箭头 | 线段 + 三角箭头头；颜色与所属 Type 一致 |
| 预览（未完成） | 当前 Mode 对应颜色的实线（可略透明） |
| Popover 打开时 | `pendingDraft` 保留刚完成图形；× → `clearPending()`；√ → 写入 `annotations` 后清除 |

---

## 交互流程

```
选 Source（File / Stream） ──> 画布显示参考图；图形栏（除 Edit）可用
Source ↻               ──> File 重载 / Stream 重新截帧
选择 Mode             ──> 约束图形栏；选中默认图形
选择图形工具          ──> 十字准星开启；按类型逐步点击绘制
绘制完成              ──> 打开属性 Popover；√ 写入列表
点 Edit               ──> 几何编辑；拖动整体或控制点
列表 ✏                ──> 打开属性 Popover 修改元数据
列表 🗑               ──> 确认删除
Save                  ──> PATCH/PUT analytics 配置；Toast 成功
Cancel / [x]          ──> 有未保存变更时确认丢弃
右键                  ──> 撤销上一次落点（进行中绘制）
Esc                   ──> 取消进行中的全部绘制
```

---

## 前端实现（vue-konva）

### 依赖与加载

| 包 | 用途 |
|----|------|
| `konva` | Canvas 2D 引擎 |
| `vue-konva` | Vue 3 绑定（`Stage` / `Layer` / `Image` / `Line` / `Arrow` / `Rect` / `Circle` / `Group`） |

Analyzer 子页**路由级 lazy-load**，避免 Konva 进入主包：

```typescript
{
  path: '/pipelines/:id/analyzer',
  component: () => import('@/views/pipeline/AnalyzerPage.vue'),
}
```

### 组件划分

| 组件 / composable | 职责 |
|-------------------|------|
| `AnalyzerPage.vue` | 顶栏、Mode、图形栏、列表、Save/Cancel；Pinia 或本地 state 持有 `annotations` |
| `AnalyzerCanvas.vue` | `v-stage` + 背景图 Layer + 标注 Layer + 十字准星 Layer + 预览 Layer |
| `useAnalyzerCanvas.ts` | 坐标换算；仅 `draftPoints` 状态；`mousedown` 左落点/右撤销；`pendingDraft` 至 Popover 确认/取消；`enabled` 门闩 |
| `useAnnotationEdit.ts` | Edit 模式：选中、整体拖动、控点拖动 |
| `AnnotationPropertyPopover.vue` | 各类型属性 Popover（锚定点击点；非 `ElDialog`） |

Konva 形状与业务 `shape` 映射：

| `shape` | Konva 节点 | 说明 |
|---------|------------|------|
| `rectangle` | `Line`（`closed: true`，4 点）或 `Rect` | 持久化仍用 4 顶点 `points`；绘制预览可用 `Rect` |
| `polygon` | `Line`（`closed: true`） | 顶点点列 `points: flat[]` |
| `line_direction` | `Group` → `Line`（线段 AB）+ `Arrow`（方向 CD） | 8 整数语义不变 |
| `direction` | `Arrow` | `pointerAtBeginning: false`，`pointerAtEnding: true` |
| 标注 / 预览 | `Line` / `Arrow` / `Rect` | 按 Type 分色实线（见视觉规范；无 `dash`） |
| 十字准星 | 2 × `Line` | `listening: false`，`dash: [6, 4]`，`stroke: '#FF0000'` |
| Edit 控点 | `Circle`（`radius: 4`，`draggable: true`） | 绑在选中标注顶点 / 端点上 |

### 坐标系

Stage 显示尺寸 = 容器宽高；**存储坐标一律为原图像素**（`config_width` × `config_height`）。

```
scale = min(stageWidth / imageWidth, stageHeight / imageHeight)
offsetX = (stageWidth  - imageWidth  * scale) / 2
offsetY = (stageHeight - imageHeight * scale) / 2

stageX = offsetX + pixelX * scale
pixelX = (stageX - offsetX) / scale
```

`useAnalyzerCanvas` 暴露 `pixelToStage` / `stageToPixel`；Stage `click` / `mousemove` 事件先换算再写入 `annotations[].points`。

### 绘制状态机

| 工具 | 状态 | 点击行为 |
|------|------|----------|
| Rectangle | `idle` → `firstCorner` → 弹窗 | 第 1 次存角点；mousemove 更新预览 `Rect`；第 2 次定对角 → 转 4 顶点 → 开弹窗 |
| Polygon | `adding` | 每次追加点；mousemove 预览末段；点击首点（stage 距离 ≤ 8/scale px）→ 闭合 → 弹窗 |
| Line+Direction | `lineA` → `lineB` → `dirC` → `dirD` | 四步各一点；第 2 步后显示 `Line`；第 4 步后显示 `Arrow` → 弹窗 |
| Direction | `arrowStart` → 弹窗 | 两点 → `Arrow` → 弹窗 |
| Edit | — | 不进入上述状态；点击图形 `selectAnnotation(id)` |

`Esc`、切换工具、切换 Mode → `resetDraft()` 清空预览与临时点列。  
指针只用 `mousedown`：`button=0` 落点，`button=2` 撤销上一落点（不使用 `click`，避免右键误触发完成）。

### Edit 模式

| 操作 | 实现 |
|------|------|
| 选中 | 点击 `Group` / `Line` / `Arrow`；`selectedId` 高亮描边（`strokeWidth + 1`） |
| 平移 | 选中 `Group` 设 `draggable: true`；`dragend` 将 delta 反算到像素坐标写回 `points` |
| 控点 | 选中时仅在 `click_points` 渲染锚点；`dragmove` 写回 `click_points` 再 `syncPointsFromClicks` |
| Rectangle | 2 锚点（对角点击）；拖动后 `rectCornersToPoints` 重建四顶点，`shape` 仍为 `rectangle` |
| Line+Direction | 4 锚点 A–D（`click_points` 顺序）；同步打包回 `points` 的 8 整数格式 |

列表 **✏** 仅开 Element Plus 弹窗改元数据，**不**切换 Edit 工具。

### 与 Element Plus 分工

| 层 | 技术 |
|----|------|
| 画布、预览、十字准星、几何编辑 | vue-konva |
| Mode 下拉、图形栏 Toggle、**Source 下拉与 ↻**、列表、Save/Cancel、Toast | Element Plus |
| 属性 Popover（锚定画布点击） | 本页组件（非 Dialog） |
| Source **File** | 隐藏 `input[type=file]` + `URL.createObjectURL` 或上传 API |
| Source **Stream** | `GET .../streams` 筛 Online+Enabled；`POST .../streams/{stream_id}/snapshot` 取截图 URL / blob → `HTMLImageElement` |

### 性能与层级

```
Layer 0  image      （Konva.Image，不监听事件）
Layer 1  annotations（已保存标注，Edit 下 draggable）
Layer 2  draft       （进行中预览，红色实线）
Layer 3  crosshair   （十字准星，红色虚线，listening: false）
Layer 4  anchors     （Edit 控点，仅选中时渲染）
```

窗口 `resize` 时重算 `scale` / `offset` 并重绘 Stage（`@vue:mounted` + `ResizeObserver`）。

---

## 数据模型

### Analyzer 模板（页面级）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "entrance_analytics",
  "source_kind": "stream",
  "source_stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "source_file_name": null,
  "source_image_url": "/media/pipelines/.../snapshot.jpg",
  "config_width": 1920,
  "config_height": 1080,
  "captured_at": "2026-07-02T10:15:00Z",
  "annotations": []
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | UUID | Analyzer **模板** id |
| `name` | string | 顶栏 **Name**；全库唯一 |
| `source_kind` | enum | `file` \| `stream` |
| `source_stream_id` | UUID? | `source_kind=stream` 时必填；对应 Streams **stream id** |
| `source_file_name` | string? | `source_kind=file` 时展示用文件名 |
| `source_image_url` | string | 当前参考图存储 URL（File 上传或 Stream 截帧产物） |
| `config_width` / `config_height` | int | 与当前参考图像素一致；写入 nvdsanalytics `property` |
| `captured_at` | datetime? | Stream 截帧或 File 最后加载时间（可选） |
| `annotations` | array | 标注列表 |

**Save** 时持久化 `source_kind` 与 file/stream 引用；再次打开 Analyzer 时按保存的 Source 类型恢复下拉选中项，并加载 `source_image_url`（Stream 类型**不**自动重新截帧，除非用户点 **↻**）。

### 标注（Annotation）

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "name": "DOOR",
  "type": "roi_filtering",
  "shape": "rectangle",
  "points": [100, 200, 400, 200, 400, 500, 100, 500],
  "click_points": [100, 200, 400, 500],
  "object_threshold": null,
  "time_threshold": null,
  "extend": null,
  "mode": null
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | UUID | 标注 id |
| `name` | string | **Analyzer 内唯一** |
| `type` | enum | `roi_filtering` \| `overcrowding` \| `line_crossing` \| `direction_detection` |
| `shape` | enum | `rectangle` \| `polygon` \| `line_direction` \| `direction` |
| `points` | int[] | 像素坐标序列；含义见下表（写入 nvdsanalytics） |
| `click_points` | int[]? | **用户点击落点**（画布圆点）；Rectangle 为两次对角点击，可与 `points` 4 顶点不同 |
| `object_threshold` | int? | Overcrowding |
| `time_threshold` | int? | Overcrowding（ms） |
| `extend` | bool? | Line Crossing |
| `mode` | string? | `strict` \| `balanced` \| `loose`（Line Crossing / Direction Detection） |

**points 含义**

| shape | points |
|-------|--------|
| `rectangle` | 4 顶点 ×2：`x1,y1,…,x4,y4`（顺时针矩形） |
| `polygon` | 顶点交替：`x1,y1,x2,y2,…` |
| `line_direction` | 8 值：`dir_x1,dir_y1,dir_x2,dir_y2,line_x1,line_y1,line_x2,line_y2` |
| `direction` | 4 值：`x1,y1,x2,y2`（箭头起止） |

### 与 nvdsanalytics 映射

| type | 配置键前缀 | 坐标键 |
|------|------------|--------|
| `roi_filtering` | `roi-filtering-stream-{stream_id}` | `roi-{name}` |
| `overcrowding` | `overcrowding-stream-{stream_id}` | `roi-{name}` |
| `line_crossing` | `line-crossing-stream-{stream_id}` | `line-crossing-{name}` |
| `direction_detection` | `direction-detection-stream-{stream_id}` | `direction-{name}` |

**Save** 时后端按 `type` 分组合并为 nvdsanalytics YAML；`stream_id` 由 Pipeline 源流与 mux pad 约定（实现层映射，本子页不展示）。

### Source 相关 API（建议）

| 方法 | 路径（示例） | 说明 |
|------|--------------|------|
| **GET** | `.../streams?enabled=true&status=online` | Source **Stream** 分组列表；展示 Name，值为 **stream id** |
| **POST** | `.../streams/{stream_id}/snapshot` | 对指定流截当前帧；响应含 `image_url`（或 binary）、`width`、`height`、`captured_at` |
| **PUT** | `.../analyzers/{id}/source` | **Save** 时写入 `source_kind`、file 引用或 `source_stream_id` 及 `source_image_url` |

Stream 列表筛选规则与 [stream.md](./stream.md) 表格 **Online**（`status=online` 且 `enabled=true`）一致。

---

## Toast 提示（Analyzer）

| 操作 | 文案 | 说明 |
|------|------|------|
| **Save** 成功 | **`Analyzer saved`** | |
| Stream 截帧失败 | **`Snapshot failed for {name}`** | 流 Offline / 拉流失败 |
| 不支持的图片格式 | **`Unsupported image format`** | File 非 jpg/png |
| 删除标注 / 换源成功 | — | |
| 校验失败（Name/Source） | 对应控件红框；**无**底部文案栏；API 错误仍 Toast | |
| API 4xx/5xx | **`{message}`** | |

---

## 边界与校验（Analyzer）

| 规则 | 说明 |
|------|------|
| Source 必填 | 未选 File 且未选 Stream 时不可绘制；**Save** 时若仍无参考图 → Source 下拉红框 |
| Stream 列表 | 仅 `enabled=true` 且 `status=online` |
| **↻** | 未选 Source 时 **disabled** |
| 标注 Name | 非空；**同一 Analyzer 内唯一** |
| Analyzer Name / Source | 非空；**Save** 时空 Name / 未选 Source → 红框；模板名冲突（409）→ Name 红框 + Toast；弹窗内**无**底部校验文案栏 |
| Polygon | 至少 3 顶点；自交允许（与 nvdsanalytics 行为一致，不额外禁止） |
| Rectangle | 两次点击不可为同一点（< 2px 视为无效，忽略第二次） |
| Mode ↔ 图形 | 严格遵循 [模式栏](#模式栏-mode) 禁用表 |
| 未 Save 离开 | **Cancel** / **[x]** 须确认丢弃 |
| Edit 与绘制 | 进入 Edit 时若有未完成点列 → 自动取消该次绘制 |

---

## 与管理端关系

| 端 | 内容 |
|----|------|
| **主页面** | Pipeline 列表；**GIE** / **Analyzer** 打开模板库弹窗 |
| **Pipeline 子页面** | Name / Type / Probe / Generator；**引用** GIE / Analyzer 模板 |
| **GIE 子页面** | GIE 模板：Name + Model + `class_attrs` |
| **Analyzer 子页面** | Analyzer 模板：参考图标注几何规则 |
| **Models（管理端）** | GIE **Model**、Class 弹窗 label |
| **Streams（管理端）** | Generator / Analyzer **Streams**；Online + Enabled |
| **Generator** | 仅在主页 **▶ / start** 时由后端代理调用；生成配置供 DeepStream 读取 |
| **DeepStream 运行时** | 读取 Generator 落盘配置；坐标按 `config-width` / `config-height` 缩放 |
