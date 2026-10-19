# Stream

前缀：`/ai_stream2/backend/streams`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。`export` 例外，直接返回 multipart。

流对象键：`id`、`name`、`group_id`、`group_name`、`url`、`width`、`height`、`fps`、`status`（`online` | `offline`）、`enabled`、`record`、`probed_at`。`width` / `height` / `fps` / `probed_at` 未探测时为 `null`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/streams` | query：`group_id`、`stream_id`、`search`、`page`、`page_size` | `items`、`total`、`page`、`page_size` | 分页列表 |
| `POST` | `/ai_stream2/backend/streams` | `name`、`group_id`、`url`、`enabled`、`record`、`status`、`width`、`height`、`fps`、`probed_at` | 流对象 | 新建；`enabled` 为真时挂 MediaMTX |
| `GET` | `/ai_stream2/backend/streams/{stream_id}` | path：`stream_id` | 流对象 | 详情 |
| `PATCH` | `/ai_stream2/backend/streams/{stream_id}` | 同新建（均可省略） | 流对象 | 更新；按 `enabled` / 改名 / `url` / `record` 挂或卸 MediaMTX |
| `DELETE` | `/ai_stream2/backend/streams/{stream_id}` | path：`stream_id` | 流对象 | 删除；已挂载则先卸 path |
| `POST` | `/ai_stream2/backend/streams/{stream_id}/probe` | path：`stream_id` | 流对象 | ffmpeg 探测并写回宽高、fps、status、`probed_at` |
| `POST` | `/ai_stream2/backend/streams/import` | multipart 字段 `file`（CSV） | `updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count` | 按 CSV 新建或改组 |
| `GET` | `/ai_stream2/backend/streams/export` | 无 | multipart 字段 `file`，文件名 `streams.csv` | 导出 CSV |
| `POST` | `/ai_stream2/backend/streams/batch/remove` | `ids` | 批量结果 | 批量删除并卸 path |
| `POST` | `/ai_stream2/backend/streams/batch/enable` | `ids` | 批量结果 | 批量启用并挂 path |
| `POST` | `/ai_stream2/backend/streams/batch/disable` | `ids` | 批量结果 | 批量停用并卸 path |
| `POST` | `/ai_stream2/backend/streams/batch/record` | `ids` | 批量结果 | 仅已挂载流开录 |
| `POST` | `/ai_stream2/backend/streams/batch/unrecord` | `ids` | 批量结果 | 批量停录 |
| `POST` | `/ai_stream2/backend/streams/batch/probe` | `ids` | 批量结果 | 批量探测 |

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`（流对象或仅 `{id}`）、`updated_count`、`failed_count`、`skipped_count`。

### GET `/ai_stream2/backend/streams`

按组子树分页。`group_id` 空则用 All（`00000000-0000-4000-8000-000000000001`）。`stream_id` 非空时只查该流，不再按组。`page` 默认 `1`，`page_size` 默认 `20`。`search` 匹配 id、name、组、url、宽高、fps、status、enabled、record、probed_at。

入参（query）：`group_id`、`stream_id`、`search`、`page`、`page_size`。

出参 `data`：`items`（流对象数组）、`total`、`page`、`page_size`。`message`：`Loaded streams`。

### POST `/ai_stream2/backend/streams`

省略时：`group_id` = All，`enabled` = `true`，`record` = `false`，`status` = `offline`，宽高 / fps / `probed_at` = `null`。`enabled` 为真时先 `MediaMTX replacePath`（path = `name`，source = `url`，record = `record`），成功后再入库。

入参：`name`、`group_id`、`url`、`enabled`、`record`、`status`、`width`、`height`、`fps`、`probed_at`。

出参 `data`：流对象。`message`：`Created stream`。失败 `500`，`Failed to create stream`。

### GET `/ai_stream2/backend/streams/{stream_id}`

入参：path `stream_id`。

出参 `data`：流对象。`message`：`Loaded stream`。不存在 `500`，`Failed to load stream`。

### PATCH `/ai_stream2/backend/streams/{stream_id}`

未传字段保持原值。已挂载且将停用或改名时先卸旧 path；目标为启用且未挂载或 name / url / record 变化时再挂新 path。

