# Pipeline

前缀：`/backend/pipelines`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

管道对象键：`id`、`name`、`type`、`status`（`stopped` | `starting` | `running`）、`input`、`config_path`、`stream_ids`、`stream_names`、`media_ids`、`media_names`、`gie_ids`、`gie_names`、`analyzer_ids`、`analyzer_names`、`start_time`、`end_time`、`duration`、`updated_at`。未填的时间、时长、`input`、`config_path` 为 `null`。名称数组与 id 数组同序。这些 id 都从 `input` 读出，不单独入库。`pgie`、`sgie`、`rtmpose`、`stgcnpp` 为 gie id。`streams`、`videos`、`images` 是数组，每项为 `{id, analyzer}`：`streams` 的 id 是 stream id，`videos` 和 `images` 的 id 是 media id，`analyzer` 为 analyzer id 或 `null`。前端按管道类型的 `source` 只写其中一个键。

`status=starting` 时不可更新、不可再启动，返回 `409`，`pipeline is running`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/pipelines` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/backend/pipelines/map` | 无 | `{管道名: id}` | 名称到 id |
| `POST` | `/backend/pipelines` | `name`、`type`、`input` | 管道对象 | 生成配置并入库，状态 `stopped` |
| `GET` | `/backend/pipelines/{pipeline_id}` | path：`pipeline_id` | 管道对象 | 详情 |
| `PATCH` | `/backend/pipelines/{pipeline_id}` | `name`、`type`、`input` | 管道对象 | 改名称、类型和 `input` |
| `DELETE` | `/backend/pipelines/{pipeline_id}` | path：`pipeline_id` | 管道对象 | 停容器并删除 |
| `POST` | `/backend/pipelines/{pipeline_id}/start` | path：`pipeline_id` | 管道对象 | 拉起 DeepStream 容器并启动 |
| `POST` | `/backend/pipelines/{pipeline_id}/probe` | path：`pipeline_id` | 管道对象 | 读容器状态写回 |
| `POST` | `/backend/pipelines/{pipeline_id}/stop` | path：`pipeline_id` | 管道对象 | 调 DeepStream `POST /stop` 结束管道进程，状态改为 `stopped` |
| `GET` | `/backend/pipelines/{pipeline_id}/logs/api` | query：`date`，可空 | `{date: {content}}` | 读该管道 `api/` 下的请求日志 |
| `GET` | `/backend/pipelines/{pipeline_id}/logs/pad` | query：`pad`、`date` | `{content}` | 读该管道某一路某一天的帧日志 |
| `GET` | `/backend/pipelines/{pipeline_id}/logs/gstreamer` | query：`date` | `{content}` | 读该管道某一天的 GStreamer 日志 |
| `DELETE` | `/backend/pipelines/{pipeline_id}/logs` | path：`pipeline_id` | `{}` | 调 DeepStream `DELETE /ai_stream2/deepstream/logs` 清空该容器日志 |
| `POST` | `/backend/pipelines/batch/stop` | `ids` | 批量结果 | 批量停止 |
| `POST` | `/backend/pipelines/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### GET `/backend/pipelines`

`page` 默认 `1`，`page_size` 默认 `20`。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`、`total`、`page`、`page_size`。`message`：`Loaded pipelines`。失败 `500`，`Failed to load pipelines`。

### GET `/backend/pipelines/map`

入参：无。

出参 `data`：键为管道名，值为 id。`message`：`Loaded pipeline map`。失败 `500`，`Failed to load pipeline map`。

### POST `/backend/pipelines`

只入库，不调用 generate。`config_path` 为 `null`，`status` 为 `stopped`，`updated_at` 为当前时间。

入参：`name`、`type`、`input`。

出参 `data`：管道对象。`message`：`Created pipeline`。失败 `500`，`Failed to create pipeline`。

### GET `/backend/pipelines/{pipeline_id}`

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Loaded pipeline`。失败 `500`，`Failed to load pipeline`。

### PATCH `/backend/pipelines/{pipeline_id}`

`status=starting` 或容器正在跑时拒绝。否则更新 `name`、`type`、`input`，保留已有 `config_path`，清空 `start_time`、`end_time`、`duration`。

入参：`name`、`type`、`input`。

出参 `data`：管道对象。`message`：`Updated pipeline`。失败 `500`，`Failed to update pipeline`。

### DELETE `/backend/pipelines/{pipeline_id}`

先处理名为 `ai_stream2_{name}` 的容器，再删库。

入参：path `pipeline_id`。

出参 `data`：删除前的管道对象。`message`：`Removed pipeline`。失败 `500`，`Failed to remove pipeline`。

### POST `/backend/pipelines/{pipeline_id}/start`

`starting` 或容器已在跑时 `409`。否则在内存中把 `input` 的 gie id 展开为 `model_zip`、`interval`、`class_attrs`，按 `streams`、`videos`、`images` 里的 id 查 `stream` 或 `media` 拼成 generator 要的来源：流是 `url`、`width`、`height`、`fps`，视频是 `path`、`width`、`height`、`fps`，图片是 `path`、`width`、`height`，把每路 `analyzer` id 收成 `analyzer`，再按本次启动时间写入产物目录：有 filesink 时写 `filesink_output` 为 `/root/media/pipeline/{name}-{时间戳}-{序号}`，有 event 时写 `capturer_output` 为 `/root/events/{name}/{时间戳}-{序号}`。这份 JSON 只发给 generate，不写回 `input`。把 zip 路径写入 `config_path`。随后经 Docker 代理创建或重启容器 `ai_stream2_{name}`（镜像 `ai_stream2_deepstream`，端口 9000）。健康检查通过后，从 `config_path` 的 `params.yml` 取出全部 `model_zip`，以及 `videos`、`images` 里的本地文件。配置 zip（`name=config`）、模型 zip（`name=model`）和媒体 zip（`name=media`，条目为去掉前导 `/` 的原路径；没有本地媒体则不传）放进同一个 multipart 调用 `/start`，再轮询 `/status`，把 `status`、`start_time`、`end_time`、`duration` 写回。日志 steps 增加 `generate`。

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Started pipeline`。失败 `500`，`Failed to start pipeline`。

