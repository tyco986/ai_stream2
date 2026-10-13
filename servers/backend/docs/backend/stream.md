# Streams（后端）

## 代理

| 本页触发 | 上游 | 上游路径 | 用途 |
|----------|------|----------|------|
| `POST /{stream_id}/probe`、`POST /batch/probe` | ffmpeg `http://puskas_ffmpeg:8080` | `POST /puskas/ffmpeg/rtsp/probe` | 用库内 `url` 探测，得到 width / height / fps / status |
| `enabled=true` 的 `POST /`、`batch/enable` | mediamtx `http://puskas_mediamtx:9997` | `POST /v3/config/paths/replace/{name}` | 挂载。`source` 为库内 `url`，`record` 为库内 `record` |
| `batch/unrecord` | mediamtx | `GET /v3/config/paths/get/{name}`，存在则 `POST /v3/config/paths/replace/{name}` | path 存在才把 `record` 写成 false；库内 `record` 一律写成 false |
| `batch/record` | mediamtx | `GET /v3/config/paths/get/{name}`，存在则 `POST /v3/config/paths/replace/{name}` | path 存在才把 `record` 写成 true；不存在放入 `skipped_ids` |
| `PATCH` | mediamtx | `GET /v3/config/paths/get/{name}`，再按需 `DELETE` / `replace` | 用 get 判断 path 是否存在。目标 `enabled=false` 或改名且 path 存在时卸旧 path；目标 `enabled=true` 且 path 不存在或 `name`/`url`/`record` 变化时挂载 |
| `batch/disable` | mediamtx | `GET /v3/config/paths/get/{name}`，存在则 `DELETE /v3/config/paths/delete/{name}` | path 存在才卸除，然后写 `enabled=false` |
| `DELETE`、`batch/remove` | mediamtx | `GET /v3/config/paths/get/{name}`，存在则 `DELETE /v3/config/paths/delete/{name}` | 先确认 path 存在再卸除，然后删库 |

其余接口不访问这两个上游。`enabled=true` 表示 MediaMTX 上有该 path；`enabled=false` 表示没有。停用会卸 path，库内 `record` 可以仍为 true。

---

## API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/groups/tree` | 左侧树（含 All、子组、直属流节点） |
| `GET` | `/groups/map` | 组 name → id |
| `POST` | `/groups/{parent_group_id}` | 在父组下新建子组 |
| `PATCH` | `/groups/{group_id}` | 重命名组（不可改 All） |
| `DELETE` | `/groups/{group_id}` | 删组及子组；其下流归 All |
| `GET` | `/groups/{group_id}/members` | Transfer 右侧已在组内流 |
| `GET` | `/groups/{group_id}/candidates` | Transfer 左侧可选流 |
| `PUT` | `/groups/{group_id}/members` | Transfer Save：设置组内成员 |
| `POST` | `/groups/import` | 上传组 CSV，整单写入或整单失败 |
| `GET` | `/groups/export` | 导出全部组为 CSV |
| `GET` | `/map` | 流 name → id |
| `GET` | `/default` | Add 弹窗缺省值 |
| `GET` | `/` | 分页列表（按 group/stream/search 筛） |
| `POST` | `/` | 新建流（仅 `enabled=true` 时挂 MediaMTX） |
| `POST` | `/import` | 上传 CSV，整单写入或整单失败 |
| `GET` | `/export` | 导出全部流为 CSV |
| `GET` | `/{stream_id}` | 流详情 |
| `PATCH` | `/{stream_id}` | 更新流（先 get path；目标 `enabled` 决定挂/卸） |
| `DELETE` | `/{stream_id}` | 删除流（卸 path） |
| `POST` | `/{stream_id}/probe` | 单条探测（代理 ffmpeg） |
| `POST` | `/ai_stream2/backend/logs/enable` | 打开页面 debug 日志 |
| `GET` | `/ai_stream2/backend/logs` | 按日期读取页面日志 |
| `POST` | `/batch/remove` | 批量删（卸 path） |
| `POST` | `/batch/enable` | 批量启用（挂 path） |
| `POST` | `/batch/disable` | 批量停用（卸 path） |
| `POST` | `/batch/record` | 批量开录（path 存在才同步 MediaMTX `record`） |
| `POST` | `/batch/unrecord` | 批量停录（path 存在才同步 MediaMTX `record`） |
| `POST` | `/batch/probe` | 批量探测（代理 ffmpeg） |

### `GET /groups/tree`

`data` 即 `All` 节点。组节点含 `id`、`name`、`type`（`group`）、`children`；流节点含 `id`、`name`、`type`（`stream`）、`enabled`、`status`、`record`。同一层先直属流、后子组，各自按 `name` 排序。

