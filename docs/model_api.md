# Models API

管理端 Models 页面后端 API。UI 规格见 [model.md](./model.md)。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/models` |
| 路径前缀 | `/ai_stream2/backend/models` |

实现时须将固定段（`map`、`batch`）注册在 `/{model_id}` 之前。

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
| `type` | `static` \| `dynamic` | 对应 UI **Static** / **Dynamic** |
| `precision` | `fp16` | 当前阶段固定；对应 UI **FP16** |
| `status` | `built` \| `not_built` \| `building` | 对应 UI **Status** 列 **Built** / **Not Built** / **Building** |

### 字段语义

| 字段 | 说明 |
|------|------|
| `model_id` | 模型 **id**（UUID）；路径 `/{model_id}` 与响应字段 `id` 同值 |
| `status` | 构建状态三态：`not_built` / `building` / `built`；Build 失败时由 `building` 恢复为 `not_built` 或 `built`（曾成功过） |
| `last_build_at` | 最近一次 Build **任务结束**时间（成功或失败均写入）；`status=building` 时不更新 |
| `version` | Build 成功后写入（如 `v1.0`）；未编译为 `null` |
| `task` / `num_class` / `classes` | Build 成功或解析 `.pt` 元数据后填充；未知为 `null` |
| `source_file` | 展示用路径或文件名；未上传为 `null` |

---

## 数据模型

### Model

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo11n",
  "version": "v1.0",
  "batch_size": 1,
  "type": "static",
  "precision": "fp16",
  "task": "det",
  "num_class": 80,
  "classes": [
    {"id": 0, "name": "person"},
    {"id": 1, "name": "bicycle"},
    {"id": 2, "name": "car"}
  ],
  "source_file": "/root/models/pt/yolo11n.pt",
  "status": "built",
  "last_build_at": "2026-06-28T10:15:00Z"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成，不可改 |
| `name` | string | 否 | 展示名；**全库唯一** |
| `version` | string | 是 | Build 成功后写入 |
| `batch_size` | int | 否 | **>0** 且 **≤128** |
| `type` | string | 否 | `static` \| `dynamic` |
| `precision` | string | 否 | 固定 `fp16` |
| `task` | string | 是 | 如 `det` / `seg` / `cls` |
| `num_class` | int | 是 | 类别数 |
| `classes` | ClassItem[] | 是 | 类别列表；UI **Classes** 列有数据时为链接，点击弹窗展示 **Class ID** / **Class Name** |
| `source_file` | string | 是 | `.pt` 路径或文件名 |
| `status` | string | 否 | `built` \| `not_built` \| `building` |
| `last_build_at` | datetime | 是 | ISO 8601 UTC |

### ClassItem

```json
{
  "id": 0,
  "name": "person"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | int | 类别 ID；对应 UI **Class ID** |
| `name` | string | 类别名；对应 UI **Class Name** |

### NameMap（name → id）

```json
{
  "yolo11n": "550e8400-e29b-41d4-a716-446655440000",
  "yolo11s": "7c9e6679-7425-40de-944b-e07fc1f90ae7"
}
```

### BuildResult（单条 Build 结束）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "success": true,
  "status": "built",
  "version": "v1.0",
  "task": "det",
  "num_class": 80,
  "classes": [
    {"id": 0, "name": "person"},
    {"id": 1, "name": "bicycle"}
  ],
  "last_build_at": "2026-06-28T10:15:00Z",
  "error": null
}
```

失败时：

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "success": false,
  "status": "not_built",
  "version": null,
  "task": null,
  "num_class": null,
  "classes": null,
  "last_build_at": "2026-06-28T10:20:00Z",
  "error": "ONNX export failed"
}
```

失败时仍更新 `last_build_at`；`status` 由 `building` 恢复：从未 Build 成功过为 `not_built`，曾成功过为 `built`；`version` / `task` / `num_class` / `classes` 不覆盖已有成功结果。

