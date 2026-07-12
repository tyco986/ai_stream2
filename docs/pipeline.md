# Pipeline 页面

管理端 Pipeline 页面，用于配置 DeepStream 推理流水线（源、模型、追踪、分析、输出等）。本文档先定义 **Analyzer** 子页面：在参考帧上标注 **nvdsanalytics** 规则（ROI / 拥挤 / 越线 / 方向），交互参考简易版 LabelMe。

**Analyzer** 与每条标注的唯一标识均为 **UUID**（RFC 4122），由后端生成、不可修改。列表默认展示 **Name**；API 与前端 key 以 **id** 为准。

画布层采用 **vue-konva + konva** 单库实现（Rect / Polygon / Line / Arrow / 拖拽 / 控点编辑均在同一 Stage 完成）。UI 壳（Mode 下拉、图形栏、列表、属性弹窗）仍用 **Element Plus**。

> 主页面布局、Pipeline CRUD、与 generator 的对接见后续章节；本节仅 **Analyzer 子页面**。

---

## Analyzer 子页面

由 Pipeline 配置流程进入（入口实现自定，如节点 **nvdsanalytics → Configure**）。用户在**参考图**上绘制分析区域/线/方向，保存后写入该 Pipeline 的 analytics 配置（坐标系与 `config-width` / `config-height` 一致，见 [数据模型](#数据模型)）。

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
| 顶栏 **Name** | 当前 **Analyzer 实例**名称（Pipeline 级配置名，非单条标注 Name）；非空；**同一 Pipeline 内唯一** |
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
| 预览 | 绘制过程中实时预览图形（半透明描边）；未完成时不写入列表 |
| 取消进行中绘制 | **Esc** 或切换图形工具 / Mode → 丢弃当前未完成的点列 |
| 完成 | 满足闭合条件后弹出**属性弹窗**；**√** 确认后写入列表并在画布固化 |

### Rectangle

| 步骤 | 行为 |
|------|------|
| 1 | 第一次单击 → 左上角（或任意对角点） |
| 2 | 移动鼠标 → 预览矩形（随十字准星更新） |
| 3 | 第二次单击 → 对角点确定；弹出 **Rectangle 属性弹窗** |

**Rectangle 属性弹窗**（ROI Filtering / Overcrowding）：

```
+----------------------------------+
| Annotation                   [x] |
+----------------------------------+
| Name  [ DOOR_____________ ]  √ × |
+----------------------------------+
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
| 2 | 移动鼠标 → 预览下一虚线段（至当前鼠标点） |
| 3 | 单击**首顶点**（命中容差 **8px** 屏幕距离）→ 闭合；弹出 **Polygon 属性弹窗** |
| 最少顶点 | **3**；未闭合前 **Esc** 取消 |

**Polygon 属性弹窗**：

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
| 4 | 第四次单击 → 方向向量终点 D；显示箭头 **C→D**；弹出 **Line+Direction 属性弹窗** |

顺序固定：**先线后方向**；方向箭头独立于线段（可不在线上）。

**Line+Direction 属性弹窗**（Line Crossing）：

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
| 2 | 第二次单击 → 箭头终点；显示方向箭头；弹出 **Direction 属性弹窗** |

**Direction 属性弹窗**：

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
| 点击控制点 | Rectangle / Polygon：**顶点**；Line+Direction：**A、B、C、D** 四点；Direction：**起点、终点** |
| 拖动控制点 | **按住左键**更新对应坐标；实时预览 |
| 点击空白 | 取消选中 |
| 列表联动 | 选中图形时列表对应行高亮；点击列表行 → 画布选中同一条 |

Edit 模式下**不显示**十字准星；**不弹出**属性窗（几何变更即时生效，**Save** 时一并持久化）。

列表 **Operations ✏** 与 Edit 模式独立：打开该条标注的**属性弹窗**（字段同新建完成时），**不**自动进入 Edit 模式。

---

## 右侧列表

| 列 | 说明 |
|----|------|
| **Name** | 标注展示名 |
| **Type** | `ROI Filtering` / `Overcrowding` / `Line Crossing` / `Direction Detection` |
| **Operations** | **✏** 打开属性弹窗（可改 Name / Threshold / Extend / Mode）；**🗑** 删除（二次确认） |

| 操作 | 行为 |
|------|------|
| **✏** | 打开对应类型属性弹窗；**Save** 弹窗内 **√** 更新列表与画布 |
| **🗑** | 确认后从列表与画布移除 |
| 行点击 | 画布选中该标注（不进入 Edit 亦可高亮） |

---

## 属性弹窗（汇总）

各类型弹窗 **Name** 行布局均为 **`Name [ input ] √ ×`**（√ × 与 Name **同一行**右对齐）。

| 图形 | Mode | 弹窗字段 |
|------|------|----------|
| Rectangle | ROI Filtering | Name |
| Rectangle | Overcrowding | Name |
| Polygon | ROI Filtering | Name |
| Polygon | Overcrowding | Name、Object Threshold、Time Threshold |
| Line+Direction | Line Crossing | Name、Extend、Mode |
| Direction | Direction Detection | Name、Mode |

校验失败（重名、空 Name、Threshold 非正整数）时 **√** **disabled** 或行内红字提示；**不**关闭弹窗。

---

## 视觉规范

| 元素 | 样式 |
|------|------|
| 十字准星 | 红色 `#FF0000`；虚线 `4px` 间隔；贯穿画布 |
| 标注描边 | 默认 **2px** 实线；选中 **3px** 高亮 |
| 控制点 | 小圆 **6px**；Edit 模式下显示 |
| 方向箭头 | 线段 + 三角箭头头；颜色与描边一致 |
| 预览（未完成） | 半透明 / 虚线 |

---

## 交互流程

```
选 Source（File / Stream） ──> 画布显示参考图；图形栏（除 Edit）可用
Source ↻               ──> File 重载 / Stream 重新截帧
选择 Mode             ──> 约束图形栏；选中默认图形
选择图形工具          ──> 十字准星开启；按类型逐步点击绘制
绘制完成              ──> 打开属性弹窗；√ 写入列表
点 Edit               ──> 几何编辑；拖动整体或控制点
列表 ✏                ──> 打开属性弹窗修改元数据
列表 🗑               ──> 确认删除
Save                  ──> PATCH/PUT analytics 配置；Toast 成功
Cancel / [x]          ──> 有未保存变更时确认丢弃
Esc                   ──> 取消进行中的绘制
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
| `useAnalyzerCanvas.ts` | 坐标换算、绘制状态机、Stage 点击路由、Esc 取消 |
| `useAnnotationEdit.ts` | Edit 模式：选中、整体拖动、控点拖动 |
| `AnnotationPropertyDialog.vue` | 各类型属性弹窗（Element Plus `ElDialog`） |

Konva 形状与业务 `shape` 映射：

| `shape` | Konva 节点 | 说明 |
|---------|------------|------|
| `rectangle` | `Line`（`closed: true`，4 点）或 `Rect` | 持久化仍用 4 顶点 `points`；绘制预览可用 `Rect` |
| `polygon` | `Line`（`closed: true`） | 顶点点列 `points: flat[]` |
| `line_direction` | `Group` → `Line`（线段 AB）+ `Arrow`（方向 CD） | 8 整数语义不变 |
| `direction` | `Arrow` | `pointerAtBeginning: false`，`pointerAtEnding: true` |
| 十字准星 | 2 × `Line` | `listening: false`，`dash: [6, 4]`，`stroke: '#f00'` |
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

### Edit 模式

| 操作 | 实现 |
|------|------|
| 选中 | 点击 `Group` / `Line` / `Arrow`；`selectedId` 高亮描边（`strokeWidth + 1`） |
| 平移 | 选中 `Group` 设 `draggable: true`；`dragend` 将 delta 反算到像素坐标写回 `points` |
| 控点 | 选中时在顶点渲染 `Circle` 锚点；锚点 `dragmove` 更新对应 `points` 下标 |
| Line+Direction | 4 个锚点：A、B、C、D 分别对应 `points` 中下标 4–7（线）、0–3（方向） |

列表 **✏** 仅开 Element Plus 弹窗改元数据，**不**切换 Edit 工具。

### 与 Element Plus 分工

| 层 | 技术 |
|----|------|
| 画布、预览、十字准星、几何编辑 | vue-konva |
| Mode 下拉、图形栏 Toggle、**Source 下拉与 ↻**、列表、属性弹窗、Save/Cancel、Toast | Element Plus |
| Source **File** | 隐藏 `input[type=file]` + `URL.createObjectURL` 或上传 API |
| Source **Stream** | `GET .../streams` 筛 Online+Enabled；`POST .../streams/{stream_id}/snapshot` 取截图 URL / blob → `HTMLImageElement` |

### 性能与层级

```
Layer 0  image      （Konva.Image，不监听事件）
Layer 1  annotations（已保存标注，Edit 下 draggable）
Layer 2  draft       （进行中预览，虚线/半透明）
Layer 3  crosshair   （十字准星，listening: false）
Layer 4  anchors     （Edit 控点，仅选中时渲染）
```

窗口 `resize` 时重算 `scale` / `offset` 并重绘 Stage（`@vue:mounted` + `ResizeObserver`）。

---

## 数据模型

### Analyzer 实例（页面级）

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
| `id` | UUID | Analyzer 配置 id |
| `name` | string | 顶栏 **Name**；Pipeline 内唯一 |
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
| `points` | int[] | 像素坐标序列；含义见下表 |
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

## Toast 提示

| 操作 | 文案 | 说明 |
|------|------|------|
| **Save** 成功 | **`Analyzer saved`** | |
| Stream 截帧失败 | **`Snapshot failed for {name}`** | 流 Offline / 拉流失败 |
| 不支持的图片格式 | **`Unsupported image format`** | File 非 jpg/png |
| 删除标注 / 换源成功 | — | |
| 校验失败 | 弹窗内联，不 Toast | |
| API 4xx/5xx | **`{message}`** | |

---

## 边界与校验

| 规则 | 说明 |
|------|------|
| Source 必填 | 未选 File 且未选 Stream 时不可绘制；**Save** 时若仍无参考图 → 校验失败 |
| Stream 列表 | 仅 `enabled=true` 且 `status=online` |
| **↻** | 未选 Source 时 **disabled** |
| 标注 Name | 非空；**同一 Analyzer 内唯一** |
| Analyzer Name | 非空；**同一 Pipeline 内唯一** |
| Polygon | 至少 3 顶点；自交允许（与 nvdsanalytics 行为一致，不额外禁止） |
| Rectangle | 两次点击不可为同一点（< 2px 视为无效，忽略第二次） |
| Mode ↔ 图形 | 严格遵循 [模式栏](#模式栏-mode) 禁用表 |
| 未 Save 离开 | **Cancel** / **[x]** 须确认丢弃 |
| Edit 与绘制 | 进入 Edit 时若有未完成点列 → 自动取消该次绘制 |

---

## 与管理端关系

| 端 | 内容 |
|----|------|
| **本页（Pipeline → Analyzer）** | 参考图（File / Stream 截帧）标注；产出 nvdsanalytics 规则 |
| **Streams（管理端）** | Source **Stream** 分组的数据源；Online + Enabled 才可截帧 |
| **DeepStream 运行时** | 读取生成配置；坐标按 `config-width` / `config-height` 缩放 |
