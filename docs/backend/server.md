# Servers（后端）

对应前端契约：[server_api.md](../frontend/server_api.md)。  
实现包：`servers/django/pages/servers/`。前缀：`/ai_stream2/backend/servers`。

---

## 代理的 API

| 本页触发 | 上游 | 上游路径 / 动作 | 用途 |
|----------|------|-----------------|------|
| `GET /{server_id}/hello_world`；`POST /refresh` 内部 | 各微服务 / per-pipeline DeepStream | 注册表 `health_upstream` | 健康探测 → online/offline |
| `POST /{server_id}/restart` | **docker_socket_proxy** | `POST /containers/{container_name}/restart` | 整容器重启 |
| `GET /{server_id}/logs` | 挂载日志文件，或 docker_socket_proxy logs | 读尾部 | Log 弹窗 |

列表按 **`category`** 区分 Infrastructure（静态）与 Pipeline（`pipelines` 动态）。无共享 `deepstream` 静态条目。`DOCKER_HOST=tcp://${PROJECT_NAME}_docker_socket_proxy:2375`。

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/` | 注册表列表 + 缓存 status / last_refresh_at |
| `GET` | `/{server_id}/hello_world` | 单条健康代理 |
| `POST` | `/refresh` | 并发探测全部；部分失败不整请求失败 |
| `POST` | `/{server_id}/restart` | Docker restart |
| `GET` | `/{server_id}/logs` | 日志文本（`tail` 默认 500） |

字段 / 错误码以 [server_api.md](../frontend/server_api.md) 为准。权限：`servers.view_server`（list / hello_world / refresh / logs）；`servers.change_server`（restart）。

固定段：`refresh`、`/{id}/hello_world` 先于 `restart` / `logs`。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | **无**业务表；状态内存缓存 |
| HTTP | `httpx`（health）；Docker API 经 proxy |
| 并发 | `asyncio` / 线程池并发 refresh |
| 鉴权 | session + `has_perm` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| 容器 restart / logs | **复用** docker_socket_proxy |
| 健康探测 | **造**（`HealthProbeService` 按 registry 转发） |
| 注册表 | **造**（模块常量或 YAML；无 CRUD API） |
| 状态缓存 | **造**（进程内 dict：`status` / `last_refresh_at`） |
| 响应外壳 | **复用** `shared.http` |

---

## 目录结构

```
servers/django/pages/servers/
  apps.py
  urls.py
  registry.py           # SERVER_REGISTRY 静态条目
  views.py
  services.py           # ServerStatusService, HealthProbeService, RestartService, ServerLogService
  clients.py            # DockerProxyClient, HealthHttpClient
  permissions.py
```

禁止 import 其它 `pages.*`。无 `models.py` / migrations（除非日后要持久化探测历史——当前不做）。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `SERVER_REGISTRY` | `id`/`name`/`container_name`/`health_upstream`/`sort_order` |
| `ServerStatusCache` | 读写下次 list/refresh 用的 status 缓存 |
| `HealthProbeService` | 单条探测；更新缓存；返回 `RefreshResult` |
| `ServerStatusService` | `list_servers` / `refresh_all`（并发） |
| `RestartService` | `DockerProxyClient.restart` → `RestartResult`；可选再 probe |
| `ServerLogService` | 文件优先 → Docker logs → `{content}` |
| `DockerProxyClient` | restart / container logs |
| `HealthHttpClient` | GET `health_upstream` |

### 默认注册表

| `id` | `container_name` |
|------|------------------|
| `ffmpeg` | `${PROJECT_NAME}_ffmpeg` |
| `export_onnx` | `ai_stream2_export_onnx` |
| `generator` | `${PROJECT_NAME}_generator` |
| `deepstream` | `${PROJECT_NAME}_deepstream` |
| `export_trt` | `${PROJECT_NAME}_export_trt` |
| `mediamtx` | `${PROJECT_NAME}_mediamtx` |
| `kafka` | `${PROJECT_NAME}_kafka` |
| `nodejs` | `${PROJECT_NAME}_nodejs` |

`docker_socket_proxy` **不**入表。`health_upstream` 按各服务实际健康 URL 配置（网内主机名）。

---

## 数据库

无持久表。进程重启后 `status`/`last_refresh_at` 可为空，直至再次 `refresh` 或 `hello_world`。

---

## 各请求写库明细

无写库。副作用：

| 请求 | 副作用 |
|------|--------|
| `GET /{id}/hello_world` | 更新该 id 缓存 |
| `POST /refresh` | 并发探测；更新全部缓存 |
| `POST /{id}/restart` | Docker restart；可选再探测 |
| `GET /{id}/logs` | 只读 |

---

## 调用顺序

```
GET /:           registry + cache → Server[]
POST /refresh:   对每条 HealthProbeService.probe（并发）→ {results}
GET hello_world: HealthHttpClient → 判 2xx + success → online|offline
POST restart:    require change → DockerProxyClient.restart(container_name)
GET logs:        ServerLogService.get(tail)
```

探测成功：`HTTP 2xx` 且 JSON 若含 `success` 则须为 `true`。

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| docker_socket_proxy | restart / 可选 logs 必需 |
| 各微服务 | health 不可达 → 该条 offline，不拖垮 refresh |
| Site Config | **不**注册 |
| 其它 page | 无；重启影响下游由调用方自行处理 |
