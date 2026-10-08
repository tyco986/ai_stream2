# Events 页面

管理端 **Events** 页：按筛选条件浏览 Pipeline 产生的业务事件（警报），支持确认（Ack）、预览图、导出与现场采集包（Collect）。

无左侧栏；主界面为**全宽表格**。UI 控件统一用 **Element Plus**（经 `shared/ui`）。

API 契约见 [event_api.md](./event_api.md)。Event 显示名由 **ingest 写入 `event_label` 快照**；Events 页自行聚合筛选项（**不**依赖 Pipeline 词典）；本页**不**向用户展示原始 code。

| 概念 | 说明 |
|------|------|
| **Event** | 一条已落库的业务事件；主键 UUID；表格展示映射后的 **Event** 名 |
| **Status** | `new`（未确认）\| `acked`（已确认） |
| **Raw** | 事件关联的原图 |
| **Visualization** | 事件关联的可视化叠加图 |
| **Ack** / **Unack** | 互斥两按钮：`new` 可 **Ack**（→ `acked`）；`acked` 可 **Unack**（→ `new`）；另一侧 `disabled` |
| **Export** | 按当前筛选导出 CSV + Raw + Visualization（zip，不加密） |
| **Collect** | 现场收数；导出加密包（CSV + 图 + Labelme + YOLO），用于优化模型；**不**经 TOTP |

权限：`events.view_event`（查看 / 预览 / Export）；`events.change_event`（Ack / Unack / Collect）。无 `view` 时导航不出现本页。

---

## 主页面

```
+==========================================================================================================+
| Events                                                                                                   |
+==========================================================================================================+
| From [▼] To [▼] Stream [▼] Pipeline [▼] Event [▼] Status [▼]  [ Search____ ] [ Export ] [ Collect ] |
| [ Refresh ] [ Ack ] [ Unack ]                                                                            |
+--+---------------------+----------+------------+-----------+--------+---------------------------------+
|[]| Timestamp           | Stream   | Pipeline   | Event     | Status | Operations                      |
+--+---------------------+----------+------------+-----------+--------+---------------------------------+
|[]| 2026-07-22 19:40:12 | cam-01   | presence-a | Intrusion | New    | 🖼 ✓ ✕                          |
|[]| 2026-07-22 19:38:01 | cam-02   | presence-a | Clear     | Acked  | 🖼 ✓ ✕                          |
+--+---------------------+----------+------------+-----------+--------+---------------------------------+
|                                                            < 1  2  3 >                                   |
+==========================================================================================================+
```

| 区域 | 说明 |
|------|------|
| 页面标题 | 固定显示 `Events` |
| 筛选行 | 左：**From** / **To** / **Stream** / **Pipeline** / **Event** / **Status**；右：**Search** / **Export** / **Collect** |
| 操作行 | 左：**Refresh** / **Ack** / **Unack**（批量） |
| 数据表格 | 全宽；Checkbox + 列 + Operations；底部分页 |

**布局补充说明**

| 项 | 说明 |
|----|------|
| 无左侧栏 | 无 Groups、无侧栏日历 |
| 工具栏按钮 | **Refresh** / **Ack** 蓝底白字；**Unack** 与产品警告色一致；**Export** / **Collect** 次按钮样式 |
| **Ack** / **Unack** 禁用 | 勾选中无对应状态行时 `disabled` |
| 行内 / 弹窗 | **Ack** 与 **Unack** 两按钮互斥：仅当前状态可点的一侧可用 |
| 行主键 | 事件 **id**（UUID）；表格默认不展示 id |
| 默认排序 | 新在前（`occurred_at` 降序，同秒再按 `id`） |
| 进入页 | 默认 **From** = **To** = 今天；其余筛选为空（全部）；拉取第一页 |

---

## 筛选

| 控件 | 说明 |
|------|------|
| **From** | 起始日 `YYYY-MM-DD`；含当天 00:00:00 起 |
| **To** | 结束日 `YYYY-MM-DD`；含当天结束；须 **From ≤ To**（否则 Toast / 校验提示） |
| **Stream** | 流下拉；选项为 Name，值为 stream id；空 = 全部 |
| **Pipeline** | Pipeline 下拉；空 = 全部 |
| **Event** | 下拉；选项**按 Pipeline 分组**（组标题 = Pipeline Name；项 = 本页聚合的 `event_label` 显示名，来源于 ingest 快照）。未选 Pipeline 时展示全部分组；已选 Pipeline 时仅该组。空 = 全部。**不**展示原始 code |
| **Status** | `All` / `New` / `Acked`；空或 All = 全部 |
| **Search** | 右对齐；按 Stream Name / Pipeline Name / Event 显示名等子串过滤（与其它筛选取交）；输入防抖后 `GET` |

