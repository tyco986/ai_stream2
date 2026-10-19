# Pipeline Analyzer

前缀：`/ai_stream2/backend/pipeline/analyzer`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。新建与更新是 multipart，不是 JSON body。

分析器对象键：`id`、`name`、`source_path`、`annotations`、`updated_at`。`source_path` 未上传时为 `null`。`annotations` 为对象。被 `pipeline.analyzer_ids` 引用时不可删。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/pipeline/analyzer` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `POST` | `/ai_stream2/backend/pipeline/analyzer` | multipart：`name`、`annotations`、`file` | 分析器对象 | 新建，图片可省略 |
| `GET` | `/ai_stream2/backend/pipeline/analyzer/{analyzer_id}` | path：`analyzer_id` | 分析器对象 | 详情 |
| `PUT` | `/ai_stream2/backend/pipeline/analyzer/{analyzer_id}` | multipart：`name`、`annotations`、`file` | 分析器对象 | 更新名称、标注，可选换图 |
| `DELETE` | `/ai_stream2/backend/pipeline/analyzer/{analyzer_id}` | path：`analyzer_id` | 分析器对象 | 删除并删图片 |
| `POST` | `/ai_stream2/backend/pipeline/analyzer/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### GET `/ai_stream2/backend/pipeline/analyzer`

`page` 默认 `1`，`page_size` 默认 `20`。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`、`total`、`page`、`page_size`。`message`：`Loaded analyzer list`。失败 `500`，`Failed to load analyzer list`。

### POST `/ai_stream2/backend/pipeline/analyzer`

`annotations` 是 JSON 文本。`file` 可省略。有文件时文件名只允许字母、数字、`.`、`_`、`-`，后缀 `.jpg`、`.jpeg`、`.png`，存到 `/root/analyzers/{id}{后缀}`。`updated_at` 为当前时间。

入参：`name`、`annotations`、`file`。

出参 `data`：分析器对象。`message`：`Created analyzer`。表单或文件非法 `500`，`Failed to create analyzer`。

### GET `/ai_stream2/backend/pipeline/analyzer/{analyzer_id}`

入参：path `analyzer_id`。

出参 `data`：分析器对象。`message`：`Loaded analyzer`。不存在 `500`，`Failed to load analyzer`。

### PUT `/ai_stream2/backend/pipeline/analyzer/{analyzer_id}`

写入 `name`、`annotations`，刷新 `updated_at`。带了 `file` 则替换图片并删旧文件。

入参：同新建。

出参 `data`：分析器对象。`message`：`Updated analyzer`。失败 `500`，`Failed to update analyzer`。

### DELETE `/ai_stream2/backend/pipeline/analyzer/{analyzer_id}`

被管道引用时 `409`，`analyzer is used by a pipeline`。成功时删除 `source_path` 文件。

入参：path `analyzer_id`。

出参 `data`：删除前的分析器对象。`message`：`Removed analyzer`。不存在 `500`，`Failed to remove analyzer`。

### POST `/ai_stream2/backend/pipeline/analyzer/batch/remove`

入参：`ids`。

成功进 `updated_rows`；被占用或不存在进 `failed_rows`（`{id}`）。`message`：`Removed analyzer`。

---

## 2. DATA

### `pipeline_analyzer`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `source_path` | TEXT | `NULL` | 可空 |
| `annotations` | JSONB | 无 | NOT NULL |
| `updated_at` | TIMESTAMPTZ | `NULL` | 可空 |
