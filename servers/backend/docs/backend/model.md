# Models（后端）

---

## 代理

| 本页触发 | 上游 | 用途 |
|----------|------|------|
| `POST /{model_id}/build` | **export_onnx** → **export_trt** | 用 `source_path` 的 zip 先导出 ONNX，再导出 TensorRT engine |
| `GET /{model_id}/status`、`POST /batch/status` | 本页状态机 | 轮询（不直连上游异步任务 API） |

---

## API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/map` | name → id |
| `GET` | `/` | 分页列表（search） |
| `POST` | `/` | 新建（multipart 字段，无文件） |
| `POST` | `/import` | 导入 engine zip；缺文件则失败 |
| `GET` | `/{model_id}` | 详情 |
| `PATCH` | `/{model_id}` | 改参数（无文件） |
| `POST` | `/{model_id}/upload` | 上传 `.pt` 或 `.onnx`，打成 zip 并写入 `source_path` |
| `DELETE` | `/{model_id}` | 删记录 + 关联文件 |
| `POST` | `/{model_id}/build` | 按 `source_path` 依次调用 export_onnx、export_trt |
| `GET` | `/{model_id}/status` | 轮询 |
| `POST` | `/batch/build` | 按 id 依次调用 export_onnx、export_trt |
| `POST` | `/batch/status` | 按 id 轮询 |
| `POST` | `/batch/remove` | 批量删；`building` → 入 `failed_ids` |

### `GET /map`

`data` 为对象，键是模型 `name`，值是模型 `id`。按 `name` 排序。

```json
{
  "yolo11n": "550e8400-e29b-41d4-a716-446655440000",
  "yolo11s": "7c9e6679-7425-40de-944b-e07fc1f90ae7"
}
```

### `GET /`

按 `search`（`name` 子串）筛选，并用 `page`、`page_size` 分页。默认 `page=1`、`page_size=20`。

`data` 含本页模型、总数和页码。字段为 `id`、`name`、`type`、`version`、`batch_size`、`batch_mode`、`precision`、`max_det`、`conf`、`iou`、`num_class`、`classes`、`source_path`、`status`、`built_at`。未 Build 成功时 `version`、`num_class`、`classes`、`built_at` 为 `null`。未传时 `max_det`、`conf`、`iou` 为 `null`。

```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "yolo11n",
      "type": "YOLO26-DET",
      "version": "v1.0",
      "batch_size": 1,
      "batch_mode": "static",
      "precision": "fp16",
      "max_det": 30,
      "conf": "0.25",
      "iou": "0.45",
      "num_class": 80,
      "classes": [
        {"id": 0, "name": "person"}
      ],
      "source_path": "/root/models/pt/yolo11n.pt",
      "status": "built",
      "built_at": "2026-06-28T10:15:00Z"
    }
  ],
  "total": 12,
  "page": 1,
  "page_size": 20
}
```

### `POST /`

新建模型。`multipart/form-data`，只传字段，不传文件。写入后 `status=not_built`，`source_path`、`version`、`num_class`、`classes`、`built_at` 为 `null`。文件走 `POST /{model_id}/upload`。

入参：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` | string | 是 | 全库唯一 |
| `type` | string | 是 | `YOLO10-DET`、`YOLO26-SEG`、`YOLO26-DET`、`YOLO26-POSE` |
| `batch_mode` | string | 是 | `static` 或 `dynamic` |
| `batch_size` | int | 是 | 1…128 |
| `precision` | string | 是 | `fp16` |
| `max_det` | int | 否 | 最大检测数 |
| `conf` | string | 否 | 两位小数，如 `0.25` |
| `iou` | string | 否 | 两位小数，如 `0.45` |
| `opt_level` | int | 否 | 缺省为 `null` |
| `device` | int | 否 | 缺省为 `0` |

`data` 为新建的模型，字段与列表中的单条相同。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo11n",
  "type": "YOLO26-DET",
  "version": null,
  "batch_size": 1,
  "batch_mode": "static",
  "precision": "fp16",
  "max_det": 30,
  "conf": "0.25",
  "iou": null,
  "num_class": null,
  "classes": null,
  "source_path": null,
  "status": "not_built",
  "built_at": null
}
```

### `POST /import`

`multipart/form-data`，字段 `file` 为 `.zip`。不调用 export_onnx、export_trt。路径注册在 `/{model_id}` 之前。

解开 zip 后必须同时满足：包内有 `labels.txt`；包内有 `meta.json`；包内有且仅有一个 `.engine` 文件。缺任一项、`.engine` 多于一个、或上传的不是 zip，返回 400，不写库。

### `GET /{model_id}`

按路径取单条。无查询参数、无请求体。

