# Events API

管理端 Events 页后端 API。UI 规格见 [event.md](./event.md)。

本前缀负责：事件列表与详情、Ack、筛选选项、Export、Collect。事件由 Pipeline 运行时写入；本 API **不**提供手工「创建事件」。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/events` |
| 路径前缀 | `/ai_stream2/backend/events` |

实现时须将固定段（`options`、`batch`、`export`、`collect`、`calendar`）注册在 `/{event_id}` 之前。

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

### 分页列表

```json
{
  "success": true,
  "message": "",
  "data": {
    "items": [],
    "total": 0,
    "page": 1,
    "page_size": 20
  }
}
```

### 枚举与常量

| 名称 | 值 | 说明 |
|------|-----|------|
| `status` | `new` \| `acked` | 对应 UI **New** / **Acked** |
| 默认排序 | `occurred_at` 降序，同值按 `id` 降序 | 列表与 Preview 结果集一致 |
| 默认 `page_size` | `20` | 可与其它管理页对齐 |

### 字段语义

| 字段 | 说明 |
|------|------|
| `event_id` | 事件 **id**（UUID）；路径 `/{event_id}` 与 `Event.id` 同值 |
| `event` | **映射后的显示名**（ingest 写入的 `event_label`；Events 页自管）；对外列表/详情/CSV 均用此字段 |
| `event_code` | 内部/落库原始码；列表可省略；详情可选返回供调试，**UI 不展示为 Code 列** |
| `from` / `to` | 筛选日起止 `YYYY-MM-DD`（含两端；站点时区） |
| `stream_id` / `pipeline_id` | UUID；筛选项与行内关联 |

### 鉴权

| 权限 | 能力 |
|------|------|
| `events.view_event` | 列表、详情、options、calendar、**Export** |
| `events.change_event` | Ack、**Collect** |

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |
| 403 | 缺 Permission |

---

## 数据模型

### Event（列表项）

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "occurred_at": "2026-07-22T19:40:12Z",
  "stream_id": "550e8400-e29b-41d4-a716-446655440000",
  "stream_name": "cam-01",
  "pipeline_id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
  "pipeline_name": "presence-a",
  "event": "Intrusion",
  "status": "new"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成 |
| `occurred_at` | datetime | 否 | 事件时间；UI 列名 **Timestamp**，展示完整本地时间 `YYYY-MM-DD HH:mm:ss` |
| `stream_id` | UUID | 否 | |
| `stream_name` | string | 否 | 展示用；流改名后历史行可快照或实时名（实现统一） |
| `pipeline_id` | UUID | 否 | |
| `pipeline_name` | string | 否 | |
| `event` | string | 否 | 映射显示名 |
| `status` | string | 否 | `new` \| `acked` |

### EventDetail

在列表项基础上增加：

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "occurred_at": "2026-07-22T19:40:12Z",
  "stream_id": "550e8400-e29b-41d4-a716-446655440000",
  "stream_name": "cam-01",
  "pipeline_id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
  "pipeline_name": "presence-a",
  "event": "Intrusion",
  "status": "new",
  "raw_url": "http://127.0.0.1:8000/ai_stream2/media/events/.../raw.jpg",
  "visualization_url": "http://127.0.0.1:8000/ai_stream2/media/events/.../viz.jpg",
  "prev_id": "11111111-1111-1111-1111-111111111111",
  "next_id": "22222222-2222-2222-2222-222222222222",
  "payload": {}
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `raw_url` | string | 是 | 原图 URL；无则 `null` |
| `visualization_url` | string | 是 | 可视化图 URL |
| `prev_id` | UUID | 是 | 当前**同一筛选条件**下结果集中上一条；无则 `null` |
| `next_id` | UUID | 是 | 下一条；无则 `null` |
| `payload` | object | 是 | 原始载荷（可选）；UI 默认不展示 |

`prev_id` / `next_id` 必须与列表筛选、排序规则一致，供 Preview 遍历**整个筛选结果集**。

### EventOptionGroup（Event 下拉）

```json
{
  "groups": [
    {
      "pipeline_id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
      "pipeline_name": "presence-a",
      "items": [
        { "value": "1", "label": "Intrusion" },
        { "value": "2", "label": "Clear" }
      ]
    }
  ]
}
```

| 字段 | 说明 |
|------|------|
| `value` | 提交筛选用的稳定值（建议内部 `event_code`）；**UI 只显示 `label`** |
| `label` | 映射显示名 |

---

## 端点

### 筛选选项与日历

#### `GET /options/events`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/options/events`

**说明**：Event 下拉（按 Pipeline 分组）。权限：`events.view_event`。

**Query**：

| 参数 | 说明 |
|------|------|
| `pipeline_id` | 可选；若传则只返回该 Pipeline 一组 |

**响应 `data`**：`EventOptionGroup`。

---

#### `GET /calendar`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/calendar`

**说明**：某月有事件的日期，供 Date 控件标记/禁用。权限：`view`。

**Query**：`year`、`month`；可选与列表相同的 `stream_id` / `pipeline_id` / `event` / `status` / `search`（与当前其它筛选对齐时重算可选日）。

**响应 `data`**：

```json
{
  "dates": ["2026-07-01", "2026-07-22"]
}
```

---

### 列表与详情

#### `GET /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events`

**权限**：`events.view_event`

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `from` | string | `YYYY-MM-DD`；建议必填（UI 默认今天） |
| `to` | string | `YYYY-MM-DD`；建议必填（UI 默认今天）；须 `from ≤ to` |
| `stream_id` | UUID | 可选 |
| `pipeline_id` | UUID | 可选 |
| `event` | string | 可选；与 options 的 `value` 一致（内部 code） |
| `status` | string | 可选；`new` \| `acked` |
| `search` | string | 可选；子串 |
| `page` | int | 默认 `1` |
| `page_size` | int | 默认 `20` |

