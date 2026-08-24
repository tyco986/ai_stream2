# Pipelines API

管理端 Pipelines 页面后端 API。UI 规格见 [pipeline.md](./pipeline.md)。

**GIE** / **Analyzer** 为与 Pipeline **同级模板**；Pipeline 通过 `gie_id` / `analyzer.template` **引用**模板 id。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL（Pipeline） | `http://127.0.0.1:8000/ai_stream2/backend/pipelines` |
| 路径前缀（Pipeline） | `/ai_stream2/backend/pipelines` |
| 路径前缀（GIE 模板） | `/ai_stream2/backend/gie-templates` |
| 路径前缀（Analyzer 模板） | `/ai_stream2/backend/analyzer-templates` |

实现时须将固定段（`map`、`batch`、`types`、`schema`）注册在 `/{pipeline_id}` 之前；GIE / Analyzer 模板同理。

相关依赖：

| API | 用途 |
|-----|------|
| [model_api.md](./model_api.md) | GIE **Model** 下拉（`status=built`）、Class 弹窗 label |
| [stream_api.md](./stream_api.md) | Generator / Analyzer Streams、Analyzer Source 截帧 |

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
| `pipeline.status` | `stopped` \| `starting` \| `running` \| `stopping` \| `error` | 对应 UI **Status**（展示可 Title Case） |
| `debouncer.mode` | `slide` \| `fold` | Debouncer **Mode** |
| `annotation.type` | `roi_filtering` \| `overcrowding` \| `line_crossing` \| `direction_detection` | Analyzer 标注类型 |
| `annotation.shape` | `rectangle` \| `polygon` \| `line_direction` \| `direction` | 几何形状 |
| `source_kind` | `file` \| `stream` | Analyzer 参考图来源 |
| `line_mode` | `strict` \| `balanced` \| `loose` | Line Crossing / Direction Detection |

`pipeline.type` 为 DeepStream / generator 注册类名字符串（如 `DetVisRTSPPipeline`、`PresenceVisVideoPipeline`）；下拉枚举以 `GET /types` 为准；选定 Type 后的表单可编辑性与默认值以 `POST /schema` 全量响应为准（见 [TypeSchema](#typeschema)）。

### 字段语义

| 字段 | 说明 |
|------|------|
| `pipeline_id` | Pipeline **id**（UUID）；路径 `/{pipeline_id}` 与响应 `id` 同值 |
| `gie_id` | GIE **模板** id；路径 `/gie-templates/{gie_id}` |
| `analyzer_template_id` | Analyzer **模板** id；路径 `/analyzer-templates/{analyzer_template_id}`；Pipeline 体内字段名为 `analyzer.template` |
| `class_id` / `class_ids` | 类别 id 列表；单元素可存 `int`，多元素存 `int[]`；**`-1`** 表示 ALL |
| `class_attrs[].class` | `"all"` 或类别 id（JSON 中数字或数字字符串均可，服务端归一） |
| `detected_*` | UI / API 可用哨兵 **`-1`**；生成 nvinfer 配置时展开为 `0` / `0` / `2147483647` / `2147483647` |

---

## 数据模型

### PipelineListItem

主页表格行（列表接口可只返回摘要；详情用完整 `Pipeline`）。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo26n-det-rtsp",
  "type": "DetVisRTSPPipeline",
  "status": "running",
  "updated_at": "2026-07-18T02:00:00Z"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成，不可改 |
| `name` | string | 否 | **全库唯一** |
| `type` | string | 否 | Pipeline Type |
| `status` | string | 否 | 运行态 |
| `updated_at` | datetime | 是 | ISO 8601 UTC |

### Pipeline

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo26n-det-rtsp",
  "type": "DetVisRTSPPipeline",
  "status": "stopped",
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
    "template": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
    "roi_filtering": { "class_id": [-1] },
    "overcrowding": { "class_id": [0] },
    "line_crossing": { "class_id": [0, 2] },
    "direction_detection": null
  },
  "sahi": null,
  "input": null,
  "output": null,
  "created_at": "2026-07-01T10:00:00Z",
  "updated_at": "2026-07-18T02:00:00Z"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | |
