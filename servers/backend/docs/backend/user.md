# User

前缀：`/ai_stream2/backend/users`。除 `login` 外全部经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

用户对象键：`id`、`username`、`passwd`、`active`、`group_id`、`group_name`、`last_login`。`group_id` / `group_name` / `last_login` 可空时为 `null`。`token` 只在登录出参中返回，用户对象不含。

admin 用户固定 `id` = `00000000-0000-4000-8000-000000000004`。不可改 `username`、`active`、`group_id`，不可删除。可改 `passwd`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `POST` | `/ai_stream2/backend/users/login` | `username`、`passwd` | `token` | 登录，签发 token 并写 `last_login` |
| `GET` | `/ai_stream2/backend/users` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/ai_stream2/backend/users/map` | 无 | `{用户名: id}` | 用户名到 id |
| `POST` | `/ai_stream2/backend/users` | `username`、`passwd`、`active`、`group_id` | 用户对象 | 新建用户，并建默认预览预设 |
| `GET` | `/ai_stream2/backend/users/{user_id}` | path：`user_id` | 用户对象 | 详情 |
| `PATCH` | `/ai_stream2/backend/users/{user_id}` | `username`、`passwd`、`active`、`group_id` | 用户对象 | 更新 |
| `DELETE` | `/ai_stream2/backend/users/{user_id}` | path：`user_id` | 用户对象 | 删除用户及其预览预设 |
| `POST` | `/ai_stream2/backend/users/batch/remove` | `ids` | 批量结果 | 批量删除 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

### POST `/ai_stream2/backend/users/login`

`active` 为真且用户名、密码匹配时，写入新 `token`（UUID 文本）和 `last_login`。不校验已有 token。

入参：`username`、`passwd`。

出参 `data`：`token`。`message`：`Logged in`。失败 `401`，`invalid username or password`。

### GET `/ai_stream2/backend/users`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、username、active、group_id、组名。按 username 排序。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`（用户对象数组）、`total`、`page`、`page_size`。`message`：`Loaded users`。失败 `500`，`Failed to load users`。

### GET `/ai_stream2/backend/users/map`

入参：无。

出参 `data`：键为 username，值为 id，按 username 排序。`message`：`Loaded user map`。失败 `500`，`Failed to load user map`。

### POST `/ai_stream2/backend/users`

`group_id` 省略或非字符串时写入 `NULL`。同时插入 `preview_preset`：`name=Default`，`layout=2x2`，`mode=grid`，四个空槽，`active=true`。

入参：`username`、`passwd`、`active`、`group_id`。

出参 `data`：用户对象。`message`：`Created user`。失败 `500`，`Failed to create user`。

### GET `/ai_stream2/backend/users/{user_id}`

入参：path `user_id`。

出参 `data`：用户对象。`message`：`Loaded user`。不存在 `404`，`user not found`。

### PATCH `/ai_stream2/backend/users/{user_id}`

`username`、`passwd`、`active` 都会写入。`group_id` 不是字符串时置为 `NULL`。admin 若改用户名、启用状态或组，返回 `409`，`admin user cannot be changed`；只改密码可以。

入参：`username`、`passwd`、`active`、`group_id`。

出参 `data`：用户对象。`message`：`Updated user`。不存在 `404`，`user not found`。

### DELETE `/ai_stream2/backend/users/{user_id}`

先删该用户的 `preview_preset`，再删用户。admin 返回 `409`，`admin user cannot be changed`。

入参：path `user_id`。

出参 `data`：删除前的用户对象。`message`：`Deleted user`。不存在 `404`，`user not found`。

### POST `/ai_stream2/backend/users/batch/remove`

入参：`ids`。

删除成功进 `updated_rows`（用户对象）；admin、不存在或其它失败进 `failed_rows`（`{id}`）。`message`：`Deleted users`。

---

## 2. DATA

### `user`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `username` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `passwd` | VARCHAR(128) | 无 | NOT NULL |
| `token` | VARCHAR(36) | `NULL` | 可空，UNIQUE |
| `active` | BOOLEAN | 无 | NOT NULL |
| `group_id` | UUID | `NULL` | 可空，REFERENCES `user_group(id)` |
| `last_login` | TIMESTAMPTZ | `NULL` | 可空 |

种子行：`id` = `00000000-0000-4000-8000-000000000004`，`username` = `admin`，`passwd` = `admin`，`active` = `TRUE`，`group_id` = admin 组。触发器拒绝删除该行，也拒绝改其 `username`、`active`、`group_id`。
