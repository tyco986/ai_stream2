# Pipeline

前缀：`/ai_stream2/backend/pipelines`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

管道对象键：`id`、`name`、`type`、`status`（`stopped` | `starting` | `running`）、`input`、`config_path`、`stream_ids`、`stream_names`、`gie_ids`、`gie_names`、`analyzer_ids`、`analyzer_names`、`start_time`、`end_time`、`duration`、`updated_at`。未填的时间、时长、`input`、`config_path` 为 `null`。名称数组与 id 数组同序。

`status=starting` 时不可更新、不可再启动，返回 `409`，`pipeline is running`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/pipelines` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/ai_stream2/backend/pipelines/map` | 无 | `{管道名: id}` | 名称到 id |
| `POST` | `/ai_stream2/backend/pipelines` | `name`、`type`、`input`、`stream_ids`、`gie_ids`、`analyzer_ids` | 管道对象 | 生成配置并入库，状态 `stopped` |
| `GET` | `/ai_stream2/backend/pipelines/{pipeline_id}` | path：`pipeline_id` | 管道对象 | 详情 |
| `PATCH` | `/ai_stream2/backend/pipelines/{pipeline_id}` | `name`、`type`、`input`、`gie_ids`、`analyzer_ids` | 管道对象 | 重新生成配置；不改 `stream_ids` |
| `DELETE` | `/ai_stream2/backend/pipelines/{pipeline_id}` | path：`pipeline_id` | 管道对象 | 停容器并删除 |
| `POST` | `/ai_stream2/backend/pipelines/{pipeline_id}/start` | path：`pipeline_id` | 管道对象 | 拉起 DeepStream 容器并启动 |
| `POST` | `/ai_stream2/backend/pipelines/{pipeline_id}/probe` | path：`pipeline_id` | 管道对象 | 读容器状态写回 |
| `POST` | `/ai_stream2/backend/pipelines/{pipeline_id}/stop` | path：`pipeline_id` | 管道对象 | 停容器，状态改为 `stopped` |
| `POST` | `/ai_stream2/backend/pipelines/batch/stop` | `ids` | 批量结果 | 批量停止 |
| `POST` | `/ai_stream2/backend/pipelines/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### GET `/ai_stream2/backend/pipelines`

`page` 默认 `1`，`page_size` 默认 `20`。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`、`total`、`page`、`page_size`。`message`：`Loaded pipelines`。失败 `500`，`Failed to load pipelines`。

### GET `/ai_stream2/backend/pipelines/map`

入参：无。

出参 `data`：键为管道名，值为 id。`message`：`Loaded pipeline map`。失败 `500`，`Failed to load pipeline map`。

### POST `/ai_stream2/backend/pipelines`

把 `input` 交给 `POST /ai_stream2/generator/generate`，得到 `config_path` 后入库。`status` 为 `stopped`，`updated_at` 为当前时间。

入参：`name`、`type`、`input`、`stream_ids`、`gie_ids`、`analyzer_ids`。

出参 `data`：管道对象。`message`：`Created pipeline`。失败 `500`，`Failed to create pipeline`。

### GET `/ai_stream2/backend/pipelines/{pipeline_id}`

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Loaded pipeline`。失败 `500`，`Failed to load pipeline`。

### PATCH `/ai_stream2/backend/pipelines/{pipeline_id}`

`status=starting` 时拒绝。否则重新生成配置，更新 `name`、`type`、`input`、`config_path`、`gie_ids`、`analyzer_ids`，清空 `start_time`、`end_time`、`duration`。`stream_ids` 保持原值。

入参：`name`、`type`、`input`、`gie_ids`、`analyzer_ids`。

出参 `data`：管道对象。`message`：`Updated pipeline`。失败 `500`，`Failed to update pipeline`。

### DELETE `/ai_stream2/backend/pipelines/{pipeline_id}`

先处理名为 `puskas_{name}` 的容器，再删库。

入参：path `pipeline_id`。

出参 `data`：删除前的管道对象。`message`：`Removed pipeline`。失败 `500`，`Failed to remove pipeline`。

### POST `/ai_stream2/backend/pipelines/{pipeline_id}/start`

`starting` 或容器已在跑时 `409`。否则创建或重启容器 `puskas_{name}`（镜像 `puskas_deepstream`，端口 9000），健康检查通过后调用 `/start`，轮询 `/status`，把 `status`、`start_time`、`end_time`、`duration` 写回。

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Started pipeline`。失败 `500`，`Failed to start pipeline`。

### POST `/ai_stream2/backend/pipelines/{pipeline_id}/probe`

请求 `http://puskas_{name}:9000/status`，写回 `status`、`start_time`、`end_time`、`duration`。

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Probed pipeline`。失败 `500`，`Failed to probe pipeline`。

### POST `/ai_stream2/backend/pipelines/{pipeline_id}/stop`

先尽量从容器读回 `start_time`、`duration`，再停容器，并把状态写成 `stopped`。

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Stopped pipeline`。失败 `500`，`Failed to stop pipeline`。

### POST `/ai_stream2/backend/pipelines/batch/stop`

入参：`ids`。

成功进 `updated_rows`，失败进 `failed_rows`（`{id}`）。`message`：`Stopped pipelines`。

### POST `/ai_stream2/backend/pipelines/batch/remove`

入参：`ids`。

成功进 `updated_rows`，失败进 `failed_rows`（`{id}`）。`message`：`Removed pipelines`。

---

## 2. DATA

### `pipeline`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `type` | VARCHAR(64) | 无 | NOT NULL |
| `status` | VARCHAR(16) | 无 | NOT NULL |
| `input` | JSONB | `NULL` | 可空 |
| `config_path` | TEXT | `NULL` | 可空 |
| `stream_ids` | UUID[] | 无 | NOT NULL |
| `gie_ids` | UUID[] | 无 | NOT NULL |
| `analyzer_ids` | UUID[] | 无 | NOT NULL |
| `start_time` | TIMESTAMPTZ | `NULL` | 可空 |
| `end_time` | TIMESTAMPTZ | `NULL` | 可空 |
| `duration` | BIGINT | `NULL` | 可空 |
| `updated_at` | TIMESTAMPTZ | `NULL` | 可空 |

GIN 索引：`pipeline_stream_ids_idx`、`pipeline_gie_ids_idx`、`pipeline_analyzer_ids_idx`。