入参：

| 字段 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `model_id` | path | string | 是 | 模型 UUID |

`data` 字段与列表中的单条相同。未 Build 成功时 `version`、`num_class`、`classes`、`built_at` 为 `null`。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo11n",
  "type": "YOLO26-DET",
  "version": "v1.0",
  "batch_size": 1,
  "batch_mode": "static",
  "precision": "fp16",
  "max_det": 30,
  "conf": "0.25",
  "iou": "0.45",
  "num_class": 80,
  "classes": [
    {"id": 0, "name": "person"}
  ],
  "source_path": "/root/models/pt/yolo11n.pt",
  "status": "built",
  "built_at": "2026-06-28T10:15:00Z"
}
```

### `PATCH /{model_id}`

改已有模型。`multipart/form-data`，只传字段，不传文件。字段都可选，缺省沿用当前值。`status=building` 时，改 `name` 以外的字段返回 409，消息为 `model is building`。`status=built` 时只允许改 `name`；改 `type`、`batch_mode`、`batch_size`、`precision`、`opt_level`、`device`、`max_det`、`conf`、`iou` 返回 409，消息为 `Partial modification is allowed for a built model`。不修改 `source_path`。

入参：

| 字段 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `model_id` | path | string | 是 | 模型 UUID |
| `name` | body | string | 否 | 全库唯一 |
| `type` | body | string | 否 | `YOLO10-DET`、`YOLO26-SEG`、`YOLO26-DET`、`YOLO26-POSE` |
| `batch_mode` | body | string | 否 | `static` 或 `dynamic` |
| `batch_size` | body | int | 否 | 1…128 |
| `precision` | body | string | 否 | `fp16` |
| `max_det` | body | int | 否 | 最大检测数 |
| `conf` | body | string | 否 | 两位小数，如 `0.25` |
| `iou` | body | string | 否 | 两位小数，如 `0.45` |
| `opt_level` | body | int | 否 | |
| `device` | body | int | 否 | |

只改 `name` 时保留 `status` 与 Build 结果。仅 `not_built` 时可以改 `type`、`batch_mode`、`batch_size`、`precision`、`opt_level`、`device`、`max_det`、`conf`、`iou`：此时 `status` 仍为 `not_built`，`version`、`num_class`、`classes`、`built_at`、`engine_path` 清空。磁盘上的 engine 不删。替换源文件用 `POST /{model_id}/upload`。

`data` 字段与列表中的单条相同。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo11n",
  "type": "YOLO26-DET",
  "version": null,
  "batch_size": 4,
  "batch_mode": "static",
  "precision": "fp16",
  "max_det": 30,
  "conf": "0.25",
  "iou": "0.45",
  "num_class": null,
  "classes": null,
  "source_path": "/root/models/pt/yolo11n.zip",
  "status": "not_built",
  "built_at": null
}
```

### `POST /{model_id}/upload`

上传模型文件。`multipart/form-data`，字段 `input`。`.pt` 打成 zip 落到 `/root/models/pt/{文件名}.zip`，`.onnx` 打成 zip 落到 `/root/models/onnx/{文件名}.zip`。成功后 `source_path` 为该 zip 路径。`status=building` 时 409，消息为 `model is building`。其它后缀返回 500。

入参：

| 字段 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `model_id` | path | string | 是 | 模型 UUID |
| `input` | body | file | 是 | `.pt` 或 `.onnx` |

`data` 字段与列表中的单条相同。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo11n",
  "type": "YOLO26-DET",
  "version": null,
  "batch_size": 1,
  "batch_mode": "static",
  "precision": "fp16",
  "max_det": 30,
  "conf": "0.25",
  "iou": null,
  "num_class": null,
  "classes": null,
  "source_path": "/root/models/pt/yolo11n.zip",
  "status": "not_built",
  "built_at": null
}
```

### `DELETE /{model_id}`

删除该模型及其 `.pt`、engine 文件。无查询参数、无请求体。`status=building` 时 409。`pipeline_gie` 中仍引用该 `model_id` 时 409，消息为 `Model is used by a GIE`。`data` 为 `null`。

入参：

| 字段 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `model_id` | path | string | 是 | 模型 UUID |

```json
null
```

### `POST /{model_id}/build`

按 `source_path` 里的 zip 构建。无查询参数、无请求体。须已有 `source_path`，且当前不是 `building`（否则 409，消息为 `model is building`）。请求内先把该 zip 与 ONNX 配方发给 `export_onnx/export`，确认 `/root/models/onnx/{文件名}.zip` 已生成后，再把这个 zip 与 TensorRT 配方发给 `export_trt/export`。只有后者成功才按返回值写入数据库；中途失败则把 `status` 从 `building` 退回。成功时 `data` 为更新后的模型。

入参：

| 字段 | 位置 | 类型 | 必填 | 说明 |
|------|------|------|------|------|
| `model_id` | path | string | 是 | 模型 UUID |

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo11n",
  "type": "YOLO26-DET",
  "version": "v1.0",
  "batch_size": 1,
  "batch_mode": "static",
  "precision": "fp16",
  "max_det": 30,
  "conf": "0.25",
  "iou": "0.45",
  "num_class": 80,
  "classes": [
    {"id": 0, "name": "person"}
  ],
  "source_path": "/root/models/onnx/yolo11n.zip",
  "status": "built",
  "built_at": "2026-06-28T10:15:00Z"
}
```

