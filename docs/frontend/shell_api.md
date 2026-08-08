# Shell API

管理端 App Shell 后端 API。UI 规格见 [shell.md](./shell.md)。

本前缀负责：**当前用户 / 有效权限**（`SessionUser`）、**登出**、**页面配置 Mode**、**站点配置版本**（备份 / 导入落盘 / 导出 / 应用；仅数据，不含代码）。导航项顺序与路由为前端固定，**不**经本 API 下发。UI：Logo+Version 进版本管理；Page Settings **仅** Mode。

登录（用户名密码建会话）见 [login_api.md](./login_api.md)。Users / Group / Permission 见 [user_api.md](./user_api.md)。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/shell` |
| 路径前缀 | `/ai_stream2/backend/shell` |

实现时须将固定段（`me`、`logout`、`settings`、`site-config`）注册在可能的动态路径之前。

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

### 枚举与常量

| 名称 | 值 | 说明 |
|------|-----|------|
| `mode` | `sidebar` \| `topbar` | 对应 UI **侧栏** / **顶栏** |
| `DEFAULT_MODE` | `sidebar` | 无已存配置时的默认 Mode |
| `DEFAULT_SITE_CONFIG_VERSION` | `"0"` | **dev/prod** 镜像构建时即存在的出厂配置版本；`is_current=true` |

### 字段语义

| 字段 | 说明 |
|------|------|
| `username` | 当前登录用户展示名；对应栏尾用户名 |
| `mode` | 壳布局 Mode；对应 Page Settings **Mode** 下拉 |
| `user_id` | 用户 **id**（UUID）；后端主键；UI **不**展示 |
| `auth_ticket` | 厂商签发的 JWT；请求头 `X-Auth-Ticket`。Import / Apply **各需一张**（`action` 分别为 `import` / `apply`） |

### 持久化边界

| 写入 API | 内容 |
|----------|------|
| `PUT /settings` | **Apply** 时写入当前用户的页面配置（目前仅 `mode`） |
| `POST /site-config/import` | 覆盖业务配置数据（配置包）；**不**改 Mode |

**不**经本 API 持久化：当前导航高亮、内容区业务页会话态、弹窗内未 Apply 的 Mode 草稿、未登录时的临时 Mode。

### 鉴权

| 场景 | 行为 |
|------|------|
| 已登录 | `GET /me`、`POST /logout`、`GET/PUT /settings`、Site Config 按当前会话用户 |
| 未登录 | `GET /me` → `401`；栏尾用户名展示 `—`；`GET/PUT /settings` → `401`；**Apply** 仅会话内生效，**不**写后端；Import/Export 不可用 |
| Step-up | Versions **Import** / **Apply**（及 `POST /site-config/import`）须有效 JWT（`X-Auth-Ticket`），且 `action` 与操作匹配；缺失/无效 → `401`。Backup / Export **不要**票 |
| 会话 | Cookie / 服务端 session；未登录 → `401` |

鉴权会话机制（Cookie / Token）与 [login_api.md](./login_api.md) 登录约定一致。

---

## 数据模型

### SessionUser（当前用户 / 有效权限）

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "username": "alice",
  "is_superuser": false,
  "permissions": [
    "users.view_user",
    "streams.view_stream",
    "streams.change_stream"
  ]
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 用户 id |
| `username` | string | 否 | 展示名；栏尾文本 |
| `is_superuser` | bool | 否 | `true` 视为拥有全部权限 |
| `permissions` | string[] | 否 | 有效 `codename` 列表；superuser 可为 `["*"]` 或展开全量（实现二选一，文档推荐展开或 `*`） |

权限目录与 Group 授权见 [user_api.md](./user_api.md) **PermissionCatalog**。

### PageSettings

```json
{
  "mode": "sidebar"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `mode` | string | 否 | `sidebar` \| `topbar` |

按**用户**隔离；换账号互不影响。

### SiteConfigManifest

配置包内清单（逻辑模型；明文 zip 内嵌）。

```json
{
  "schema_version": 3,
  "app_version": "1.2.0",
  "site_id": "site-prod-01",
  "exported_at": "2026-07-22T10:15:00Z",
  "checksum": "sha256:…"
}
```

| 字段 | 说明 |
|------|------|
| `schema_version` | 配置结构版本；与当前镜像不匹配则 Import **拒绝** |
| `app_version` | 导出时应用版本（信息性） |
| `site_id` | 可选站点标识 |
| `checksum` | 包完整性 |

包内业务载荷为领域配置（streams / groups / 权限模板等），**不是**数据库文件整库拷贝。

### 配置包加密（age）

| 项 | 约定 |
|----|------|
| 库 | **age**（X25519）；CLI 或兼容库 |
| 明文 | 内存 zip：`manifest` + 各 page 切片 |
| 产物 | 密文字节（`.age` / `.bin`） |
| Backup Save（本机落盘） | age 封给**本站**公钥 → 仅本站 `site.key` 可解 |
| Export（给厂商） | age 封给**厂商** `dev.pub` → 仅厂商 `dev.key` 可解；本地世代需先用本站私钥解开再重封 |
| Import / 厂商回包 | 文件须为本站收件人；用本站 `site.key` 解密 |
| 钥匙放置 | 见下表；细则与生成容器见 [backend/shell.md](../backend/shell.md)「钥匙」 |

| 路径 | 内容 | 进 django 镜像 |
|------|------|----------------|
| `/app/keys/dev.pub` | 厂商 age 公钥 | 是 |
| `/app/keys/ticket.pub` | 厂商 JWT 验签公钥 | 是 |
| `/secrets/age/site.key` | 本站 age 私钥 | 否（卷；django / keys-init 首次生成） |
| `/secrets/age/site.pub` | 本站 age 公钥 | 否（卷） |

厂商私钥 `dev.key` / `ticket.key` 仅厂商本机。公钥可公开、不能解密。工程师只搬密文 + 两张 JWT。nodejs **不**挂载本站私钥。

### AuthTicket（JWT）

Import 与 Apply **各一张**；不可混用。

```json
{
  "action": "import",
  "pkg_hash": "a1b2c3…",
  "site_id": "site-prod-01",
  "exp": 1780000000,
  "jti": "7c9e6679-7425-40de-944b-e07fc1f90ae7"
}
```

| Claim | 必填 | 说明 |
|-------|------|------|
| `action` | 是 | `import` \| `apply`；须与本次 API 一致 |
| `pkg_hash` | 是 | 密文 `SHA256` 小写 hex。Import = 请求体文件；Apply = 该版 `payload_path` 文件；二者通常同包同 hash |
| `site_id` | 否 | 有则须等于本站 |
| `exp` | 是 | 过期 |
| `jti` | 是 | 全局唯一；校验成功后记已用，重复 → `401` |

| 项 | 约定 |
|----|------|
| 算法 | **EdDSA**（Ed25519） |
| 请求头 | `X-Auth-Ticket: <jwt>` |
| 签发 | 厂商 `ticket.key` |
| 验签 | 镜像 `/app/keys/ticket.pub` |
| 与 age | 无关；验票通过后再用 `site.key` 解密 |

### SiteConfigVersion

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "version": "3",
  "description": "after streams update",
  "created_at": "2026-07-25T10:00:00Z",
  "is_current": true
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 快照主键 |
| `version` | string | 否 | 数字，可含小数（`/^\d+(\.\d+)?$/`，如 `"0"`、`"1.5"`）；全库唯一；镜像构建自带 `"0"` |
| `description` | string | 是 | 备份说明；出厂行可为 `image default` 等 |
| `created_at` | datetime | 否 | 创建时间；出厂行可为构建/种子时间 |
| `is_current` | bool | 否 | 当前已应用快照；至多一行 `true`；出厂时版本 `0` 为 `true` |

载荷：卷上 `payload_path` 指向 **age 密文**（封给本站）。**dev/prod** 镜像构建时写入版本 `0` 的默认 payload（及对应 DB 种子），故部署后无需先 Backup 才有版本。

---

## 端点

### 当前用户 / 登出

#### `GET /me`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/me`

**说明**：进入管理端壳时拉取栏尾用户名、有效权限（驱动导航显隐）。改自身 Group 后须重新拉取。

**响应 `data`**：`SessionUser`。

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |

---

#### `POST /logout`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/logout`

**说明**：清除会话。UI 见 [shell.md](./shell.md) 用户区；成功后前端跳转 [login.md](./login.md)。

**响应 `data`**：`{}`。

---

### 页面配置（Mode）

#### `GET /settings`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/settings`

**说明**：进入壳时读取已应用配置；打开 Page Settings 时用于填充表单草稿。无记录时返回 `{ "mode": "sidebar" }`（`DEFAULT_MODE`），**不**强制先写库。

**响应 `data`**：`PageSettings`。

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |

---

#### `PUT /settings`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/settings`

**说明**：Page Settings 点击 **Apply** 时调用；将 Mode 草稿覆盖为当前用户已应用配置。**不**要求授权票。

**请求体**：

```json
{
  "mode": "topbar"
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| `mode` | 是 | `sidebar` \| `topbar` |

**响应 `data`**：更新后的 `PageSettings`。

| HTTP | 场景 |
|------|------|
| 400 | `mode` 非枚举值 |
| 401 | 未登录 |

---

### Site Config

#### `POST /site-config/export`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/export`

**说明**：导出**当前运行中**配置：明文 zip → age 封给**厂商** `dev.pub` → 密文文件流（API 保留；壳 UI 以版本行内 Export 为主）。权限：`users.export_site_config`。须登录。**不要**授权票、**不**填密码。

**请求体**：无（或 `{}`）。

**响应**：文件流（`application/octet-stream`，`.age` / `.bin`）。成功写用户审计 `Export`。

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |
| 403 | 缺权限 |
| 500 | 缺少 age 钥匙文件等 |

---

#### `POST /site-config/import`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/import`

**说明**：**直接**将外部 age 密文包（本站收件人）解密并写入运行中配置（不经版本表）。权限：`users.import_site_config` + `X-Auth-Ticket`。壳 UI **不**再暴露此入口（改走 Versions）。`multipart/form-data` 字段名 `file`。

**行为**：校验 JWT（`action=import`，`pkg_hash`=文件 hash）→ 本站私钥解密 → 校验 `schema_version` → 事务写入 → 消耗 `jti` → 写审计。**不**改 `mode`。

**响应 `data`**：`{ "schema_version": 3, "applied": true }`。

| HTTP | 场景 |
|------|------|
| 400 | 解密失败、包损坏、`schema_version` 不匹配 |
| 401 | 未登录或 JWT 无效（含 `action`/`pkg_hash`/`jti`/`exp`） |
| 403 | 缺权限 |

---

### Site Config Versions

#### `GET /site-config/versions`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/versions`

**说明**：版本管理表格；Logo 旁 Version 取 `is_current: true` 的 `version`（出厂为 `"0"`）。权限：`users.export_site_config`（或 import 权亦可；实现以 export 为准）。列表**至少**含镜像种子行 `version=0`。

**响应 `data`**：

```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "version": "0",
      "description": "image default",
      "created_at": "2026-07-01T00:00:00Z",
      "is_current": true
    }
  ]
}
```

建议按 `created_at` 降序（或 `version` 数值降序）。

---

#### `POST /site-config/versions`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/versions`

**说明**：Backup **Save**——当前运行配置 → 明文 zip → age 封给**本站**公钥 → 密文落盘。权限：`users.export_site_config`。**不要**授权票。**不**自动 `is_current=true`。

**请求体**：

```json
{
  "version": "4",
  "description": "manual backup"
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| `version` | 是 | `/^\d+(\.\d+)?$/`；唯一；**不能**与已有号冲突（含出厂 `"0"`） |
| `description` | 否 | 说明 |

**响应 `data`**：新建的 `SiteConfigVersion`（`is_current: false`）。

| HTTP | 场景 |
|------|------|
| 400 | `version` 格式非法或重名 |
| 401 | 未登录 |
| 403 | 缺权限 |
| 500 | 缺少本站公钥等 |

---

#### `POST /site-config/versions/{version_id}/import`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/versions/{version_id}/import`

**说明**：Versions **Import**——用厂商打回的 age 密文包（本站收件人）替换已有快照 payload（`version` / `description` 不变）。权限：`users.import_site_config` + `X-Auth-Ticket`。`multipart/form-data` 仅 `file`。

**行为**：

1. 校验 JWT：`action=import`，`pkg_hash` = 上传文件 SHA256；权限与版本存在  
2. 本站私钥解密；失败 → `400`  
3. 校验内层 zip 的 `schema_version`  
4. 覆盖该行密文 `payload_path`（仍封本站）  
5. 消耗 `jti`；**不**改 `is_current`；写审计  

**响应 `data`**：该行 `SiteConfigVersion`。

| HTTP | 场景 |
|------|------|
| 400 | 解密失败、包损坏、`schema_version` 不匹配 |
| 401 | 未登录或 JWT 无效 |
| 403 | 缺权限 |
| 404 | 版本不存在 |

---

#### `POST /site-config/versions/{version_id}/export`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/versions/{version_id}/export`

**说明**：行内 **Export**——将该版本地（本站收件人）密文用本站私钥解开，再 age 封给**厂商** `dev.pub` 后下载。权限：`users.export_site_config`。**不要**授权票。

**响应**：文件流 `application/octet-stream`（`.age` / `.bin`）。

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |
| 403 | 缺权限 |
| 404 | 版本不存在 |
| 500 | 缺少钥匙文件 |

---

#### `POST /site-config/versions/{version_id}/apply`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/site-config/versions/{version_id}/apply`

**说明**：版本管理 **Apply**。权限：`users.import_site_config` + `X-Auth-Ticket`。**不**改 `mode`。成功后 Logo 旁 Version 变为该行 `version`。

**行为**：校验 JWT：`action=apply`，`pkg_hash` = 该行 payload 文件 SHA256（**另一张票**，不能复用 import 票）→ 若已是当前则 `400` → 本站私钥解密 → 写运行配置 → 更新 `is_current` → 消耗 `jti` → 审计。

**响应 `data`**：

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "version": "2",
  "applied": true
}
```

| HTTP | 场景 |
|------|------|
| 400 | 已是当前版本；解密失败；快照损坏 / `schema_version` 不匹配 |
| 401 | 未登录或 JWT 无效 |
| 403 | 缺权限 |
| 404 | 版本不存在 |

---

## 错误码

| HTTP | 场景 |
|------|------|
| 400 | `Invalid mode`；解密失败；配置包校验 / `schema_version` 不匹配；`version` 格式非法或重名；Apply 当前版本 |
| 401 | 未登录；JWT 验签失败 / 过期 / `action` 不符 / `pkg_hash` 不符 / `jti` 已用 / `site_id` 不符 |
| 403 | 缺 Import/Export 权限 |
| 404 | 版本 id 不存在 |
| 500 | age / `ticket.pub` 缺失或不可读 |

---

## 调用节点

### 壳加载 / 登出

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入管理端 | `GET` | `/me` |
| 进入管理端 | `GET` | `/settings`（可与 `/me` 并行） |
| **Logout** | `POST` | `/logout` |

### 页面配置弹窗 · Mode

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 打开齿轮弹窗 | — | 用已应用 `mode` 填草稿；或再 `GET /settings` |
| **Mode** 下拉改值 | — | 仅改弹窗草稿；**不**调 API |
| **Cancel** / `×` / Esc | — | 丢弃 Mode 草稿；关弹窗；不调 API |
| **Apply**（已登录） | `PUT` | `/settings`；成功后应用壳并关弹窗 |
| **Apply**（未登录） | — | 仅会话内应用草稿；不调 `PUT /settings` |

### Logo + Version · 版本管理

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进壳 / 刷新 Version | `GET` | `/site-config/versions`（取 `is_current`） |
| 点击 Logo+Version | `GET` | `/site-config/versions` → 打开弹窗 |
| Backup **Save** | `POST` | `/site-config/versions` |
| Versions **Import** | `POST` | `/site-config/versions/{version_id}/import`（file + import JWT） |
| 行内 **Export** | `POST` | `/site-config/versions/{version_id}/export` |
| Versions **Apply** | `POST` | `/site-config/versions/{version_id}/apply`（**另一张** apply JWT） |

### 不经本 API

| 调用节点 | 说明 |
|----------|------|
| 点击导航项 | 前端路由 |
| 登录（用户名密码） | [login_api.md](./login_api.md) `POST /` |
| Users 页 CRUD | [user_api.md](./user_api.md) |
| 发镜像 / 代码更新 | 线下交付；不经配置包 |
| JWT 签发 | 厂商侧：对密文算 `pkg_hash`，分别签 `action=import` 与 `action=apply` 两张票 |

---

## 典型流程

### 首次进入管理端（已登录）

```
GET /me
  → SessionUser → 栏尾 username；permissions 驱动导航显隐