```json
{
  "id": "00000000-0000-4000-8000-000000000001",
  "name": "All",
  "type": "group",
  "children": [
    {
      "id": "a1000001-0000-4000-8000-000000000001",
      "name": "cam-04",
      "type": "stream",
      "enabled": true,
      "status": "offline",
      "record": false
    },
    {
      "id": "b2000001-0000-4000-8000-000000000001",
      "name": "Site A",
      "type": "group",
      "children": []
    }
  ]
}
```

### `GET /groups/map`

`data` 为对象，键是组 `name`，值是组 `id`（含 `All`）。

```json
{
  "All": "00000000-0000-4000-8000-000000000001",
  "Site A": "b2000001-0000-4000-8000-000000000001"
}
```

### `POST /groups/import`

`multipart/form-data`，字段 `file` 为 CSV。表头两列：`group`、`parent`，都是组名。不访问 MediaMTX。路径注册在 `/groups/{group_id}` 之前。

先校验再写库。任一条件成立则整单失败，不写任何行：文件内 `group` 重复；`parent` 在库中不存在；`parent` 为空且 `group` 不是 `All`；试图给 `All` 指定父组。`group` 已存在时只改它的父组。新组插入到该父组下。

```csv
group,parent
All,
Site A,All
```

`data` 为本次新建条数和只改父组的条数。

```json
{
  "created_count": 1,
  "updated_count": 0
}
```

### `GET /groups/export`

无查询参数、无请求体。不访问 MediaMTX。导出 `stream_group` 全表，按 `name` 排序。响应为 `multipart/form-data`，字段 `file` 的文件名是 `groups.csv`。`All` 的 `parent` 为空。路径注册在 `/groups/{group_id}` 之前。

```csv
group,parent
All,
Site A,All
```

### `POST /groups/{parent_group_id}`

在该父组下新建子组。父组须已存在（在 `All` 下新建时路径传 `All` 的 id）。`name` 全库唯一。

请求体：

```json
{
  "name": "Garage"
}
```

`data` 为新建的组：`id`、`name`、`parent_id`。

```json
{
  "id": "b2000004-0000-4000-8000-000000000004",
  "name": "Garage",
  "parent_id": "00000000-0000-4000-8000-000000000001"
}
```

### `PATCH /groups/{group_id}`

重命名该组。`name` 全库唯一。`All`（`parent_id` 为空）不可改。

请求体：

```json
{
  "name": "East Gate"
}
```

`data` 为更新后的组：`id`、`name`、`parent_id`。

```json
{
  "id": "b2000003-0000-4000-8000-000000000003",
  "name": "East Gate",
  "parent_id": "b2000002-0000-4000-8000-000000000002"
}
```

### `DELETE /groups/{group_id}`

删除该组及其全部子组，这些组里的流 `group_id` 改为 `All`。不可删除 `All`。不改流的 `enabled`。无请求体。

`data` 含被删组 id 数组，以及改归 `All` 的流数量。

```json
{
  "deleted_group_ids": ["b2000003-0000-4000-8000-000000000003"],
  "moved_stream_count": 1
}
```

### `GET /groups/{group_id}/members`

该组的直属流。`data` 为对象，键是流 `name`，值是流 `id`。

```json
{
  "cam-01": "c3000001-0000-4000-8000-000000000001"
}
```

### `GET /groups/{group_id}/candidates`

不在该组的流。`data` 为对象，键是流 `name`，值是流 `id`。

```json
{
  "cam-04": "a1000001-0000-4000-8000-000000000001"
}
```

### `PUT /groups/{group_id}/members`

把该组的直属流设成请求体里的最终名单。名单内的流 `group_id` 改为该组；原在该组但不在名单中的流 `group_id` 改为 `All`。

请求体：

```json
{
  "stream_ids": ["c3000001-0000-4000-8000-000000000001"]
}
```

`data` 为本次移入数和移出数。

```json
{
  "updated_count": 1,
  "removed_count": 0
}
```

### `GET /`

按 `group_id`（该组及子组内的流）、`stream_id`（只这一条）、`search` 筛选，并用 `page`、`page_size` 分页。未传筛选时视为 `group_id` 为 `All`。

`data` 含本页流、总数和页码。流字段为 `id`、`name`、`group_id`、`group_name`、`url`、`width`、`height`、`fps`、`status`、`enabled`、`record`、`probed_at`。未 Test 成功时 `width`、`height`、`fps`、`probed_at` 为 `null`。

