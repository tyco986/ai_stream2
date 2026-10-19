# Event

前缀：`/ai_stream2/backend/events`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

事件对象键：`id`、`occurred_at`、`stream_id`、`stream_name`、`pipeline_id`、`pipeline_name`、`event_code`、`event_label`、`status`（`new` | `handled`）、`raw_path`、`visualization_path`、`annotations`。路径和 `annotations` 为空时为 `null`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/events` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `POST` | `/ai_stream2/backend/events/{event_id}/status` | `status` | 事件对象 | 改状态；`handled` 时把关联通知标为已读 |

### GET `/ai_stream2/backend/events`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、stream_id、stream_name、pipeline_id、pipeline_name、event_code、event_label、status。按 `occurred_at` 降序，再按 `id` 降序。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`（事件对象数组）、`total`、`page`、`page_size`。`message`：`Loaded event list`。失败 `500`，`Failed to load event list`。

### POST `/ai_stream2/backend/events/{event_id}/status`

写入 `status`。值为 `handled` 时，把 `notice.source=event` 且 `ref_id` 等于该事件、尚未已读的通知的 `read_at` 写成当前时间。

入参：path `event_id`；body `status`。

出参 `data`：更新后的事件对象。`message`：`Updated event status`。不存在 `500`，`Failed to update event status`。

---

## 2. DATA

### `event`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `occurred_at` | TIMESTAMPTZ | 无 | NOT NULL |
| `stream_id` | UUID | 无 | NOT NULL |
| `stream_name` | VARCHAR(150) | 无 | NOT NULL |
| `pipeline_id` | UUID | 无 | NOT NULL |
| `pipeline_name` | VARCHAR(150) | 无 | NOT NULL |
| `event_code` | VARCHAR(64) | 无 | NOT NULL |
| `event_label` | VARCHAR(150) | 无 | NOT NULL |
| `status` | VARCHAR(16) | 无 | NOT NULL |
| `raw_path` | TEXT | `NULL` | 可空 |
| `visualization_path` | TEXT | `NULL` | 可空 |
| `annotations` | JSONB | `NULL` | 可空 |

索引：`event_occurred_at_id_idx`（`occurred_at`，`id`）、`event_stream_id_idx`（`stream_id`）、`event_pipeline_id_event_code_idx`（`pipeline_id`，`event_code`）、`event_status_idx`（`status`）。
