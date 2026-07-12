# Recordings API

生产端 Recordings 页面后端 API。UI 规格见 [recordings.md](./recordings.md)。

分组树与流字段复用 [stream_api.md](./stream_api.md)（`GET /groups/tree`、`GET /{stream_id}` 等）。**布局预设**与 [preview_api.md](./preview_api.md) **同形**（路径前缀不同、**Default** seed 不同）。本前缀另提供**录像日历**、**段列表**与**回放地址**。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/recordings` |
| 路径前缀 | `/ai_stream2/backend/recordings` |

实现时须将固定段（`active_layout`、`availability`、`segments`、`playback`、`map`、`batch/delete`）及 `/layouts/{preset_id}/shot`（`GET` / `PUT`）注册在 `/layouts/{preset_id}` 之前。

---

## 约定

### 响应外壳

```json
{
  "success": true,
  "message": "",
  "data": {}
}
```

失败时 `success: false`，HTTP 状态码 4xx/5xx，`message` 为错误说明。

### 常量

| 名称 | 值 | 说明 |
|------|-----|------|
| `DEFAULT_LAYOUT_ID` | `00000000-0000-4000-8000-000000000003` | 内置预设 **Default**（Recordings 独立命名空间）；首启 seed；不可删除 |
| `DEFAULT_LAYOUT_NAME` | `Default` | 内置预设名；用户 **Save As** 不可用此名 |

> Preview 与 Recordings 的 **Default** 使用**不同** `DEFAULT_LAYOUT_ID`；两页预设**不共享**存储。

### 枚举

| 字段 | 值 | 槽位数 |
|------|-----|--------|
| `layout` | `1x1` | 1 |
| `layout` | `2x2` | 4 |
| `layout` | `3x3` | 9 |
| `layout` | `4x4` | 16 |
| `view_mode` | `grid` | Grid 宫格视图 |
| `view_mode` | `focus` | Focus 视图（Recordings UI **hidden**；字段仍读写） |

API 使用 ASCII `x`（如 `2x2`）；UI 展示为 `2×2`。

### 字段语义

| 字段 | 说明 |
|------|------|
| `layout` | 宫格规格；与 UI **Layout** 下拉一致 |
| `slots` | 有序槽位列表；元素为 **stream id**（UUID）或 `null`；长度须等于当前 `layout` 槽位数 |
| `stream_count` | 只读；`slots` 中非 `null` 元素个数 |
| `view_mode` | `grid` \| `focus`；随 **Save** / **Save As** 持久化 |
| `name` | 预设展示名；**全库唯一**（**本前缀**内）；保留名 `Default` |
| `preset_id` | 布局预设 **id**（UUID） |
| `date` | 日历 / 段列表日期；`YYYY-MM-DD`（本地日历日，无时区偏移语义） |
| `month` | 日历月份；`YYYY-MM` |
| `start` / `end` | 段起止墙钟时刻；ISO 8601 `YYYY-MM-DDTHH:MM:SS`（选中日当日） |
| `file` | 录像文件在存储中的**相对路径**（如 `cam-01/2026-07-09_18-00-00-000000.mp4`）；由后端索引，前端 opaque 传递 |

### 持久化边界

| 写入 API | 内容 |
|----------|------|
| `POST /layouts` | **Save As** 新建 |
| `PUT /layouts/{preset_id}/shot` | **Save** / **Save As** 成功后上传 Grid 合成缩略图 |
| `PATCH /layouts/{preset_id}` | **Save**（`layout` / `slots` / `view_mode`）或 **Manage ✏️**（`name`） |
| `PUT /active_layout` | **Preset** 下拉切换激活预设 |

**不**经本 API 持久化：当前选中日期、全局播放时刻 **T**、音源槽、Groups ▦ 筛选、未 **Save** 的会话内编辑。

### slots 校验

与 [preview_api.md](./preview_api.md) 相同：`len(slots)` = layout 槽位数；非 `null` 为已存在 **stream id**；同一 stream id 最多 1 槽。

---

## 数据模型

### LayoutPreset

与 Preview 同结构；**Default** seed 为 `1x1`：

```json
{
  "id": "00000000-0000-4000-8000-000000000003",
  "name": "Default",
  "layout": "1x1",
  "view_mode": "grid",
  "slots": [null],
  "stream_count": 0,
  "shot_url": "/ai_stream2/backend/recordings/layouts/00000000-0000-4000-8000-000000000003/shot"
}
```

### LayoutPresetSummary / ActiveLayout / NameMap

同 [preview_api.md](./preview_api.md)；`shot_url` 前缀为本 Base URL。

### StreamAvailability

单流在某月（或范围内）**有录像**的日期列表：

```json
{
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "dates": ["2026-07-01", "2026-07-03", "2026-07-09"]
}
```

「有录像」= 该自然日存在**至少一段**录像（**不要求**覆盖 `00:00–24:00`）。前端对多流取日期**交集**决定日历可选日（见 [recordings.md](./recordings.md)）。

### AvailabilityResult

```json
{
  "month": "2026-07",
  "streams": [
    {
      "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
      "dates": ["2026-07-01", "2026-07-03"]
    },
    {
      "stream_id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
      "dates": ["2026-07-01", "2026-07-02", "2026-07-03"]
    }
  ]
}
```

`stream_id` 不存在或未开启 `recording` 时，该流 `dates` 为 `[]`（或整请求 `404`，实现二选一；推荐单流空数组、不阻断其它流）。

### RecordingSegment

```json
{
  "id": "seg-550e8400-e29b-41d4-a716-446655440001",
  "file": "cam-01/2026-07-09_18-00-00-000000.mp4",
  "start": "2026-07-09T18:00:00",
  "end": "2026-07-09T23:59:59"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | 段唯一 id；可代替 `file` 调 **playback** |
| `file` | string | 存储相对路径 |
| `start` | string | 段覆盖起始墙钟时刻（含） |
| `end` | string | 段覆盖结束墙钟时刻（不含或含，实现统一为 **不含** `end`，即 `start ≤ T < end`） |

段按 `start` 升序；允许同日多段（间隙、跨文件）。

### SegmentsResult

```json
{
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "date": "2026-07-09",
  "segments": [
    {
      "id": "seg-550e8400-e29b-41d4-a716-446655440001",
      "file": "cam-01/2026-07-09_18-00-00-000000.mp4",
      "start": "2026-07-09T18:00:00",
      "end": "2026-07-09T23:59:59"
    }
  ]
}
```

### PlaybackResult

```json
{
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "file": "cam-01/2026-07-09_18-00-00-000000.mp4",
  "url": "http://127.0.0.1:8000/ai_stream2/media/recordings/cam-01/2026-07-09_18-00-00-000000.mp4",
  "content_type": "video/mp4"
}
```

`url` 为浏览器 / 播放器可直接请求的 HTTP(S) 地址（MP4 或 HLS 由实现决定；文档阶段默认 **MP4**）。

---

## 端点

### 当前激活预设

#### `GET /active_layout`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/active_layout`

**说明**：读取当前激活的布局预设 **id**；进页时调用。

**Query**：无

**响应 `data`**：`ActiveLayout`（同 Preview）

---

#### `PUT /active_layout`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/active_layout`

**说明**：**Load** 选中某预设后持久化当前激活项。

**请求体**：

```json
{
  "preset_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

**响应 `data`**：`ActiveLayout`

行为同 [preview_api.md](./preview_api.md) **`PUT /active_layout`**。

---

### 布局预设

`GET /layouts`、`GET /layouts/map`、`GET /layouts/{preset_id}`、`POST /layouts`、`PATCH /layouts/{preset_id}`、`DELETE /layouts/{preset_id}`、`POST /layouts/batch/delete`、`PUT|GET /layouts/{preset_id}/shot` 的请求 / 响应 / 校验与 [preview_api.md](./preview_api.md) **相同**，仅 URL 前缀换为 `/ai_stream2/backend/recordings`，**Default** 为 `layout: 1x1`、`slots: [null]`、`DEFAULT_LAYOUT_ID` 见上文。

---

### 录像 · 日历

#### `GET /availability`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/availability`

**说明**：查询一批流在指定月份（或日期范围）内**哪些自然日有录像**；供日历渲染与切换月份。前端对当前宫格 **S** 中各流结果取日期**交集**。

**Query**：

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_ids` | string | 是 | 逗号分隔的 **stream id**（UUID）列表 |
| `month` | string | 是 | `YYYY-MM`；返回该月各日是否在 `dates` 中 |

**响应 `data`**：`AvailabilityResult`

**示例**：

```
GET /availability?stream_ids=7c9e6679-7425-40de-944b-e07fc1f90ae7,a1b2c3d4-e5f6-4789-a012-3456789abcde&month=2026-07
```

**行为**：

- 仅索引 **`recording: true`** 的流；`recording: false` → 该流 `dates: []`
- 某 **stream id** 不存在 → 该流 `dates: []` 或整请求 `404`（推荐前者）
- 切换日历月份时前端对 **S** 中非空槽重新请求

---

### 录像 · 段列表

#### `GET /segments`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/segments`

**说明**：某流在**指定自然日**的全部录像段；供时间轴绘制与 seek 解析。选中日 / 换日后对每个已绑槽各请求一次（可并行）。

**Query**：

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_id` | UUID | 是 | 流 **id** |
| `date` | string | 是 | `YYYY-MM-DD` |

**响应 `data`**：`SegmentsResult`

**示例**：

```
GET /segments?stream_id=7c9e6679-7425-40de-944b-e07fc1f90ae7&date=2026-07-09
```

**行为**：

- 无录像 → `segments: []`（**200**）
- 流不存在 → `404`
- `date` 非法 → `400`

前端缓存 `{stream_id, date} → segments`；时间轴填充段取各槽 `segments` 的**时刻交集**。

---

### 录像 · 回放地址

#### `GET /playback`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/playback`

**说明**：解析可播放 URL。用户在时间轴 seek 命中某段后，按 `file` 或 `segment_id` 取地址；播放器 seek 至 `(T − segment.start)`。

**Query**（`file` 与 `segment_id` **二选一**）：

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_id` | UUID | 是 | 流 **id** |
| `file` | string | 与 `segment_id` 二选一 | **segments** 响应中的相对路径 |
| `segment_id` | string | 与 `file` 二选一 | **segments** 响应中的 `id` |

**响应 `data`**：`PlaybackResult`

**示例**：

```
GET /playback?stream_id=7c9e6679-7425-40de-944b-e07fc1f90ae7&file=cam-01/2026-07-09_18-00-00-000000.mp4
```

**行为**：

- 文件不存在或无权访问 → `404`
- 返回的 `url` 须支持 **HTTP Range**（便于 seek）

---

## 错误码（建议）

### 布局预设

同 [preview_api.md](./preview_api.md) **错误码**表。

### 录像

| HTTP | `message` 示例 | 场景 |
|------|----------------|------|
| 400 | `Invalid month` | `month` 非 `YYYY-MM` |
| 400 | `Invalid date` | `date` 非 `YYYY-MM-DD` |
| 400 | `stream_ids required` | 缺少 `stream_ids` |
| 400 | `file or segment_id required` | **playback** 未提供文件标识 |
| 404 | `Stream not found` | 无效 **stream id**（硬校验路径） |
| 404 | `Recording not found` | **playback** 文件不存在 |
| 404 | `Segment not found` | 无效 `segment_id` |

---

## 调用节点

### 页面加载

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 Recordings 页 | `GET` | `/active_layout` |
| 进入 Recordings 页 | `GET` | `/layouts/{preset_id}` |
| 进入 Recordings 页 | `GET` | `/groups/tree`（[stream_api](./stream_api.md)） |
| 进入 Recordings 页 | `GET` | `/layouts` |
| **S** 非空且默认选中今天 | `GET` | `/availability?stream_ids=...&month={YYYY-MM}` |
| 默认选中今天后 | `GET` | `/segments?stream_id={id}&date={YYYY-MM-DD}`（每已绑槽） |

### Preset 栏

与 [preview_api.md](./preview_api.md) **Preset 栏** / **Manage 弹窗**相同；路径换为本前缀。

### 左侧栏 · Groups

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **Groups ↻** | `GET` | `/groups/tree`（[stream_api](./stream_api.md)） |

### 日历

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 切换月份 | `GET` | `/availability?stream_ids={S}&month={YYYY-MM}` |
| 绑流 / 清槽 / Load 预设后 **S** 变化 | `GET` | `/availability?...`（当前月） |
| 点击可选日 | `GET` | `/segments?stream_id={id}&date={YYYY-MM-DD}`（每已绑槽） |

### 回放

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| seek 命中 segment | `GET` | `/playback?stream_id={id}&file={file}` 或 `segment_id=...` |
| 跨段连续播放 | `GET` | `/playback`（下一段 file） |

---

## 典型流程

### 首次进入 Recordings 页

```
GET /active_layout
GET /layouts/{preset_id}
GET /groups/tree
GET /layouts

# 若 S 非空且今天对 S 可选：
GET /availability?stream_ids=...&month=2026-07
GET /segments?stream_id=...&date=2026-07-09   # 每已绑槽
# T ← 各槽 segments 时刻交集起点（见 recordings.md）
```

### 用户绑入首路流

```
# 前端更新 slots（未 Save 则 Preset 标 *）
GET /availability?stream_ids={新 S}&month={当前月}
# 若今天 newly 可选 → 选中今天 → GET /segments × N
```

### 切换日历月份

```
GET /availability?stream_ids={S}&month=2026-08
# 选中日在非本月 → 清除选中日、停播
```

### 时间轴 seek

```
# 前端用已缓存 segments 解析 T → segment
GET /playback?stream_id={id}&file={segment.file}
# 各已绑槽并行；无 segment 的槽不请求 playback
```

### Save 布局（同 Preview）

```
PATCH /layouts/{preset_id}
PUT /layouts/{preset_id}/shot
```

---

## 与 stream_api / preview_api 关系

| 本页能力 | API |
|----------|-----|
| 左侧分组树 | `GET .../streams/groups/tree` |
| 流 `recording` / 树字色 | 树节点字段；[stream_api](./stream_api.md) |
| 布局预设 | 本前缀 `/layouts`、`/active_layout`（**独立**于 Preview 存储） |
| 日历可选日 | 本前缀 `GET /availability`；交集在前端 |
| 时间轴 / seek | 本前缀 `GET /segments`、`GET /playback` |

`slots` 仅存 **stream id**；不存录像路径。全局时刻 **T** 与选中日**不**写入后端。