```json
{
  "items": [
    {
      "id": "c3000001-0000-4000-8000-000000000001",
      "name": "cam-01",
      "group_id": "b2000002-0000-4000-8000-000000000002",
      "group_name": "Entrance",
      "url": "rtsp://user:pass@192.168.1.10/s1",
      "width": 1920,
      "height": 1080,
      "fps": 25,
      "status": "online",
      "enabled": true,
      "record": false,
      "probed_at": "2026-06-28T10:15:00Z"
    }
  ],
  "total": 42,
  "page": 1,
  "page_size": 20
}
```

### `GET /default`

无查询参数、无请求体。不读库、不访问 MediaMTX。`data` 为新建流的缺省字段，与 `POST /` 缺省一致。`name`、`url` 无缺省，不返回。路径注册在 `/{stream_id}` 之前。

```json
{
  "group_id": "00000000-0000-4000-8000-000000000001",
  "group_name": "All",
  "enabled": true,
  "record": false
}
```

### `POST /`

新建流。`name`、`url` 全库唯一。缺省 `group_id` 为 `All`，`enabled` 为 `true`，`record` 为 `false`。`status` 为 `offline`，`width`、`height`、`fps`、`probed_at` 为 `null`。仅 `enabled=true` 时把该流挂到 MediaMTX（`source` 为 `url`，`record` 为库内 `record`）。

请求体：

```json
{
  "name": "cam-01",
  "group_id": "00000000-0000-4000-8000-000000000001",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "enabled": true,
  "record": false
}
```

`data` 为新建的流，字段与列表中的单条相同。

### `POST /import`

`multipart/form-data`，字段 `file` 为 CSV。表头三列：`name`、`url`、`group`。`group` 为组名（含 `All`）。不访问 MediaMTX。路径注册在 `/{stream_id}` 之前。

先校验再写库。任一条件成立则整单失败，不写任何行：组名不存在；文件内 `name` 或 `url` 重复；`name` 或 `url` 已属于另一条流。`name` 与 `url` 都对上同一条已有流时，只改该流的 `group`。新行插入时 `enabled=false`、`record=false`，`status` 为 `offline`，`width`、`height`、`fps`、`probed_at` 为 `null`。

```csv
name,url,group
cam-01,rtsp://user:pass@192.168.1.10/s1,All
```

`data` 为本次新建条数和只改组的条数。

```json
{
  "created_count": 1,
  "updated_count": 0
}
```

### `GET /export`

无查询参数、无请求体。不访问 MediaMTX。导出 `stream` 全表，按 `name` 排序。响应为 `multipart/form-data`，字段 `file` 的文件名是 `streams.csv`，内容为表头 `name,url,group` 加每条流一行。`group` 为组名。路径注册在 `/{stream_id}` 之前。

```csv
name,url,group
cam-01,rtsp://user:pass@192.168.1.10/s1,Entrance
```

### `GET /{stream_id}`

`data` 为该流，字段与列表中的单条相同。未 Test 成功时 `width`、`height`、`fps`、`probed_at` 为 `null`。

```json
{
  "id": "c3000001-0000-4000-8000-000000000001",
  "name": "cam-01",
  "group_id": "b2000002-0000-4000-8000-000000000002",
  "group_name": "Entrance",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "width": 1920,
  "height": 1080,
  "fps": 25,
  "status": "online",
  "enabled": true,
  "record": false,
  "probed_at": "2026-06-28T10:15:00Z"
}
```

### `PATCH /{stream_id}`

部分更新 `name`、`group_id`、`url`、`enabled`、`record`。不改 `status`、`width`、`height`、`fps`、`probed_at`。请求未带 `enabled` 时，目标状态用库内 `enabled`。先 `GET` path：path 存在且目标为停用或改名时卸旧 path；目标为启用且 path 不存在或 `name`、`url`、`record` 变化时挂载。

请求体（只包含要改的字段）：

```json
{
  "name": "cam-01",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "enabled": true,
  "record": false
}
```

`data` 为更新后的流，字段与列表中的单条相同。

### `DELETE /{stream_id}`

删除该流。若已挂 MediaMTX 则先卸 path。无请求体。`data` 为 `null`。

```json
null
```

### `POST /{stream_id}/probe`

用库内 `url` 调 ffmpeg 探测，不经 MediaMTX。本接口无请求体。上游 `POST /rtsp/probe` 入参为：

```json
{
  "url": "rtsp://user:pass@192.168.1.10/s1"
}
```

成功时写入 `width`、`height`、`fps`、`status=online`、`probed_at`。失败时只把 `status` 写成 `offline`。

`data`：

```json
{
  "id": "c3000001-0000-4000-8000-000000000001",
  "status": "online",
  "width": 1920,
  "height": 1080,
  "fps": 25,
  "probed_at": "2026-06-28T10:15:00Z"
}
```

### `POST /ai_stream2/backend/logs/enable`