### BuildStatus（轮询）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "building",
  "last_build_at": "2026-06-28T09:00:00Z"
}
```

`status` 不为 `building` 时，可返回完整 `BuildResult` 字段（实现可选，或客户端改调列表刷新）。

---

## 端点

### 列表与 CRUD

#### `GET /map`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/map`

**说明**：全部模型的 **name → id** 映射表。

**响应 `data`**：`NameMap`。

---

#### `GET /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models`

**说明**：表格分页列表；工具栏 **Refresh** 与初次加载共用。

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `search` | string | 按 `name` 或 `source_file` 子串过滤（可选）；对应页面右对齐 **Search** |
| `page` | int | 默认 `1` |
| `page_size` | int | 默认 `20` |

**响应 `data`**：分页结构，`items` 为 `Model[]`。

```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "yolo11n",
      "version": "v1.0",
      "batch_size": 1,
      "type": "static",
      "precision": "fp16",
      "task": "det",
      "num_class": 80,
      "classes": [
        {"id": 0, "name": "person"},
        {"id": 1, "name": "bicycle"}
      ],
      "source_file": "/root/models/pt/yolo11n.pt",
      "status": "built",
      "last_build_at": "2026-06-28T10:15:00Z"
    }
  ],
  "total": 12,
  "page": 1,
  "page_size": 20
}
```

---

#### `POST /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models`

**说明**：工具栏 **Add** → 弹窗 **Save**（新建）。

