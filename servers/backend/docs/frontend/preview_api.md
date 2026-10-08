# Preview API

生产端 Preview 页面后端 API。UI 规格见 [preview.md](./preview.md)。

分组树与流字段复用 [stream_api.md](./stream_api.md)（`GET /groups/tree`、`GET /{stream_id}` 等）；本前缀仅负责**布局预设**与**当前激活预设**。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/preview` |
| 路径前缀 | `/ai_stream2/backend/preview` |

实现时须将固定段（`active_layout`、`map`、`batch/delete`）及 `/layouts/{preset_id}/shot`（`GET` / `PUT`）注册在 `/layouts/{preset_id}` 之前。

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
| `DEFAULT_LAYOUT_ID` | `00000000-0000-4000-8000-000000000002` | 内置预设 **Default**；首启 seed；不可删除 |
| `DEFAULT_LAYOUT_NAME` | `Default` | 内置预设名；用户 **Save As** 不可用此名 |

### 枚举

| 字段 | 值 | 槽位数 |
|------|-----|--------|
| `layout` | `1x1` | 1 |
| `layout` | `2x2` | 4 |
| `layout` | `3x3` | 9 |
| `layout` | `4x4` | 16 |
| `view_mode` | `grid` | Grid 宫格视图 |
| `view_mode` | `focus` | Focus 焦点视图 |

API 使用 ASCII `x`（如 `2x2`）；UI 展示为 `2×2`。

### 字段语义

| 字段 | 说明 |
|------|------|
| `layout` | 宫格规格；与 UI **Layout 栏**居中下拉一致 |
| `slots` | 有序槽位列表；元素为 **stream id**（UUID）或 `null`（空槽）；长度须等于当前 `layout` 槽位数 |
| `stream_count` | 只读；`slots` 中非 `null` 元素个数；列表/摘要用 |
| `view_mode` | `grid` \| `focus`；Grid / Focus 视图；随 **Save** / **Save As** 持久化 |
| `name` | 预设展示名；**全库唯一**；保留名 `Default` 仅系统内置 |
| `preset_id` | 布局预设 **id**（UUID）；与 `LayoutPreset.id` 同值；**active_layout**、路径 `/layouts/{preset_id}` 均用此名 |

### 持久化边界

| 写入 API | 内容 |
|----------|------|
| `POST /layouts` | **Save As** 新建（`name` / `layout` / `slots` / `view_mode`） |
| `PUT /layouts/{preset_id}/shot` | **Save** / **Save As** 成功后上传 **Grid 离屏合成**缩略图 |
| `PATCH /layouts/{preset_id}` | **Save** 直接覆盖当前激活预设（**含 Default**；`layout` / `slots` / `view_mode`） |
| `PUT /active_layout` | **Preset** 下拉切换激活预设；服务重启后进页恢复 |

**不**经本 API 持久化：`focus_index`、当前选中槽下标、左侧树选中、未 **Save** 的会话内 `layout` / `slots` / `view_mode` 编辑。

### slots 校验

| 规则 | 说明 |
|------|------|
| 长度 | `len(slots)` = 当前 `layout` 槽位数 |
| 空槽 | `null` |
| 流引用 | 非 `null` 须为已存在 **stream id**；响应时可剔除已删流（置 `null`）并附 `missing_stream_ids` |
| 去重 | 同一 **stream id** 在 `slots` 中最多出现 1 次 |

---

## 数据模型

### LayoutPreset

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "值班 2x2",
  "layout": "2x2",
  "view_mode": "focus",
  "slots": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "a1b2c3d4-e5f6-4789-a012-3456789abcde",
    null,
    null
  ],
  "stream_count": 2,
  "shot_url": "/ai_stream2/backend/preview/layouts/550e8400-e29b-41d4-a716-446655440000/shot"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成，不可改 |
| `name` | string | 否 | 展示名；**全库唯一** |
| `layout` | string | 否 | `1x1` \| `2x2` \| `3x3` \| `4x4` |
| `view_mode` | string | 否 | `grid` \| `focus` |
| `slots` | array | 否 | 元素为 UUID 或 `null` |
| `stream_count` | int | 否 | 只读；非空槽计数 |
| `shot_url` | string | 是 | 只读；**Grid 合成** layout 缩略图 URL；**Rename** 弹窗 **Shot** 图标预览；由前端 **Save** / **Save As** 合成后 **PUT** 写入 |

内置 **Default** 示例：

```json
{
  "id": "00000000-0000-4000-8000-000000000002",
  "name": "Default",
  "layout": "2x2",
  "view_mode": "grid",
  "slots": [null, null, null, null],
  "stream_count": 0
}
```

### LayoutPresetSummary（列表行）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "值班 2x2",
  "layout": "2x2",
  "stream_count": 2
}
```