打开页面 debug 日志。无请求体。成功后 stream 与 stream group 的接口才会把入参和出参写入日志文件。`data` 为空对象。

```json
{}
```

### `GET /ai_stream2/backend/logs`

按日期读取某一页的日志文件。查询参数：`date`（`YYYY-MM-DD`）、`page`（`stream` 或 `stream_group`）。

`data.content` 为当天文件原文，每行一条 JSON：`ts`、`method`、`url`、`code`、`input`、`output`、`latency`、`ip`、`in_bytes`、`out_bytes`。`date` 或 `page` 不合法时 400；当天文件不存在时 404。

```json
{
  "content": "{\"ts\":\"2026-10-08 10:15:00.123\",\"method\":\"GET\",\"url\":\"/ai_stream2/backend/streams\",\"code\":200,\"input\":\"\",\"output\":\"{}\",\"latency\":12.5,\"ip\":\"127.0.0.1\",\"in_bytes\":0,\"out_bytes\":128}\n"
}
```

### `POST /batch/remove`

删除这些流。已挂 MediaMTX 的先卸 path，再删库。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data` 为实际删除条数。

```json
{
  "removed_count": 2
}
```

### `POST /batch/enable`

把这些流写成 `enabled=true`，并按库内 `url`、`record` 挂 MediaMTX。不改 `status`、`width`、`height`、`fps`、`probed_at`。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data` 为本次写成启用的条数。

```json
{
  "updated_count": 2
}
```

### `POST /batch/disable`

把这些流写成 `enabled=false`。先 `GET` path，存在才卸除（停拉流、停录）。库内 `record` 保持原值。不改 `status`、`width`、`height`、`fps`、`probed_at`。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data` 为本次写成停用的条数。

```json
{
  "updated_count": 2
}
```

### `POST /batch/record`

先 `GET` path。path 存在才把 `record` 写成 `true`，并同步 MediaMTX `record=true`。path 不存在的流不改库、不挂 path，id 放入 `skipped_ids`。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data` 为本次开录条数，以及因 path 不存在而跳过的 id。

```json
{
  "updated_count": 1,
  "skipped_ids": ["c3000002-0000-4000-8000-000000000002"]
}
```

### `POST /batch/unrecord`

把这些流的 `record` 写成 `false`。path 存在才同步 MediaMTX `record=false`；不存在只改库。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data` 为本次停录条数。

```json
{
  "updated_count": 2
}
```

### `POST /batch/probe`

用各流库内 `url` 调 ffmpeg 探测，不经 MediaMTX。成功写入该条 `width`、`height`、`fps`、`status=online`、`probed_at`；失败只把该条 `status` 写成 `offline`。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data.results` 与请求 `ids` 顺序一致。成功一条：

```json
{
  "results": [
    {
      "id": "c3000001-0000-4000-8000-000000000001",
      "status": "online",
      "width": 1920,
      "height": 1080,
      "fps": 25,
      "probed_at": "2026-06-28T10:15:00Z"
    },
    {
      "id": "c3000002-0000-4000-8000-000000000002",
      "status": "offline"
    }
  ]
}
```

---

## 数据库

本页仅两张表：`stream_group`、`stream`。

### `stream_group`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | 后端生成；`All` 固定为 `ALL_GROUP_ID` |
| `name` | varchar | UNIQUE，非空 | 全库唯一 |
| `parent_id` | UUID FK → `stream_group.id` | 可空；`SET_NULL` 或禁删根 | `All` 为 `NULL`；其余指向父组 |

### `stream`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | 后端生成，不可改 |
| `name` | varchar | UNIQUE，非空 | — | 全库唯一；启用时作 MediaMTX path 名 |
| `group_id` | UUID FK → `stream_group.id` | 非空 | `ALL_GROUP_ID` | 直属组 |
| `url` | text | UNIQUE，非空 | — | 完整 RTSP（含凭据）；启用时 = MediaMTX `source` |
| `width` | int | 可空 | `NULL` | 像素宽；仅 Test **成功**写入 |
| `height` | int | 可空 | `NULL` | 像素高；仅 Test **成功**写入 |
| `fps` | numeric(6, 2) | 可空 | `NULL` | 仅 Test **成功**写入 |
| `status` | varchar | 非空 | `offline` | `online` \| `offline`；**仅 Test** 写 |
| `enabled` | bool | 非空 | `true` | **true=挂载代理 / false=卸除**；不改 `status` |
| `record` | bool | 非空 | `false` | 意图开录；仅 `enabled=true` 时写入 MediaMTX `record` |
| `probed_at` | timestamptz | 可空 | `NULL` | 最近一次 Test **成功**时间 |
