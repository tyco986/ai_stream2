# Pipeline Analyzer

前缀：`/backend/pipeline/analyzer`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

分析器对象键：`id`、`name`、`media_id`、`annotations`、`updated_at`。`media_id` 未指定时为 `null`。`annotations` 为对象。被 `pipeline.analyzer_ids` 引用时不可删。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/pipeline/analyzer` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `POST` | `/backend/pipeline/analyzer` | `name`、`annotations`、`media_id` | 分析器对象 | 新建，`media_id` 可省略 |
| `GET` | `/backend/pipeline/analyzer/{analyzer_id}` | path：`analyzer_id` | 分析器对象 | 详情 |
| `PUT` | `/backend/pipeline/analyzer/{analyzer_id}` | `name`、`annotations`、`media_id` | 分析器对象 | 更新名称、标注；`media_id` 为空则保持原值 |
| `DELETE` | `/backend/pipeline/analyzer/{analyzer_id}` | path：`analyzer_id` | 分析器对象 | 删除并删图片 |
| `POST` | `/backend/pipeline/analyzer/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### GET `/backend/pipeline/analyzer`

`page` 默认 `1`，`page_size` 默认 `20`。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`、`total`、`page`、`page_size`。`message`：`Loaded analyzer list`。失败 `500`，`Failed to load analyzer list`。

### POST `/backend/pipeline/analyzer`

`annotations` 是 JSON 对象。`media_id` 可省略，省略或空字符串写入 `null`，须指向已有 `media.id`。`updated_at` 为当前时间。

入参：`name`、`annotations`、`media_id`。

出参 `data`：分析器对象。`message`：`Created analyzer`。失败 `500`，`Failed to create analyzer`。

### GET `/backend/pipeline/analyzer/{analyzer_id}`

入参：path `analyzer_id`。

出参 `data`：分析器对象。`message`：`Loaded analyzer`。不存在 `500`，`Failed to load analyzer`。

### PUT `/backend/pipeline/analyzer/{analyzer_id}`

写入 `name`、`annotations`，刷新 `updated_at`。`media_id` 非空时改指向，空则保持原值。不删除媒体文件。

入参：同新建。

出参 `data`：分析器对象。`message`：`Updated analyzer`。失败 `500`，`Failed to update analyzer`。

### DELETE `/backend/pipeline/analyzer/{analyzer_id}`

被管道引用时 `409`，`analyzer is used by a pipeline`。不删除媒体文件。

入参：path `analyzer_id`。

出参 `data`：删除前的分析器对象。`message`：`Removed analyzer`。不存在 `500`，`Failed to remove analyzer`。

### POST `/backend/pipeline/analyzer/batch/remove`

入参：`ids`。

成功进 `updated_rows`；被占用或不存在进 `failed_rows`（`{id}`）。`message`：`Removed analyzer`。

---

## 2. DATA

### `pipeline_analyzer`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `media_id` | UUID | `NULL` | 可空，REFERENCES `media(id)` |
| `annotations` | JSONB | 无 | NOT NULL |
| `updated_at` | TIMESTAMPTZ | `NULL` | 可空 |
