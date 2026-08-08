# Login API

Login 页后端 API。UI 规格见 [login.md](./login.md)。

本前缀负责：**用户名密码登录**、**登录页强制改密**、**超管密码框 JWT 票破局**（建会话）。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/login` |
| 路径前缀 | `/ai_stream2/backend/login` |

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

### 种子管理员

| 项 | 值 |
|----|-----|
| 环境变量 | `SEED_ADMIN_USERNAME` / `SEED_ADMIN_PASSWORD` / `SEED_ADMIN_GROUP`（默认 `admin` / `admin` / `admin`） |
| 时机 | 容器启动 `ensure_seed_admin`；**仅当库中尚无任何 `is_superuser`** 时创建或提升超管 |
| 已有超管 | **忽略** `SEED_*` 用户创建；仍 ensure `SEED_ADMIN_GROUP` 并把超管加入该组 |
| 新建后 | `is_superuser=true`、`must_change_password=true`；组挂权限目录全部 permission，用户加入该组 |

无自动登录中间件；一律经本 `POST /`。

---

## 端点

#### `POST /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/login`

**请求体**：

```json
{
  "username": "admin",
  "password": "••••••••",
  "new_password": "optional"
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| `username` | 是 | 用户名；票路径须为唯一超管 |
| `password` | 是 | 普通密码，或厂商签发的 JWT（三段 `.`） |
| `new_password` | 否 | 强制改密时必填；票登录时可选，用于重置超管密码 |

**密码路径**

| 情况 | `data` | 会话 |
|------|--------|------|
| 凭据正确且无需改密 | `{}` | 建立 |
| 凭据正确且 `must_change_password`，未带 `new_password` | `{ "must_change_password": true }` | **不**建立 |
| 凭据正确且带合法 `new_password` | `{}` | 改密后建立 |

**票路径**（`password` 呈 JWT）

| 项 | 约定 |
|----|------|
| `action` | `admin_login` |
| 必填 claims | `exp`、`jti`、`action` |
| 无 | `pkg_hash`（与 import/apply 不同） |
| 可选 | `site_id` |
| 验签 | 镜像 `ticket.pub`（EdDSA） |
| 用户 | `username` 对应用户须 `is_superuser` 且 `is_active` |
| 成功 | 消耗 `jti`；建会话；可选 `new_password` 重置密码并清 `must_change_password` |
| 门闸 | **不**要求先改密即可进系统 |

身份与权限以进壳后 [shell_api.md](./shell_api.md) `GET /me` 为准。

| HTTP | 场景 |
|------|------|
| 400 | 缺字段；`new_password` 非法 |
| 401 | 用户名或密码错误；票无效/过期/`jti` 已用；或 `is_active=false` |

---

## 调用节点

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **Login** | `POST` | `/login/` |

---

## 典型流程

```
# 首次种子密码登录 → 同页改密
POST /login/ { "username": "admin", "password": "admin" }
  → { "must_change_password": true }（无会话）
POST /login/ { "username": "admin", "password": "admin", "new_password": "…" }
  → {} + 会话

# 厂商破局
POST /login/ { "username": "admin", "password": "<JWT>" }
  → {} + 会话
POST /login/ { "username": "admin", "password": "<JWT>", "new_password": "…" }
  → 重置密码 + 会话
```
