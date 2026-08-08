# Streams API

管理端 Streams 页面后端 API。UI 规格见 [stream.md](./stream.md)。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/streams` |
| 路径前缀 | `/ai_stream2/backend/streams` |

组相关路径挂在同一前缀下：`/groups/...`。流资源集合为前缀根路径 `/`，单条流为 `/{stream_id}`。

实现时须将固定段（`tree`、`list`、`map`）注册在 `/{group_id}` 之前；流的 `map`、`batch` 同理，须先于 `/{stream_id}`。

---

## 约定

### 响应外壳

```json
{
  "success": true,
  "message": "",
  "data": {}
}
```

失败时 `success: false`，HTTP 状态码 4xx/5xx，`message` 为错误说明。

### 分页列表

```json
{
  "success": true,
  "message": "",
  "data": {
    "items": [],
    "total": 0,
    "page": 1,
    "page_size": 20
  }
}
```

### 常量

| 名称 | 值 | 说明 |
|------|-----|------|
| `ALL_GROUP_ID` | `00000000-0000-4000-8000-000000000001` | 根组 **All**，落库；未删除组归属外的流默认归属此组 |

`All` 为真实组记录，不可删除、不可 Rename。

### 字段语义

| 字段 | 说明 |
|------|------|
| `stream_id` | 流 **id**（UUID）；路径 `/{stream_id}` 与 `Stream.id` 同值 |
| `group_id` | 组 **id**（UUID）；路径 `/groups/{group_id}`、query `group_id` 与 `Group.id` 同值 |
| `parent_group_id` | **POST** `/groups/{parent_group_id}` 专用；父组 **id**，与 `Group.id` 同值 |
| `status` | 仅由 **Test** 写入：`online` / `offline`；与 `enabled` **互不影响** |
| `enabled` | 用户启停控制；**不**写入 `status` |
| `recording` | 用户录像开关 |
| `url` | 完整 RTSP URL（含用户名与密码）；管理端始终返回，无额外权限裁剪 |

---

## 数据模型

### Group

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "Entrance",
  "parent_id": "a1b2c3d4-e5f6-4789-a012-3456789abcde"
}
```

顶层组的 `parent_id` 为 `ALL_GROUP_ID`。`All` 组无 `parent_id` 或 `parent_id` 为 `null`（实现二选一，文档以 `null` 表示根）。

### Stream

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "name": "cam-01",
  "group_id": "550e8400-e29b-41d4-a716-446655440000",
  "group_name": "Entrance",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "resolution": "1920x1080",
  "fps": 25,
  "status": "online",
  "enabled": true,
  "recording": false,
  "last_test_at": "2026-06-28T10:15:00Z"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成，不可改 |
| `name` | string | 否 | 展示名，可编辑 |
| `group_id` | UUID | 否 | 默认 `ALL_GROUP_ID` |
| `group_name` | string | 否 | 冗余展示 |
| `url` | string | 否 | 含凭据 |
| `resolution` | string | 是 | `WxH`；Test 成功后写入 |
| `fps` | int | 是 | Test 成功后写入 |
| `status` | string | 否 | `online` \| `offline` |
| `enabled` | bool | 否 | 用户控制 |
| `recording` | bool | 否 | 用户控制 |
| `last_test_at` | datetime | 是 | 最近一次 Test 成功时间 |

### TreeNode（左侧树）

**组节点**（`type: "group"`）：`id`、`name`、`type`、`children`。

**流节点**（`type: "stream"`）：在组节点字段基础上增加 `enabled`、`status`、`recording`（与 [Stream](#stream) 同义；**不含** `url` / `resolution` / `fps` 等）。Preview / Recordings 绑槽、起播时另 `GET /{stream_id}` 取完整字段。

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "Site A",
  "type": "group",
  "children": [
    {
      "id": "8d0e7780-8536-51ef-a055-f18fc2g01bf8",
      "name": "cam-01",
      "type": "stream",
      "enabled": true,
      "status": "online",
      "recording": true
    }
  ]
}
```

| 字段 | 节点 | 说明 |
|------|------|------|
| `type` | 组 / 流 | `group` \| `stream` |
| `enabled` | 流 | 用户启停 |
| `status` | 流 | `online` \| `offline`；仅 **Test** 写入库 |
| `recording` | 流 | 用户录像开关 |

`stream` 节点仅出现在**直属该组**的流下。三端 Groups 树 UI 规则见 [stream.md](./stream.md) **流节点样式**。

### StreamSummary（Transfer List 行）

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "name": "cam-04",
  "group_id": "00000000-0000-4000-8000-000000000001",
  "group_name": "All"
}
```

