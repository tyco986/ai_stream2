# Streams（后端）

对应前端契约：[stream_api.md](../frontend/stream_api.md)。  
实现包：`servers/django/pages/streams/`。前缀：`/ai_stream2/backend/streams`。

---

## 代理的 API

| 本页触发 | 上游 server | 上游路径 | 用途 |
|----------|-------------|----------|------|
| `POST /{stream_id}/probe`、`POST /batch/probe`、`POST /probe` | **ffmpeg**（容器 `${PROJECT_NAME}_ffmpeg`） | `POST /{PROJECT_NAME}/ffmpeg/rtsp/probe`（批量可用 `.../rtsp/batch/probe`） | 探测 RTSP → width / height / fps / online\|offline |
| `POST /publishers` | **ffmpeg** | `POST /{PROJECT_NAME}/ffmpeg/rtsp/publishers`（multipart，`loop` 固定 true） | 上传视频并推 RTSP；返回 `{name,url}` |
| `enabled=true`：`POST /`、`PATCH`、`batch/enable`；以及已启用时改 `url`/`name`/`recording` | **mediamtx**（容器 `${PROJECT_NAME}_mediamtx`） | `POST /v3/config/paths/replace/{path}` | **挂载**代理：`source`=库内 `url`；`record`=`recording` |
| `enabled=false`：`PATCH`、`batch/disable`；以及 `DELETE` / `batch/remove` | **mediamtx** | path delete（或等价卸除） | **取消挂载**；删流时若仍挂着也卸 path |

其余端点不代理。MediaMTX **不**透传给前端；仅本页 Service 副作用调用。

**挂载规则（唯一真相）**：`enabled=true` ↔ MediaMTX 上存在该流 path；`enabled=false` ↔ 无 path。`recording` 只在已挂载时写入 `record`；Disable 卸 path 即停拉流并停录（库内 `recording` 可仍为 true，下次 Enable 再按该值开录）。

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
| `GET` | `/map` | 流 name → id |
| `GET` | `/` | 分页列表（按 group/stream/search 筛） |
| `POST` | `/` | 新建流（仅 `enabled=true` 时挂 MediaMTX） |
| `POST` | `/publishers` | 上传视频推 RTSP（`add_stream`；loop 固定 true） |
| `GET` | `/{stream_id}` | 流详情 |
| `PATCH` | `/{stream_id}` | 更新流（`enabled` 控制挂/卸；已启用时改 `url`/`recording`/`name` 同步 path） |
| `DELETE` | `/{stream_id}` | 删除流（卸 path） |
| `POST` | `/{stream_id}/probe` | 单条探测（代理 ffmpeg） |
| `POST` | `/ai_stream2/backend/logs/enable` | 打开页面 debug 日志 |
| `GET` | `/ai_stream2/backend/logs` | 按日期读取页面日志 |
| `POST` | `/batch/remove` | 批量删（卸 path） |
| `POST` | `/batch/enable` | 批量启用（挂 path） |
| `POST` | `/batch/disable` | 批量停用（卸 path） |
| `POST` | `/batch/record` | 批量开录（仅已启用流；同步 MediaMTX `record`） |
| `POST` | `/batch/unrecord` | 批量停录（同步 MediaMTX `record`） |
| `POST` | `/batch/probe` | 批量探测（代理 ffmpeg） |

### `GET /groups/tree`

`data` 即 `All` 节点。组节点含 `id`、`name`、`type`（`group`）、`children`；流节点含 `id`、`name`、`type`（`stream`）、`enabled`、`status`、`recording`。同一层先直属流、后子组，各自按 `name` 排序。

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
      "recording": false
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

