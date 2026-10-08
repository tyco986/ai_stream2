# Users API

管理端 Users 页与 RBAC 后端 API。UI 规格见 [user.md](./user.md)。登录见 [login_api.md](./login_api.md)。有效权限 / 登出 / Mode / Site Config Versions 见 [shell_api.md](./shell_api.md)。

本前缀负责：**用户 / Group / Permission 管理**、**用户审计**。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/users` |
| 路径前缀 | `/ai_stream2/backend/users` |

实现时须将固定段（`batch`、`groups`、`permissions`）注册在 `/{user_id}` 等动态段之前。

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

### 鉴权头

| 项 | 说明 |
|----|------|
| 会话 | 已登录请求携带全局约定的 Cookie / Authorization（与 [shell_api.md](./shell_api.md)、[login_api.md](./login_api.md) 相同） |

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |
| 403 | 已登录但缺 Permission；或非 superuser 尝试授予 superuser |

Site Config Versions **Import** / **Apply** 各须一张 JWT（`X-Auth-Ticket`）见 [shell_api.md](./shell_api.md)（Backup / Export **不要**票）。

### 字段语义

| 字段 | 说明 |
|------|------|
| `user_id` | 用户 **id**（UUID）；路径 `/{user_id}` 与 `User.id` 同值 |
| `group_id` | Group **id**（UUID）；路径 `/groups/{group_id}` 与 `Group.id` 同值 |
| `permission_id` | Django Permission 整型主键；或稳定字符串 key（实现若用 codename 作键，响应中仍同时给 `id` 与 `codename`） |
| `codename` | 完整权限名：`app_label.codename`（如 `users.view_user`） |

### Django 映射

| API | Django |
|-----|--------|
| User | `django.contrib.auth.models.User`（或自定义 User；字段对齐本文） |
| Group | `django.contrib.auth.models.Group` |
| Permission | `django.contrib.auth.models.Permission` |
| 用户↔组 | `User.groups` M2M |
| 组↔权限 | `Group.permissions` M2M |

用户**默认不**直接写 `user.user_permissions`；管理端只通过 Group 授权。

---

## 权限目录（PermissionCatalog）

后端维护产品权限集（`post_migrate` 注册自定义 Permission 或使用模型默认权限）。Groups 弹窗右侧矩阵由 catalog 驱动。

### 模块与动作

| `module` | 说明 | 动作列 |
|----------|------|--------|
| `streams` | Streams 页 | `view` / `add` / `change` / `delete` |
| `previews` | Previews 页 | 同上 |
| `recordings` | Recordings 页 | 同上 |
| `models` | Models 页 | 同上 |
| `pipelines` | Pipelines 页 | 同上 |
| `servers` | Servers 页 | `view` / `change`（无 add/delete 行内资源时列可禁用） |
| `events` | Events 页 | `view` / `change` |
| `users` | Users / Groups | `view` / `add` / `change` / `delete` |

Groups 弹窗矩阵列仅为上表动作；壳层 Site Config 的 `users.export_site_config` / `users.import_site_config` **不**进本矩阵（见 [shell_api.md](./shell_api.md)）。

每个动作对应一个 `codename`，推荐命名：

| 示例 codename | 含义 |
|---------------|------|
| `users.view_user` | 查看用户列表 / Log |
| `users.add_user` | 创建用户 |
| `users.change_user` | 编辑用户 |
| `users.delete_user` | 删除用户 |
| `users.view_group` | 打开 Groups 弹窗 |
| `users.add_group` | 新建 Group |
| `users.change_group` | 改名 / 改 Group 权限 |
| `users.delete_group` | 删除 Group |
| `users.export_site_config` | Export 配置包（壳层；不在 Groups 矩阵） |
| `users.import_site_config` | Import 配置包（壳层；不在 Groups 矩阵） |
| `events.view_event` | 查看事件 / Preview / Export |
| `events.change_event` | Ack / Collect |
| `streams.view_stream` | … | 其它模块同理 |

`is_superuser=true` 视为拥有目录内全部权限。

### PermissionCatalog 响应形

```json
{
  "modules": [
    {
      "key": "streams",
      "label": "Streams",
      "actions": [
        {
          "key": "view",
          "label": "view",
          "permission_id": 11,
          "codename": "streams.view_stream"
        },
        {
          "key": "add",
          "label": "add",
          "permission_id": 12,
          "codename": "streams.add_stream"
        }
      ]
    }
  ]
}
```

前端按 `modules[].actions[]` 渲染勾选格；**Save** 只提交勾选的 `permission_id` 列表。

---

## 数据模型

### User

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "username": "alice",
  "is_active": true,
  "is_superuser": false,
  "groups": [
    {
      "id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
      "name": "admin"
    }
  ],
  "last_action_at": "2026-07-22T10:15:00Z",
  "last_action_label": "Edit"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成，不可改 |
| `username` | string | 否 | 唯一；对应 UI **Name** |
| `is_active` | bool | 否 | 对应 Active |
| `is_superuser` | bool | 否 | 对应 Superuser |
| `groups` | object[] | 否 | `{ id, name }[]`；可空数组 |
| `last_action_at` | datetime | 是 | 最近审计时间 |
| `last_action_label` | string | 是 | 短标签；与 `last_action_at` 同有同无 |

列表与详情均**不**返回密码哈希。

### Group

```json
{
  "id": "a1b2c3d4-e5f6-4789-a012-3456789abcde",
  "name": "admin",
  "permission_ids": [11, 12, 21]
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 后端生成（若沿用 Django 整型 id，则类型为 int，全项目统一；**本文以 UUID 为准**，实现可用 Char UUID 主键或映射表） |
| `name` | string | 否 | 唯一 |
| `permission_ids` | int[] | 否 | 已授权 Permission 主键列表 |

> 若实现坚持 Django 默认整型 `Group.id` / `User.id`，则本文所有 UUID 改为 int，并在 OpenAPI 中统一；UI 仍不展示 id。

### UserLogEntry

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "at": "2026-07-22T10:15:00Z",
  "label": "Edit",
  "detail": "Edit user"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `at` | datetime | 时间 |
| `label` | string | 短标签（供 Last Action） |
| `detail` | string | Log 弹窗行文案 |

---

## 端点

### 权限目录

#### `GET /permissions/catalog`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/permissions/catalog`

**说明**：Groups 弹窗右侧矩阵。权限：`users.view_group`。

**响应 `data`**：`PermissionCatalog`（见上）。

---

### Groups

#### `GET /groups`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/groups`

**说明**：Groups 弹窗左侧列表；User 弹窗 Groups 多选选项。权限：`users.view_group` 或 `users.view_user`（选项读取）。

**响应 `data`**：

```json
{
  "items": []
}
```

`items` 为 `Group[]`（可含 `permission_ids`；列表若嫌重可省略，详情再取）。

---

#### `GET /groups/{group_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/groups/{group_id}`

**响应 `data`**：`Group`（含完整 `permission_ids`）。

| HTTP | 场景 |
|------|------|
| 404 | Group 不存在 |

---

#### `POST /groups`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/groups`

**权限**：`users.add_group`

**请求体**：

```json
{
  "name": "operator",
  "permission_ids": []
}
```

**响应 `data`**：创建后的 `Group`。

| HTTP | 场景 |
|------|------|
| 400 | name 空或重名 |

---

#### `PATCH /groups/{group_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/groups/{group_id}`

**权限**：`users.change_group`

**请求体**（字段均可选）：

```json
{
  "name": "operator",
  "permission_ids": [11, 12, 21]
}
```

**说明**：`permission_ids` 若出现则为**全量覆盖**（与 UI Save 一致）。

**响应 `data`**：更新后的 `Group`。

---

#### `DELETE /groups/{group_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/groups/{group_id}`

**权限**：`users.delete_group`

**行为**：删除 Group；用户 M2M 解除；**不**删除用户。

| HTTP | 场景 |
|------|------|
| 404 | 不存在 |

---

### Users · 列表与 CRUD

#### `GET /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users`

**权限**：`users.view_user`

**Query**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `search` | string | 按 username 过滤（可选） |
| `page` | int | 默认 `1` |
| `page_size` | int | 默认 `20` |

**响应 `data`**：分页，`items` 为 `User[]`。

---

#### `GET /{user_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/{user_id}`

**权限**：`users.view_user`

**响应 `data`**：`User`。

| HTTP | 场景 |
|------|------|
| 404 | 不存在 |

---

#### `POST /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users`

**权限**：`users.add_user`

**请求体**：

```json
{
  "username": "dave",
  "password": "••••••••",
  "is_active": true,
  "is_superuser": false,
  "group_ids": ["a1b2c3d4-e5f6-4789-a012-3456789abcde"]
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| `username` | 是 | 唯一 |
| `password` | 是 | |
| `is_active` | 否 | 默认 `true` |
| `is_superuser` | 否 | 默认 `false`；仅当前用户为 superuser 时可 `true` |
| `group_ids` | 否 | 默认 `[]` |

**响应 `data`**：创建后的 `User`。

| HTTP | 场景 |
|------|------|
| 400 | 校验失败 / 重名 |
| 403 | 非 superuser 设置 `is_superuser` |

---

#### `PATCH /{user_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/{user_id}`

**权限**：`users.change_user`

**请求体**（皆可选）：

```json
{
  "username": "alice",
  "password": "new-secret",
  "is_active": true,
  "is_superuser": false,
  "group_ids": ["a1b2c3d4-e5f6-4789-a012-3456789abcde"]
}
```

| 字段 | 说明 |
|------|------|
| `password` | 出现则重置密码；省略则不变 |
| `group_ids` | 出现则为 Group 成员**全量覆盖** |

**响应 `data`**：更新后的 `User`。

| HTTP | 场景 |
|------|------|
| 403 | 非法提升 superuser；或禁用/降权导致系统无剩余 superuser（可选保护） |
| 404 | 不存在 |

---

#### `DELETE /{user_id}`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/{user_id}`

**权限**：`users.delete_user`

**行为**：不可删除当前登录用户 → `400`。

| HTTP | 场景 |
|------|------|
| 400 | 删除自身 |
| 404 | 不存在 |

---

#### `POST /batch/delete`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/batch/delete`

**权限**：`users.delete_user`

**请求体**：

```json
{
  "user_ids": [
    "550e8400-e29b-41d4-a716-446655440000",
    "7c9e6679-7425-40de-944b-e07fc1f90ae7"
  ]
}
```

**行为**：跳过当前用户 id；响应标明实际删除数与跳过数。

**响应 `data`**：

```json
{
  "deleted_count": 1,
  "skipped_count": 1
}
```

---

### 用户审计

#### `GET /{user_id}/logs`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/users/{user_id}/logs`

**权限**：`users.view_user`

**Query**：`page`、`page_size`（可选）。

**响应 `data`**：分页，`items` 为 `UserLogEntry[]`，默认新在前。

写用户 / 登录 / Import / Export 等时由后端追加日志，并更新该用户 `last_action_*`。

---

## 错误码

| HTTP | 场景 |
|------|------|
| 400 | 校验失败；删除自身 |
| 401 | 未登录 |
| 403 | 缺 Permission；非 superuser 授予 superuser |
| 404 | user / group 不存在 |

---

## 调用节点

### Users 页

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 Users | `GET` | `/users/?page=&page_size=` |
| **Refresh** | `GET` | 同上 |
| **Add** Save | `POST` | `/users/` |
| **✏** Save | `PATCH` | `/users/{user_id}` |
| 行内 **🗑** | `DELETE` | `/users/{user_id}` |
| 工具栏 **Remove** | `POST` | `/users/batch/delete` |
| **📓** Log | `GET` | `/users/{user_id}/logs` |
| 打开 **Groups** | `GET` | `/users/groups` + `/users/permissions/catalog` |
| 选中 Group | `GET` | `/users/groups/{group_id}`（若列表未带 permissions） |
| Groups **Save** | `PATCH` | `/users/groups/{group_id}`（含 `permission_ids`） |
| Groups **+** | `POST` | `/users/groups` |
| Groups 删 | `DELETE` | `/users/groups/{group_id}` |

### 不经本 API

| 调用节点 | 说明 |
|----------|------|
| 登录（建会话） | [login_api.md](./login_api.md) |
| 有效权限 / 登出 | [shell_api.md](./shell_api.md) `GET /me`、`POST /logout` |
| Page Settings **Mode** Apply | [shell_api.md](./shell_api.md) `PUT /settings` |
| Site Config Versions / 授权票 | [shell_api.md](./shell_api.md) |
| 发镜像 / 代码更新 | 线下交付镜像；不经配置包 |

---

## 典型流程

### 打开 Groups 并保存权限

```
GET /users/permissions/catalog
GET /users/groups
GET /users/groups/{group_id}
# UI 勾选后
PATCH /users/groups/{group_id}
  { "permission_ids": [11, 12, 21] }
```
