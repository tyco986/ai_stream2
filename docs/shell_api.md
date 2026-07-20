# Shell API

管理端 App Shell 后端 API。UI 规格见 [shell.md](./shell.md)。

本前缀负责：**当前用户**（栏尾用户名）与 **页面配置**（齿轮弹窗，目前仅 `mode`）。导航项顺序与路由为前端固定，**不**经本 API 下发。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/shell` |
| 路径前缀 | `/ai_stream2/backend/shell` |

实现时须将固定段（`me`、`settings`）注册在可能的动态路径之前。

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

### 字段语义

| 字段 | 说明 |
|------|------|
| `username` | 当前登录用户展示名；对应栏尾用户名 |
| `mode` | 壳布局 Mode；对应 Page Settings **Mode** 下拉 |
| `user_id` | 用户 **id**（UUID）；后端主键；UI **不**展示 |

### 持久化边界

| 写入 API | 内容 |
|----------|------|
| `PUT /settings` | **Apply** 时写入当前用户的页面配置（目前仅 `mode`） |

**不**经本 API 持久化：当前导航高亮、内容区业务页会话态、弹窗内未 Apply 的草稿、未登录时的临时 Mode（见下）。

### 鉴权

| 场景 | 行为 |
|------|------|
| 已登录 | `GET /me`、`GET/PUT /settings` 按当前会话用户读写 |
| 未登录 | `GET /me` → `401`；栏尾用户名展示 `—`；`GET/PUT /settings` → `401`；**Apply** 仅会话内生效，用 `DEFAULT_MODE` 或会话值，**不**写后端 |

鉴权机制（Cookie / Token 等）由全局 Auth 约定；本前缀假定请求已携带当前用户身份。

---

## 数据模型

### CurrentUser

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "username": "alice"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | UUID | 否 | 用户 id |
| `username` | string | 否 | 展示名；栏尾文本 |

### PageSettings

```json
{
  "mode": "sidebar"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `mode` | string | 否 | `sidebar` \| `topbar` |

按**用户**隔离；换账号互不影响。后续弹窗新增配置项时扩展本对象字段。

---

## 端点

### 当前用户

#### `GET /me`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/shell/me`

**说明**：进入管理端壳时拉取栏尾用户名。

**响应 `data`**：`CurrentUser`。

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |

---

### 页面配置

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

**说明**：Page Settings 点击 **Apply** 时调用；将弹窗草稿覆盖为当前用户已应用配置。下拉改值**不**调用本接口。

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

## 错误码

| HTTP | 场景 |
|------|------|
| 400 | `Invalid mode`：`mode` 非 `sidebar` / `topbar` |
| 401 | 未登录 |

---

## 调用节点

页面操作与 API 对应关系。

### 壳加载

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入管理端 | `GET` | `/me` |
| 进入管理端 | `GET` | `/settings`（可与 `/me` 并行） |

### 页面配置弹窗

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 打开齿轮弹窗 | — | 用壳已缓存的已应用 `mode` 填草稿；或再 `GET /settings` |
| **Mode** 下拉改值 | — | 仅改弹窗草稿；**不**调 API |
| **Cancel** / `×` / Esc | — | 丢弃草稿；关弹窗；不调 API |
| **Apply**（已登录） | `PUT` | `/settings`；成功后应用壳并关弹窗 |
| **Apply**（未登录） | — | 仅会话内应用草稿；不调 `PUT /settings` |

### 不经本 API

| 调用节点 | 说明 |
|----------|------|
| 点击导航项 | 前端路由；无后端调用 |
| 弹窗内改 Mode 未 Apply | 草稿；不调 API、不改壳 |

---

## 典型流程

### 首次进入管理端（已登录）

```
GET /me
  → username → 栏尾展示

GET /settings
  → mode → 渲染侧栏或顶栏壳
```

### 切换 Mode（Apply）

```
# Page Settings 下拉选 topbar（仅草稿）
# 点击 Apply
PUT /settings
  { "mode": "topbar" }
  → 成功后切换壳布局并关弹窗；保留当前路由与业务页状态
```

### 取消编辑

```
# 下拉改过草稿后点 Cancel / × / Esc
# 不调用 PUT；壳与已存 settings 不变
```

### 未登录

```
GET /me → 401；栏尾「—」
GET /settings → 401；已应用 Mode = sidebar（DEFAULT_MODE）
# 齿轮可开；Apply 仅会话内生效；Cancel 丢弃草稿
```
