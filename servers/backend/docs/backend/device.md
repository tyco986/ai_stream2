# Device

路径：`GET /backend/device`。经 `AuthFilter`，权限为 `server_crud` 的读位。

JSON 成功外壳：`{success, message, data}`。`data` 的键是 GPU 序号字符串，值为该卡当前状态。无卡时 `data` 为 `{}`。

对象键：`name`、`memory_total`、`memory_used`、`memory_utilization`、`gpu_utilization`。显存单位为字节。两个 utilization 为 0–100 的整数。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/device` | 无 | `{序号: 设备对象}` | 读取各 GPU 显存与利用率 |

### GET `/backend/device`

通过 NVML 读取本机每张 GPU 的名称、显存总量、已用显存、显存利用率、GPU 利用率。

入参：无。

出参 `data`：键为 `"0"`、`"1"` 这类序号。`message`：`Loaded devices`。失败 `500`，`Failed to load devices`。