**请求**：`multipart/form-data`

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` | string | 是 | trim 后非空；**全库唯一** |
| `type` | string | 是 | `static` \| `dynamic` |
| `batch_size` | int | 是 | 整数；**>0** 且 **≤128** |
| `precision` | string | 否 | 省略时默认 `fp16`；传其他值拒绝 |
| `source_file` | file（UploadFile） | 否 | `.pt` 源文件；Add 弹窗 **Import** 所选文件；省略时 `source_file` 为 `null` |

**响应 `data`**：完整 `Model`（含新 `id`）；`status` 为 `not_built`，`last_build_at` 为 `null`；若上传了 `source_file` 则写入模型记录。

---

#### `GET /{model_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/{model_id}`

**说明**：单条详情（可选，列表字段已足够时可不调）。

**响应 `data`**：`Model`。

---

#### `DELETE /{model_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/{model_id}`

**说明**：单条删除（若实现行内删除；当前 UI 以批量为主）。

**行为**：删除记录及关联 `.pt` / engine 文件（若策略允许）。

**响应 `data`**：`null` 或 `{}`。

---

### 单条动作

#### `POST /{model_id}/upload`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/{model_id}/upload`

**说明**：Operations **⬆ Upload**；替换已有模型的 `.pt` 源文件。

**请求**：`multipart/form-data`

| 字段 | 类型 | 说明 |
|------|------|------|
| `file` | file | `.pt` 文件；必填 |

**响应 `data`**：更新后的 `Model`（`source_file` 已更新）。

**行为**：

| 场景 | 说明 |
|------|------|
| 首次上传（原 `source_file` 为 `null`） | 直接写入 `.pt`；`status` 仍为 `not_built` |
| 覆盖已有 `.pt` | 前端须**二次确认**（见 [model.md](./model.md) Upload 确认）；确认后覆盖文件；`status` 回退为 `not_built`；`version` / `task` / `num_class` / `classes` 清空；旧 engine 文件删除或作废 |
| `status=building` | 拒绝上传，返回 409 |

---

#### `POST /{model_id}/build`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/{model_id}/build`

**说明**：Operations **🔨 Build**；异步编译 pt → engine。

**前置**：须已存在 `source_file` / 已上传 `.pt`；否则 400。

**请求体**：无（或 `{}`）

**响应 `data`**（受理即返，不阻塞至编译结束）：

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "building"
}
```

若 `status` 已为 `building`，返回 409。

编译结束后：写入 `last_build_at`；成功则 `status=built` 并更新 `version`、`task`、`num_class`、`classes`；失败则恢复为 `not_built` 或 `built`。

---

#### `GET /{model_id}/build/status`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/{model_id}/build/status`

**说明**：Build 进行中轮询；或 WebSocket 推送的 HTTP 兜底。

**响应 `data`**：`BuildStatus`；`status=building` 时轮询；结束后可返回 `BuildResult` 或客户端刷新列表。

---

#### `GET /{model_id}/logs`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/{model_id}/logs`

**说明**：Operations **📓 Log** 弹窗 / 抽屉。

**Query**（可选）：

| 参数 | 说明 |
|------|------|
| `tail` | 仅返回末尾 N 行；默认全部或上限（如 10000 行） |

**响应 `data`**：

```json
{
  "content": "2026-06-28 10:15:00 INFO export onnx ...\n2026-06-28 10:16:00 INFO trt build ...\n"
}
```

---

### 批量（Batch）

请求体统一：

```json
{
  "ids": [
    "550e8400-e29b-41d4-a716-446655440000",
    "7c9e6679-7425-40de-944b-e07fc1f90ae7"
  ]
}
```

#### `POST /batch/delete`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/models/batch/delete`

**说明**：工具栏 **Remove**（二次确认后）。

**行为**：按 **model id** 删除；进行中的 Build 应取消或拒绝（实现二选一，建议拒绝并返回未删 id 列表）。

**响应 `data`**：

```json
{
  "deleted_count": 2,
  "failed_ids": []
}
```

---

## 错误码（建议）

| HTTP | 场景 |
|------|------|
| 400 | `batch_size` 非法；`precision` 非 `fp16`；`source_file` 非 `.pt`；Build 前未上传源文件 |
| 404 | `model_id` 不存在 |
| 409 | `name` 重复；`status=building` 时重复发起 Build 或 Upload |
| 422 | 请求体校验失败 |

---

## 调用节点

页面操作与 API 对应关系。

### 页面加载

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 Models 页 | `GET` | `/map`（可选） |
| 进入 Models 页 | `GET` | `/?page=1&page_size=20` |
| **Search** 输入 | `GET` | `/?search={q}&page=1&page_size={page_size}` |

### 工具栏

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **Refresh** | `GET` | `/?page={current}&page_size={current}&search={current}` |
| **Add** → Save | `POST` | `/`（`multipart/form-data`；可选 `source_file`） |
| **Remove** → 确认 | `POST` | `/batch/delete` |
| 翻页 | `GET` | `/?page={n}&page_size={page_size}&search={current}` |

### Operations

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **⬆ Upload**（首次） | `POST` | `/{model_id}/upload` |
| **⬆ Upload**（覆盖） | 二次确认 → `POST` | `/{model_id}/upload` |
| **🔨 Build** | `POST` | `/{model_id}/build` |
| Build 进行中轮询 | `GET` | `/{model_id}/build/status`（直至 `status` ≠ `building`） |
| **📓 Log** | `GET` | `/{model_id}/logs` |

### 表格

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **Classes** 链接 → 弹窗 | — | 使用当前行 `classes`；无需额外请求 |

### 变更后刷新

| 场景 | 建议刷新 |
|------|----------|
| Add / Upload / Build 结束 / Remove | `GET /`（保留 `page`；按 **model id** 恢复勾选） |
| Add / Remove | `GET /map`（可选） |

---

## 典型流程

### 首次进入页面

```
GET /map
GET /?page=1&page_size=20
```

### 新建模型

```
POST /  multipart/form-data  name, type, batch_size, [precision], [source_file]
GET /?page=1
```

### 编译模型

```
POST /{model_id}/build
GET /{model_id}/build/status   # status=building 时轮询
GET /?page={current}             # 结束后刷新表格
```

### 批量删除

```
POST /batch/delete  { ids: [...] }
GET /?page={current}
GET /map
```
