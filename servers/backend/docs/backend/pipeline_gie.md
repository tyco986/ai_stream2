# Pipeline GIE

前缀：`/ai_stream2/backend/pipeline/gie`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

GIE 对象键：`id`、`name`、`model_id`、`model_name`、`class_attrs`、`interval`、`engine_path`、`updated_at`。`engine_path` 来自关联模型，不在本表。被 `pipeline.gie_ids` 引用时不可删。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/pipeline/gie` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/ai_stream2/backend/pipeline/gie/models` | 无 | `{模型名: {id, status}}` | 可选模型 |
| `GET` | `/ai_stream2/backend/pipeline/gie/map` | 无 | `{GIE 名: id}` | 名称到 id |
| `POST` | `/ai_stream2/backend/pipeline/gie` | `model_id`、`name`、`class_attrs`、`interval` | GIE 对象 | 新建 |
| `GET` | `/ai_stream2/backend/pipeline/gie/{gie_id}` | path：`gie_id` | GIE 对象 | 详情 |
| `PUT` | `/ai_stream2/backend/pipeline/gie/{gie_id}` | `model_id`、`name`、`class_attrs`、`interval` | GIE 对象 | 全量更新 |
| `DELETE` | `/ai_stream2/backend/pipeline/gie/{gie_id}` | path：`gie_id` | GIE 对象 | 删除 |
| `POST` | `/ai_stream2/backend/pipeline/gie/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### GET `/ai_stream2/backend/pipeline/gie`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、name、model_name、class_attrs、interval、engine_path。按 name 排序。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`、`total`、`page`、`page_size`。`message`：`Loaded gie list`。失败 `500`，`Failed to load gie list`。

### GET `/ai_stream2/backend/pipeline/gie/models`

入参：无。

出参 `data`：键为模型名，值为 `{id, status}`，按模型名排序。`message`：`Loaded models`。失败 `500`，`Failed to load models`。

### GET `/ai_stream2/backend/pipeline/gie/map`

入参：无。

出参 `data`：键为 GIE 名，值为 id。`message`：`Loaded gie map`。失败 `500`，`Failed to load gie map`。

### POST `/ai_stream2/backend/pipeline/gie`

`model_id` 必须存在。`model_name` 写入当时的模型名，`updated_at` 为当前时间。

入参：`model_id`、`name`、`class_attrs`、`interval`。

出参 `data`：GIE 对象。`message`：`Created gie`。模型不存在或其它失败 `500`，`Failed to create gie`。

### GET `/ai_stream2/backend/pipeline/gie/{gie_id}`

入参：path `gie_id`。

出参 `data`：GIE 对象。`message`：`Loaded gie`。不存在 `500`，`Failed to load gie`。

### PUT `/ai_stream2/backend/pipeline/gie/{gie_id}`

一次写入 `model_id`、`name`、`class_attrs`、`interval`，并刷新 `model_name`、`updated_at`。

入参：同新建。

出参 `data`：GIE 对象。`message`：`Updated gie`。失败 `500`，`Failed to update gie`。

### DELETE `/ai_stream2/backend/pipeline/gie/{gie_id}`

被管道引用时 `409`，`gie is used by a pipeline`。

入参：path `gie_id`。

出参 `data`：删除前的 GIE 对象。`message`：`Removed gie`。不存在 `500`，`Failed to remove gie`。

### POST `/ai_stream2/backend/pipeline/gie/batch/remove`

入参：`ids`。

成功进 `updated_rows`；被占用或不存在进 `failed_rows`（`{id}`）。`message`：`Removed gie`。

---

## 2. DATA

### `pipeline_gie`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `model_id` | UUID | 无 | NOT NULL |
| `model_name` | VARCHAR(150) | `NULL` | 可空 |
| `class_attrs` | JSONB | `NULL` | 可空 |
| `interval` | INT | 无 | NOT NULL |
| `updated_at` | TIMESTAMPTZ | `NULL` | 可空 |