| `name` | string | 否 | 全库唯一 |
| `type` | string | 否 | |
| `status` | string | 否 | 只读运行态；创建时 `stopped` |
| `drawer` | Drawer | 否 | Probe Drawer |
| `logger` | `{ "interval": int }` | 否 | 仅 `interval`；`root` 由服务端生成 |
| `messager` | `{ "interval": int }` | 否 | 仅 `interval`；topic/host/port 服务端默认 |
| `debouncer` | Debouncer \| null | 是 | Type 不需要时为 `null` |
| `streams` | UUID[] | 否 | Generator 流 id；可空数组（非 RTSP Type） |
| `gie_id` | UUID | 否 | 引用 GIE 模板 |
| `interval` | int | 否 | PGIE / generator 跳帧；`≥ 0` |
| `tracker` | `{ "class_id": int \| int[] }` \| null | 是 | 未启用为 `null` |
| `analyzer` | PipelineAnalyzer \| null | 是 | 未启用为 `null` |
| `sahi` | Sahi \| null | 是 | SAHI Type 必填；非 SAHI 为 `null` |
| `input` | string \| null | 是 | 文件/视频 Type 输入路径；RTSP 为 `null` |
| `output` | string \| null | 是 | 文件/视频 Type 输出路径；RTSP 为 `null` |
| `created_at` / `updated_at` | datetime | 是 | |

### Drawer

| 字段 | 类型 | 说明 |
|------|------|------|
| `show_label` | bool | |
| `show_conf` | bool | |
| `interval` | int | `≥ 0` |
| `fade_time` | int | `≥ 0` |

### Debouncer

| 字段 | 类型 | 说明 |
|------|------|------|
| `length` | int | `≥ 1` |
| `threshold` | number | `[0, 1]` |
| `class_ids` | int[] | 非空；可含 `-1`（仅 ALL） |
| `mode` | string | `slide` \| `fold` |

### PipelineAnalyzer

| 字段 | 类型 | 说明 |
|------|------|------|
| `streams` | UUID[] | ⊆ `Pipeline.streams`；非空 |
| `template` | UUID | Analyzer 模板 id |
| `roi_filtering` | `{ "class_id": int \| int[] }` \| null | Class 未配则为 `null` |
| `overcrowding` | 同上 | |
| `line_crossing` | 同上 | |
| `direction_detection` | 同上 | |

### Sahi

| 字段 | 类型 | 说明 |
|------|------|------|
| `nvsahipreprocess.slice_width` | int | `> 0` |
| `nvsahipreprocess.slice_height` | int | `> 0` |
| `nvsahipreprocess.overlap_width_ratio` | number | `[0, 1)` |
| `nvsahipreprocess.overlap_height_ratio` | number | `[0, 1)` |
| `nvsahipostprocess.match_threshold` | number | `[0, 1]` |

SAHI Type 的 schema `sahi.available: true` 时必填；非 SAHI 为 `null`。`available: true` 时区块含 `params.nvsahipreprocess` / `params.nvsahipostprocess`（带 `default`）。

### SchemaField

表单叶子字段（`POST /schema` 嵌套在区块 `params` 内）。

| 字段 | 类型 | 说明 |
|------|------|------|
| `available` | bool | `true` → 可编辑；`false` → 不可编辑 |
| `default` | any | 该字段默认值；类型与对应 Pipeline 持久化字段一致 |

### SchemaBlock

表单区块（如 `drawer`、`streams`、`pgie`）。

| 字段 | 类型 | 说明 |
|------|------|------|
| `available` | bool | 区块级可编辑；`false` 时整块禁用 |
| `params` | object | 子字段 map：键为字段名，值为 `SchemaField` 或嵌套结构；**该 Type 约定的全部子字段必须出现，禁止缺省** |
| `default` | any | 可选；仅用于无嵌套 `params` 的简单区块（如 generator `interval` 可为 `{ "available": true, "default": 50 }`） |

### TypeSchema

`POST /schema` 响应 `data`。与 UI **Pipeline** / **Generator** 分组对应；顶层固定二键：`pipeline`、`generator`（同级，禁止缺键）。

