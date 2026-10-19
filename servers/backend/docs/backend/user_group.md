# User Group

前缀：`/ai_stream2/backend/users/groups`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。

组对象键：`id`、`name`、`stream_crud`、`model_crud`、`preview_crud`、`pipeline_crud`、`recording_crud`、`server_crud`、`user_crud`、`event_crud`。每个 CRUD 值为 4 个字符，下标依次是 create、read、update、delete，`1` 表示允许。

admin 组固定 `id` = `00000000-0000-4000-8000-000000000003`。不可改、不可删。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/users/groups` | query：`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `GET` | `/ai_stream2/backend/users/groups/map` | 无 | `{组名: id}` | 组名到 id |
| `POST` | `/ai_stream2/backend/users/groups` | `name` 与八个 `*_crud` | 组对象 | 新建组 |
| `GET` | `/ai_stream2/backend/users/groups/{group_id}` | path：`group_id` | 组对象 | 详情 |
| `PATCH` | `/ai_stream2/backend/users/groups/{group_id}` | `name` 与八个 `*_crud` | 组对象 | 全量更新 |
| `DELETE` | `/ai_stream2/backend/users/groups/{group_id}` | path：`group_id` | 组对象 | 删除组，成员 `group_id` 置空 |
| `GET` | `/ai_stream2/backend/users/groups/{group_id}/members` | path：`group_id` | `{用户名: id}` | 组内用户 |
| `GET` | `/ai_stream2/backend/users/groups/{group_id}/candidates` | path：`group_id` | `{用户名: id}` | 不在该组的用户 |
| `PUT` | `/ai_stream2/backend/users/groups/{group_id}/members` | `user_ids` | `{用户名: id}` | 设置组成员 |

CRUD 键：`stream_crud`、`model_crud`、`preview_crud`、`pipeline_crud`、`recording_crud`、`server_crud`、`user_crud`、`event_crud`。

### GET `/ai_stream2/backend/users/groups`

`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、name 和八个 CRUD 列。按 name 排序。

入参（query）：`search`、`page`、`page_size`。

出参 `data`：`items`（组对象数组）、`total`、`page`、`page_size`。`message`：`Loaded user groups`。失败 `500`，`Failed to load user groups`。

### GET `/ai_stream2/backend/users/groups/map`

入参：无。

出参 `data`：键为组名，值为 id，按组名排序。`message`：`Loaded user group map`。失败 `500`，`Failed to load user group map`。

### POST `/ai_stream2/backend/users/groups`

入参：`name` 与八个 CRUD 键，全部写入。

出参 `data`：组对象。`message`：`Created user group`。失败 `500`，`Failed to create user group`。

### GET `/ai_stream2/backend/users/groups/{group_id}`

入参：path `group_id`。

出参 `data`：组对象。`message`：`Loaded user group`。不存在 `404`，`group not found`。

### PATCH `/ai_stream2/backend/users/groups/{group_id}`

一次写入 `name` 和八个 CRUD 列。admin 组 `409`，`admin group cannot be changed`。

入参：`name` 与八个 CRUD 键。

出参 `data`：组对象。`message`：`Updated user group`。不存在 `404`，`group not found`。

### DELETE `/ai_stream2/backend/users/groups/{group_id}`

先把该组成员的 `group_id` 置为 `NULL`，再删组。admin 组 `409`，`admin group cannot be changed`。

入参：path `group_id`。

出参 `data`：删除前的组对象。`message`：`Deleted user group`。不存在 `404`，`group not found`。

### GET `/ai_stream2/backend/users/groups/{group_id}/members`

入参：path `group_id`。

出参 `data`：键为 username，值为用户 id，按 username 排序。`message`：`Loaded user group members`。组不存在 `404`，`group not found`。

### GET `/ai_stream2/backend/users/groups/{group_id}/candidates`

入参：path `group_id`。

出参 `data`：`group_id` 不是该组的用户（含未分组）。`message`：`Loaded user group candidates`。组不存在 `404`，`group not found`。

### PUT `/ai_stream2/backend/users/groups/{group_id}/members`

`user_ids` 中的用户改入该组；原先在组内但不在列表中的用户 `group_id` 置为 `NULL`。admin 用户必须留在 admin 组：把 admin 移出 admin 组，或把 admin 放进其它组，返回 `409`，`admin user cannot be changed`。

入参：path `group_id`；body `user_ids`（字符串数组，缺省为空数组）。

出参 `data`：更新后的成员映射，键同 members。`message`：`Updated user group members`。组不存在 `404`，`group not found`。

---

## 2. DATA

### `user_group`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `stream_crud` | CHAR(4) | 无 | NOT NULL |
| `model_crud` | CHAR(4) | 无 | NOT NULL |
| `preview_crud` | CHAR(4) | 无 | NOT NULL |
| `pipeline_crud` | CHAR(4) | 无 | NOT NULL |
| `recording_crud` | CHAR(4) | 无 | NOT NULL |
| `server_crud` | CHAR(4) | 无 | NOT NULL |
| `user_crud` | CHAR(4) | 无 | NOT NULL |
| `event_crud` | CHAR(4) | 无 | NOT NULL |

种子行：`id` = `00000000-0000-4000-8000-000000000003`，`name` = `admin`，八列均为 `1111`。触发器拒绝更新或删除该行。