入参：与新建相同，字段均可省略。

出参 `data`：更新后的流对象。`message`：`Updated stream`。失败 `500`，`Failed to update stream`。

### DELETE `/ai_stream2/backend/streams/{stream_id}`

已挂载则先卸 path，再删库。

入参：path `stream_id`。

出参 `data`：删除前的流对象。`message`：`Deleted stream`。失败 `500`，`Failed to delete stream`。

### POST `/ai_stream2/backend/streams/{stream_id}/probe`

用库内 `url` 调 ffmpeg。探测被接受后写入 `status`、`width`、`height`、`fps`、`probed_at`。

入参：path `stream_id`。

出参 `data`：探测后的流对象。`message`：`Probed stream`。失败 `500`，`Failed to probe stream`。

### POST `/ai_stream2/backend/streams/import`

multipart，字段名 `file`。首行必须是 `name,url,group`。组名须已存在。同名且同 url 只改 `group_id`；name 与 url 都是新的则插入，`status=offline`、`enabled=false`、`record=false`。完全未变进 `skipped_rows`。组不存在、name/url 交叉冲突、列数不对进 `failed_rows`（`name`、`url`、`group_name`）。文件无法解析时 `400`，`Failed to import streams`。CSV 头非法 `400`，`invalid csv`。

入参：multipart `file`。

出参 `data`：批量结果；`updated_rows` / `skipped_rows` 为流对象。`message`：`Imported streams`。

### GET `/ai_stream2/backend/streams/export`

`200`，`Content-Type: multipart/form-data; boundary=ai_stream2_stream_export`。字段 `file`，文件名 `streams.csv`，正文 `name,url,group` 加数据行，按 name 排序。失败 JSON `500`，`Failed to export streams`。

### POST `/ai_stream2/backend/streams/batch/remove`

入参：`ids`（字符串数组）。

逐条：找不到或卸 path / 删库失败进 `failed_rows`；成功进 `updated_rows`（删除前的流对象）。`message`：`Deleted streams`。

### POST `/ai_stream2/backend/streams/batch/enable`

入参：`ids`。

挂 path 并 `enabled=true` 成功进 `updated_rows`；否则 `failed_rows`。`message`：`Enabled streams`。

### POST `/ai_stream2/backend/streams/batch/disable`

入参：`ids`。

卸 path 并 `enabled=false` 成功进 `updated_rows`；否则 `failed_rows`。`message`：`Disabled streams`。

### POST `/ai_stream2/backend/streams/batch/record`

入参：`ids`。

未挂载进 `skipped_rows`（保持原 `record`）。已挂载且改录成功进 `updated_rows`（`record=true`）。找不到或改 path / 写库失败进 `failed_rows`。`message`：`Recording enabled`。

### POST `/ai_stream2/backend/streams/batch/unrecord`

入参：`ids`。

已挂载则 path 的 record 置假，再写 `record=false`。成功 `updated_rows`，否则 `failed_rows`。`message`：`Recording disabled`。

### POST `/ai_stream2/backend/streams/batch/probe`

入参：`ids`。

探测并写库成功进 `updated_rows`；找不到或探测未接受进 `failed_rows`。`message`：`Probed streams`。

---

## 2. DATA

### `stream`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无（插入用 `gen_random_uuid()`） | PRIMARY KEY |
| `name` | VARCHAR(150) | 无 | NOT NULL，UNIQUE |
| `group_id` | UUID | 无 | NOT NULL，REFERENCES `stream_group(id)` |
| `url` | TEXT | 无 | NOT NULL，UNIQUE |
| `width` | INT | `NULL` | 可空 |
| `height` | INT | `NULL` | 可空 |
| `fps` | NUMERIC(6, 2) | `NULL` | 可空 |
| `status` | VARCHAR(16) | 无 | NOT NULL |
| `enabled` | BOOLEAN | 无 | NOT NULL |
| `record` | BOOLEAN | 无 | NOT NULL |
| `probed_at` | TIMESTAMPTZ | `NULL` | 可空 |

索引：`stream_group_id_idx`（`group_id`）。