### TestResult（单条）

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "success": true,
  "resolution": "1920x1080",
  "fps": 25,
  "status": "online",
  "last_test_at": "2026-06-28T10:15:00Z"
}
```

失败时：

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "success": false,
  "error": "Connection refused"
}
```

失败不更新 `resolution` / `fps` / `last_test_at`；`status` 置为 `offline`。

### NameMap（name → id）

`map` 接口返回对象：**key** 为 `name`，**value** 为 UUID。组名、流名均**全库唯一**（见 [stream.md](./stream.md)）。

```json
{
  "cam-01": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "cam-02": "8d0e7780-8536-51ef-a055-f18fc2g01bf8"
}
```

---

## 端点

### 组 · 树与列表

#### `GET /groups/tree`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/tree`

**说明**：左侧分组树；含 `All` 根节点、嵌套子组、各组**直属**流节点。

**Query**：无

**响应 `data`**：

```json
{
  "root": {
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
        "children": [
          {
            "id": "b2000002-0000-4000-8000-000000000002",
            "name": "Entrance",
            "type": "group",
            "children": [
              {
                "id": "c3000001-0000-4000-8000-000000000001",
                "name": "cam-01",
                "type": "stream",
                "enabled": true,
                "status": "online",
                "recording": true
              },
              {
                "id": "c3000002-0000-4000-8000-000000000002",
                "name": "cam-02",
                "type": "stream",
                "enabled": true,
                "status": "online",
                "recording": false
              },
              {
                "id": "b2000003-0000-4000-8000-000000000003",
                "name": "East Gate",
                "type": "group",
                "children": [
                  {
                    "id": "c3000006-0000-4000-8000-000000000006",
                    "name": "cam-06",
                    "type": "stream",
                    "enabled": false,
                    "status": "offline",
                    "recording": false
                  }
                ]
              }
            ]
          },
          {
            "id": "b2000004-0000-4000-8000-000000000004",
            "name": "Garage",
            "type": "group",
            "children": []
          }
        ]
      }
    ]
  }
}
```

---

#### `GET /groups/map`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/map`

**说明**：全部组的 **name → id** 映射表。

**响应 `data`**：

```json
{
  "All": "00000000-0000-4000-8000-000000000001",
  "Site A": "b2000001-0000-4000-8000-000000000001",
  "Entrance": "b2000002-0000-4000-8000-000000000002"
}
```

---

#### `GET /groups/list`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/list`

**说明**：扁平组列表，供 Stream Add/Edit 弹窗 **Group** 下拉（含 `All`）。

**响应 `data`**：

```json
{
  "items": [
    {
      "id": "00000000-0000-4000-8000-000000000001",
      "name": "All"
    }
  ]
}
```

---

#### `POST /groups/{parent_group_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/{parent_group_id}`

**说明**：在路径所指的**父组**下新增子组（左侧 ⋮ → Add Group 确认）。从 `All` 下新增时传 `ALL_GROUP_ID`。

**请求体**：

```json
{
  "name": "Garage"
}
```

**响应 `data`**：创建的 `Group` 对象（含新 `id`）。

---

#### `PATCH /groups/{group_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/{group_id}`

**说明**：Rename 组（左侧 ⋮ → Rename 确认）。`{group_id}` 为待重命名组 id。不可修改 `All`。

**请求体**：

```json
{
  "name": "East Gate"
}
```

**响应 `data`**：更新后的 `Group`。

---

#### `DELETE /groups/{group_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/{group_id}`

**说明**：删除组（左侧 ⋮ → Delete 确认）。`{group_id}` 为待删除组 id。

**行为**：

- **级联删除**该组及所有子组
- 上述组内（含子组）所有流的 `group_id` 改为 `ALL_GROUP_ID`
- 不可删除 `All`

**响应 `data`**：

```json
{
  "deleted_group_ids": ["550e8400-e29b-41d4-a716-446655440000"],
  "moved_stream_count": 3
}
```

---

### 组 · Transfer List

#### `GET /groups/{group_id}/members`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/{group_id}/members`

**说明**：Transfer List 右侧「Target Group」表格（已在该组的流）。

**响应 `data`**：

```json
{
  "group_id": "550e8400-e29b-41d4-a716-446655440000",
  "group_name": "Entrance",
  "items": []
}
```

`items` 元素为 `StreamSummary`。

---

#### `GET /groups/{group_id}/candidates`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/{group_id}/candidates`

**说明**：Transfer List 左侧「Available」表格（当前不在该组的流）。

**Query**（可选）：

| 参数 | 说明 |
|------|------|
| `search` | 按 name 过滤 |

