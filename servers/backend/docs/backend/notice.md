# Notice

前缀：`/ai_stream2/backend/notices`。全部接口经 `AuthFilter`。当前用户由请求头 `authorization` 的 token 解析。

JSON 成功外壳：`{success, message, data}`。

通知对象键：`id`、`created_at`、`read_at`、`user_id`、`level`、`source`、`text`、`ref_id`。`read_at`、`user_id`、`ref_id` 为空时为 `null`。`user_id` 为空表示全员可见。

列表和未读数只含当前用户的通知，以及 `user_id` 为空的通知。标记已读和删除按 `id` 操作，不校验归属。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/notices` | query：`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/ai_stream2/backend/notices/unread` | 无 | `total` | 未读条数 |
| `POST` | `/ai_stream2/backend/notices/{notice_id}/read_at` | `read_at` | 通知对象 | 写入已读时间 |
| `DELETE` | `/ai_stream2/backend/notices/{notice_id}` | path：`notice_id` | 通知对象 | 删除 |

### GET `/ai_stream2/backend/notices`

`page` 默认 `1`，`page_size` 默认 `20`。按 `created_at` 降序，再按 `id` 降序。

入参（query）：`page`、`page_size`。

出参 `data`：`items`（通知对象数组）、`total`、`page`、`page_size`。`message`：`Loaded notice list`。失败 `500`，`Failed to load notice list`。

### GET `/ai_stream2/backend/notices/unread`

统计 `read_at` 为空、且属于当前用户或 `user_id` 为空的行。

入参：无。

出参 `data`：`total`。`message`：`Loaded unread notice count`。失败 `500`，`Failed to load unread notice count`。

### POST `/ai_stream2/backend/notices/{notice_id}/read_at`

入参：path `notice_id`；body `read_at`（时间戳文本）。

出参 `data`：更新后的通知对象。`message`：`Updated notice read time`。不存在 `500`，`Failed to update notice read time`。

### DELETE `/ai_stream2/backend/notices/{notice_id}`

入参：path `notice_id`。

出参 `data`：删除前的通知对象。`message`：`Removed notice`。不存在 `500`，`Failed to remove notice`。

---

## 2. DATA

### `notice`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `created_at` | TIMESTAMPTZ | 无 | NOT NULL |
| `read_at` | TIMESTAMPTZ | `NULL` | 可空 |
| `user_id` | UUID | `NULL` | 可空，REFERENCES `"user"(id)` ON DELETE CASCADE |
| `level` | VARCHAR(16) | 无 | NOT NULL |
| `source` | VARCHAR(64) | 无 | NOT NULL |
| `text` | TEXT | 无 | NOT NULL |
| `ref_id` | UUID | `NULL` | 可空 |

索引：`notice_user_id_created_at_idx`（`user_id`，`created_at`）。