筛选变更均重置到第 1 页。当前筛选条件同时驱动：**列表**、**Preview 上一条/下一条结果集**、**Export**、**Collect**。

---

## 工具栏操作

| 元素 | 可用条件 | 行为 |
|------|----------|------|
| **Refresh** | 始终（有 view） | 按当前筛选与分页重新 `GET` |
| **Ack** | ≥1 勾选行为 `new`；有 `change` | 将勾选中的 `new` → `acked`（已是 `acked` 的跳过） |
| **Unack** | ≥1 勾选行为 `acked`；有 `change` | 将勾选中的 `acked` → `new`（已是 `new` 的跳过） |
| **Export** | 有 view；当前筛选下 `total≥1`（否则 disabled 或点后 Toast 无数据） | 按**当前筛选**导出 zip（见 [Export](#export)） |
| **Collect** | 有 `change`；当前筛选下 `total≥1` | 按**当前筛选**导出**加密**包（见 [Collect](#collect)）；**不**要求 TOTP |

勾选仅服务批量 **Ack** / **Unack**；**Export** / **Collect** **不**按勾选，只按筛选。

---

## 数据表格

### 列定义

| 列名 | 类型 | 说明 |
|------|------|------|
| Checkbox | — | 行多选；供批量 Ack / Unack |
| Timestamp | `str` | **完整时间戳**；展示格式 `YYYY-MM-DD HH:mm:ss`（站点时区）；对应 `occurred_at` |
| Stream | `str` | 流 Name |
| Pipeline | `str` | Pipeline Name |
| Event | `str` | **ingest 写入的显示名**（`event_label`）；仍**不**把「Code」作为列名 |
| Status | `str` | `New` / `Acked` |
| Operations | 图标组 | Preview / Ack / Unack |

### Operations

图标顺序固定：**Preview → Ack → Unack**。

| 顺序 | 图标 | 名称 | 权限 | 行为 |
|:----:|:----:|------|------|------|
| 1 | 🖼 | Preview | `view` | 打开 [Preview 弹窗](#preview-弹窗) |
| 2 | ✓ | Ack | `change` | `new` → `acked`；`acked` 时 `disabled` |
| 3 | ✕ | Unack | `change` | `acked` → `new`；`new` 时 `disabled` |

无对应权限时图标隐藏。

---

## Preview 弹窗

点击 🖼 打开。以**图片**为主，并展示该条警报信息。

```
+--------------------------------------------------+
|  Event                                    [x]    |
|  Timestamp · Stream · Pipeline · Event · Status  |
|  [ Raw | Visualization ]                           |
|  +--------------------------------------------+  |
|  | [‹]           图片展示区              [›]  |  |
|  +--------------------------------------------+  |
|              [ Playback ]  [ Ack ]  [ Unack ]     |
+--------------------------------------------------+
```

| 区域 | 说明 |
|------|------|
| 信息区 | **Timestamp**（完整 `YYYY-MM-DD HH:mm:ss`）、Stream、Pipeline、Event（映射名）、Status；**不**展示原始 code 为必填文案 |
| **Raw / Visualization** | 互斥切换；默认 **Visualization**（若无可视化图则默认 Raw） |
| 图片区 | 展示当前模式对应图片；加载中 / 缺失时占位 |
| **Prev** / **Next** | **叠在图片画布左右两侧**的小按钮（半透明深色底）；按**当前筛选结果集**顺序切换（与表格排序一致：新在前）；非独立侧栏按钮 |
| 滚轮 | 弹窗内：向上 = Prev，向下 = Next |
| **Playback** | 跳转 Recordings [事件回放入口](./recordings.md#事件回放入口)；复用播放器；**无宫格**；时间轴定位到本事件 `occurred_at` |
| **Ack** / **Unack** | 有 `change`；两按钮始终展示、互斥可用：`new` 时仅 **Ack**，`acked` 时仅 **Unack**；成功后 Status 更新，**留在当前条** |
| 关闭 | `×` / Esc / 点遮罩 |

### Playback → Recordings

点击 **Playback** 打开（或路由进入）Recordings 的**事件回放入口**（见 [recordings.md](./recordings.md) **事件回放入口**）：

| 项 | 说明 |
|----|------|
| 播放器 | **复用** Recordings 共用播放工具条 + 画面组件 |
| 布局 | **无宫格**、无 Preset / Layout / 多槽；单路画面铺满 |
| 流 / 日 | 本事件的 `stream_id` + `occurred_at` 所在日期 |
| 时刻 | 时间轴 / 播放头跳到事件 **Timestamp**（`occurred_at`） |
| 无录像 | Toast（如 `No recording at this time`）；可不跳转或跳转后空态（文档以**Toast 且不跳转**为准） |
| 权限 | 需能进 Recordings（如 `recordings.view_*`）；缺权则隐藏 **Playback** 或点后 403 Toast |

深链建议 query：`stream_id`、`date=YYYY-MM-DD`、`t`（当日秒或 `HH:mm:ss`）或 `occurred_at` ISO；`mode=event`（或等价）表示单路事件回放壳。

### 结果集导航

| 规则 | 说明 |
|------|------|
| 范围 | **整个筛选结果集**（含跨页），不是仅当前页 |
| 顺序 | 与列表相同（`occurred_at` 降序，`id` 稳定次序） |
| 边界 | 第一条无 Prev（disabled）；最后一条无 Next（disabled） |
| 实现 | 前端可持有当前索引 + 按需 `GET` 邻接详情；或详情接口返回 `prev_id` / `next_id`（见 API） |

切换条目时重置为默认图模式（Visualization 优先），并加载该条 Raw / Visualization URL。

---

## Export

点击 **Export**：按**当前筛选条件**生成 zip 并下载。

| 包内 | 说明 |
|------|------|
| `events.csv` | 筛选命中的全部事件行（字段与列表语义一致，含映射后 Event 名） |
| `raw/` | 各事件原图（按约定文件名，如 `{event_id}.jpg`） |
| `visualization/` | 各事件可视化图（同名规则） |

不加密。无命中行时 Toast，不下载。权限：`events.view_event`。

---

## Collect

点击 **Collect**：按**当前筛选条件**生成**加密**压缩包并下载。用于现场部署收集数据、拿回后优化模型。

| 包内 | 说明 |
|------|------|
| `events.csv` | 同 Export |
| `raw/` | 原图 |
| `visualization/` | 可视化图 |
| `labelme/` | Labelme 格式数据集 |
| `yolo/` | YOLO 格式数据集 |

| 项 | 说明 |
|----|------|
| 加密 | 压缩包加密；算法与口令/密钥管理由实现定（如导出时口令、或站点配置密钥） |
| TOTP | **不**要求 |
| 权限 | `events.change_event` |
| 无数据 | Toast，不下载 |

Labelme / YOLO 目录结构与标注字段与检测框来源由实现与模型侧约定；文档要求包内**同时**包含两套格式。

---

## 交互流程

```
进入 Events              ──> From=To=今天；GET 列表
改筛选 / Search          ──> 重置页码；GET 列表
Refresh                  ──> 再 GET
勾选 + Ack / Unack       ──> 批量定向 ack 或 unack；刷新表
行内 ✓ / ✕              ──> 单条 ack 或 unack（互斥）
🖼 Preview               ──> 打开弹窗；滚轮/Prev/Next 遍历筛选结果集
弹窗 Playback            ──> Recordings 单路播放；时间轴 = 事件 Timestamp
弹窗 Ack / Unack         ──> 当前条定向 ack 或 unack；不关闭
Export                   ──> 按筛选打 zip（csv+raw+viz）
Collect                  ──> 按筛选打加密包（csv+图+labelme+yolo）
```

---

## Toast（建议文案）

| 场景 | 文案 |
|------|------|
| 已确认 | `Event acknowledged` / `{n} event(s) acknowledged` |
| 已撤回 | `Event unacknowledged` / `{n} event(s) unacknowledged` |
| 无数据可导出 | `No events to export` |
| Export 成功 | 可无 Toast，直接下载 |
| Collect 成功 | 可无 Toast，直接下载 |
| 无录像可播 | `No recording at this time` |
| From > To | `From must be on or before To` |
| 无权限 | 沿用全局 `403` 映射 |

---

## 与其它文档

| 文档 | 关系 |
|------|------|
| [event_api.md](./event_api.md) | 本页 HTTP |
| [pipeline.md](./pipeline.md) | Pipeline 筛选项来源（与 Event 显示名无词典耦合） |
| [stream.md](./stream.md) | Stream 筛选项来源 |
| [recordings.md](./recordings.md) | **Playback** 单路回放入口 |
| [shell.md](./shell.md) | 导航 **Events** |
| [user_api.md](./user_api.md) | `events.view_event` / `events.change_event` |
