# Pipelines（后端）

对应前端契约：[pipeline_api.md](../frontend/pipeline_api.md)。  
实现包：`servers/django/pages/pipelines/`（同包挂三个 URL 前缀）。  

| 前缀 | 用途 |
|------|------|
| `/ai_stream2/backend/pipelines` | Pipeline CRUD / schema / start-stop |
| `/ai_stream2/backend/gie-templates` | GIE 模板 |
| `/ai_stream2/backend/analyzer-templates` | Analyzer 模板 |

---

## 代理的 API

| 本页触发 | 上游 | 用途 |
|----------|------|------|
| `POST /`（create）/ `PUT` | **generator** + **docker_socket_proxy** | 生成 `/root/configs/generator/{name}/`，写 `/root/configs/deepstream/{name}.yaml`，**create** DeepStream 容器 `{project}_{name}`（不 start）；端口自 2000 起分配 |
| `POST /{pipeline_id}/start` | **docker_socket_proxy** + **deepstream 容器** | docker **start** → health → `POST /start_pipeline` |
| `POST /{id}/stop` | **docker_socket_proxy** | docker **stop**（无 stop_pipeline API） |
| `DELETE` | **docker_socket_proxy** | docker **rm** + 删配置目录；释放端口 |
| `POST .../source/stream` | 可选经 ffmpeg/stream 截帧能力 | Analyzer 参考图；失败 message `Snapshot failed for {name}` |

一条 Pipeline 对应一个 DeepStream 容器；共享 `${PROJECT}_deepstream` 不再作为常驻基础设施。

---

## 暴露的 API

### Pipelines

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/` | 分页列表 |
| `GET` | `/types` | 类型名列表 |
| `POST` | `/schema` | 选定 type → 全量 `TypeSchema` |
| `GET` | `/map` | id → name |
| `GET` | `/{pipeline_id}` | 完整 Pipeline |
| `POST` | `/` | 新建（`status=stopped`） |
| `PUT` | `/{pipeline_id}` | 全量更新；`running`/`starting` → `409` |
| `DELETE` | `/{pipeline_id}` | 删除；运行中 → `409` |
| `POST` | `/batch/delete` | 批量；失败入 `failed_ids` |
| `POST` | `/{pipeline_id}/start` | 异步启动 |
| `POST` | `/{pipeline_id}/stop` | 停止 |
| `GET` | `/{pipeline_id}/status` | 运行态轮询 |
| `GET` | `/{pipeline_id}/logs` | 日志（`tail`/`offset`） |

### GIE templates（`/gie-templates`）

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET`/`POST` | `/` | 列表 / 新建 |
| `GET`/`PUT`/`DELETE` | `/{gie_id}` | 详情 / 更新 / 删（被引用 → `409`） |
| `POST` | `/batch/delete` | 批量 |

### Analyzer templates（`/analyzer-templates`）

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET`/`POST` | `/` | 列表 / 新建 |
| `GET`/`PUT`/`DELETE` | `/{analyzer_template_id}` | 详情 / 全量 annotations / 删（被引用 → `409`） |
| `POST` | `/batch/delete` | 批量 |
| `POST` | `/{id}/source/file` | 上传参考图 |
| `POST` | `/{id}/source/stream` | 从流截帧 |

字段 / 枚举 / TypeSchema 形以 [pipeline_api.md](../frontend/pipeline_api.md) 为准。权限：`pipelines.view_pipeline` / `add` / `change` / `delete`（三前缀共用本页权）。

固定段：`map`、`batch`、`types`、`schema` 先于 `/{id}`。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | `Pipeline`、`GieTemplate`、`AnalyzerTemplate`（annotations JSON） |
| HTTP | `httpx` → generator、deepstream |
| Schema | 类型注册表（静态或 introspect generator）；`POST /schema` 全量键 |
| 鉴权 | session + `has_perm` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| 配置生成 | **复用** generator（create / update） |
| 容器生命周期 | **复用** docker_socket_proxy（create / start / stop / rm） |
| 运行时拉起 | **复用** 对应 DeepStream 容器的 `start_pipeline` |
| 端口分配 | **造** `DeepStreamPortManager`（自 2000 起） |
| Pipeline/GIE/Analyzer CRUD | **造** |
| TypeSchema | **造**（`TypeRegistry`；禁止前端按 type 名写死字段） |
| model / stream 引用 | UUID 软引用；GIE `model_id` 须 built（经 shared 只读或 start 时校验） |
| Analyzer 截帧 | **造** client 调网内截帧；不 import streams models |
| Site Config 切片 | **注册** pipelines + gie + analyzer（含 annotations / 参考图路径） |
| 响应外壳 / 分页 | **复用** `shared.*` |

---

## 目录结构

```
servers/django/
  config/urls.py              # include pipelines, gie-templates, analyzer-templates
  pages/pipelines/
    apps.py
    urls_pipelines.py
    urls_gie.py
    urls_analyzer.py
    models.py                 # Pipeline, GieTemplate, AnalyzerTemplate
    serializers.py
    views/
    services.py               # PipelineService, GieTemplateService, AnalyzerTemplateService,
                              # TypeRegistry, StartStopOrchestrator, PipelineLogService
    clients.py                # GeneratorClient, DeepStreamClient, SnapshotClient
    permissions.py
    migrations/
