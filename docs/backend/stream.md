# Streams（后端）

对应前端契约：[stream_api.md](../frontend/stream_api.md)。  
实现包：`servers/django/pages/streams/`。前缀：`/ai_stream2/backend/streams`。

---

## 代理的 API

| 本页触发 | 上游 server | 上游路径 | 用途 |
|----------|-------------|----------|------|
| `POST /{stream_id}/test`、`POST /batch/test`、`POST /probe` | **ffmpeg**（容器 `${PROJECT_NAME}_ffmpeg`） | `POST /{PROJECT_NAME}/ffmpeg/rtsp/test`（批量可用 `.../rtsp/batch/test`） | 探测 RTSP → resolution / fps / online\|offline |
| `POST /publishers` | **ffmpeg** | `POST /{PROJECT_NAME}/ffmpeg/rtsp/publishers`（multipart，`loop` 固定 true） | 上传视频并推 RTSP；返回 `{name,url}` |
| `enabled=true`：`POST /`、`PATCH`、`batch/enable`；以及已启用时改 `url`/`name`/`recording` | **mediamtx**（容器 `${PROJECT_NAME}_mediamtx`） | `POST /v3/config/paths/replace/{path}` | **挂载**代理：`source`=库内 `url`；`record`=`recording` |
| `enabled=false`：`PATCH`、`batch/disable`；以及 `DELETE` / `batch/delete` | **mediamtx** | path delete（或等价卸除） | **取消挂载**；删流时若仍挂着也卸 path |

其余端点不代理。MediaMTX **不**透传给前端；仅本页 Service 副作用调用。

**挂载规则（唯一真相）**：`enabled=true` ↔ MediaMTX 上存在该流 path；`enabled=false` ↔ 无 path。`recording` 只在已挂载时写入 `record`；Disable 卸 path 即停拉流并停录（库内 `recording` 可仍为 true，下次 Enable 再按该值开录）。

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/groups/tree` | 左侧树（含 All、子组、直属流节点） |
| `GET` | `/groups/map` | 组 name → id |
| `GET` | `/groups/list` | 扁平组列表（弹窗 Group 下拉） |
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
| `POST` | `/{stream_id}/test` | 单条 Test（代理 ffmpeg） |
| `GET` | `/{stream_id}/logs` | Log 弹窗文本 |
| `POST` | `/batch/delete` | 批量删（卸 path） |
| `POST` | `/batch/enable` | 批量启用（挂 path） |
| `POST` | `/batch/disable` | 批量停用（卸 path） |
| `POST` | `/batch/record` | 批量开录（仅已启用流；同步 MediaMTX `record`） |
| `POST` | `/batch/unrecord` | 批量停录（同步 MediaMTX `record`） |
| `POST` | `/batch/test` | 批量 Test（代理 ffmpeg） |

字段 / 错误码 / 固定段顺序以 [stream_api.md](../frontend/stream_api.md) 为准。权限：`streams.view_stream` / `add` / `change` / `delete`。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | `Group`（本 app）、`Stream`；UUID 主键；`ALL_GROUP_ID` 种子 |
| HTTP 客户端 | `httpx`（调 ffmpeg、mediamtx；超时可配） |
| 分页 | `shared.pagination` → `{items,total,page,page_size}` |
| 鉴权 | Django session + `has_perm`（经 `shared.auth`） |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| RTSP 探测 | **复用** ffmpeg `rtsp/test`（或 `rtsp/batch/test`）；上游 `data` 已含 `resolution`/`fps`，本页只包装 `id`/`status`/`last_test_at` 入库 |
| RTSP 代理 | **复用** MediaMTX；**Enable 挂载 / Disable 卸除**；path=`name`，`source`=`url`；`recording`→`record`（参考 `servers/mediamtx/scripts/4_enable_recording.sh` / `4_disable_recording.sh`） |
| 树 / Transfer / 级联删组 | **造**（`GroupService` / `StreamService`） |
| name 全库唯一 | **造**（DB unique + 校验） |
| url 全库唯一 | **造**（DB unique + 校验） |
| 响应外壳 / 分页 / 鉴权 | **复用** `shared.*` |
| Site Config 切片 | **造**（向 `shared.site_config` 注册 groups+streams 导出导入） |
| 流日志文本 | **造**（本页日志缓冲或文件；**不**代理 docker logs） |

---

## 目录结构

```
servers/django/pages/streams/
  apps.py
  urls.py                 # 固定段先于 {id}
  models.py               # Group, Stream
  serializers.py
  views.py
  services.py             # GroupService, StreamService, StreamTestService, StreamLogService
  clients.py              # FFmpegClient（rtsp/test）；MediaMTXClient（paths）
  permissions.py          # streams.* codename 常量 + DRF 类
  migrations/             # 含 All 组 data migration
