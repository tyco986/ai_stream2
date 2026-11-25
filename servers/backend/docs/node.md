# Node

路径：`GET /backend/nodes`。经 `AuthFilter`，权限为 `node_crud` 的读位。

JSON 成功外壳：`{success, message, data}`。`data` 的键是 Beszel system id。没有节点时 `data` 为 `{}`。

每个节点有 `system`、`cpu`、`memory`、`gpu`。`memory` 与 `gpu.*.memory_total`、`gpu.*.memory_used` 单位是 MB。`cpu.utilization` 与 `gpu.*.utilization` 是 0–100 的整机占用。`system.uptime` 是 `{days, hours, minutes}`。

取不到的单值是 `null`。没有统计或没有显卡时 `gpu` 为 `{}`。

数据来自 Beszel Hub：`systems`、`system_details`、`system_stats`（`type=1m` 的最新一条）。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/nodes` | 无 | `{system id: 节点}` | 读取各节点的系统、CPU、内存、显卡 |

### GET `/backend/nodes`

入参：无。

出参 `data`：键为 Beszel system id。`message`：`node_list_ok`。失败 `500`，`node_list_error`。