### ActiveLayout

```json
{
  "preset_id": "00000000-0000-4000-8000-000000000002"
}
```

无持久化记录时 **GET** 返回 `preset_id: DEFAULT_LAYOUT_ID`。

### NameMap（name → id）

```json
{
  "Default": "00000000-0000-4000-8000-000000000002",
  "值班 2x2": "550e8400-e29b-41d4-a716-446655440000"
}
```

---

## 端点

### 当前激活预设

#### `GET /active_layout`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/active_layout`

**说明**：读取当前激活的布局预设 **id**；进页时调用，避免服务重启后须重新 **Load**。

**Query**：无

**响应 `data`**：`ActiveLayout`

```json
{
  "preset_id": "00000000-0000-4000-8000-000000000002"
}
```

---

#### `PUT /active_layout`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/active_layout`

**说明**：**Load** 选中某预设后调用，持久化当前激活项。

**请求体**：

```json
{
  "preset_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

**行为**：

- `preset_id` 须存在
- 删除当前激活的命名预设后，前端须 **PUT** 为 `DEFAULT_LAYOUT_ID`

**响应 `data`**：`ActiveLayout`

---

### 布局预设 · 列表与映射

#### `GET /layouts`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts`

**说明**：全部布局预设；**Preset** 下拉、**Manage** 表格。须始终含 **Default**。

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `search` | string | 可选；按 `name` 子串过滤（**Manage** 弹窗工具行右对齐 **Search**） |

**响应 `data`**：

```json
{
  "items": [
    {
      "id": "00000000-0000-4000-8000-000000000002",
      "name": "Default",
      "layout": "2x2",
      "stream_count": 0
    },
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "值班 2x2",
      "layout": "2x2",
      "stream_count": 2
    }
  ],
  "preset_id": "00000000-0000-4000-8000-000000000002"
}
```

`preset_id` 为当前激活项 **id**（与 `GET /active_layout` 一致）；可选冗余字段，便于 **Preset** 下拉标 ✓。

---

#### `GET /layouts/map`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/map`

**说明**：全部预设 **name → id** 映射。

**响应 `data`**：`NameMap`（见上文）

---

#### `GET /layouts/{preset_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/{preset_id}`

**说明**：单条预设详情；进页恢复、**Load** 应用宫格。

**响应 `data`**：`LayoutPreset`

**slots 含已删流时**（可选扩展字段）：

```json
{
  "id": "00000000-0000-4000-8000-000000000002",
  "name": "Default",
  "layout": "2x2",
  "view_mode": "grid",
  "slots": [null, null, null, null],
  "stream_count": 0,
  "missing_stream_ids": ["7c9e6679-7425-40de-944b-e07fc1f90ae7"]
}
```

`missing_stream_ids` 为响应中已从 `slots` 置 `null` 的 **stream id** 列表。

---

### 布局预设 · 增删改

#### `POST /layouts`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts`

**说明**：**Save As Layout** 弹窗新建命名预设（非 **Default**）。**Name** 右侧 **Shot** 图标预览 **Grid 合成图**；**POST** body 含 `name`、`layout`、`slots`、`view_mode`；**Create** 成功后 **PUT** shot。

**请求体**：