```

禁止 import `pages.models` / `pages.streams` / `pages.events`。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `Pipeline` | 配置 JSON + `status` |
| `GieTemplate` | `model_id` + `class_attrs` |
| `AnalyzerTemplate` | source + `annotations[]` |
| `TypeRegistry` | `list_types` / `get_schema(type)` |
| `PipelineService` | CRUD / map / list；运行中禁改删 |
| `GieTemplateService` / `AnalyzerTemplateService` | CRUD；引用检查；聚合 `pipelines[]` |
| `StartStopOrchestrator` | start/stop 状态机 + 调上游 |
| `GeneratorClient` / `DeepStreamClient` | HTTP |
| `SnapshotClient` | source/stream 截帧 |
| `PipelineLogService` | 读日志 |

### Start 状态机

```
stopped|error → starting →（generator OK）→ deepstream → running
                      └→ generator 失败 → error（不调 deepstream）
                      └→ deepstream 失败 → error
running|starting → stop → stopping → stopped
```

受理 start 即返 `{status: starting}`；客户端轮询 `GET .../status`。

---

## 数据库

### `pipelines_pipeline`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | |
| `name` | varchar | UNIQUE | |
| `type` | varchar | 非空 | 如 `DetVisRTSPPipeline` |
| `status` | varchar | 非空 | 默认 `stopped` |
| `config` | JSON | 非空 | drawer/logger/messager/debouncer/streams/gie_id/interval/tracker/analyzer … |
| `created_at` / `updated_at` | timestamptz | | |

`streams` / `gie_id` / `analyzer.template` 均在 JSON 或旁路列；实现可用显式列便于索引：`gie_id` UUID 可空、索引。

### `pipelines_gietemplate`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | |
| `name` | varchar | UNIQUE | |
| `model_id` | UUID | 非空 | 软引用；须 built |
| `model_name` | varchar | 可空 | 冗余展示 |
| `class_attrs` | JSON | 非空 | ≥1 行 |
| `created_at` / `updated_at` | | | |

`class_attrs_summary`、`pipelines[]` **不**存表，响应时生成。

### `pipelines_analyzertemplate`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | |
| `name` | varchar | UNIQUE | |
| `source_kind` | varchar | 非空 | `file`\|`stream` |
| `source_stream_id` | UUID | 可空 | |
| `source_file_name` | varchar | 可空 | |
| `source_image_path` | varchar | 可空 | → `source_image_url` |
| `config_width` / `config_height` | int | 非空 | |
| `captured_at` | timestamptz | 可空 | |
| `annotations` | JSON | 非空 | 默认 `[]` |
| `updated_at` | | | |

列表计数列由 annotations 聚合，不冗余存（或缓存列可选）。

---

## 各请求写库明细

| 请求 | 写入 |
|------|------|
| `POST /pipelines/` | INSERT；`status=stopped` |
| `PUT /pipelines/{id}` | UPDATE config/name/…；禁改 status；运行中 `409` |
| `DELETE` / batch | DELETE；运行中 → failed |
| `POST .../start` | `status=starting` → 上游 → `running`/`error` |
| `POST .../stop` | `stopping` → `stopped` |
| GIE/Analyzer POST/PUT | INSERT/UPDATE；删除前查引用 |
| `source/file` \| `source/stream` | 写图文件 + UPDATE source_* |
| `POST /schema`、`GET types/status/logs` | 只读（status 可同步上游） |

Site Config import：重建三表；导入后**不**自动 start；对曾 running 的行落为 `stopped`。

---

## 调用顺序

### Schema / CRUD

```
GET /types → TypeRegistry.list
POST /schema {pipeline_type} → 全量 TypeSchema（缺键禁止）
POST/PUT pipeline → 校验 schema available 字段与引用 → 落库（不调 generator）
```

### Start

```
View → change 权
  → status ∈ {running, starting}? 409
  → status=starting
  → GeneratorClient.generate(pipeline)
       fail → status=error → 返回
  → DeepStreamClient.start(...)
       fail → status=error
       ok → status=running
```

### Stop

```
status=stopping → DeepStreamClient.stop → status=stopped
```

### Analyzer source/stream

```
SnapshotClient.capture(stream_id) → 写 media → 更新模板
offline/失败 → 400/502，message 含流名
```

### 生成配置时的哨兵

| API | nvinfer |
|-----|---------|
| `detected_*=-1` | `0` / `0` / `2147483647` / `2147483647` |
| `class_id=-1` | ALL |

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| generator | Start 必需 |
| deepstream | Start 成功后 / stop / status / logs |
| models | `model_id` 软引用；built 校验经 shared 只读 |
| streams | UUID 列表；截帧经 SnapshotClient |
| events | 运行时写 Kafka；**不**依赖本页词典；DeepStream 侧 `event_coder` 由 pipeline 构造器注入 |
| Site Config | **注册** 三实体切片 |
| servers | 运维重启上游；无代码依赖 |