### POST `/backend/pipelines/{pipeline_id}/probe`

请求 `http://ai_stream2_{name}:9000/status`，写回 `status`、`start_time`、`end_time`、`duration`。

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Probed pipeline`。失败 `500`，`Failed to probe pipeline`。

### POST `/backend/pipelines/{pipeline_id}/stop`

先尽量从容器读回 `start_time`、`duration`，再请求 `http://ai_stream2_{name}:8094/stop` 结束管道进程，容器保持运行，并把状态写成 `stopped`。DeepStream 返回 `409` 时原样 `409`，`pipeline is not running`。

入参：path `pipeline_id`。

出参 `data`：管道对象。`message`：`Stopped pipeline`。其它失败 `500`，`Failed to stop pipeline`。

### GET `/backend/pipelines/{pipeline_id}/logs/api`

`date` 可省略。填写时须为 `YYYY-MM-DD`。文件在 `/root/logs/deepstream/<name>/api/deepstream_YYYY-MM-DD.jsonl`。

入参（query）：`date`。

出参 `data`：日期到 `{content}`。`content` 为该日日志行解析后的对象数组，空行和无法解析的行不包含在内。`latency` 是两位小数字符串。不填 `date` 返回该目录下全部日期；填写时只返回这一天，文件不存在则为 `{}`。`message`：`pipeline_logs_ok`。`date` 非法时 `400`，管道不存在时 `500`，`message` 均为 `pipeline_logs_error`。

### GET `/backend/pipelines/{pipeline_id}/logs/pad`

`pad`、`date` 都必填。`pad` 为十进制路号，`0` 到 `999`，对应目录 `000` 这种三位。`date` 为 `YYYY-MM-DD`。文件在 `/root/logs/deepstream/<name>/pad/<pad>/<page>_YYYY-MM-DD.jsonl`。一个管道只有一种 `page`。

出参 `data`：`{content}`。`content` 为该日日志行解析后的对象数组，空行和无法解析的行不包含在内。`latency` 是两位小数字符串。目录或文件不存在时为 `[]`。`message`：`pipeline_logs_ok`。`pad` 或 `date` 非法时 `400`，管道不存在时 `500`，`message` 均为 `pipeline_logs_error`。

### GET `/backend/pipelines/{pipeline_id}/logs/gstreamer`

`date` 必填，须为 `YYYY-MM-DD`。文件在 `/root/logs/deepstream/<name>/gstreamer/gstreamer_YYYY-MM-DD.log`。

出参 `data`：`{content}`，`content` 为文件文本。文件不存在时为空字符串。`message`：`pipeline_logs_ok`。`date` 非法时 `400`，管道不存在时 `500`，`message` 均为 `pipeline_logs_error`。

### DELETE `/backend/pipelines/{pipeline_id}/logs`

请求 `http://ai_stream2_{name}:8094/ai_stream2/deepstream/logs`。子进程还在时 DeepStream 返回 `409`。已退出时放开接口日志句柄，删除 `/root/logs/deepstream`，再把该目录建回来。

入参：path `pipeline_id`。

出参 `data`：`{}`。`message`：`pipeline_logs_ok`。子进程还在时 `409`，`stop the pipeline before clearing logs`。管道不存在或 DeepStream 未成功时 `500`，`pipeline_logs_error`。

### POST `/backend/pipelines/batch/stop`

入参：`ids`。

成功进 `updated_rows`，失败进 `failed_rows`（`{id}`）。`message`：`Stopped pipelines`。

### POST `/backend/pipelines/batch/remove`

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
| `status` | TEXT | 无 | NOT NULL |
| `input` | JSONB | `NULL` | 可空 |
| `config_path` | TEXT | `NULL` | 可空 |
| `start_time` | TIMESTAMPTZ | `NULL` | 可空 |
| `end_time` | TIMESTAMPTZ | `NULL` | 可空 |
| `duration` | BIGINT | `NULL` | 可空 |
| `updated_at` | TIMESTAMPTZ | `NULL` | 可空 |

无额外索引。
