# Servers API

管理端 Servers 页面后端 API。UI 规格见 [server.md](./server.md)。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/servers` |
| 路径前缀 | `/ai_stream2/backend/servers` |

实现时须将固定段（`refresh`）及 `/{server_id}/hello_world` 注册在 `/{server_id}/restart`、`/{server_id}/logs` 之前。

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
| `status` | `online` \| `offline` | 对应 UI **Online** / **Offline** |

### 字段语义

| 字段 | 说明 |
|------|------|
| `id` | `servers/` 下文件夹名（如 `ffmpeg`）；路径 `/{server_id}` 与此同值；与 `name` 同值 |
| `name` | 与 **`id` 同值**；对应 UI **Name** 列 |
| `container_name` | Docker 容器名；用于 `restart` 与日志；**列表接口可不返回**（仅后端内部） |
| `status` | 最近一次健康探测结果；`online` / `offline` |
| `last_refresh_at` | 该服务**最近一次探测完成**时间（无论成败）；未探测为 `null` |

### Docker 重启

容器重启经 **`docker_socket_proxy`**（默认容器 `ai_stream2_docker_socket_proxy`）调用 Docker Engine API，执行 **整容器** `restart`，而非向容器内 FastAPI 发信号。

| 项 | 典型值 |
|----|--------|
| 网内 `DOCKER_HOST` | `tcp://ai_stream2_docker_socket_proxy:2375` |
| 宿主机 `DOCKER_HOST` | `tcp://127.0.0.1:2375` |
| API | `POST /containers/{container_name}/restart` |

代理须启用 `CONTAINERS`、`POST`、`ALLOW_RESTARTS`（见 `servers/docker_socket_proxy/scripts/1_run_container.sh`）。

### 健康探测

管理端不直连各微服务端口；由本后端**统一代理为 `hello_world`**。

对外（管理端 / `POST /refresh`）每条服务均为：

```
GET /ai_stream2/backend/servers/{server_id}/hello_world
```

后端根据注册表将上述请求转发至该服务实际的健康检测 API；容器内是否暴露同名 `hello_world` 路由无关。

探测成功：`HTTP 2xx` 且响应 JSON 含 `success: true`（若存在该字段）→ `online`；否则 → `offline`。

响应示例：

```json
{
  "success": true,
  "message": ""
}
```

---

## 数据模型

### Server

```json
{
  "id": "ffmpeg",
  "name": "ffmpeg",
  "status": "online",
  "last_refresh_at": "2026-06-30T14:22:00Z",
  "category": "infrastructure"
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | string | 否 | Infrastructure：文件夹名；Pipeline：`pipeline:{uuid}` |
| `name` | string | 否 | Infrastructure：与 id 同值；Pipeline：pipeline name |
| `status` | string | 否 | `online` \| `offline` |
| `last_refresh_at` | datetime | 是 | 最近探测完成时间 |
| `category` | string | 否 | `infrastructure` \| `pipeline` |

### RefreshResult

单条服务的探测结果（用于 `POST /refresh` 与可选的单条重探）：

```json
{
  "id": "ffmpeg",
  "status": "online",
  "last_refresh_at": "2026-06-30T14:22:00Z",
  "latency_ms": 42,
  "detail": ""
}
```

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `id` | string | 否 | `servers/` 文件夹名 |
| `status` | string | 否 | `online` \| `offline` |
| `last_refresh_at` | datetime | 否 | 本次探测完成时间 |
| `latency_ms` | int | 是 | 往返耗时 |
| `detail` | string | 是 | 失败原因摘要（如 `connection refused`） |

### RestartResult

```json
{
  "id": "ffmpeg",
  "container_name": "ai_stream2_ffmpeg",
  "restarted_at": "2026-06-30T14:25:00Z"
}
```

---

## 注册表（实现参考）

后端静态注册表维护 Infrastructure；Pipeline 行由 `pipelines` 表动态生成。`id` / `name` / `container_name` / `health_upstream` / **`category`**。默认 Infrastructure：

| `id` | `container_name` |
|------|------------------|
| `ffmpeg` | `ai_stream2_ffmpeg` |
| `export_onnx` | `ai_stream2_export_onnx` |
| `generator` | `ai_stream2_generator` |
| `export_trt` | `ai_stream2_export_trt` |
| `mediamtx` | `ai_stream2_mediamtx` |
| `kafka` | `ai_stream2_kafka` |
| `nodejs` | `ai_stream2_nodejs` |

Pipeline 行：`id=pipeline:{uuid}`，`name`=pipeline name，`container_name={project}_{name}`。无共享 `deepstream` 静态条目。`docker_socket_proxy` 不在此表。

---

## 端点

### 列表

#### `GET /`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/servers`

