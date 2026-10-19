# Model

前缀：`/ai_stream2/backend/models`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。新建与更新是 multipart 表单字段，不是 JSON body。

模型对象键：`id`、`name`、`type`、`version`、`batch_size`、`batch_mode`、`input_tensors`、`output_tensors`、`precision`、`opt_level`、`device`、`workspace`、`static_plugin`、`max_det`、`conf`、`iou`、`num_class`、`classes`、`source_path`、`status`（`not_built` | `building` | `built`）、`built_at`、`trt_version`、`ever_built`。`engine_path` 只在库内，接口不返回。

`type`：`YOLO10-DET`、`YOLO26-SEG`、`YOLO26-DET`、`YOLO26-POSE`、`STGCNPP`、`RTMPOSE`。`batch_mode`：`static` | `dynamic`。`precision`：`fp16`。`batch_size`：1–128。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/models` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `POST` | `/ai_stream2/backend/models` | multipart：`name`、`type`、`batch_size`、`batch_mode`、`precision`、`opt_level`、`device`、`max_det`、`conf`、`iou` | 模型对象 | 新建记录，不含权重 |
| `GET` | `/ai_stream2/backend/models/map` | 无 | `{模型名: id}` | 名称到 id |
| `POST` | `/ai_stream2/backend/models/import` | multipart 字段 `file`（可多个 zip） | 批量结果 | 导入已构建引擎包 |
| `GET` | `/ai_stream2/backend/models/{model_id}` | path：`model_id` | 模型对象 | 详情 |
| `PATCH` | `/ai_stream2/backend/models/{model_id}` | multipart，字段同新建，均可省略 | 模型对象 | 更新；构建中或已构建时不能改构建参数 |
| `DELETE` | `/ai_stream2/backend/models/{model_id}` | path：`model_id` | 模型对象 | 删除记录并删源文件、引擎文件 |
| `POST` | `/ai_stream2/backend/models/{model_id}/upload` | multipart 字段 `input` | 模型对象 | 上传 `.pt` / `.onnx` / `.zip` 源文件 |
| `POST` | `/ai_stream2/backend/models/{model_id}/build` | path：`model_id` | 模型对象 | 经 export_onnx、export_trt 构建引擎 |
| `GET` | `/ai_stream2/backend/models/{model_id}/status` | path：`model_id` | `id`、`status`、`built_at`、`version`、`num_class`、`classes` | 构建状态 |
| `POST` | `/ai_stream2/backend/models/batch/build` | `ids` | 批量结果 | 批量构建 |
| `POST` | `/ai_stream2/backend/models/batch/status` | `ids` | 批量结果 | 批量查状态 |
| `POST` | `/ai_stream2/backend/models/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### GET `/ai_stream2/backend/models`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配列表各列。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`（模型对象数组）、`total`、`page`、`page_size`。`message`：`Loaded models`。失败 `500`，`Failed to load models`。

### POST `/ai_stream2/backend/models`

multipart 表单。必填：`name`、`type`、`batch_size`、`batch_mode`、`precision`。省略时：`device` = `0`，`workspace` = `8192`，`status` = `not_built`，`ever_built` = `false`，`opt_level` / `max_det` / `conf` / `iou` / 张量 / 路径 / 时间为 `null`。非法类型、模式、精度或 batch 时 `500`，`Failed to create model`。表单解析失败 `400`。

入参：`name`、`type`、`batch_size`、`batch_mode`、`precision`、`opt_level`、`device`、`max_det`、`conf`、`iou`。

出参 `data`：模型对象。`message`：`Created model`。

### GET `/ai_stream2/backend/models/map`

入参：无。

出参 `data`：键为模型名，值为 id，按名排序。`message`：`Loaded model map`。失败 `500`，`Failed to load model map`。

### POST `/ai_stream2/backend/models/import`

字段名 `file`，可多个。每个文件须为 zip，内含 `labels.txt`、`meta.json`、且恰好一个 `.engine`。zip 主文件名即模型名。`meta.json` 须含 `type`、`batch_size`、`batch_mode`、`precision`、`device`、`workspace`。同名且引擎文件名与上述字段都相同进 `skipped_rows`。不完整、或同名但字段不同进 `failed_rows`。新模型写入 `engine_path`，`ever_built=true`，`source_path` 为空；`status` 缺省为 `built`。

入参：multipart `file`。

出参 `data`：批量结果。`message`：`Imported models`。无文件 `400`，`Failed to import models`。

### GET `/ai_stream2/backend/models/{model_id}`

入参：path `model_id`。

出参 `data`：模型对象。`message`：`Loaded model`。不存在 `500`，`Failed to load model`。

