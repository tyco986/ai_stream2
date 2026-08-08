# Models（后端）

对应前端契约：[model_api.md](../frontend/model_api.md)。  
实现包：`servers/django/pages/models_page/` 或 `pages/models/`（Django app_label=`models`；包名避免与 `django.db.models` 混淆时用 `ml_models`/`models_page`，**URL 与权限仍为** `/backend/models`、`models.*`）。前缀：`/ai_stream2/backend/models`。

---

## 代理的 API

| 本页触发 | 上游 | 用途 |
|----------|------|------|
| `POST /{model_id}/build`（后台线程） | **export_onnx** → **export_trt** | Django 编排：`.pt` → ONNX 目录 → TensorRT engine |
| `GET /{model_id}/build/status`、logs | 本页状态机 + build 日志文件 | 轮询与 Log（不直连上游异步任务 API） |

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/map` | name → id |
| `GET` | `/` | 分页列表（search） |
| `POST` | `/` | 新建（multipart；可选 `.pt`） |
| `GET` | `/{model_id}` | 详情 |
| `DELETE` | `/{model_id}` | 删记录 + 关联文件 |
| `POST` | `/{model_id}/upload` | 替换 `.pt` |
| `POST` | `/{model_id}/build` | 启动异步 Build |
| `GET` | `/{model_id}/build/status` | 轮询 |
| `GET` | `/{model_id}/logs` | Build 日志（`tail`） |
| `POST` | `/batch/delete` | 批量删；`building` → 入 `failed_ids` |

字段 / 错误码以 [model_api.md](../frontend/model_api.md) 为准。权限：`models.view_model` / `add_model` / `change_model` / `delete_model`。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | `ModelArtifact`（避免名 `Model` 冲突时可内部类名 `MlModel`）；`ClassItem` JSON 或子表 |
| 文件 | 卷存 `.pt` / engine；路径写入 `source_file` |
| HTTP | `httpx` → export_trt |
| 鉴权 | session + `has_perm` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| TensorRT 编译 | **复用** export_onnx（pt→onnx）+ export_trt（onnx→engine）；由 `BuildOrchestrator` 编排 |
| 元数据 / 状态机 | **造**（`ModelService` / `BuildOrchestrator`） |
| name 唯一 / batch_size≤128 / precision=fp16 | **造**（校验） |
| Site Config 切片 | **造**（元数据 + classes；**不含**巨大 engine 二进制，或按体积策略只导元数据+pt 路径约定） |
| 响应外壳 / 分页 | **复用** `shared.*` |

切片约定：导出模型行字段与 classes；二进制文件由现场卷保留或按 manifest 相对路径拷贝——实现固定为「元数据进 zip；`.pt`/engine 按相对路径同卷存在则一并打包」。

---

## 目录结构

```
servers/django/pages/models/   # 或 models_page/
  apps.py                      # label=models
  urls.py                      # map、batch 先于 {id}
  models.py                    # MlModel (+ ClassItem JSONField)
  serializers.py
  views.py
  services.py                  # ModelService, BuildOrchestrator, ModelLogService
  clients.py                   # ExportTrtClient
  permissions.py
  migrations/
```

禁止 import `pages.pipelines`（反向：pipelines 只存 `model_id`）。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `MlModel` | 一行模型；`status` 三态 |
| `ModelService` | list / map / create / get / delete / upload / batch_delete |
| `BuildOrchestrator` | start build → `building`；回调/轮询结束写元数据 |
| `ExportTrtClient` | 提交任务、查状态、拉日志 |
| `ModelLogService` | 读缓冲/文件 → `{content}` |

状态机：

| 事件 | 转移 |
|------|------|
| create / upload 覆盖 | → `not_built`；清空 version/task/num_class/classes；作废 engine |
| build 受理 | → `building`；已是 building → `409` |
| build 成功 | → `built`；写 version/task/classes/`last_build_at` |
| build 失败 | → 曾成功过则 `built` 否则 `not_built`；更新 `last_build_at`；**不**覆盖成功元数据 |
| upload 于 building | `409` |
| batch delete 含 building | 跳过，记 `failed_ids` |

---

## 数据库

### `models_mlmodel`（表名随 app）

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | |
| `name` | varchar | UNIQUE | — | |
| `version` | varchar | 可空 | `NULL` | Build 成功写入 |
| `batch_size` | int | 非空 | — | 1…128 |
| `batch_mode` | varchar | 非空 | — | `static`\|`dynamic` |
| `precision` | varchar | 非空 | `fp16` | 仅 fp16 |
| `task` | varchar | 可空 | `NULL` | det/seg/cls |
| `num_class` | int | 可空 | `NULL` | |
| `classes` | JSON | 可空 | `NULL` | `[{id,name}]` |
| `source_file` | varchar | 可空 | `NULL` | 展示路径/文件名 |
| `source_path` | varchar | 可空 | — | 卷内绝对/相对路径（可合并进 source_file） |
| `engine_path` | varchar | 可空 | `NULL` | |
| `status` | varchar | 非空 | `not_built` | |
| `last_build_at` | timestamptz | 可空 | `NULL` | 任务**结束**时写 |
| `had_successful_build` | bool | 非空 | `false` | 失败回退用；或由 version 非空推断 |

索引：`name`；列表 search 可用 `name`/`source_file` icontains。

---

## 各请求写库明细

| 请求 | 写入 |
|------|------|
| `POST /` | INSERT；`status=not_built`；可选存 `.pt` |
| `POST .../upload` | 写文件；UPDATE source；`status=not_built`；清空元数据；删旧 engine |
| `POST .../build` | UPDATE `status=building`；调 export_trt |
| build 回调/轮询结束 | UPDATE status/元数据/`last_build_at` |
| `DELETE` / batch | DELETE 行 + 删文件；building → failed |
| GET / map / status / logs | 只读（status 轮询可附带同步上游态） |

---

## 调用顺序

```
POST build:
  校验 source 存在 → status≠building → ExportTrtClient.submit
  → status=building → {id, status}
轮询:
  ExportTrtClient.status → 若结束则落库 → BuildStatus/BuildResult
upload:
  building? 409 : 写 pt → 回 not_built
```

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| export_onnx + export_trt | Build 必需；任一步不可达 → build 失败落库 |
| pipelines | 只被引用 `model_id`（须 `built`）；本页不 import pipelines |
| Site Config | **注册**模型元数据切片 |
| servers 页 | 运维 restart export_trt；无代码依赖 |