**响应 `data`**：

```json
{
  "items": []
}
```

---

#### `PUT /groups/{group_id}/members`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/groups/{group_id}/members`

**说明**：Transfer List **Save**；将右侧列表设为该组最终成员。

**请求体**：

```json
{
  "stream_ids": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "8d0e7780-8536-51ef-a055-f18fc2g01bf8"
  ]
}
```

**行为**：

- 列表中的流：`group_id` → 目标 `group_id`
- 原在该组、但不在列表中的流：`group_id` → `ALL_GROUP_ID`

**响应 `data`**：

```json
{
  "updated_count": 2,
  "removed_count": 1
}
```

---

### 流 · 列表与 CRUD

#### `GET /map`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/map`

**说明**：全部流的 **name → id** 映射表。

**响应 `data`**：

```json
{
  "cam-01": "c3000001-0000-4000-8000-000000000001",
  "cam-02": "c3000002-0000-4000-8000-000000000002",
  "cam-04": "a1000001-0000-4000-8000-000000000001"
}
```

---

#### `GET /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams`

**说明**：右侧表格分页列表。

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `group_id` | UUID | 点左侧**组**；返回该组直属流 + 所有子组流（递归） |
| `stream_id` | UUID | 点左侧**流**；仅返回该条；与 `group_id` 互斥 |
| `search` | string | 搜索框 |
| `page` | int | 默认 `1` |
| `page_size` | int | 默认 `20` |

均未传时等价于 `group_id=ALL_GROUP_ID`（递归）。

**响应 `data`**：分页结构，`items` 为 `Stream[]`。

```json
{
  "items": [
    {
      "id": "c3000001-0000-4000-8000-000000000001",
      "name": "cam-01",
      "group_id": "b2000002-0000-4000-8000-000000000002",
      "group_name": "Entrance",
      "url": "rtsp://user:pass@192.168.1.10/s1",
      "resolution": "1920x1080",
      "fps": 25,
      "status": "online",
      "enabled": true,
      "recording": false,
      "last_test_at": "2026-06-28T10:15:00Z"
    }
  ],
  "total": 42,
  "page": 1,
  "page_size": 20
}
```

---

#### `POST /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams`

**说明**：工具栏 **Add** → 弹窗 **Save**（新建）。

**请求体**：

```json
{
  "name": "cam-01",
  "group_id": "00000000-0000-4000-8000-000000000001",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "enabled": true,
  "recording": false
}
```

**响应 `data`**：完整 `Stream`（含新 `id`）。

---

#### `GET /{stream_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/{stream_id}`

**说明**：编辑前拉取详情（Operations **✏ Edit** 打开弹窗时，可选）。

**响应 `data`**：`Stream`。

---

#### `PATCH /{stream_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/{stream_id}`

**说明**：更新流字段（部分字段即可）。用于：

- Operations **✏** → 弹窗 **Save**（**Name 可改**）
- Operations **▶ / ■** 启停切换（仅传 `enabled`）
- Operations **🎥** 录像开关（仅传 `recording`）

**请求体**（示例）：

弹窗 Save：

```json
{
  "name": "cam-01-renamed",
  "group_id": "550e8400-e29b-41d4-a716-446655440000",
  "url": "rtsp://user:pass@192.168.1.10/s1",
  "enabled": true,
  "recording": false
}
```

启停切换：

```json
{
  "enabled": false
}
```

录像开关：

```json
{
  "recording": true
}
```

**响应 `data`**：更新后的 `Stream`。

---

#### `DELETE /{stream_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/{stream_id}`

**说明**：Operations **🗑** 单行删除（二次确认后）。

**响应 `data`**：`null` 或 `{}`。

---

### 流 · 单条动作

#### `POST /{stream_id}/test`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/{stream_id}/test`

**说明**：Operations **↻** 或弹窗全宽 **Test**（单条）。

**请求体**：无（或 `{}`）

**响应 `data`**：`TestResult`。

成功时更新该流 `resolution`、`fps`、`status`、`last_test_at`。

---

#### `GET /{stream_id}/logs`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/{stream_id}/logs`

**说明**：Operations **📓** Log 弹窗。

**响应 `data`**：

```json
{
  "content": "2026-06-28 10:15:00 INFO ...\n"
}
```

---

### 流 · 批量（Batch）

请求体统一：

```json
{
  "ids": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "8d0e7780-8536-51ef-a055-f18fc2g01bf8"
  ]
}
```

#### `POST /batch/delete`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/batch/delete`

**说明**：工具栏 **Remove**（二次确认后）。

**响应 `data`**：

```json
{
  "deleted_count": 2
}
```

---

