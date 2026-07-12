# Servers 页面

管理端 Servers 页面，用于查看各 Docker 微服务运行状态、探测健康、重启容器与查看日志。表格为**只读**（无 Checkbox、无 Add/Remove、无 Search、无分页）。

每条服务的唯一标识为 **`id`**（`servers/` 下文件夹名，如 `ffmpeg`、`deepstream`），由后端注册表配置、不可通过 UI 增删。表格 **Name** 列与 **`id` 同值**；行内 Operations、API 请求均以 **id** 为准。

## 主页面

```
+========================================================================================+
| Servers                                                                                |
+========================================================================================+
| [ Refresh ]                                                                            |
+----------+--------+-------------+---------------------+
| Name     | Status | Last Refresh| Operations          |
+----------+--------+-------------+---------------------+
| ffmpeg     | Online | 06-30 14:22 | ↻ 📓                |
| dsyolo     | Online | 06-30 14:22 | ↻ 📓                |
| generator  | Offline| 06-30 14:22 | ↻ 📓                |
| deepstream | Online | 06-30 14:22 | ↻ 📓                |
| export_trt | Online | 06-30 14:22 | ↻ 📓                |
| mediamtx   | Online | 06-30 14:22 | ↻ 📓                |
| kafka      | Online | 06-30 14:22 | ↻ 📓                |
| nodejs     | Offline| 06-30 14:22 | ↻ 📓                |
+----------+--------+-------------+---------------------+
```

| 区域 | 说明 |
|------|------|
| 页面标题 | 固定显示 `Servers` |
| 工具栏 | 仅 **Refresh**（左对齐）；位于表格上方 |
| 数据表格 | 全宽；只读；含行内 Operations |

**布局补充说明**

| 项 | 说明 |
|----|------|
| 工具栏按钮样式 | **Refresh** 为绿底白字 |
| **Refresh** | 始终可用；**全局**探测所有已注册服务（见下文） |
| 表格行序 | 与后端注册表 `sort_order` 一致；默认按 Name 字母序 |

---

## 工具栏

| 元素 | 样式 | 可用条件 | 行为 |
|------|------|----------|------|
| **Refresh** | 绿底白字 | 始终 | 调用 `POST /servers/refresh`；后端代理请求各服务健康探测接口，更新全部行的 **Status**、**Last Refresh** |

点击 **Refresh** 后，工具栏按钮进入 loading（禁用重复点击），直至全部探测结束。

---

## 数据表格

行主键为 **id**（`servers/` 文件夹名，默认不展示）。

### 列定义

| 列名 | 类型 | 说明 |
|------|------|------|
| Name | `str` | `servers/` 下文件夹名；与 **`id` 同值**；只读 |
| Status | `str` | 只读；`Online` / `Offline`；由最近一次健康探测结果决定 |
| Last Refresh | `datetime` | 只读；该服务**最近一次探测完成**时间；格式同 Streams **Last Test**（如 `06-30 14:22`）；从未探测时为 `—` |
| Operations | 图标组 | Restart / Log（见下文） |

### Status 展示规则

| API `status` | UI 展示 |
|--------------|---------|
| `online` | **Online** |
| `offline` | **Offline** |

探测超时、连接拒绝或非 2xx 响应均记为 `offline`。

---

## Operations 列

图标顺序固定：**Restart → Log**。

| 顺序 | 图标 | 名称 | 行为 |
|:----:|:----:|------|------|
| 1 | ↻ | Restart | **二次确认**后通过 `docker_socket_proxy` **重启 Docker 容器**（整容器 `restart`，非仅容器内 FastAPI 进程） |
| 2 | 📓 | Log | 打开该服务只读日志面板（抽屉或弹窗） |

行内 **Restart / Log** 均按当前行 **id** 操作。

### Restart 确认

```
+--------------------------------+
| Confirm Restart            [x] |
+--------------------------------+
| Restart container for        |
| "ffmpeg"?                    |
| The service will be briefly  |
| unavailable.                 |
+--------------------------------+
|        [ Restart ]  [ Cancel ] |
+--------------------------------+
```

确认后调用 `POST /servers/{id}/restart`；成功后可选自动对该行再执行一次健康探测。

### Restart 进行中

↻ 图标旋转；该行 Restart 禁用，直至重启请求返回（或超时）。

---