**说明**：进入 Servers 页；返回全部已注册服务及当前 `status` / `last_refresh_at`。

**响应 `data`**：

```json
{
  "items": []
}
```

`items` 为 `Server[]`，顺序与注册表 `sort_order` 一致。

---

#### `GET /{server_id}/hello_world`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/servers/{server_id}/hello_world`

**说明**：单条服务健康探测；后端代理至该服务 `health_upstream`，对外统一为 `hello_world`。`POST /refresh` 内部对注册表每条并发调用此逻辑。

**响应 `data`**：与上游健康检测一致；代理层保证外壳含 `success` 字段时语义与全局约定一致。

| HTTP | 场景 |
|------|------|
| 404 | `server_id` 不在注册表 |
| 502 | 上游不可达 |
| 504 | 探测超时 |

---

### 全局刷新

#### `POST /refresh`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/servers/refresh`

**说明**：工具栏 **Refresh**；对注册表每条并发调用 `GET /{server_id}/hello_world`（内部等价，无需前端逐条请求），更新各行状态。

**请求体**：无（或 `{}`）

**响应 `data`**：

```json
{
  "results": []
}
```

`results` 为 `RefreshResult[]`，顺序与注册表一致。单条探测失败不导致整请求失败；失败项 `status=offline` 且 `detail` 有说明。

---

### 单条服务

#### `POST /{server_id}/restart`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/servers/{server_id}/restart`

**说明**：Operations **↻ Restart**（二次确认后）。

**请求体**：无（或 `{}`）

**响应 `data`**：`RestartResult`。

成功后实现可对该 `server_id` 再执行一次 `GET /{server_id}/hello_world` 并返回最新 `status`（可选字段 `refresh: RefreshResult`）。

| HTTP | 场景 |
|------|------|
| 404 | `server_id` 不在注册表 |
| 502 | `docker_socket_proxy` 不可用或 Docker API 失败 |
| 504 | 重启超时 |

---

#### `GET /{server_id}/logs`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/servers/{server_id}/logs`

**说明**：Operations **📓 Log** 弹窗。

**Query**（可选）：

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `tail` | int | `500` | 返回尾部行数上限 |

**响应 `data`**：

```json
{
  "content": "2026-06-30 14:22:00 INFO ...\n"
}
```

日志来源优先级（实现约定）：挂载目录下服务日志文件 → Docker 容器 `logs` API（若代理权限允许）→ `docker logs` 经 socket proxy。

---

## 错误码

| HTTP | 场景 |
|------|------|
| 404 | `server_id` 不存在 |
| 502 | 健康探测上游不可达（单条 refresh 时） |
| 504 | 探测或重启超时 |

---

## 调用节点

页面操作与 API 对应关系。

### 页面加载

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| 进入 Servers 页 | `GET` | `/` |
| 进入后自动探测（可选） | `POST` | `/refresh` |

### 工具栏

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **Refresh** | `POST` | `/refresh` |

### Operations

| 调用节点 | 方法 | 路径 |
|----------|------|------|
| **↻ Restart** → 确认 | `POST` | `/{server_id}/restart` |
| Restart 后刷新（可选） | `GET` | `/{server_id}/hello_world` |
| **📓 Log** | `GET` | `/{server_id}/logs` |

### 变更后刷新

| 场景 | 建议刷新 |
|------|----------|
| **Refresh** | 使用 `POST /refresh` 响应更新全表 |
| **Restart** 成功 | `GET /{server_id}/hello_world` 或再次 `GET /` |

---

## 典型流程

### 首次进入页面

```
GET /
POST /refresh          # 可选：进入即探测
```

### 全局刷新

```
POST /refresh
# 用 data.results 更新表格 Status、Last Refresh
```

### 重启服务

```
POST /ffmpeg/restart
GET /ffmpeg/hello_world   # 可选
```

### 查看日志

```
GET /ffmpeg/logs?tail=500
```