GET /settings
  → mode → 渲染侧栏或顶栏壳
```

### 登出

```
POST /logout
  → 清会话；前端跳转 Login 页
```

### 切换 Mode（Apply）

```
PUT /settings
  { "mode": "topbar" }
  → 成功后切换壳布局并关弹窗
```

### Export 配置包（API；壳 UI 以行内 Export 为主）

```
POST /site-config/export
  → age 封厂商公钥 → 密文下载
```

### 版本管理 · Save / Import / Apply

```
GET /site-config/versions
  → { items }；Logo 旁显示 is_current.version

POST /site-config/versions
  { "version": "4", "description": "manual backup" }
  → zip → age 封本站 → 落盘；is_current: false

POST /site-config/versions/{id}/import
  multipart file=…（厂商打回、本站收件人）
  Header X-Auth-Ticket: <jwt action=import>
  → 验 JWT → 本站私钥解密 → 覆盖 payload；消耗 jti

POST /site-config/versions/{id}/export
  → 本站解本地包 → age 封厂商 → 文件流

POST /site-config/versions/{id}/apply
  Header X-Auth-Ticket: <jwt action=apply>（另一张）
  → 验 JWT（pkg_hash=该行 payload）→ 解密 → 写运行配置；消耗 jti
```

### 未登录

```
GET /me → 401；栏尾「—」
GET /settings → 401；已应用 Mode = sidebar（DEFAULT_MODE）
# 齿轮可开；Apply 仅会话内生效；Import/Export 不可用
```