#### `POST /batch/enable`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/batch/enable`

**说明**：工具栏 **Enable**。

**响应 `data`**：

```json
{
  "updated_count": 2
}
```

---

#### `POST /batch/disable`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/batch/disable`

**说明**：工具栏 **Disable**。

**响应 `data`**：同 `batch/enable`。

---

#### `POST /batch/test`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/streams/batch/test`

**说明**：工具栏 **Test**。

**响应 `data`**：

```json
{
  "results": []
}
```

`results` 为 `TestResult[]`，顺序与请求 `ids` 一致。

---

## 调用节点

页面操作与 API 对应关系。

### 页面加载

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 Streams 页 | `GET` | `/groups/tree` |
| 进入 Streams 页 | `GET` | `/groups/map`（可选） |
| 进入 Streams 页 | `GET` | `/map`（可选） |
| 进入 Streams 页 | `GET` | `/?group_id={ALL_GROUP_ID}&page=1&page_size=20` |

### 左侧栏（Groups）

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 点击组名 | `GET` | `/?group_id={group_id}&page=1` |
| 点击树内流 Name | `GET` | `/?stream_id={stream_id}` |
| **Groups ↻ Refresh** | `GET` | `/groups/tree`；并 `GET /?...` 按当前筛选刷新表格 |
| ⋮ → Add Group → √ | `POST` | `/groups/{parent_group_id}` |
| ⋮ → Rename → √ | `PATCH` | `/groups/{group_id}` |
| ⋮ → Delete → 确认 | `DELETE` | `/groups/{group_id}` |
| ⋮ → Add Stream（打开弹窗） | `GET` | `/groups/{group_id}/candidates` |
| ⋮ → Add Stream（打开弹窗） | `GET` | `/groups/{group_id}/members` |
| Transfer List → Save | `PUT` | `/groups/{group_id}/members` |
| Transfer List Save 后刷新 | `GET` | `/groups/tree` |
| Transfer List Save 后刷新 | `GET` | `/?group_id=...`（当前筛选） |

### 右侧栏

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| Search 输入 | `GET` | `/?...&search={q}`（保留当前 `group_id` / `stream_id`） |
| 翻页 | `GET` | `/?...&page={n}` |
| 工具栏 Add → 打开弹窗 | `GET` | `/groups/list` |
| 工具栏 Add → Save | `POST` | `/` |
| 工具栏 Remove → 确认 | `POST` | `/batch/delete` |
| 工具栏 Enable | `POST` | `/batch/enable` |
| 工具栏 Disable | `POST` | `/batch/disable` |
| 工具栏 Test | `POST` | `/batch/test` |
| Operations ▶ / ■ | `PATCH` | `/{stream_id}`（`{ "enabled": ... }`） |
| Operations 🎥 | `PATCH` | `/{stream_id}`（`{ "recording": ... }`） |
| Operations ↻ | `POST` | `/{stream_id}/test` |
| Operations ✏ → 打开弹窗 | `GET` | `/{stream_id}`（可选） |
| Operations ✏ → 打开弹窗 | `GET` | `/groups/list` |
| Operations ✏ → Save | `PATCH` | `/{stream_id}` |
| Operations 📓 | `GET` | `/{stream_id}/logs` |
| Operations 🗑 → 确认 | `DELETE` | `/{stream_id}` |
| 弹窗 Test | `POST` | `/{stream_id}/test` |

### 变更后刷新

| 场景 | 建议刷新 |
|------|----------|
| 增删改组 | `GET /groups/tree`；`GET /groups/map`；`GET /groups/list`；当前列表 |
| 增删改流、批量操作、Test | 当前列表 `GET /`；`GET /map`（可选） |
| Transfer List Save | `GET /groups/tree` + 当前列表 |

---

## 典型流程

### 首次进入页面

```
GET /groups/tree
GET /groups/map
GET /map
GET /?group_id=00000000-0000-4000-8000-000000000001&page=1
```

### 点击子组（如 Entrance）

```
GET /?group_id={group_id}&page=1
```

### 新建流

```
GET /groups/list
POST /  { name, group_id, url, enabled, recording }
GET /groups/tree
GET /?...   # 当前筛选
```

### 组内分配流（Transfer List）

```
GET /groups/{group_id}/candidates
GET /groups/{group_id}/members
PUT /groups/{group_id}/members  { stream_ids: [...] }
GET /groups/tree
GET /?group_id={group_id}&page=1
```

### 删除组（含子组）

```
DELETE /groups/{group_id}
GET /groups/tree
GET /groups/map
GET /groups/list
GET /?group_id=ALL_GROUP_ID&page=1
```
