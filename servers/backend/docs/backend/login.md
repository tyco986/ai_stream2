# Login（后端）

对应前端契约：[login_api.md](../frontend/login_api.md)。  
实现包：`servers/django/pages/login/`。前缀：`/ai_stream2/backend/login`。

---

## 代理的 API

无

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `POST` | `/` | 用户名+密码登录，或超管用户名+JWT 票破局 → Django session Cookie |

字段 / 错误码以 [login_api.md](../frontend/login_api.md) 为准。无 Permission 校验（`AllowAny`）。`401`：凭据错误、票无效或 `is_active=false`。

身份与权限**不**由本接口返回；进壳后 `GET /shell/me`。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| 会话 | Django session（Cookie；与 shell 一致） |
| ORM | 无本页表；认证读 `users.User`；票消耗写 `shell_usedauthticket` |
| 鉴权 | `AllowAny`；成功后 `login(request, user)` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| 密码校验 / 建会话 | **复用** `authenticate` + `login` |
| 响应外壳 | **复用** `shared.http` |
| 授权票验签 | **复用** `pages.shell.services.AuthTicketService`（`action=admin_login`，无 `pkg_hash`） |
| 种子 superuser | **造**（`ensure_seed_admin`；属 users；仅当库中尚无任何 `is_superuser`） |

---

## 目录结构

```
servers/django/pages/login/
  apps.py
  urls.py
  serializers.py      # username, password, optional new_password
  views.py            # LoginView
  services.py         # LoginService
```

---

## 类与职责

| 类 | 职责 |
|----|------|
| `LoginSerializer` | 校验 `username` / `password`；可选 `new_password` |
| `LoginService` | 密码登录 / 强制改密 / JWT 票登录 |
| `LoginView` | 薄：反序列化 → Service → `api_success(data)` |

---

## 数据库

本页**无表**。读写：`django_session`；只读 `users_user`；票路径写 `shell_usedauthticket`。

| 设置 | 约定 |
|------|------|
| `SEED_ADMIN_USERNAME` / `SEED_ADMIN_PASSWORD` | 仅空库（无超管）时由 `ensure_seed_admin` 使用一次；已有超管则忽略 |
| 无 `DEV_AUTO_LOGIN` | 已移除；一律经本 `POST /` 建会话 |

---

## 各请求写库明细

| 请求 | 表 / 存储 | 写入 / 变更 |
|------|-----------|-------------|
| `POST /` 密码成功（无需改密） | `django_session` | 建立会话 |
| `POST /` 需改密且未带 `new_password` | — | **不**建会话；`data.must_change_password=true` |
| `POST /` 改密成功 | `users_user` + session | 更新密码、清 `must_change_password`、建会话 |
| `POST /` 票成功 | `shell_usedauthticket` + session | 消耗 `jti`；可选改密；建会话 |
| 审计 | shared | Login / Ticket login |

---

## 调用顺序

```
POST /:
  LoginSerializer → LoginService.authenticate_and_login
    → password 呈 JWT → ticket 路径（超管 + action=admin_login）
    → 否则 authenticate(username, password)
      → must_change_password 且无 new_password → {must_change_password:true}（无会话）
      → 否则改密（若需要）+ login → {}
```

登出、读权限：shell。用户 CRUD：users。

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| `users.User` | 认证目标；本页不 CRUD |
| shell `AuthTicketService` / `UsedAuthTicket` | 票破局 |
| shell | 销会话 / `GET /me`；本页只建会话 |
| Site Config | **不**注册切片 |