### `GET /{model_id}/status`

轮询 Build。进行中时 `data` 为 `id`、`status`、`built_at`。结束后字段与 Build 结果一致：`status` 为 `built` 或回退值，并带 `version`、`num_class`、`classes`。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "building",
  "built_at": null
}
```

### `POST /batch/build`

按 id 构建。每条与 `POST /{model_id}/build` 相同：须已有 `source_path`，且当前不是 `building`。先把 zip 与 ONNX 配方发给 `export_onnx/export`，确认 ONNX zip 已生成后，再发给 `export_trt/export`。只有后者成功才写入该条；中途失败则把该条 `status` 从 `building` 退回，并把 id 记入 `failed_ids`。

请求体：

```json
{
  "ids": [
    "550e8400-e29b-41d4-a716-446655440000",
    "7c9e6679-7425-40de-944b-e07fc1f90ae7"
  ]
}
```

`data.items` 为本次构建成功的模型，字段与 `POST /{model_id}/build` 的 `data` 相同。

```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "yolo11n",
      "type": "YOLO26-DET",
      "version": "v1.0",
      "batch_size": 1,
      "batch_mode": "static",
      "precision": "fp16",
      "max_det": 30,
      "conf": "0.25",
      "iou": "0.45",
      "num_class": 80,
      "classes": [
        {"id": 0, "name": "person"}
      ],
      "source_path": "/root/models/onnx/yolo11n.zip",
      "status": "built",
      "built_at": "2026-06-28T10:15:00Z"
    }
  ],
  "failed_ids": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7"
  ]
}
```

### `POST /batch/status`

按 id 轮询。无查询参数。进行中的条目只含 `id`、`status`、`built_at`。已结束的条目再带 `version`、`num_class`、`classes`。不存在的 id 记入 `failed_ids`。

请求体：

```json
{
  "ids": [
    "550e8400-e29b-41d4-a716-446655440000"
  ]
}
```

```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "status": "building",
      "built_at": null
    }
  ],
  "failed_ids": []
}
```

### `POST /batch/remove`

按 id 删除。`building` 的跳过并记入 `failed_ids`。

请求体：

```json
{
  "ids": [
    "550e8400-e29b-41d4-a716-446655440000"
  ]
}
```

```json
{
  "removed_count": 1,
  "failed_ids": []
}
```

---
## 数据库
### `model`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | |
| `name` | varchar | UNIQUE | — | |
| `type` | varchar | 非空 | — | `YOLO10-DET`、`YOLO26-SEG`、`YOLO26-DET`、`YOLO26-POSE` |
| `version` | varchar | 可空 | `NULL` | Build 成功写入 |
| `batch_size` | int | 非空 | — | 1…128 |
| `batch_mode` | varchar | 非空 | — | `static`\|`dynamic` |
| `precision` | varchar | 非空 | `fp16` | 仅 fp16 |
| `max_det` | int | 可空 | `NULL` | 最大检测数 |
| `conf` | numeric(4, 2) | 可空 | `NULL` | 置信度阈值 |
| `iou` | numeric(4, 2) | 可空 | `NULL` | IoU 阈值 |
| `num_class` | int | 可空 | `NULL` | |
| `classes` | JSON | 可空 | `NULL` | `[{id,name}]` |
| `source_path` | varchar | 可空 | — | 卷内绝对/相对路径 |
| `engine_path` | varchar | 可空 | `NULL` | |
| `status` | varchar | 非空 | `not_built` | `not_built` 从未成功构建（新建、覆盖上传、或首次构建失败）；`building` 构建已受理、进行中（此时拒绝再上传/再构建/删除）；`built` 当前 engine 可用（构建成功，或再次构建失败但曾成功，保留旧 engine） |
| `built_at` | timestamptz | 可空 | `NULL` | 任务**结束**时写 |
| `ever_built` | bool | 非空 | `false` | 失败回退用 |

索引：`name`；列表 search 可用 `name` icontains。

---
