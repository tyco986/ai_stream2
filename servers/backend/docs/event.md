# Event

前缀：`/backend/events`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。`GET /backend/events/{event_id}/content` 成功时直接返回原文件，不套这层外壳。

事件对象键：`id`、`occurred_at`、`stream_id`、`stream_name`、`media_id`、`media_name`、`pipeline_id`、`pipeline_name`、`event_code`、`level`、`event_label`、`read_at`、`read_by`、`raw_path`、`visualization_path`、`annotations`。`level` 原样保存 DeepStream 消息中的值。路径、`stream_id`、`stream_name`、`media_id`、`media_name`、`pipeline_name`、`read_at`、`read_by` 和 `annotations` 为空时为 `null`。`read_at` 为 `null` 表示未读。`read_by` 为最后一次标已读的用户 id。RTSP 写 `stream_id`，image、video 写 `media_id`，两者只填一个。三个 name 读时分别关联 `stream`、`media`、`pipeline`，源已删除则为 `null`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/events` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/backend/events/{event_id}/content` | path：`event_id`；query：`kind` | 文件字节 | 原文件 |
| `GET` | `/backend/events/{event_id}/content/info` | path：`event_id`；query：`kind` | `width`、`height`、`size`、`created_at`、`updated_at` | MediaInfo 探测结果 |
| `GET` | `/backend/events/ws` | WebSocket 握手 | 文本帧：`{"refresh": true}` 或 `{"action","event"}` | 入库、已读、未读、删除后通知全部已连接客户端 |
| `POST` | `/backend/events/{event_id}/read` | path：`event_id` | 事件对象 | 标已读 |
| `POST` | `/backend/events/{event_id}/unread` | path：`event_id` | 事件对象 | 标未读 |
| `DELETE` | `/backend/events/{event_id}` | path：`event_id` | 事件对象 | 删除 |
| `POST` | `/backend/events/batch/read` | `ids` | `updated_rows`、`failed_rows`、`updated_count`、`failed_count` | 批量标已读 |
| `POST` | `/backend/events/batch/unread` | `ids` | 同上 | 批量标未读 |
| `POST` | `/backend/events/batch/remove` | `ids` | 同上 | 批量删除 |

### GET `/backend/events`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、stream_id、media_id、pipeline_id、event_code、event_label。`level` 为 `alert` 或 `transit` 时只返回该级别；空或 `all` 不过滤。按 `occurred_at` 降序，再按 `id` 降序。

入参（query）：`search`、`level`、`page`、`page_size`。

出参 `data`：`items`（事件对象数组）、`total`、`page`、`page_size`。`message`：`Loaded event list`。失败 `500`，`Failed to load event list`。

### GET `/backend/events/{event_id}/content`

`kind` 只能是 `raw` 或 `visualization`，分别取 `raw_path`、`visualization_path` 指向的磁盘文件，用 `newFileResponse` 原样返回。路径为空或 `kind` 非法时失败。文件字节不入库。

入参：path `event_id`；query `kind`。

失败 `500`，`Failed to load event content`。

### GET `/backend/events/{event_id}/content/info`

`kind` 与 content 相同。用 MediaInfo 读该文件。`width`、`height` 为整数像素，`size` 为 MB、保留一位小数，`created_at`、`updated_at` 是 UTC 的 `YYYY-MM-DDTHH:MM:SSZ`。这些结果不入库。

入参：path `event_id`；query `kind`。

出参 `data`：`width`、`height`、`size`、`created_at`、`updated_at`。`message`：`Loaded event content info`。失败 `500`，`Failed to load event content info`。

### GET `/backend/events/ws`

WebSocket。握手走 `AuthFilter`，读权限与列表相同。请求头没有 `authorization` 时使用 cookie `authorized-token`。入库、已读、未读、删除提交成功后发两种文本帧。`{"refresh": true}` 在连续变更静默 `0.3` 秒后发一帧，供列表和角标重新拉取。横幅帧立刻发，不进入这 `0.3` 秒合并。单条 `action` 为 `insert`、`read`、`unread`、`remove`，`event` 是对象。批量 `action` 为 `batch_read`、`batch_unread`、`batch_remove`，`event` 是成功行的数组；没有成功行不发横幅。`event` 每项只含 `id`、`occurred_at`、`event_label`、`stream_name`、`media_name`、`pipeline_name`、`read_at`、`read_by`。

### POST `/backend/events/{event_id}/read`

`read_at` 写成当前时间，`read_by` 写成当前用户 id。

入参：path `event_id`。

出参 `data`：更新后的事件对象。`message`：`Marked event read`。不存在 `500`，`Failed to mark event read`。

### POST `/backend/events/{event_id}/unread`

`read_at`、`read_by` 写成 `null`。

入参：path `event_id`。

出参 `data`：更新后的事件对象。`message`：`Marked event unread`。不存在 `500`，`Failed to mark event unread`。

### DELETE `/backend/events/{event_id}`

入参：path `event_id`。

出参 `data`：删除前的事件对象。`message`：`Removed event`。不存在 `500`，`Failed to remove event`。

### POST `/backend/events/batch/read`、`/unread`、`/remove`

入参：`ids`（字符串数组）。

逐条成功进 `updated_rows`，失败进 `failed_rows`（`id`）。`message`：`Marked events read`、`Marked events unread`、`Removed events`。

---

## 2. DATA

### `event`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `occurred_at` | TIMESTAMPTZ | 无 | NOT NULL |
| `stream_id` | UUID | `NULL` | 可空；与 `media_id` 恰好一个非空 |
| `media_id` | UUID | `NULL` | 可空；与 `stream_id` 恰好一个非空 |
| `pipeline_id` | UUID | 无 | NOT NULL |
| `event_code` | VARCHAR(64) | 无 | NOT NULL |
| `level` | VARCHAR(16) | 无 | NOT NULL；原样保存消息中的值 |
| `event_label` | VARCHAR(150) | 无 | NOT NULL |
| `read_at` | TIMESTAMPTZ | `NULL` | 可空；`NULL` 为未读 |
| `read_by` | UUID | `NULL` | 可空；REFERENCES `"user"` (`id`) ON DELETE SET NULL；最后一次标已读的用户 |
| `raw_path` | TEXT | `NULL` | 可空 |
| `visualization_path` | TEXT | `NULL` | 可空 |
| `annotations` | JSONB | `NULL` | 可空 |

约束：`event_source_check`，`(stream_id IS NULL) <> (media_id IS NULL)`。索引：`event_occurred_at_id_idx`（`occurred_at`，`id`）、`event_stream_id_idx`（`stream_id`）、`event_media_id_idx`（`media_id`）、`event_pipeline_id_event_code_idx`（`pipeline_id`，`event_code`）、`event_level_occurred_at_id_idx`（`level`，`occurred_at`，`id`）、`event_read_at_idx`（`read_at`）。
