# Users（后端）

对应前端契约：[user_api.md](../frontend/user_api.md)。  
实现包：`servers/django/pages/users/`。前缀：`/ai_stream2/backend/users`。

---

## 代理的 API

无

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/permissions/catalog` | PermissionCatalog（Groups 矩阵） |
| `GET` | `/groups` | Group 列表 |
| `GET` | `/groups/{group_id}` | Group 详情（含 `permission_ids`） |
| `POST` | `/groups` | 新建 Group |
| `PATCH` | `/groups/{group_id}` | 改名 / **全量覆盖** `permission_ids` |
| `DELETE` | `/groups/{group_id}` | 删 Group（用户仅解绑） |
| `GET` | `/` | 用户分页列表 |
| `GET` | `/{user_id}` | 用户详情 |
| `POST` | `/` | 新建用户 |
| `PATCH` | `/{user_id}` | 更新用户（`group_ids` 全量覆盖） |
| `DELETE` | `/{user_id}` | 删用户（禁删自身） |
| `POST` | `/batch/delete` | 批量删（跳过当前用户） |
| `GET` | `/{user_id}/logs` | 用户审计分页 |

字段 / 错误码 / 固定段顺序以 [user_api.md](../frontend/user_api.md) 为准。

权限：`users.view_user` / `add_user` / `change_user` / `delete_user`；`users.view_group` / `add_group` / `change_group` / `delete_group`。壳层 `users.export_site_config` / `users.import_site_config` **不**进 Groups 矩阵（见 shell）。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | 自定义 `User`（UUID PK）；Django `Group` / `Permission`；本页审计表 |
| 分页 | `shared.pagination` → `{items,total,page,page_size}` |
| 鉴权 | Django session + `has_perm` |
| 审计 | `shared.audit` 写本页日志表（或经 shared 接口） |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| User / Group / Permission / M2M | **复用** Django auth |
| 密码哈希 | **复用** `set_password` |
| PermissionCatalog 结构 | **造**（`shared.permissions_catalog` + 本页 `CatalogService`） |
| 各业务 page 权限行 | **造**：各 page `Meta.permissions` 注册；`app_label.codename` 对齐前端（如 `streams.view_stream`） |
| 壳权 `export/import_site_config` | **造**：留在 `PermissionHost`（或 users 专用） |
| 用户审计持久化 | **造**（`UserAuditLog` + `UserAuditService`） |
| Site Config 切片 | **造**（groups + group↔perm；不含密码哈希） |
| 响应外壳 / 分页 | **复用** `shared.*` |

---

## 目录结构

```
servers/django/pages/users/
  apps.py
  urls.py                 # permissions/groups/batch 先于 {user_id}
  models.py               # User, PermissionHost, UserAuditLog；（可选）UuidGroup 映射
  serializers.py
  views.py
  services.py             # CatalogService, GroupService, UserService, UserAuditService
  permissions.py
  management/commands/    # ensure_seed_admin
  migrations/
