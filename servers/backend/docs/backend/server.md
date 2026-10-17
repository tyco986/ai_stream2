# Servers（后端）

对应前端契约：[server_api.md](../frontend/server_api.md)。  
实现包：`servers/django/pages/servers/`。前缀：`/ai_stream2/backend/servers`。

---

API

| 本页触发 | 上游 | 上游路径 / 动作 | 用途 |
|----------|------|-----------------|------|
| `GET /{server_id}/hello_world`；`POST /refresh` 内部 | 各微服务 / per-pipeline DeepStream | 注册表 `health_upstream` | 健康探测 → online/offline |
| `POST /{server_id}/restart` | **docker_socket_proxy** | `POST /containers/{container_name}/restart` | 整容器重启 |
| `GET /{server_id}/logs` | 挂载日志文件，或 docker_socket_proxy logs | 读尾部 | Log 弹窗 |

列表按 **`category`** 区分 Infrastructure（静态）与 Pipeline（`pipelines` 动态）。无共享 `deepstream` 静态条目。`DOCKER_HOST=tcp://${PROJECT_NAME}_docker_socket_proxy:2375`。

---

## API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/` | 注册表列表 + 缓存 status / last_refresh_at |
| `POST` | `/refresh` | 并发探测全部；部分失败不整请求失败 |
| `POST` | `/{server_id}/restart` | Docker restart |
| `GET` | `/{server_id}/logs` | 日志文本（`tail` 默认 500） |

## 数据库

### `server`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | |
| `probed_at` | timestamptz | 可空 | `NULL` | 最近一次探测时间 |
| `deepstream` | JSON | 可空 | `NULL` | |
| `generator` | bool | 非空 | `false` | |
| `export_onnx` | bool | 非空 | `false` | |
| `export_trt` | bool | 非空 | `false` | |
| `mediamtx` | bool | 非空 | `false` | |
| `kafka` | bool | 非空 | `false` | |
| `postgresql` | bool | 非空 | `false` | |
| `frontend` | bool | 非空 | `false` | |
| `backend` | bool | 非空 | `false` | |

---