```

禁止 import 其它 `pages.*`。ffmpeg 经 `FFmpegClient`（`${PROJECT_NAME}_ffmpeg:8080`）；mediamtx 经 `MediaMTXClient`（`${PROJECT_NAME}_mediamtx:9997`）。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `Group` | id、name、parent（自引用；All 的 parent=null） |
| `Stream` | id、name、group、url、resolution、fps、status、enabled、recording、last_test_at |
| `GroupService` | tree / map / list / create / rename / delete（级联+流归 All）/ members / candidates / set_members |
| `StreamService` | list / map / crud / batch enable\|disable\|delete；按 `enabled` 调 `MediaMTXClient` 挂/卸 path |
| `StreamTestService` | 单条/批量 test：调 `FFmpegClient` → 写 status 等 |
| `StreamLogService` | `get_logs(stream_id)` → `{content}`（文件/缓冲，**无**日志表） |
| `FFmpegClient` | `test(url)` / `batch_test(urls)` → 取 `data`（`resolution`/`fps`）；再包装为 `TestResult` |
| `MediaMTXClient` | `upsert_path(name, source_url, record)` / `delete_path(name)` → Control API |
| `*View` | 薄：鉴权 → Service → `ApiResponse` |

---

## 数据库

本页仅两张表：`streams_group`、`streams_stream`（Django 默认 `app_label`=`streams`）。**不**存 MediaMTX 状态、不存日志正文、不存 `group_name`（响应时 join）。

### `streams_group`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | 后端生成；`All` 固定为 `ALL_GROUP_ID` |
| `name` | varchar | UNIQUE，非空 | 全库唯一 |
| `parent_id` | UUID FK → `streams_group.id` | 可空；`SET_NULL` 或禁删根 | `All` 为 `NULL`；其余指向父组 |

种子（data migration）：一行 `id=ALL_GROUP_ID`、`name=All`、`parent_id=NULL`。

### `streams_stream`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | 后端生成，不可改 |
| `name` | varchar | UNIQUE，非空 | — | 全库唯一；启用时作 MediaMTX path 名 |
| `group_id` | UUID FK → `streams_group.id` | 非空 | `ALL_GROUP_ID` | 直属组 |
| `url` | text | UNIQUE，非空 | — | 完整 RTSP（含凭据）；启用时 = MediaMTX `source` |
| `resolution` | varchar | 可空 | `NULL` | `WxH`；仅 Test **成功**写入 |
| `fps` | int | 可空 | `NULL` | 仅 Test **成功**写入 |
| `status` | varchar | 非空 | `offline` | `online` \| `offline`；**仅 Test** 写 |
| `enabled` | bool | 非空 | `true` | **true=挂载代理 / false=卸除**；不改 `status` |
| `recording` | bool | 非空 | `false` | 意图开录；仅 `enabled=true` 时写入 MediaMTX `record` |
| `last_test_at` | timestamptz | 可空 | `NULL` | 最近一次 Test **成功**时间 |

索引建议：`group_id`；列表筛选用。

---

## 各请求写库明细

只列**写**操作。GET / map / tree / candidates / members / logs：**只读**，不改库。MediaMTX 为网外副作用，不入库。

| 请求 | 表 | 写入 / 变更 | MediaMTX |
|------|-----|-------------|----------|
| `POST /groups/{parent_group_id}` `{name}` | `streams_group` | **INSERT** `id`(新)、`name`、`parent_id`=`parent_group_id` | — |
| `PATCH /groups/{group_id}` `{name}` | `streams_group` | **UPDATE** 该行 `name`（禁改 `All`） | — |
| `DELETE /groups/{group_id}` | `streams_stream` | 该组及子树内所有流 **UPDATE** `group_id`=`ALL_GROUP_ID` | —（不改 enabled） |
| | `streams_group` | **DELETE** 该组及全部子组行 | — |
| `PUT /groups/{group_id}/members` `{stream_ids}` | `streams_stream` | 列表内：**UPDATE** `group_id`=目标组；原在该组但不在列表：**UPDATE** `group_id`=`ALL_GROUP_ID` | — |
| `POST /` `{name,group_id?,url,enabled?,recording?}` | `streams_stream` | **INSERT**：`id` 新；`name`/`url`；`group_id` 缺省 `ALL_GROUP_ID`；`enabled` 缺省 `true`；`recording` 缺省 `false`；`status=offline`；`resolution`/`fps`/`last_test_at`=`NULL` | 仅 `enabled=true` → `upsert_path`；否则不挂 |
| `PATCH /{stream_id}` 部分字段 | `streams_stream` | **UPDATE** body 中的 `name`/`group_id`/`url`/`enabled`/`recording`；**不**改 `status`/`resolution`/`fps`/`last_test_at` | 见下「PATCH → MediaMTX」 |
| `DELETE /{stream_id}` | `streams_stream` | **DELETE** 该行 | 若曾挂载 → `delete_path` |
| `POST /batch/delete` `{ids}` | `streams_stream` | 逐条 **DELETE** | 同上卸 path |
| `POST /batch/enable` `{ids}` | `streams_stream` | **UPDATE** `enabled=true`（仅原 false 计入 `updated_count`） | 每条 `upsert_path(name, url, record=recording)` |
| `POST /batch/disable` `{ids}` | `streams_stream` | **UPDATE** `enabled=false` | 每条 `delete_path(name)` |
| `POST /{stream_id}/test`、`POST /batch/test` | `streams_stream` | 成功：**UPDATE** `resolution`、`fps`、`status=online`、`last_test_at=now`；失败：**UPDATE** `status=offline`；**不**改 `resolution`/`fps`/`last_test_at` | —（Test 用库内 `url` 探源，不经 MediaMTX） |

**PATCH → MediaMTX**（落库后按最终行状态）：

| 变化 | 动作 |
|------|------|
| `enabled` false→true | `upsert_path(name, url, record=recording)` |
| `enabled` true→false | `delete_path(name)` |
| 保持 `enabled=true`，且 `name`/`url`/`recording` 变 | `upsert_path`（`name` 变：先 `delete_path(旧名)` 再 upsert 新名） |
| 保持 `enabled=false` | 不调 MediaMTX（含只改 `recording`：只写库，Enable 后再生效） |

Site Config **import**：重建/更新行后，对 **`enabled=true`** 的流通 `upsert_path`；`enabled=false` 确保无残留 path（`delete_path` 幂等）。

---

## 调用顺序

### 组树 / CRUD

```
GET tree:     GroupService.build_tree → {root}
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
DELETE/: / batch/delete: delete_path（若有）→ 删库
```

Preview / Recordings **只消费**已挂载 path（WebRTC / 录像文件），**不**配置 MediaMTX。未 Enable 的流无代理可读。

### Test（含批量）

```
View → require change 权
    → StreamTestService.test(ids)
         → 对每条: FFmpegClient.test(库内 url) → data.resolution/fps
         → 包装 id/status/last_test_at（或 error）写入库
         → TestResult | {results}
```

### Logs

```
View → StreamLogService.get_logs → {content}
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