```

禁止 import 其它业务 `pages.*`。`PermissionHost` **仅**承载壳权；业务 codename 归各 page 模型 `Meta.permissions`（迁移后 `has_perm("streams.view_stream")` 等）。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `User` | UUID PK；`AbstractUser`；`AUTH_USER_MODEL` |
| `PermissionHost` | 无数据行；仅 `export_site_config` / `import_site_config` |
| `UserAuditLog` | 按用户追加审计行；驱动 `last_action_*` |
| `CatalogService` | 组装 PermissionCatalog（modules/actions/`permission_id`/`codename`） |
| `GroupService` | list / get / create / patch（perm 全量替换）/ delete |
| `UserService` | list / crud / batch_delete；`group_ids` 全量替换；禁自删 |
| `UserAuditService` | `append(user, label, detail)`；供 login/shell/本页写 |
| Views + permission 类 | 薄封装 |

**Group 主键**：契约为 UUID。实现用自定义 `Group` 替代或旁路映射表，全项目统一；**禁止** API 混用 int/UUID。

---

## 数据库

### `users_user`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | 后端生成，不可改 |
| `username` | varchar | UNIQUE | UI Name |
| `password` | varchar | 非空 | 哈希；API **不**回传 |
| `is_active` | bool | 非空 | 默认 `true` |
| `is_superuser` | bool | 非空 | 默认 `false` |
| `last_login` | timestamptz | 可空 | Django 标准 |
| … | | | `AbstractUser` 其余字段按需 |

授权**只**经 `User.groups`；管理端**不**写 `user_permissions`。

### Group / Permission

Django `auth_group`、`auth_permission`、`auth_group_permissions`、`users_user_groups`（表名随实现）。`permission_ids` = Permission 整型 PK。

### `users_userauditlog`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | |
| `user_id` | UUID FK → User | 非空，索引 | 被审计用户 |
| `at` | timestamptz | 非空 | 默认 now |
| `label` | varchar | 非空 | 短标签（Last Action） |
| `detail` | text | 非空 | Log 行文案 |

列表 `last_action_at` / `last_action_label` = 该用户最新一条日志；无则 `null`。

---

## 权限注册（唯一真相）

| 来源 | `app_label.codename` 示例 |
|------|---------------------------|
| 各业务 page 模型 `Meta.permissions` | `streams.view_stream`、`events.view_event`、`previews.view_preview` … |
| users 默认 + 显式 group 权 | `users.view_user`、`users.view_group` … |
| `PermissionHost` | `users.export_site_config`、`users.import_site_config` |

`GET /permissions/catalog` **只**返回矩阵模块（不含 export/import）。`is_superuser=true` → 目录内全部权；Import/Apply 仍须 JWT（shell）。

过渡：现有把业务权挂在 `PermissionHost` 的种子须迁到各 page；文档与契约以 `streams.*` / `events.*` 等为准。

---

## 各请求写库明细

只列**写**。GET catalog / groups / users / logs：只读。

| 请求 | 表 | 写入 / 变更 |
|------|-----|-------------|
| `POST /groups` | Group + M2M | INSERT；可选初始 `permission_ids` |
| `PATCH /groups/{id}` | Group | UPDATE `name`（若有） |
| | `group_permissions` | 若 body 含 `permission_ids`：**全量替换** |
| `DELETE /groups/{id}` | Group | DELETE；用户 M2M 级联解绑 |
| `POST /` | User | INSERT；`set_password`；`groups.set(group_ids)` |
| | 审计 | Create |
| `PATCH /{user_id}` | User | 部分 UPDATE；有 `password` 则重置；有 `group_ids` 则全量替换 |
| | 审计 | Edit |
| `DELETE /{user_id}` | User | DELETE（禁当前用户） |
| | 审计 | 可选记在操作者 |
| `POST /batch/delete` | User | 逐条 DELETE；跳过当前用户 → `deleted_count` / `skipped_count` |

`is_superuser`：仅当前用户为 superuser 时可设 `true`；否则 `403`。可选：禁止降权/禁用导致系统无剩余 superuser。

Site Config **import**：重建 Group 与 group↔perm；**不**导入用户密码；用户↔组策略在切片 schema 中固定（默认只同步组与权限模板）。

---

## 调用顺序

```
GET catalog:  CatalogService.build → modules[]
POST group:   唯一 name → create → set permissions
PATCH group:  部分更新；permission_ids 全量替换
POST user:    唯一 username → create → set_password → groups.set
PATCH user:   部分更新；group_ids 全量替换
DELETE self:  400
batch/delete: 过滤当前 user_id → 删其余
GET logs:     UserAuditLog 按 at DESC 分页
```

登录审计：login 成功经 `UserAuditService`（shared 调用，不 import login 包反向依赖）。

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| login / shell | 会话与 `GET /me`；本页不建/销会话 |
| 各业务 page | 仅经 Permission 行；**不** import 其 models |
| `shared.permissions_catalog` | catalog 元数据与种子协调 |
| `shared.audit` / 本页日志表 | 审计落库 |
| Site Config | **注册**切片：groups + permissions 绑定 |
| JWT / age | 不在本页；壳层 Import/Apply |
