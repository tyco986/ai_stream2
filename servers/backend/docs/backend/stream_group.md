# Stream Group

前缀：`/ai_stream2/backend/streams/groups`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。`export` 例外，直接返回 multipart。

组对象键：`id`、`name`、`parent_id`。树节点另有 `type`（`group` | `stream`）与 `children`。流叶子键：`id`、`name`、`type`、`enabled`、`status`、`record`。

All 组固定 `id` = `00000000-0000-4000-8000-000000000001`，`parent_id` 为空。不可改名、不可删除、不可改父级。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/streams/groups/tree` | 无 | 根节点 `id`、`name`、`type`、`children` | 组树，子节点含组与直属流 |
| `GET` | `/ai_stream2/backend/streams/groups/map` | 无 | `{组名: id}` | 组名到 id |
| `POST` | `/ai_stream2/backend/streams/groups/import` | multipart 字段 `file`（CSV） | 树（同 tree） | 按 CSV 新建组或改父级 |
| `GET` | `/ai_stream2/backend/streams/groups/export` | 无 | multipart 字段 `file`，文件名 `groups.csv` | 导出 CSV |
| `POST` | `/ai_stream2/backend/streams/groups/{parent_group_id}` | `name` | `id`、`name`、`parent_id` | 在父组下新建 |
| `PATCH` | `/ai_stream2/backend/streams/groups/{group_id}` | `name` | `id`、`name`、`parent_id` | 重命名（不可改 All） |
| `DELETE` | `/ai_stream2/backend/streams/groups/{group_id}` | path：`group_id` | 树 | 删组及子组；其下流归 All |
| `GET` | `/ai_stream2/backend/streams/groups/{group_id}/members` | path：`group_id` | `{流名: id}` | 直属该组的流 |
| `GET` | `/ai_stream2/backend/streams/groups/{group_id}/candidates` | path：`group_id` | `{流名: id}` | 不在该组的流 |
| `PUT` | `/ai_stream2/backend/streams/groups/{group_id}/members` | `stream_ids` | 树 | 设置直属成员；移出的流归 All |

### GET `/ai_stream2/backend/streams/groups/tree`

根为 All。`children` 先放直属流再放子组。组节点：`id`、`name`、`type=group`、`children`。流节点：`id`、`name`、`type=stream`、`enabled`、`status`、`record`。

入参：无。

出参 `data`：根节点。`message`：`Loaded group tree`。失败 `500`，`Failed to load group tree`。

### GET `/ai_stream2/backend/streams/groups/map`

入参：无。

出参 `data`：对象，键为组名，值为组 id，按组名排序。`message`：`Loaded group map`。失败 `500`，`Failed to load group map`。

### POST `/ai_stream2/backend/streams/groups/import`

multipart，字段名 `file`。首行必须是 `group,parent`。父组名须已存在（含本文件中尚未写出、但库里已有的组；新建行的父级须先于该行已在库中）。`All` 的 parent 必须为空，否则 `400`，`cannot move All`。非 All 的 parent 为空或找不到，或会把组挂到自身子树下：`400`，`parent not found`。文件内组名重复：`400`，`group name exists`。列数不对或表头非法：`400`，`invalid csv`。已存在的组只改 `parent_id`；新组插入。

入参：multipart `file`。

出参 `data`：导入后的树。`message`：`Imported groups`。文件无法解析时 `400`，`Failed to import groups`。

### GET `/ai_stream2/backend/streams/groups/export`

`200`，`Content-Type: multipart/form-data; boundary=ai_stream2_group_export`。字段 `file`，文件名 `groups.csv`，正文 `group,parent` 加数据行，按组名排序。All 的 parent 为空。失败 JSON `500`，`Failed to export groups`。

### POST `/ai_stream2/backend/streams/groups/{parent_group_id}`

父组必须存在。

入参：path `parent_group_id`；body `name`。

出参 `data`：`id`、`name`、`parent_id`。`message`：`Created group`。失败 `500`，`Failed to create group`。

### PATCH `/ai_stream2/backend/streams/groups/{group_id}`

只更新 `name`。`parent_id` 为空的 All 不会被更新。

入参：path `group_id`；body `name`。

出参 `data`：`id`、`name`、`parent_id`。`message`：`Renamed group`。失败 `500`，`Failed to rename group`。

### DELETE `/ai_stream2/backend/streams/groups/{group_id}`

删该组及全部子组。这些组下的流先改到 All。All 删除返回 `500`，message 为 `cannot delete All`。

入参：path `group_id`。

出参 `data`：删除后的树。`message`：`Deleted group`。失败 `500`，`Failed to delete group`。

### GET `/ai_stream2/backend/streams/groups/{group_id}/members`

入参：path `group_id`。

出参 `data`：对象，键为流名，值为流 id，仅直属该组，按流名排序。`message`：`Loaded group members`。组不存在 `500`，`Failed to load group members`。

### GET `/ai_stream2/backend/streams/groups/{group_id}/candidates`

入参：path `group_id`。

出参 `data`：对象，键为流名，值为流 id，`group_id` 不是该组的流。`message`：`Loaded group candidates`。组不存在 `500`，`Failed to load group candidates`。

### PUT `/ai_stream2/backend/streams/groups/{group_id}/members`

`stream_ids` 中的流改入该组。原先直属该组但不在列表中的流改到 All。

入参：path `group_id`；body `stream_ids`（字符串数组）。

出参 `data`：更新后的树。`message`：`Updated group members`。失败 `500`，`Failed to update group members`。

---

## 2. DATA

### `stream_group`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `parent_id` | UUID | `NULL` | 可空，REFERENCES `stream_group(id)` ON DELETE SET NULL |

种子行：`id` = `00000000-0000-4000-8000-000000000001`，`name` = `All`，`parent_id` = `NULL`。删除该行会被触发器拒绝。