**响应 `data`**：分页，`items` 为列表项 `Event[]`。

---

#### `GET /{event_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/{event_id}`

**权限**：`events.view_event`

**Query**：与列表相同的筛选参数（`from` / `to` / `stream_id` / `pipeline_id` / `event` / `status` / `search`），用于计算 `prev_id` / `next_id`。缺省筛选时邻接按「仅 id 全局序」或不返回邻接（文档要求：Preview 打开时**必须带上当前筛选** query）。

**响应 `data`**：`EventDetail`。

| HTTP | 场景 |
|------|------|
| 404 | 不存在 |

---

### Ack / Unack

#### `POST /{event_id}/ack`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/{event_id}/ack`

**权限**：`events.change_event`

**请求体**：

```json
{
  "action": "ack"
}
```

| 字段 | 说明 |
|------|------|
| `action` | `ack`：`new` → `acked`；`unack`：`acked` → `new`。已是目标状态则幂等返回当前行 |

**响应 `data`**：更新后的列表形 `Event`（或 `EventDetail`，实现统一）。

---

#### `POST /batch/ack`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/batch/ack`

**权限**：`events.change_event`

**请求体**：

```json
{
  "event_ids": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "550e8400-e29b-41d4-a716-446655440000"
  ],
  "action": "ack"
}
```

| 字段 | 说明 |
|------|------|
| `event_ids` | 勾选行 id |
| `action` | `ack`：仅将 `new` → `acked`；`unack`：仅将 `acked` → `new`。已是目标状态的行跳过 |

不存在的 id 忽略。

**响应 `data`**：

```json
{
  "updated_count": 2,
  "acked_count": 2,
  "unacked_count": 0
}
```

---

### Export / Collect

筛选条件与 `GET /` 的 query **同一套**（`from`、`to`、`stream_id`、`pipeline_id`、`event`、`status`、`search`）。**不**按勾选 id 导出。

#### `POST /export`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/export`

**权限**：`events.view_event`

**请求**：query 或 JSON body 携带与列表相同的筛选字段（实现二选一，文档以 **query 与 GET / 对齐** 为准）。

**响应**：zip 文件流（如 `application/zip`）。包内：

| 路径 | 内容 |
|------|------|
| `events.csv` | 命中行 |
| `raw/{event_id}.…` | 原图 |
| `visualization/{event_id}.…` | 可视化图 |

无命中 → `400` 或空包策略：文档要求 **400** + message 无数据。

---

#### `POST /collect`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/events/collect`

**权限**：`events.change_event`

**说明**：现场采集包；**加密** zip；**不**经 TOTP。筛选同 Export。

**响应**：加密压缩包文件流。包内：

| 路径 | 内容 |
|------|------|
| `events.csv` | 命中行 |
| `raw/` | 原图 |
| `visualization/` | 可视化图 |
| `labelme/` | Labelme 数据集 |
| `yolo/` | YOLO 数据集 |

加密口令/密钥：实现可选「请求体 `passphrase`」或站点配置；须在实现说明中固定一种。无命中 → `400`。

---

## 错误码

| HTTP | 场景 |
|------|------|
| 400 | 缺筛选；无数据可导出；校验失败 |
| 401 | 未登录 |
| 403 | 缺 Permission |
| 404 | event 不存在 |

---

## 调用节点

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 / 改筛选 / Refresh | `GET` | `/events/?…` |
| Date 月历 | `GET` | `/events/calendar?year=&month=&…` |
| Event 下拉 | `GET` | `/events/options/events` |
| 🖼 Preview | `GET` | `/events/{event_id}?` + 当前筛选 |
| Prev / Next | `GET` | `/events/{prev_id\|next_id}?` + 同一筛选 |
| 行内 / 弹窗 Ack | `POST` | `/events/{event_id}/ack`（`action=ack`） |
| 行内 / 弹窗 Unack | `POST` | `/events/{event_id}/ack`（`action=unack`） |
| 工具栏 Ack | `POST` | `/events/batch/ack`（`action=ack`） |
| 工具栏 Unack | `POST` | `/events/batch/ack`（`action=unack`） |
| **Export** | `POST` | `/events/export?…` |
| **Collect** | `POST` | `/events/collect?…` |

### 不经本 API

| 调用节点 | 说明 |
|----------|------|
| 事件写入 | Pipeline / 消息消费落库 |
| Preview **Playback** | 前端路由至 [recordings.md](./recordings.md) 事件回放入口；录像片段仍走 Recordings API |
| Stream / Pipeline 下拉选项 | [stream_api.md](./stream_api.md) / [pipeline_api.md](./pipeline_api.md) 列表或精简 options |

---

## 典型流程

### 打开 Preview 并翻条

```
GET /events?from=2026-07-22&to=2026-07-22&page=1
# 点击某行 Preview
GET /events/{event_id}?from=2026-07-22&to=2026-07-22&stream_id=&…
  → EventDetail（含 raw_url、visualization_url、prev_id、next_id）
# Next
GET /events/{next_id}?from=2026-07-22&to=2026-07-22&…（同一筛选）
```

### Export

```
POST /events/export?from=2026-07-22&to=2026-07-22&status=new
  → application/zip
```

### Collect

```
POST /events/collect?from=2026-07-22&to=2026-07-22
  → encrypted archive（csv + raw + visualization + labelme + yolo）
```
