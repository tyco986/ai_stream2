# Preview

前缀：`/ai_stream2/backend/preview`。全部接口经 `AuthFilter`。当前用户由请求头 `authorization` 的 token 解析。

JSON 成功外壳：`{success, message, data}`。

预设对象键：`id`、`user_id`、`name`、`layout`、`mode`、`slots`、`active`。`layout` 为 `1x1`、`2x2`、`3x3`、`4x4`，槽位数依次为 1、4、9、16。`slots` 的键是从 `"1"` 起的槽号。入库存流 id 或 `null`；出参把能找到的流展开为对象，否则为 `null`。流对象键：`id`、`name`、`group_id`、`group_name`、`url`、`width`、`height`、`fps`、`status`、`mount`、`record`、`probed_at`。

同一用户同时只有一条 `active=true`。名为 `Default` 的预设不可删。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/preview/map` | 无 | `{预设名: id}` | 当前用户的预设名到 id |
| `GET` | `/ai_stream2/backend/preview/tree` | 无 | 根节点 `id`、`name`、`type`、`children` | 流组树，供选槽 |
| `GET` | `/ai_stream2/backend/preview/active` | 无 | 预设对象 | 当前用户的激活预设 |
| `GET` | `/ai_stream2/backend/preview/{preset_id}` | path：`preset_id` | 预设对象 | 当前用户的一条预设 |
| `POST` | `/ai_stream2/backend/preview` | `name`、`layout`、`mode`、`slots`、`active` | 预设对象 | 新建预设 |
| `PATCH` | `/ai_stream2/backend/preview/{preset_id}` | 同新建 | 预设对象 | 全量更新 |
| `DELETE` | `/ai_stream2/backend/preview/{preset_id}` | path：`preset_id` | 预设对象 | 删除，不可删 Default |

### GET `/ai_stream2/backend/preview/map`

入参：无。

出参 `data`：键为预设名，值为 id，只含当前用户，按名排序。`message`：`Loaded preset map`。失败 `500`，`Failed to load preset map`。

### GET `/ai_stream2/backend/preview/tree`

根为 All。`children` 先放直属流再放子组。组节点：`id`、`name`、`type=group`、`children`。流节点：`id`、`name`、`type=stream`、`mount`、`status`、`record`。

入参：无。

出参 `data`：根节点。`message`：`Loaded preview tree`。失败 `500`，`Failed to load preview tree`。

### GET `/ai_stream2/backend/preview/active`

入参：无。

出参 `data`：当前用户 `active=true` 的预设，`slots` 已展开。`message`：`Loaded active preset`。没有激活预设 `500`，`Failed to load active preset`。

### GET `/ai_stream2/backend/preview/{preset_id}`

只返回属于当前用户的预设。

入参：path `preset_id`。

出参 `data`：预设对象。`message`：`Loaded preset`。不存在或不属于当前用户 `500`，`Failed to load preset`。

### POST `/ai_stream2/backend/preview`

`user_id` 取当前用户。`slots` 必须是对象，键恰好为 `"1"` 到槽位数，值为流 id 字符串或 `null`。`active=true` 时先把该用户其它预设的 `active` 置为 `false`。

入参：`name`、`layout`、`mode`、`slots`、`active`。

出参 `data`：预设对象。`message`：`Created preset`。布局或槽非法 `500`，`Failed to create preset`。

### PATCH `/ai_stream2/backend/preview/{preset_id}`

一次写入 `name`、`layout`、`mode`、`slots`、`active`。槽校验同新建。`active=true` 时取消该用户其它预设的激活。只改当前用户自己的行。

入参：同新建。

出参 `data`：预设对象。`message`：`Updated preset`。失败 `500`，`Failed to update preset`。

### DELETE `/ai_stream2/backend/preview/{preset_id}`

`name=Default` 不可删。只删当前用户的预设。

入参：path `preset_id`。

出参 `data`：删除前的预设对象。`message`：`Removed preset`。失败 `500`，`Failed to remove preset`。

---

## 2. DATA

### `preview_preset`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `user_id` | UUID | 无 | NOT NULL，REFERENCES `"user"(id)` |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE (`user_id`, `name`) |
| `layout` | VARCHAR(16) | 无 | NOT NULL |
| `mode` | VARCHAR(16) | 无 | NOT NULL |
| `slots` | JSONB | 无 | NOT NULL |
| `active` | BOOLEAN | 无 | NOT NULL |

部分唯一索引 `preview_preset_one_active_idx`：每个 `user_id` 至多一行 `active=true`。新建用户时插入 `name=Default`、`layout=2x2`、`mode=grid`、`slots={"1":null,"2":null,"3":null,"4":null}`、`active=true`。
