# Event

前缀：`/backend/events`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

事件对象键：`id`、`occurred_at`、`stream_id`、`stream_name`、`media_id`、`media_name`、`pipeline_id`、`pipeline_name`、`event_code`、`event_label`、`read_at`、`raw_path`、`visualization_path`、`annotations`。路径、`stream_id`、`stream_name`、`media_id`、`media_name`、`pipeline_name`、`read_at` 和 `annotations` 为空时为 `null`。`read_at` 为 `null` 表示未读。RTSP 写 `stream_id`，image、video 写 `media_id`，两者只填一个。三个 name 读时分别关联 `stream`、`media`、`pipeline`，源已删除则为 `null`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/events` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/backend/events/ws` | WebSocket 握手 | 文本帧：事件对象 | 入库成功后把该行推给全部已连接客户端 |
| `POST` | `/backend/events/{event_id}/read` | path：`event_id` | 事件对象 | 标已读，并给关联通知写 `read_at` |
| `POST` | `/backend/events/{event_id}/unread` | path：`event_id` | 事件对象 | 标未读，并清空关联通知的 `read_at` |
| `DELETE` | `/backend/events/{event_id}` | path：`event_id` | 事件对象 | 删除 |
| `POST` | `/backend/events/batch/read` | `ids` | `updated_rows`、`failed_rows`、`updated_count`、`failed_count` | 批量标已读 |
| `POST` | `/backend/events/batch/unread` | `ids` | 同上 | 批量标未读 |
| `POST` | `/backend/events/batch/remove` | `ids` | 同上 | 批量删除 |

### GET `/backend/events`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、stream_id、media_id、pipeline_id、event_code、event_label。按 `occurred_at` 降序，再按 `id` 降序。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`（事件对象数组）、`total`、`page`、`page_size`。`message`：`Loaded event list`。失败 `500`，`Failed to load event list`。

### GET `/backend/events/ws`

WebSocket。握手走 `AuthFilter`，读权限与列表相同。请求头没有 `authorization` 时使用 cookie `authorized-token`。连接期间，Redis 入库成功后的那一行以一条文本帧发出，内容就是事件对象本身。

### POST `/backend/events/{event_id}/read`

`read_at` 写成当前时间。`notice.source=event` 且 `ref_id` 等于该事件、尚未已读的通知同样写入 `read_at`。

入参：path `event_id`。

出参 `data`：更新后的事件对象。`message`：`Marked event read`。不存在 `500`，`Failed to mark event read`。

### POST `/backend/events/{event_id}/unread`

`read_at` 写成 `null`。上述关联通知的 `read_at` 也写成 `null`。

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
| `event_label` | VARCHAR(150) | 无 | NOT NULL |
| `read_at` | TIMESTAMPTZ | `NULL` | 可空；`NULL` 为未读 |
| `raw_path` | TEXT | `NULL` | 可空 |
| `visualization_path` | TEXT | `NULL` | 可空 |
| `annotations` | JSONB | `NULL` | 可空 |

约束：`event_source_check`，`(stream_id IS NULL) <> (media_id IS NULL)`。索引：`event_occurred_at_id_idx`（`occurred_at`，`id`）、`event_stream_id_idx`（`stream_id`）、`event_media_id_idx`（`media_id`）、`event_pipeline_id_event_code_idx`（`pipeline_id`，`event_code`）、`event_read_at_idx`（`read_at`）。