### PATCH `/ai_stream2/backend/models/{model_id}`

未传字段保持原值。`status` 为 `building` 或 `built` 时，改 `type`、`batch_size`、`batch_mode`、`precision`、`opt_level`、`device`、`max_det`、`conf`、`iou` 返回 `409`。构建中 message 为 `Failed to update model`；已构建 message 为 `Partial modification is allowed for a built model`。只改 `name` 允许。构建参数变化时清空 `version`、`num_class`、`classes`、`built_at`、`engine_path`、`trt_version`。

入参：与新建相同，均可省略。

出参 `data`：模型对象。`message`：`Updated model`。表单失败 `400`。

### DELETE `/ai_stream2/backend/models/{model_id}`

被 `pipeline_gie` 引用时 `409`，`Model is used by a GIE`。`status=building` 时 `409`，`Failed to remove model`。成功时删 `source_path`、`engine_path` 文件。

入参：path `model_id`。

出参 `data`：删除前的模型对象。`message`：`Removed model`。

### POST `/ai_stream2/backend/models/{model_id}/upload`

字段名 `input`。文件名只允许字母、数字、`.`、`_`、`-`，后缀 `.pt`、`.onnx` 或 `.zip`。zip 须通过源包校验。写入 `/root/models/source/`，并更新 `source_path`。构建中 `409`，`model is building`。

入参：path `model_id`；multipart `input`。

出参 `data`：模型对象。`message`：`Uploaded model file`。失败 `400` 或 `500`，`Failed to upload model file`。

### POST `/ai_stream2/backend/models/{model_id}/build`

须已有 `source_path` 且当前不是 `building`，否则 `500`（已在构建时 `409`，`model is building`）。先把 `status` 写成 `building`，再把源文件与 recipe yaml 交给 `POST /ai_stream2/export_onnx/export`，再把 onnx 结果交给 `POST /ai_stream2/export_trt/export`。成功写回张量、类别、`engine_path`、`status=built`、`built_at`、`trt_version`、`ever_built=true`。中途失败会结束构建状态。

入参：path `model_id`。

出参 `data`：构建后的模型对象。`message`：`Built model`。失败 `500`，`Failed to build model`。

### GET `/ai_stream2/backend/models/{model_id}/status`

入参：path `model_id`。

出参 `data`：`id`、`status`、`built_at`、`version`、`num_class`、`classes`。`message`：`Loaded model status`。失败 `500`，`Failed to load model status`。

### POST `/ai_stream2/backend/models/batch/build`

入参：`ids`。

逐条走与单条 build 相同的导出链。成功进 `updated_rows`（模型对象），失败进 `failed_rows`（`{id}`）。`message`：`Built model`。

### POST `/ai_stream2/backend/models/batch/status`

入参：`ids`。

找到的进 `updated_rows`，键同单条 status；找不到进 `failed_rows`（`{id}`）。`message`：`Loaded model status`。

### POST `/ai_stream2/backend/models/batch/remove`

入参：`ids`。

删除成功进 `updated_rows`（模型对象）；被占用、构建中或不存在进 `failed_rows`（`{id}`）。`message`：`Removed model`。

---

## 2. DATA

### `model`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `type` | VARCHAR(16) | 无 | NOT NULL |
| `version` | VARCHAR(64) | `NULL` | 可空 |
| `batch_size` | INT | 无 | NOT NULL |
| `batch_mode` | VARCHAR(16) | 无 | NOT NULL |
| `input_tensors` | JSONB | `NULL` | 可空 |
| `output_tensors` | JSONB | `NULL` | 可空 |
| `precision` | VARCHAR(16) | 无 | NOT NULL |
| `opt_level` | INT | `NULL` | 可空 |
| `device` | INT | 无 | NOT NULL |
| `workspace` | INT | 无 | NOT NULL |
| `static_plugin` | TEXT | `NULL` | 可空 |
| `max_det` | INT | `NULL` | 可空 |
| `conf` | NUMERIC(4, 2) | `NULL` | 可空 |
| `iou` | NUMERIC(4, 2) | `NULL` | 可空 |
| `num_class` | INT | `NULL` | 可空 |
| `classes` | JSONB | `NULL` | 可空 |
| `source_path` | TEXT | `NULL` | 可空 |
| `engine_path` | TEXT | `NULL` | 可空 |
| `status` | VARCHAR(16) | 无 | NOT NULL |
| `built_at` | TIMESTAMPTZ | `NULL` | 可空 |
| `trt_version` | VARCHAR(64) | `NULL` | 可空 |
| `ever_built` | BOOLEAN | 无 | NOT NULL |