```json
{
  "pipeline": {
    "type": "DetVisRTSPPipeline",
    "params": {
      "drawer": {
        "available": true,
        "params": {
          "show_label": { "available": true, "default": true },
          "show_conf": { "available": true, "default": true },
          "interval": { "available": true, "default": 50 },
          "fade_time": { "available": true, "default": 1 }
        }
      },
      "logger": {
        "available": true,
        "params": {
          "interval": { "available": true, "default": 50 }
        }
      },
      "messager": {
        "available": true,
        "params": {
          "interval": { "available": true, "default": 0 }
        }
      },
      "debouncer": {
        "available": false,
        "params": {
          "length": { "available": false, "default": 10 },
          "threshold": { "available": false, "default": 0.5 },
          "class_ids": { "available": false, "default": [] },
          "mode": { "available": false, "default": "fold" }
        }
      }
    }
  },
  "generator": {
    "type": "DetVisRTSPGenerator",
    "params": {
      "streams": {
        "available": true,
        "params": {}
      },
      "pgie": {
        "available": true,
        "params": {}
      },
      "interval": {
        "available": true,
        "default": 50
      },
      "tracker": {
        "available": true,
        "params": {
          "class_id": { "available": true, "default": [-1] }
        }
      },
      "analyzer": {
        "available": true,
        "params": {
          "streams": { "available": true, "default": [] },
          "template": { "available": true, "default": null },
          "roi_filtering": { "available": true, "default": { "class_id": [-1] } },
          "overcrowding": { "available": true, "default": null },
          "line_crossing": { "available": true, "default": null },
          "direction_detection": { "available": true, "default": null }
        }
      },
      "sahi": {
        "available": false
      },
      "input": {
        "available": false
      },
      "output": {
        "available": false
      }
    }
  }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `pipeline.type` | string | Probe / DeepStream 侧类型名（通常与请求的 `pipeline_type` 同值） |
| `pipeline.params` | object | 固定含 `drawer` / `logger` / `messager` / `debouncer`（全量，禁止缺键） |
| `generator.type` | string | Generator 侧类型名（如 `DetVisRTSPGenerator`） |
| `generator.params` | object | 固定含 `streams` / `pgie` / `interval` / `tracker` / `analyzer` / `sahi` / `input` / `output`（全量，禁止缺键） |

约定：

| 规则 | 说明 |
|------|------|
| 全量 | 每个 Type 的响应必须带齐 `pipeline` / `generator` 全部区块与叶子字段；用 `available: false` 表示不可用，**不要省略键** |
| `available` | 决定 UI 是否可编辑；与 [pipeline.md](./pipeline.md) Type 规则一致 |
| `default` | 选中 Type 后写入表单的初始值；切换 Type 时用新 schema 覆盖（`pipeline` / `generator` 叶子字段） |

### GieTemplate

```json
{
  "id": "9a1b2c3d-4567-89ab-cdef-0123456789ab",
  "name": "yolo26n-default",
  "model_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "model_name": "yolo26n",
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
  ],
  "class_attrs_summary": "all: conf=0.25, topk=300",
  "pipelines": [
    { "id": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", "name": "yolo26n-det-rtsp" }
  ],
  "created_at": "2026-07-01T10:00:00Z",
  "updated_at": "2026-07-18T02:00:00Z"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | |
| `name` | string | 否 | 全库唯一 |
| `model_id` | UUID | 否 | Models **model id**；须 `status=built` |
| `model_name` | string | 是 | 列表展示用只读冗余 |
| `class_attrs` | ClassAttrRow[] | 否 | 至少 1 行；`class` 唯一 |
| `class_attrs_summary` | string | 是 | 列表 **Class Attrs** 列；服务端生成 |
| `pipelines` | `{ id, name }[]` | 否 | **只读、服务端聚合**；`pipeline.gie_id ===` 本模板的 Pipeline 列表（可空数组）；列表 **Pipelines** 列 |

### ClassAttrRow

| 字段 | 类型 | 说明 |
|------|------|------|
| `class` | string \| int | `"all"` 或 `0 .. num_class-1` |
| `conf` | number | `[0, 1]` |
| `topk` | int | `[0, 300]` |
| `detected_min_w` / `detected_min_h` | int | `-1` 或 `≥ 0` |
| `detected_max_w` / `detected_max_h` | int | `-1` 或 `≥ 0` |

过滤语义（生成配置时）：含 `all` → 不写 `filter-out-class-ids`；无 `all` → 仅启用数字 `class`，其余滤掉。见 [pipeline.md](./pipeline.md) / `PgieParser`。

### AnalyzerTemplateListItem

主页 Analyzer 弹窗表格行。

```json
{
  "id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
  "name": "entrance_default",
  "roi_filtering_count": 2,
  "overcrowding_count": 1,
  "line_crossing_count": 1,
  "direction_detection_count": 0,
  "pipelines": [
    { "id": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", "name": "yolo26n-det-rtsp" }
  ],
  "updated_at": "2026-07-18T02:00:00Z"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `pipelines` | `{ id, name }[]` | **只读、服务端聚合**；`pipeline.analyzer.template ===` 本模板的 Pipeline 列表（可空数组）；列表 **Pipelines** 列 |

### AnalyzerTemplate

```json
{
  "id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
  "name": "entrance_analytics",
  "source_kind": "stream",
  "source_stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "source_file_name": null,
  "source_image_url": "/media/analyzer-templates/.../snapshot.jpg",
  "config_width": 1920,
  "config_height": 1080,
  "captured_at": "2026-07-02T10:15:00Z",
  "annotations": []
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | |
| `name` | string | 否 | 全库唯一 |
| `source_kind` | string | 否 | `file` \| `stream` |
| `source_stream_id` | UUID | 是 | `stream` 时必填 |
| `source_file_name` | string | 是 | `file` 时展示名 |
| `source_image_url` | string | 否 | 参考图 URL |
| `config_width` / `config_height` | int | 否 | 原图像素；写入 nvdsanalytics |
| `captured_at` | datetime | 是 | |
| `annotations` | Annotation[] | 否 | |

### Annotation

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
| `name` | string | **同一 Analyzer 模板内唯一** |
| `type` | string | 见枚举 |
| `shape` | string | 见枚举 |
| `points` | int[] | 像素坐标；见 [pipeline.md](./pipeline.md) points 表 |
| `click_points` | int[] \| null | 用户点击落点（UI 圆点）；Rectangle 为两次点击 |
| `object_threshold` | int \| null | Overcrowding |
| `time_threshold` | int \| null | Overcrowding（ms） |
| `extend` | bool \| null | Line Crossing |
| `mode` | string \| null | `strict` \| `balanced` \| `loose` |

---

## 接口：Pipelines

Base：`/ai_stream2/backend/pipelines`

### 列表与元数据

#### `GET /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/pipelines`

**说明**：主页表格；**Refresh** / **Search** / 翻页。

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `search` | string | 按 `name` 子串过滤（可选） |
| `page` | int | 默认 `1` |
| `page_size` | int | 默认 `20` |

**响应 `data`**：分页；`items` 为 `PipelineListItem[]`（或含摘要字段的 `Pipeline[]`）。

---

#### `GET /types`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/pipelines/types`

**说明**：Pipeline 子页 **Type** 下拉选项（**仅** type 名；不含可编辑性 / 默认值）。

**响应 `data`**：

```json
{
  "items": [
    { "type": "DetVisRTSPPipeline" },
    { "type": "SegVisRTSPPipeline" },
    { "type": "PresenceVisVideoPipeline" }
  ]
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `items[].type` | string | 注册表中的 pipeline type 名 |

表单能力与默认值**不**由此接口返回；见 `POST /schema`。

---

#### `POST /schema`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/pipelines/schema`

**说明**：用户在 Pipeline 子页**选定或切换 Type** 后调用；返回该 Type 下 Pipeline / Generator 的**全量**表单 schema。

**请求**：`application/json`

```json
{
  "pipeline_type": "DetVisRTSPPipeline"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `pipeline_type` | string | 是 | 须为 `GET /types` 返回的某一 `type` |

**响应 `data`**：`TypeSchema`（见 [TypeSchema](#typeschema)）。

| HTTP | 场景 |
|------|------|
| 400 | `pipeline_type` 缺失或为空 |
| 404 | `pipeline_type` 不在注册表 |

---

#### `GET /map`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/pipelines/map`

**说明**：可选；`id → name` 映射（其它模块引用）。

**响应 `data`**：`{ "<uuid>": "<name>", ... }`。

---

#### `GET /{pipeline_id}`

**说明**：Pipeline 子页编辑回填。

**响应 `data`**：完整 `Pipeline`。

编辑打开时：先用详情回填；再以当前 `type` 调用 `POST /schema`，用 schema 的 `available` 控制可编辑态（**不要**用 schema `default` 覆盖已保存值）。

---

#### `POST /`

**说明**：主页 **Add** → 子页 **Save**（新建）。

**请求**：`application/json`，body 为完整 `Pipeline`（无 `id` / `status` / 时间戳）。

**校验**：见 [pipeline.md](./pipeline.md) Toast / 校验；`gie_id` 须存在且模型 `built`；`streams` 同 resolution/fps（当 schema 要求 streams 时）；`analyzer.streams ⊆ streams`。

**响应 `data`**：完整 `Pipeline`（含新 `id`，`status=stopped`）。

---

#### `PUT /{pipeline_id}`

**说明**：行内 **✏** → 子页 **Save**。

**请求**：完整可写字段（通常不允许改 `id`；`status` 只读忽略）。

**响应 `data`**：更新后的 `Pipeline`。

**行为**：若 `status=running`，拒绝修改关键拓扑字段或要求先 Stop（实现二选一，建议 **409** 提示先停止）。

---

#### `DELETE /{pipeline_id}`

**说明**：行内 **🗑**。

**行为**：`running` 时拒绝或先停再删（建议拒绝 **409**）。

**响应 `data`**：`null` 或 `{}`。

---

### 批量

#### `POST /batch/delete`

**说明**：工具栏 **Remove**。

**请求**：

```json
{ "ids": ["550e8400-e29b-41d4-a716-446655440000"] }
```

**响应 `data`**：

```json
{
  "deleted_count": 1,
  "failed_ids": []
}
```

`failed_ids`：不存在或仍在运行无法删除的 id。

---

### 运行时动作

#### `POST /{pipeline_id}/start`

**说明**：Operations **▶**。管理端触发 Generator 的**唯一**入口（前端不直连 Generator）。

**行为**（后端顺序，异步）：

1. 校验 Pipeline 可启动（存在、非 `running` / `starting` 等）；`status` → `starting`
2. **代理调用 Generator**：以已保存的 Pipeline 配置及引用的 GIE / Analyzer 模板为输入，生成并落盘 DeepStream 配置
3. Generator **成功** → 拉起 DeepStream 进程/容器任务 → `status` → `running`；DeepStream 启动失败 → `error`
4. Generator **失败** → **不**拉起 DeepStream → `status` → `error`；`message` / 日志写入失败原因

**不**在 `POST /`、`PUT /{id}`（Save）或 `POST /schema` 时调用 Generator。

**响应 `data`**（受理即返，不阻塞至 running）：

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "starting"
}
```

或返回当前 `PipelineListItem`。客户端应轮询 `GET /{pipeline_id}/status` 直至 `running` / `error` / `stopped`。

| HTTP | 场景 |
|------|------|
| 404 | `pipeline_id` 不存在 |
| 409 | 已在 `running` / `starting`，或缺少必填配置无法启动 |
| 502 | Generator 或 DeepStream 上游不可达（也可先 202 受理，最终 `status=error`） |

---

#### `POST /{pipeline_id}/stop`

**说明**：Operations **■**。

**行为**：`status` → `stopping` → `stopped`。

**响应 `data`**：当前状态摘要。

---

#### `GET /{pipeline_id}/status`

**说明**：行内 **↻** 或轮询；刷新单行 Status。

**响应 `data`**：

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "running",
  "message": ""
}
```

---

#### `GET /{pipeline_id}/logs`

**说明**：Operations **📓**。

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `tail` | int | 可选；返回末尾 N 行 |
| `offset` | int | 可选；分页 |

**响应 `data`**：

```json
{
  "lines": ["..."],
  "updated_at": "2026-07-18T02:00:00Z"
}
```

---

## 接口：GIE Templates

Base：`/ai_stream2/backend/gie-templates`

#### `GET /`

**说明**：主页 **GIE** 弹窗列表；Pipeline 子页 **PGIE** 下拉也可调（可不分页或 `page_size` 较大）。

**Query**：`search` / `page` / `page_size`（可选）。

**响应 `data`**：分页；`items` 为带 `pipelines` 占用信息的 `GieTemplate[]`（列表至少含 `id` / `name` / `model_name` / `class_attrs_summary` / `pipelines`）。

---

#### `GET /{gie_id}`

**说明**：GIE 子页编辑回填。

**响应 `data`**：完整 `GieTemplate`。

---

#### `POST /`

**说明**：GIE 弹窗 **Add** → 子页 **Save**。

**请求**：

```json
{
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

**校验**：Name 唯一；`model_id` 存在且 `built`；`class_attrs` 非空、class 唯一、字段范围见 UI 规格。

**响应 `data`**：完整 `GieTemplate`。

---

#### `PUT /{gie_id}`

**说明**：GIE 弹窗 **✏** → 子页 **Save**。

**响应 `data`**：更新后的 `GieTemplate`。

---

#### `DELETE /{gie_id}`

**说明**：行内 **🗑**。

**行为**：若仍被 Pipeline 引用 → **409**（推荐）或级联清空引用（须文档声明）。

**响应 `data`**：`null` 或 `{}`。

---

#### `POST /batch/delete`

**说明**：GIE 弹窗 **Remove**。

**请求**：`{ "ids": ["..."] }`

**响应 `data`**：`{ "deleted_count": n, "failed_ids": [] }`（引用中的进 `failed_ids`）。

---

## 接口：Analyzer Templates

Base：`/ai_stream2/backend/analyzer-templates`

#### `GET /`

**说明**：主页 **Analyzer** 弹窗列表；Pipeline **Template** 下拉。

**Query**：`search` / `page` / `page_size`。

**响应 `data`**：分页；`items` 为 `AnalyzerTemplateListItem[]`（含计数字段与 `pipelines` 占用信息）。

计数规则：按 `annotations[].type` 聚合。

---

#### `GET /{analyzer_template_id}`

**说明**：Analyzer 子页编辑回填。

**响应 `data`**：完整 `AnalyzerTemplate`。

---

#### `POST /`

**说明**：Analyzer 弹窗 **Add** → 子页 **Save**。

**请求**：完整 `AnalyzerTemplate`（无 `id`）；可先建空标注再改。

**响应 `data`**：完整 `AnalyzerTemplate`。

---

#### `PUT /{analyzer_template_id}`

**说明**：Analyzer 弹窗 **✏** → 子页 **Save**（含全部 `annotations`）。

**响应 `data`**：更新后的 `AnalyzerTemplate`。

---

#### `DELETE /{analyzer_template_id}`

**说明**：行内 **🗑**；被 Pipeline 引用时建议 **409**。

---

#### `POST /batch/delete`

**说明**：Analyzer 弹窗 **Remove**。

**请求 / 响应**：同 GIE `batch/delete`。

---

### Source

#### `POST /{analyzer_template_id}/source/file`

**说明**：Analyzer Source **File** 上传参考图。

**请求**：`multipart/form-data`

| 字段 | 类型 | 说明 |
|------|------|------|
| `file` | file | `jpg` / `jpeg` / `png` |

**响应 `data`**：

```json
{
  "source_kind": "file",
  "source_file_name": "snapshot.jpg",
  "source_image_url": "/media/analyzer-templates/.../snapshot.jpg",
  "config_width": 1920,
  "config_height": 1080,
  "captured_at": "2026-07-18T02:00:00Z"
}
```

非法格式 → **400**，文案对齐 UI **`Unsupported image format`**。

---

#### `POST /{analyzer_template_id}/source/stream`

**说明**：选择 Stream 后截帧（或 Source **↻**）。

**请求**：

```json
{
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7"
}
```

**行为**：可转调 Streams `POST .../streams/{stream_id}/snapshot`；写入模板的 `source_*` / `config_*`。

**失败**：流 offline → **400** / **502**，文案 **`Snapshot failed for {name}`**。

**响应 `data`**：同 File 上传响应形状（`source_kind=stream`，含 `source_stream_id`）。

> 亦可直接使用 [stream_api.md](./stream_api.md) 的 snapshot，再随 **PUT** 模板一并提交 URL；实现二选一，前端须统一。

---

## 错误码（建议）

| HTTP | 场景 |
|------|------|
| 400 | 字段范围非法；图片格式不支持；截帧失败；`class_attrs` 空；resolution/fps 不一致 |
| 404 | pipeline / gie / analyzer 模板 id 不存在 |
| 409 | `name` 重复；运行中不可改/删；模板仍被 Pipeline 引用 |
| 422 | 请求体校验失败 |

---

## 调用节点

### 主页面 — Pipeline 表

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入页 / Refresh / 翻页 / Search | `GET` | `/pipelines/?...` |
| **Add** → Save | `POST` | `/pipelines/` |
| **Remove** | `POST` | `/pipelines/batch/delete` |
| **✏** 打开 | `GET` | `/pipelines/{id}` |
| **✏** Save | `PUT` | `/pipelines/{id}` |
| **🗑** | `DELETE` | `/pipelines/{id}` |
| **▶** | `POST` | `/pipelines/{id}/start` |
| **■** | `POST` | `/pipelines/{id}/stop` |
| **↻** | `GET` | `/pipelines/{id}/status` |
| **📓** | `GET` | `/pipelines/{id}/logs` |

### 主页面 — GIE 弹窗

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 打开弹窗 | `GET` | `/gie-templates/` |
| **Add** / **✏** Save | `POST` / `PUT` | `/gie-templates/` / `/{id}` |
| **Remove** / **🗑** | `POST` `/batch/delete` 或 `DELETE /{id}` | |

### 主页面 — Analyzer 弹窗

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 打开弹窗 | `GET` | `/analyzer-templates/` |
| **Add** / **✏** Save | `POST` / `PUT` | `/analyzer-templates/` / `/{id}` |
| **Remove** / **🗑** | `POST` `/batch/delete` 或 `DELETE /{id}` | |
| Source File / Stream ↻ | `POST` | `.../source/file` 或 `.../source/stream` |

### Pipeline 子页依赖

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| Type 下拉选项 | `GET` | `/pipelines/types` |
| 选定 / 切换 Type | `POST` | `/pipelines/schema`（body `{ "pipeline_type" }`） |
| 编辑打开（回填后） | `POST` | `/pipelines/schema`（用详情中的 `type`；仅应用 `available`） |
| PGIE 下拉 | `GET` | `/gie-templates/` |
| Analyzer Template 下拉 | `GET` | `/analyzer-templates/` |
| Streams 多选 | `GET` | `/streams/?enabled=true&status=online`（见 stream_api） |
| Class 弹窗 label | `GET` | `/models/{model_id}`（见 model_api；经 GIE 的 `model_id`） |

---

## 典型流程

### 新建并启动一条 RTSP Pipeline

```
1. GET  /pipelines/types         → Type 下拉
2. POST /pipelines/schema        → body { "pipeline_type": "DetVisRTSPPipeline" }；填默认值 / available
3. POST /gie-templates/          → 得到 gie_id
4. POST /analyzer-templates/     → 可选；得到 analyzer template id
5. POST /pipelines/              → 仅落库；不调 Generator
6. POST /pipelines/{id}/start    → 后端：Generator → 成功后再拉起 DeepStream；status=starting
7. GET  /pipelines/{id}/status   → 直至 running 或 error
```

### 仅改 GIE 阈值（多 Pipeline 共用模板）

```
1. GET  /gie-templates/{id}
2. PUT  /gie-templates/{id}      → 仅更新模板；不调 Generator
3. POST /pipelines/{id}/start    → 下次 ▶ 时按新模板重新 generate 再启动
```
