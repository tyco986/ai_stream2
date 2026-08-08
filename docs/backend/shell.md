# Shell（后端）

对应前端契约：[shell_api.md](../frontend/shell_api.md)。  
实现包：`servers/django/pages/shell/`。前缀：`/ai_stream2/backend/shell`。

---

## 代理的 API

无

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/me` | `SessionUser`（username、permissions） |
| `POST` | `/logout` | 清会话 |
| `GET` | `/settings` | 读用户 `mode`；无记录 → `sidebar` |
| `PUT` | `/settings` | 持久化 `mode` |
| `POST` | `/site-config/export` | 当前配置 age 封 `dev.pub` 后下载（export 权；不要票） |
| `POST` | `/site-config/import` | 解密写入运行配置（须 JWT `action=import`；API 保留） |
| `GET` | `/site-config/versions` | 版本列表 |
| `POST` | `/site-config/versions` | Backup：age 封本站公钥落盘（不要票） |
| `POST` | `/site-config/versions/{id}/import` | 回包换 payload（须 JWT `action=import`） |
| `POST` | `/site-config/versions/{id}/export` | 解本站密文 → 重封 `dev.pub` → 下载 |
| `POST` | `/site-config/versions/{id}/apply` | 解密应用（须 **另一张** JWT `action=apply`） |

字段 / 错误码 / 流程以 [shell_api.md](../frontend/shell_api.md) 为准。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| 会话 | Django session（Cookie；与 login 一致） |
| ORM | Django ORM；本页表：`PageSettings`、`SiteConfigVersion`、`UsedAuthTicket` |
| 配置包 | `zipfile` + `json` + checksum；落盘/下载前 **age** |
| age | CLI 或兼容库；镜像 `dev.pub`；卷 `site.key` |
| JWT | `PyJWT`；算法 **EdDSA**（Ed25519）；镜像 `ticket.pub` 验签 |
| 权限 | Django `User` / `Permission` / `user.get_all_permissions()` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| Session / User / has_perm | **复用** Django auth |
| age 加解密 | **复用** age CLI 或兼容库 |
| JWT 验签 | **复用** `PyJWT` |
| 响应外壳 `{success,message,data}` | **造**（`shared.http`） |
| 票业务校验（action / pkg_hash / jti） | **造**（`AuthTicketService`） |
| Site Config 包格式 / registry | **造**（`shared.site_config`；age 可放 shared） |
| PermissionCatalog | **造**（`shared.permissions_catalog`） |
| 用户审计 | **造**（经 shared；不直连 `pages.users` models） |

---

## 目录结构

```
servers/django/
  pages/shell/
    apps.py
    urls.py
    models.py              # PageSettings, SiteConfigVersion, UsedAuthTicket
    serializers.py
    views.py
    services.py            # SessionUserService, SettingsService, AuthTicketService, SiteConfigOrchestrator
    permissions.py
    migrations/
  shared/
    http/
    auth/
    permissions_catalog/
    site_config/           # registry + age seal/open