```json
{
  "name": "值班 2x2",
  "layout": "2x2",
  "view_mode": "focus",
  "slots": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "a1b2c3d4-e5f6-4789-a012-3456789abcde",
    null,
    null
  ]
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| `name` | 是 | 不可为 `Default`；不可与已有名冲突 |
| `layout` | 是 | |
| `view_mode` | 是 | `grid` \| `focus` |
| `slots` | 是 | |

**响应 `data`**：创建的 `LayoutPreset`。

**成功后前端**：取响应 `data.id` 作为 `preset_id` → **PUT** `/layouts/{preset_id}/shot` → **PUT /active_layout**；**Preset 栏**更新。

---

#### `PATCH /layouts/{preset_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/{preset_id}`

**说明**：**Save** 图标直接覆盖当前激活预设（**含 Default**）；`layout` / `slots` / `view_mode`，**不可改** `name`；成功后前端 **PUT** shot。**Save As** 走 **POST** 再 **PUT** shot。**Manage ✏️** 重命名走 **PATCH** 仅 `name`（**不**更新 shot）。

**请求体**（字段可选，至少一项）：

```json
{
  "name": "值班台",
  "layout": "3x3",
  "view_mode": "grid",
  "slots": [null, null, null, null, null, null, null, null, null]
}
```

| 场景 | 允许字段 | 限制 |
|------|----------|------|
| **Save**（**含 Default**） | `layout`, `slots`, `view_mode` | **不可** `name` |
| **Manage ✏️** 重命名 | `name` | 不可为 `Default`；不可与已有名冲突；**Default** 预设不可改 `name` |
| **Default** | — | 不可 **Delete** |

**响应 `data`**：更新后的 `LayoutPreset`。

---

#### `DELETE /layouts/{preset_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/{preset_id}`

**说明**：**Manage** 行内 **🗑** 或工具行 **Delete**（二次确认）。不可删除 **Default**。

**行为**：

- 删除指定预设
- 若删的是当前激活项：响应可提示前端 **PUT /active_layout** 为 `DEFAULT_LAYOUT_ID` 并 **GET** **Default** 应用

**响应 `data`**：

```json
{
  "deleted_id": "550e8400-e29b-41d4-a716-446655440000",
  "was_active": true
}
```

`was_active: true` 时前端须切回 **Default**。

---

#### `POST /layouts/batch/delete`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/batch/delete`

**说明**：**Manage** 工具行 **Delete**（与 **Search** 同行）；批量删除已勾选预设。不可包含 **Default**。

**请求体**：

```json
{
  "ids": [
    "550e8400-e29b-41d4-a716-446655440000",
    "a1b2c3d4-e5f6-4789-a012-3456789abcde"
  ]
}
```

**行为**：

- 逐项删除；`ids` 含 `DEFAULT_LAYOUT_ID` → `403`
- 若删除集合含当前激活项：响应 `active_deleted: true`；前端 **PUT /active_layout** 为 `DEFAULT_LAYOUT_ID` 并 **GET** **Default**

**响应 `data`**：

```json
{
  "deleted_ids": ["550e8400-e29b-41d4-a716-446655440000"],
  "active_deleted": true
}
```

单条删除仍可用 **DELETE /layouts/{preset_id}**（**Manage** 行 **🗑**）。

---

#### `PUT /layouts/{preset_id}/shot`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/{preset_id}/shot`

**说明**：上传并覆盖该预设的 **Grid 合成** layout 缩略图（**非**视口截图；**Focus** 下亦按 `layout` + `slots` 离屏拼 Grid，不含侧栏/滚动条）。**Save**：点击时合成，`PATCH` 成功后 **PUT**。**Save As**：打开弹窗时合成，`POST` 成功后 **PUT**（与弹窗 **Shot** 图标同源）。**Rename** 仅改 `name`，**不**调用本接口。

**请求**：`multipart/form-data`，字段 `file`（`image/jpeg` 或 `image/png`）。

**响应 `data`**：更新后的 `LayoutPreset`（含 `shot_url`）。

---

#### `GET /layouts/{preset_id}/shot`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/preview/layouts/{preset_id}/shot`

**说明**：返回该预设**已存储**的 Grid 合成缩略图。**Rename** 弹窗 **Shot** 图标预览使用 `shot_url` 或本 URL。

**响应**：`image/jpeg` 或 `image/png`。无已存图时 `404`（弹窗图标 **disabled**）。

---

## 错误码（建议）

| HTTP | `message` 示例 | 场景 |
|------|----------------|------|
| 400 | `Invalid view_mode` | `view_mode` 非 `grid` / `focus` |
| 400 | `Invalid layout` | `layout` 非枚举值 |
| 400 | `Slots length mismatch` | `slots` 长度与 `layout` 不符 |
| 400 | `Duplicate stream in slots` | 同一 stream id 多槽 |
| 400 | `Reserved name` | `name` 为 `Default` |
| 404 | `Preset not found` | `preset_id` 不存在 |
| 404 | `Stream not found` | `slots` 含无效 stream id（若实现为硬校验） |
| 409 | `Name already exists` | `name` 冲突 |
| 403 | `Cannot modify Default name` | 对 **Default** 改 `name` |
| 403 | `Cannot delete Default` | 删除 **Default** 或 batch 含 **Default** |

---

## 调用节点

页面操作与 API 对应关系。

### 页面加载

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 Preview 页 | `GET` | `/active_layout` |
| 进入 Preview 页 | `GET` | `/layouts/{preset_id}` |
| 进入 Preview 页 | `GET` | `/groups/tree`（[stream_api](./stream_api.md)） |
| 进入 Preview 页 | `GET` | `/layouts`（**Preset** 下拉，可与上并行） |

### Preset 栏

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **Preset** 展开 | `GET` | `/layouts` |
| **Preset** 选预设 | `GET` | `/layouts/{preset_id}` |
| **Preset** 选预设后 | `PUT` | `/active_layout` |
| **Save** 图标 | `PATCH` | `/layouts/{preset_id}`（`layout` + `slots` + `view_mode`） |
| **Save** 成功后 | `PUT` | `/layouts/{preset_id}/shot`（multipart `file`） |
| **Save As** → Create | `POST` | `/layouts` |
| **Save As** 弹窗 **Shot** 图标 | — | Grid 离屏合成预览（新建）；不调 `GET` |
| **Save As** 成功后 | `PUT` | `/layouts/{preset_id}/shot`（`preset_id` 取自 **POST** 响应 `data.id`） |
| **Save As** 成功后 | `PUT` | `/active_layout` |
| **Clear** 图标 | — | 仅前端会话（`slots` 全 `null`）；**不**调用 API，直至 **Save** |

### Manage 弹窗

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 打开弹窗 | `GET` | `/layouts?search={q}` |
| **✏️** | `GET` | `/layouts/{preset_id}`（弹窗只读字段） |
| **✏️** → Save | `PATCH` | `/layouts/{preset_id}`（`{ "name": "..." }`） |
| **✏️** 弹窗 **Shot** 图标 | `GET` | `/layouts/{preset_id}/shot` 或 `shot_url`（只读预览） |
| **🗑** → 确认 | `DELETE` | `/layouts/{preset_id}` |
| **Delete**（批量）→ 确认 | `POST` | `/layouts/batch/delete` |
| 删当前激活项后 | `PUT` | `/active_layout`（`DEFAULT_LAYOUT_ID`） |

### 播放相关（非本前缀）

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 宫格拉流 | — | 使用 `LayoutPreset.slots` 中的 **stream id** 调 [stream_api](./stream_api.md) 取 `url` |

---

## 典型流程

### 首次进入 Preview 页

```
GET /active_layout
  → preset_id（无记录则为 DEFAULT_LAYOUT_ID）

GET /layouts/{preset_id}
  → 已保存的 layout + slots + view_mode → 渲染；Preset 栏显示对应 name

GET /groups/tree
GET /layouts
```

### 用户改动宫格（未 Save）

```
# 用户改 Layout / 绑流 / Clear / Grid↔Focus
# 仅更新前端会话；Preset 栏标 *
# 不调用 PATCH，直至用户 Save
```

### Save 覆盖当前激活预设（含 Default）

```
PATCH /layouts/{preset_id}
  { "layout": "3x3", "slots": [...], "view_mode": "focus" }

PUT /layouts/{preset_id}/shot
  multipart file（点击 Save 时已合成的 Grid 图）

# Toast「Layout saved」；Preset 栏去 *
```

### Save As 新建命名预设

```
POST /layouts
  { "name": "值班 2x2", "layout": "2x2", "view_mode": "focus", "slots": [...] }
  → 响应 data.id 即为 preset_id

PUT /layouts/{preset_id}/shot
  multipart file（打开 Save As 弹窗时已合成的 Grid 图）

PUT /active_layout
  { "preset_id": "<同上 preset_id>" }
```

### Preset 下拉切换预设

```
GET /layouts/{preset_id}
PUT /active_layout
  { "preset_id": "<preset_id>" }
# 应用已保存 layout + slots + view_mode；清除 *；之后未 Save 的编辑标 *
```

### 服务重启后再进页

```
GET /active_layout
  → 仍为上次 PUT 的 preset_id

GET /layouts/{preset_id}
  → 恢复宫格，无需用户重新 Load
```

### 删除当前激活的命名预设

```
DELETE /layouts/{preset_id}

PUT /active_layout
  { "preset_id": "00000000-0000-4000-8000-000000000002" }

GET /layouts/{DEFAULT_LAYOUT_ID}
```

---

## 与 stream_api 关系

| 本页能力 | API |
|----------|-----|
| 左侧分组树 | `GET .../streams/groups/tree`（流节点含 `enabled` / `status` / `recording`） |
| 流 `url` / `resolution` / `fps` | 绑槽后 `GET .../streams/{stream_id}` |
| 格内 `status` | 树节点 `status`；↻ 后随树更新 |
| 布局预设 | 本前缀 `/layouts`、`/active_layout` |

`slots` 仅存 **stream id**；不存 RTSP URL。