## Log 弹窗

点击 📓 打开，只读日志文本 + **Close**。内容来自该服务容器日志或挂载日志目录（由后端聚合）。

---

## 健康探测（Refresh）

**Refresh** 为全局操作：后端对注册表中**每一条**服务并发发起健康探测，并写回各行 **Status** 与 **Last Refresh**。

管理端与 UI 侧统一调用 **`hello_world`**；各服务容器内是否自带该路由无关——由 Django 后端代理层将 `hello_world` 映射到该服务实际的健康检测 API。

管理端**不直接**访问各容器端口；一律经后端代理 `GET /ai_stream2/backend/servers/{id}/hello_world`（或 `POST /refresh` 内部等价调用）。

首次进入页面时，后端可对列表执行与 **Refresh** 相同的探测，或返回上次缓存结果并在后台异步刷新（实现二选一，推荐进入即探测）。

---

## 注册服务一览

以下为默认注册表（实现时可配置，UI 不可编辑）：

| id（= Name） | 容器名 |
|--------------|--------|
| `ffmpeg` | `ai_stream2_ffmpeg` |
| `dsyolo` | `DeepStream-Yolo` |
| `generator` | `ai_stream2_generator` |
| `deepstream` | `ai_stream2_deepstream` |
| `export_trt` | `ai_stream2_export_trt` |
| `mediamtx` | `ai_stream2_mediamtx` |
| `kafka` | `ai_stream2_kafka` |
| `nodejs` | `ai_stream2_nodejs` |

`docker_socket_proxy`（`ai_stream2_docker_socket_proxy`）为基础设施，**不出现在**本表。健康探测对上述条目一律经后端代理为 **`hello_world`**。

---

## 交互流程

```
进入 Servers 页     ──> GET /servers（可选附带探测结果或随后 POST /refresh）
工具栏 Refresh      ──> POST /refresh ──> 更新全部 Status / Last Refresh
行内 ↻ Restart      ──> 确认 ──> POST /{id}/restart（docker_socket_proxy）
行内 📓 Log         ──> GET /{id}/logs ──> 只读弹窗
```

---

## Toast 提示

轻量 **Toast**（自动消失）；文案为英文；`{name}`、`{n}` 等为运行时替换。无 Toast 的操作见表中「—」。

| 操作 | 文案 | 说明 |
|------|------|------|
| 工具栏 **Refresh** 完成 | — | 表格 **Status** / **Last Refresh** 更新 |
| 行内 **↻ Restart** 确认后成功 | **`{name} restarted`** | `{name}` 为服务 **Name** |
| 行内 **↻ Restart** 失败 | **`Restart failed for {name}`** | 可选附带 `detail` |
| 行内 **📓 Log** | — | 只读弹窗 |
| API 4xx/5xx | **`{message}`** | 沿用后端 `message` |

> **Confirm** 弹窗（**Restart** 二次确认）**不是** Toast。

---

## 按钮样式规范

| 元素 | 样式 |
|------|------|
| Refresh | 绿底白字 |
| Restart（确认框） | 蓝底白字 |
| Cancel | 白底蓝字 |
| Operations ↻ | 蓝 |
| Status Online | 绿字（或绿标签，与 Streams 一致） |
| Status Offline | 红字（或红标签） |

---

## 与后端约定（概要）

| 操作 | 说明 |
|------|------|
| 列表 | `GET /servers` |
| 全局刷新 | `POST /servers/refresh` |
| 重启容器 | `POST /servers/{id}/restart` |
| 日志 | `GET /servers/{id}/logs` |

路径参数 **`id`** 为 `servers/` 下文件夹名（如 `ffmpeg`）。

具体路径与字段见 [server_api.md](./server_api.md)。

---

## 唯一标识

| 实体 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 服务 | `id` | `str` | `servers/` 文件夹名；不可改；API 与前端行 key |
| 服务 | `name` | `str` | 与 **`id` 同值**；表格列 **Name** |

---

## 边界与校验（建议）

| 规则 | 说明 |
|------|------|
| 只读 | UI 不提供增删改服务条目 |
| Restart | 二次确认；依赖 `docker_socket_proxy` 可用 |
| Refresh | 单服务探测失败不影响其他服务；失败行 `offline` |
| 日志 | 只读；过长时后端截断并注明（如仅返回尾部 N 行） |