```

禁止：`pages.shell` import 其它 `pages.*`。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `PageSettings` | `user_id` + `mode` |
| `SiteConfigVersion` | 版本元数据 + `payload_path` |
| `UsedAuthTicket` | 已消耗 `jti` |
| `SessionUserService` | `get_session_user(user)` |
| `SettingsService` | `get_mode` / `set_mode` |
| `AuthTicketService` | `validate(jwt, action, pkg_hash, site_id)` → 验签 + claims + 记 `jti` |
| `AgeCrypto` | seal / open（读钥匙路径） |
| `SiteConfigOrchestrator` | export / import / versions + 审计 |
| Views + permission 类 | 薄封装 |

---

## 数据库

本页表：`shell_pagesettings`、`shell_siteconfigversion`、`shell_usedauthticket`。

**不**由本页建表、但会读写：`django_session`、`auth_user`（只读）。Import / Apply 写各业务 page 表（经 `shared.site_config`）；审计经 shared。

### 钥匙（完整约定）

两套独立钥匙，**不可混用**：

| 套 | 算法 | 用途 |
|----|------|------|
| **age** | X25519（`age-keygen`） | 配置包封包 / 解包 |
| **ticket** | Ed25519（JWT `EdDSA`） | 授权票签发 / 验签 |

**禁止**：在 **nodejs** 容器生成或挂载本站私钥；前端只调 API。现场加解密与验票只在 **django**（或短命 `keys-init` 挂同一卷）。

#### 文件清单与容器内路径

| 文件 | 角色 | 容器内路径 | 进镜像？ |
|------|------|------------|----------|
| `dev.pub` | 厂商 age 公钥 | `/app/keys/dev.pub` | **是**（`COPY`） |
| `ticket.pub` | 厂商 JWT 验签公钥 | `/app/keys/ticket.pub` | **是**（`COPY`） |
| `site.key` | 本站 age identity（含私钥） | `/secrets/age/site.key` | **否**（卷） |
| `site.pub` | 本站 age 公钥 | `/secrets/age/site.pub` | **否**（卷；可由 `site.key` 导出） |
| `dev.key` | 厂商 age 私钥 | **不进现场** | — |
| `ticket.key` | 厂商 JWT 签名私钥 | **不进现场** | — |

dev / prod 镜像路径相同；差异仅在构建时拷入的公钥是否为正式厂商钥。

#### 谁在哪生成

| 钥匙 | 生成位置 | 命令（示例） | 产物去向 |
|------|----------|--------------|----------|
| `dev.key` / `dev.pub` | **厂商本机**（一次） | `age-keygen -o dev.key` → `age-keygen -y dev.key > dev.pub` | 私钥仅厂商保管；`dev.pub` 纳入 django 镜像构建上下文后 `COPY` 到 `/app/keys/dev.pub` |
| `ticket.key` / `ticket.pub` | **厂商本机**（一次） | `openssl genpkey -algorithm Ed25519 -out ticket.key` → `openssl pkey -in ticket.key -pubout -out ticket.pub` | 私钥仅厂商保管；`ticket.pub` → 镜像 `/app/keys/ticket.pub` |
| `site.key` / `site.pub` | **现场出厂 / 首次部署** | 见下「本站钥初始化」 | 写入卷 `/secrets/age/`；`site.pub` 拷贝给厂商本机 `sites/<site_id>.pub` |

#### 本站钥初始化（容器）

| 项 | 约定 |
|----|------|
| 推荐载体 | **django** 镜像内脚本，或同网络短命 **`keys-init`** 容器（基于同一镜像 / 含 `age`） |
| 触发 | 出厂或首次 `run`：若 `/secrets/age/site.key` **不存在**则生成；已存在则 **跳过**（勿覆盖） |
| 卷 | 宿主机目录（如 `${PROJECT_ROOT}/secrets/age`）→ 容器 `/secrets/age` |
| 生成 | `age-keygen -o /secrets/age/site.key`；`age-keygen -y /secrets/age/site.key > /secrets/age/site.pub`；`chmod 600 site.key` |
| 与 django 启动 | migrate 前或入口脚本中调用；缺失 `site.key` 时 Backup/Import/Export/Apply **失败**并明确错误 |
| 不生成 | 厂商 `dev.*` / `ticket.*`（现场无私钥）；公钥只读镜像内文件 |

#### 镜像构建（公钥）

django `Dockerfile`（dev / prod 同约定）：

```dockerfile
COPY keys/dev.pub /app/keys/dev.pub
COPY keys/ticket.pub /app/keys/ticket.pub
```

构建上下文中的 `keys/*.pub` 来自厂商下发；**私钥不得出现在构建上下文**。更换厂商公钥 → 改文件后重新 `build` 发版。

#### 运行时挂载（本站私钥）

`2_run_*_container.sh`（或编排）须挂载，例如：

```text
-v "${ROOT}/secrets/age:/secrets/age"
```

django 进程只读 `site.key`（写仅限 init）。nodejs **不**挂本站私钥卷。

#### 厂商侧本机布局（非容器）

```text
vendor-keys/
  dev.key          # age 解「封给厂商」的 Export 包
  dev.pub
  ticket.key       # 签 JWT
  ticket.pub
  sites/
    <site_id>.pub  # 各站 site.pub，打回包时 age 封给该站
```

#### 加解密方向（回顾）

| 动作 | 收件人 / 验签 | 现场使用的文件 |
|------|---------------|----------------|
| Backup | 封 `site.pub` | 写卷上密文 |
| Export | 解 `site.key` → 封 `dev.pub` | 下载给厂商 |
| Import / Apply | 验 `ticket.pub` → 解 `site.key` | 写 payload / 运行库 |

#### 缺失与轮换

| 情况 | 行为 |
|------|------|
| 缺 `/app/keys/dev.pub` 或 `ticket.pub` | Export / 验票相关接口 `500`，`message` 指明缺钥 |
| 缺 `/secrets/age/site.key` | Backup / Import / Export / Apply `500` |
| 轮换本站钥 | 旧密文须先导出/迁移；新 `site.key` 写入卷后旧 payload 可能无法解开 |
| 轮换厂商公钥 | 更新构建上下文公钥并重建镜像；旧「封给旧 dev.pub」的包仅旧 `dev.key` 可解 |

### `shell_pagesettings`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID / bigint | PK | — | 行主键 |
| `user_id` | UUID FK → `auth_user` | UNIQUE，非空 | — | 一用户一行 |
| `mode` | varchar | 非空 | — | `sidebar` \| `topbar` |

无行时 `GET /settings` 返回 `DEFAULT_MODE=sidebar`，不自动 INSERT。

### `shell_siteconfigversion`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | 快照主键 |
| `version` | varchar | UNIQUE，非空 | — | `/^\d+(\.\d+)?$/`；镜像种子为 `"0"` |
| `description` | text | 可空 | `NULL` | 备份说明；种子可为 `image default` |
| `created_at` | timestamptz | 非空 | now | 创建时间 |
| `is_current` | bool | 非空 | `false` | 全表至多一行 true；种子行出厂为 `true` |
| `payload_path` | varchar / text | 非空 | — | age 密文（本站收件人） |

**镜像种子（dev / prod 相同约定）**：构建时即带版本 `0`——migration / data seed **INSERT** `version=0`、`is_current=true`、出厂默认配置 payload（与 Backup 同形的本站收件人密文，构建或镜像初始化阶段写入）。首次可用时 Logo 为 `0`，无需先 Backup。

### `shell_usedauthticket`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID / bigint | PK | — | 行主键 |
| `jti` | varchar | UNIQUE，非空 | — | JWT `jti` |
| `used_at` | timestamptz | 非空 | now | 消耗时间 |
| `action` | varchar | 非空 | — | `import` \| `apply`（审计用） |

校验成功且业务写库成功后 INSERT；已存在同 `jti` → `401`。

---

## 各请求写库明细

只列**写**。`GET /me`、`GET /settings`、`GET /site-config/versions`：只读。

| 请求 | 表 | 写入 / 变更 |
|------|-----|-------------|
| `POST /logout` | `django_session` | 清会话 |
| `PUT /settings` | `shell_pagesettings` | UPSERT `mode` |
| `POST /site-config/export` | 审计 | zip → age 封 `dev.pub`；不改 `mode` |
| `POST /site-config/import` | `shell_usedauthticket` | 验 JWT `action=import` → 记 `jti` |
| | 各业务 page 表 | `site.key` 解密 → registry 写入 |
| | 审计 | Import |
| `POST /site-config/versions` | `shell_siteconfigversion` + 卷 | INSERT；zip → age 封本站；`is_current=false` |
| `POST .../versions/{id}/import` | `shell_usedauthticket` | 验 JWT `action=import`，`pkg_hash`=上传文件 → 记 `jti` |
| | 文件卷 | 覆盖密文 payload |
| `POST .../versions/{id}/export` | — | open `site.key` → seal `dev.pub` |
| `POST .../versions/{id}/apply` | `shell_usedauthticket` | 验 JWT `action=apply`，`pkg_hash`=该行 payload → 记 `jti` |
| | 各业务 page 表 | 解密 → 事务写入 |
| | `shell_siteconfigversion` | 目标 `is_current=true`；其余 `false` |
| | 审计 | Apply |

---

## 调用顺序

### `GET /me` / `POST /logout` / `GET|PUT /settings`

同前：`SessionUserService` / shared logout / `SettingsService`。

### Site Config

```
Export:
  → zip →（行内先 open site.key）→ seal(dev.pub) → 流 + 审计

Backup:
  → zip → seal(site.pub) → 落盘 + INSERT

Import（版本行）:
  → AuthTicketService.validate(jwt, action="import", pkg_hash=SHA256(file))
  → open(site.key) → 校验 schema → 覆盖 payload
  → 记 jti

Apply:
  → AuthTicketService.validate(jwt, action="apply", pkg_hash=SHA256(payload_file))
  → open(site.key) → registry 写入 → 更新 is_current
  → 记 jti
```

`AuthTicketService.validate`：`ticket.pub` 验签 → 查 `action` / `pkg_hash` / `exp` / 可选 `site_id` → `jti` 未用。

建会话见 login。

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| login | 建会话；本页只读 / 销 |
| users / catalog | 权限与审计经 `shared` |
| 各业务 page | 切片经 `shared.site_config` |
| age / ticket 钥匙 | 见上文「钥匙」；镜像公钥 + 卷上 `site.key` 必齐 |
| JWT 签发 | 仅厂商本机；现场 django 只验 `ticket.pub` |
| 本站钥 init | django 或 keys-init；**不**在 nodejs |
| 版本 `0` | 镜像构建种子；migrate/seed 保证存在且出厂 `is_current` |
