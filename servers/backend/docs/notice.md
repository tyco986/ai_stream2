# Notice

前缀：`/backend/notices`。全部接口经 `AuthFilter`。当前用户由请求头 `authorization` 的 token 解析。

JSON 成功外壳：`{success, message, data}`。

通知对象键：`id`、`created_at`、`read_at`、`level`、`source`、`text`、`ref_id`。`read_at`、`ref_id` 为空时为 `null`。通知全员可见。`read_at` 为 `null` 表示未读。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/notices` | query：`level`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `POST` | `/backend/notices` | `created_at`、`level`、`source`、`text`、`ref_id` | 通知对象 | 新建 |
| `GET` | `/backend/notices/count` | 无 | `total`、`event`、`system` | 未读数量 |
| `POST` | `/backend/notices/{notice_id}/read` | path：`notice_id` | 通知对象 | 标已读 |
| `POST` | `/backend/notices/{notice_id}/unread` | path：`notice_id` | 通知对象 | 标未读 |
| `DELETE` | `/backend/notices/{notice_id}` | path：`notice_id` | 通知对象 | 删除 |

### GET `/backend/notices`

`page` 默认 `1`，`page_size` 默认 `20`。`level` 为空返回全部；只接受 `system`。按 `created_at` 降序，再按 `id` 降序。

入参（query）：`level`、`page`、`page_size`。

出参 `data`：`items`（通知对象数组）、`total`、`page`、`page_size`。`message`：`Loaded notice list`。失败 `500`，`Failed to load notice list`。`level` 非法同样失败。

### POST `/backend/notices`

`level` 只能是 `system`。`created_at` 可省略，省略时用当前时间。`ref_id` 可省略，省略时为 `null`。`read_at` 为 `null`。

入参：`created_at`、`level`、`source`、`text`、`ref_id`。

出参 `data`：通知对象。`message`：`Created notice`。失败 `500`，`Failed to create notice`。`level` 非法同样失败。

### GET `/backend/notices/count`

`system` 统计 `notice` 中 `read_at` 为空且 `level` 为 `system` 的行。`event` 统计 `event` 中 `read_at` 为空且 `level` 为 `alert` 的行。`total` 是两者之和。

入参：无。

出参 `data`：`total`、`event`、`system`。`message`：`Loaded notice counts`。失败 `500`，`Failed to load notice counts`。

### POST `/backend/notices/{notice_id}/read`

`read_at` 写成当前时间。

入参：path `notice_id`。

出参 `data`：更新后的通知对象。`message`：`Marked notice read`。不存在 `500`，`Failed to mark notice read`。

### POST `/backend/notices/{notice_id}/unread`

`read_at` 写成 `null`。

入参：path `notice_id`。

出参 `data`：更新后的通知对象。`message`：`Marked notice unread`。不存在 `500`，`Failed to mark notice unread`。

### DELETE `/backend/notices/{notice_id}`

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
| `level` | VARCHAR(16) | 无 | NOT NULL |
| `source` | VARCHAR(64) | 无 | NOT NULL |
| `text` | TEXT | 无 | NOT NULL |
| `ref_id` | UUID | `NULL` | 可空 |

索引：`notice_created_at_id_idx`（`created_at`，`id`）、`notice_unread_idx`（`created_at`，`WHERE read_at IS NULL`）。