`data` 含本页流、总数和页码。流字段为 `id`、`name`、`group_id`、`group_name`、`url`、`width`、`height`、`fps`、`status`、`enabled`、`recording`、`probed_at`。未 Test 成功时 `width`、`height`、`fps`、`probed_at` 为 `null`。

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
      "recording": false,
      "probed_at": "2026-06-28T10:15:00Z"
    }
  ],
  "total": 42,
  "page": 1,
  "page_size": 20
}
```

### `POST /`

新建流。`name`、`url` 全库唯一。缺省 `group_id` 为 `All`，`enabled` 为 `true`，`recording` 为 `false`。`status` 为 `offline`，`width`、`height`、`fps`、`probed_at` 为 `null`。仅 `enabled=true` 时把该流挂到 MediaMTX（`source` 为 `url`，`record` 为 `recording`）。

请求体：

```json
{
  "name": "cam-01",
  "group_id": "00000000-0000-4000-8000-000000000001",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "enabled": true,
  "recording": false
}
```

`data` 为新建的流，字段与列表中的单条相同。

### `POST /publishers`

上传视频并推 RTSP。`name` 可省略，缺省用文件名。`loop` 未传也按 `true` 转给 ffmpeg。不写流表，不挂 MediaMTX。

请求体：

```json
{
  "input": "<video file>",
  "name": "clip",
  "loop": true
}
```

`data`：

```json
{
  "name": "clip",
  "url": "rtsp://mediamtx/clip"
}
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
  "recording": false,
  "probed_at": "2026-06-28T10:15:00Z"
}
```

### `PATCH /{stream_id}`

部分更新 `name`、`group_id`、`url`、`enabled`、`recording`。不改 `status`、`width`、`height`、`fps`、`probed_at`。`enabled` 从 false 变为 true 时挂 MediaMTX；从 true 变为 false 时卸 path。保持 `enabled=true` 且 `name`、`url` 或 `recording` 变化时同步 path（`name` 变了先卸旧 path）。保持 `enabled=false` 时不调 MediaMTX。

请求体（只包含要改的字段）：

```json
{
  "name": "cam-01",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "enabled": true,
  "recording": false
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

把这些流写成 `enabled=true`，并按库内 `url`、`recording` 挂 MediaMTX（`source`=`url`，`record`=`recording`）。不改 `status`、`width`、`height`、`fps`、`probed_at`。

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

把这些流写成 `enabled=false`，并卸 MediaMTX path（停拉流、停录）。库内 `recording` 保持原值。不改 `status`、`width`、`height`、`fps`、`probed_at`。

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

仅对已启用流把 `recording` 写成 `true`，并同步 MediaMTX `record=true`。未启用的流不改库、不挂 path。

请求体：

```json
{
  "ids": [
    "c3000001-0000-4000-8000-000000000001",
    "c3000002-0000-4000-8000-000000000002"
  ]
}
```

`data` 为本次开录条数，以及因未启用而跳过的 id。

```json
{
  "updated_count": 1,
  "skipped_ids": ["c3000002-0000-4000-8000-000000000002"]
}
```

### `POST /batch/unrecord`

把这些流的 `recording` 写成 `false`。已启用的同步 MediaMTX `record=false`；未启用的只改库。

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
| `recording` | bool | 非空 | `false` | 意图开录；仅 `enabled=true` 时写入 MediaMTX `record` |
| `probed_at` | timestamptz | 可空 | `NULL` | 最近一次 Test **成功**时间 |

---

## 调用顺序

### 组树 / CRUD

```
GET tree:     StreamGroupService.tree → All 节点
POST group:   校验 parent → 唯一 name → create
DELETE group: 禁删 All → 收集子树 id → 流改 ALL_GROUP_ID → 删组
PUT members:  校验组 → 将 stream_ids 设为该 group_id（其余规则见契约）
```

### 流列表 / CRUD（enabled ↔ MediaMTX 挂载）

```
GET /:   解析 group_id|stream_id|search|page → StreamService.list → 分页外壳
POST /:  默认 group=All → 唯一 name → create
         → 若 enabled: MediaMTXClient.upsert_path(name, url, record=recording)
PATCH/:  部分更新；enabled/recording 不改 status
         → enabled↑: upsert_path；enabled↓: delete_path
         → 已启用且 url/name/recording 变: upsert_path
batch/enable:  写 enabled=true → upsert_path
batch/disable: 写 enabled=false → delete_path
DELETE/: / batch/remove: delete_path（若有）→ 删库
```

Preview / Recordings **只消费**已挂载 path（WebRTC / 录像文件），**不**配置 MediaMTX。未 Enable 的流无代理可读。

### Test（含批量）

```
View → require change 权
    → StreamTestService.test(ids)
         → 对每条: FFmpegClient.probe(库内 url) → data.width/height/fps
         → 包装 id/status/probed_at（或 error）写入库
         → TestResult | {results}
```

### Logs

```
View → POST /ai_stream2/backend/logs/enable → 打开 debug
View → GET /ai_stream2/backend/logs?date=&page= → {content}
```

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| ffmpeg 容器 | Test 必需；不可达 → 该条 Test 失败（offline + error） |
| mediamtx 容器 | Enable / Disable / 已启用时改 url·recording·name / 删流 必需；不可达 → 该写操作失败（库与挂载一致，失败不半写） |
| shell / login | 仅会话与权限；不 import |
| Site Config | 本页注册切片；编排在 shell；导入后仅对 `enabled=true` upsert，并对 false 卸残留 |
| Preview / Recordings | 只读消费已 Enable 的 MediaMTX path / 录像；**不**由本页之外配置 path |
