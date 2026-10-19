# Server

路径：`/ai_stream2/backend/server`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。全库只有一行，`id` = `00000000-0000-4000-8000-000000000005`。

对象键：`id`、`probed_at`、`deepstream`、`generator`、`export_onnx`、`export_trt`、`mediamtx`、`kafka`、`postgresql`、`frontend`、`backend`。`deepstream` 为数组，元素键：`id`（pipeline id）、`name`、`healthy`。其余服务键为布尔值。`probed_at` 未探测时为 `null`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/server` | 无 | 服务器对象 | 读取上次探测结果 |
| `POST` | `/ai_stream2/backend/server/probe` | 无 | 服务器对象 | 探测各服务并写回 |

### GET `/ai_stream2/backend/server`

入参：无。

出参 `data`：服务器对象。`message`：`Loaded server`。失败 `500`，`Failed to load server`。

### POST `/ai_stream2/backend/server/probe`

按 pipeline 名探测 `http://puskas_{name}:9000/health`，结果写入 `deepstream`。同时探测 generator、export_onnx、export_trt、mediamtx、kafka、frontend、backend 的健康地址，以及执行 `SELECT 1` 判断 postgresql。超时 3 秒。写回 `probed_at=now()` 和各布尔值。

入参：无。

出参 `data`：探测后的服务器对象。`message`：`Probed server`。失败 `500`，`Failed to probe server`。

---

## 2. DATA

### `server`

| 列 | 类型 | 默认值 | 约束 |
|----|------|--------|------|
| `id` | UUID | 无 | PRIMARY KEY |
| `probed_at` | TIMESTAMPTZ | `NULL` | 可空 |
| `deepstream` | JSONB | 种子为 `[]` | 可空 |
| `generator` | BOOLEAN | 无 | NOT NULL |
| `export_onnx` | BOOLEAN | 无 | NOT NULL |
| `export_trt` | BOOLEAN | 无 | NOT NULL |
| `mediamtx` | BOOLEAN | 无 | NOT NULL |
| `kafka` | BOOLEAN | 无 | NOT NULL |
| `postgresql` | BOOLEAN | 无 | NOT NULL |
| `frontend` | BOOLEAN | 无 | NOT NULL |
| `backend` | BOOLEAN | 无 | NOT NULL |

种子行：`id` = `00000000-0000-4000-8000-000000000005`，`deepstream` = `[]`，各布尔列为 `FALSE`，`probed_at` 为空。触发器拒绝删除任意行。
