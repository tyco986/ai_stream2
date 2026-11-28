# generator

前缀：`/ai_stream2/generator`。监听 `0.0.0.0:8091`。镜像 `ai_stream2_generator`，容器名相同，端口 `8091:8091`。

JSON 成功外壳：`{success, message, data, file}`。`file` 无附件时为 `null`。`POST /generate` 与 `POST /history` 成功时不套这层 JSON，改为 `multipart/form-data`，字段为 `success`、`message`、`data`（JSON）、`file`（zip）。

历史目录为容器内 `/root/history/generator`，只认 `.zip`。历史落盘和请求日志默认关闭，分别由 `/history/enable`、`/logs/enable` 打开。接口规范在 `/openapi.json`，页面在 `/docs`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/docs` | 无 | Swagger UI | 接口文档页 |
| `GET` | `/openapi.json` | 无 | OpenAPI 3 | 接口规范 |
| `GET` | `/ai_stream2/generator/health` | 无 | 空对象 | 健康检查 |
| `GET` | `/ai_stream2/generator/types` | 无 | 流水线类型对象 | 可生成的流水线类型 |
| `POST` | `/ai_stream2/generator/generate` | multipart：`input` | 请求 YAML 转成的对象 | 生成配置 zip |
| `POST` | `/ai_stream2/generator/history/enable` | 无 | 空对象 | 打开历史落盘 |
| `POST` | `/ai_stream2/generator/history/disable` | 无 | 空对象 | 关闭历史落盘 |
| `GET` | `/ai_stream2/generator/history` | query：`filename` | `{文件名: params}` | 列出历史包的 `params.yml` |
| `POST` | `/ai_stream2/generator/history` | query：`filename` | 每个文件一块 multipart | 下载 zip |
| `DELETE` | `/ai_stream2/generator/history` | query：`filename` | 空对象 | 删除 zip |
| `POST` | `/ai_stream2/generator/logs/enable` | 无 | 空对象 | 打开请求日志 |
| `POST` | `/ai_stream2/generator/logs/disable` | 无 | 空对象 | 关闭请求日志 |
| `DELETE` | `/ai_stream2/generator/logs` | 无 | 空对象 | 清空日志目录下的日志文件 |

### GET `/ai_stream2/generator/health`

入参：无。

出参 `data`：`{}`。`message`：空。`file`：`null`。失败时连接被拒绝。

### GET `/ai_stream2/generator/types`

入参：无。

出参 `data`：流水线名到描述对象的映射。`message`：空。`file`：`null`。

### POST `/ai_stream2/generator/generate`

成功时 `Content-Type` 为 `multipart/form-data`。zip 文件名是上传 YAML 的主名加 `.zip`。历史开关打开时同时写入 `/root/history/generator`，默认不写。

multipart。`input` 是 YAML 文件，至少含 `generator`。

入参：`input`。

出参 `data`：该 YAML 转成的对象。`file`：配置 zip。失败时抛出运行时错误，连接中断。

### POST `/ai_stream2/generator/history/enable`

入参：无。之后 `POST /generate` 成功时把 zip 写入 `/root/history/generator`。进程重启后恢复为关闭。

出参 `data`：`{}`。`message`：空。`file`：`null`。

### POST `/ai_stream2/generator/history/disable`

入参：无。之后不再落盘历史。已有 zip 的查询、下载、删除不受影响。

出参 `data`：`{}`。`message`：空。`file`：`null`。

### GET `/ai_stream2/generator/history`

`filename` 可省略。只返回扩展名为 `.zip` 的文件，键为文件名，值为包内 `params.yml`；没有 `params.yml` 时值为 `{}`。

入参（query）：`filename`。

出参 `data`：`{文件名: params 对象}`。`message`：空。`file`：`null`。

### POST `/ai_stream2/generator/history`

`filename` 可省略。省略时下载目录下全部 `.zip`。存在时每个命中的 zip 作为 `data` 里的一个 part 返回。外层字段 `success`、`data`、`message`；块内再含 `success`、`data`（该 zip 的 `params.yml`）、`message`、`file`（zip）。指定了但不存在时 `400`，`message` 为 `history file not found: ` 加文件名。

入参（query）：`filename`。

出参：multipart。不存在 `400`。

### DELETE `/ai_stream2/generator/history`

`filename` 可省略。省略时删除目录下全部 `.zip`。指定了但不存在时 `400`，`message` 为 `history file not found: ` 加文件名。

入参（query）：`filename`。

出参 `data`：`{}`。`message`：空。不存在 `400`。

### POST `/ai_stream2/generator/logs/enable`

入参：无。之后每个请求写入 `/root/logs/generator` 下的 jsonl。进程重启后恢复为关闭。

出参 `data`：`{}`。`message`：空。`file`：`null`。

### POST `/ai_stream2/generator/logs/disable`

入参：无。之后不再写请求日志。已有日志文件保留。

出参 `data`：`{}`。`message`：空。`file`：`null`。

### DELETE `/ai_stream2/generator/logs`

入参：无。删除容器内 `/root/logs/generator` 下的日志文件，目录保留。

出参 `data`：`{}`。`message`：空。`file`：`null`。

---

## 2. Script

路径相对仓库根。`3_*` 打到 `http://127.0.0.1:${GENERATOR_HOST_PORT:-8091}`，请求头带 `Accept-Language: en`，非 `200` 以非零退出。stdout 是一行 JSON：`request`（`method`、`url`、`headers`、`content`）和 `response`（`code`、`content`）。带 zip 的响应 `content` 记为 `multipart(zip)`，不打印文件字节。

| 脚本 | 参数 | 作用 |
|------|------|------|
| `scripts/0_pull_base_image.sh` | 无 | 拉取 `debian:bookworm-slim` |
| `scripts/1_build_image.sh` | 无 | 用本目录 `Dockerfile` 构建 `ai_stream2_generator` |
| `scripts/2_run_dev_container.sh` | 无 | 重建并启动容器，挂载仓库根的 `models`、`history`、`logs`、`attachments` |
| `scripts/3_health.sh` | 无 | `GET /health` |
| `scripts/3_generate.sh` | `--yaml` | `POST /generate` |
| `scripts/3_generate_all.sh` | 无 | 对 `templates` 下每个 yaml 调用 `3_generate.sh` |
| `scripts/3_enable_history.sh` | 无 | `POST /history/enable` |
| `scripts/3_disable_history.sh` | 无 | `POST /history/disable` |
| `scripts/3_get_history.sh` | `--filename`（可省略） | `GET /history` |
| `scripts/3_download_history.sh` | `--filename`（可省略） | `POST /history` |
| `scripts/3_remove_history.sh` | `--filename`（可省略） | `DELETE /history` |
| `scripts/3_enable_logs.sh` | 无 | `POST /logs/enable` |
| `scripts/3_disable_logs.sh` | 无 | `POST /logs/disable` |
| `scripts/3_remove_logs.sh` | 无 | `DELETE /logs` |

### `0_pull_base_image.sh`

`docker pull debian:bookworm-slim`。

### `1_build_image.sh`

构建上下文是 `servers/generator`，标签 `ai_stream2_generator`。

### `2_run_dev_container.sh`

先建网络 `ai_stream2`，删掉同名旧容器再 `docker run -d`。`-p 8091:8091`，挂载 `/etc/localtime` 以及仓库根的 `models`、`history`、`logs`、`attachments` 到容器 `/root/models`、`/root/history`、`/root/logs`、`/root/attachments`。结束后打印 `http://127.0.0.1:8091/ai_stream2/generator/health`。

### `3_health.sh`

无参数。

### `3_generate.sh`

`--yaml` 必填，须是已存在的文件。用法：`scripts/3_generate.sh --yaml path/to/config.yaml`。

### `3_generate_all.sh`

无参数。按文件名排序依次调用 `3_generate.sh`。用法：`scripts/3_generate_all.sh`。

### `3_enable_history.sh`

无参数。用法：`scripts/3_enable_history.sh`。

### `3_disable_history.sh`

无参数。用法：`scripts/3_disable_history.sh`。

### `3_get_history.sh`

`--filename` 可省略。省略时 `filename` 为 JSON `null`，列出全部 `.zip`。用法：`scripts/3_get_history.sh [--filename file.zip]`。

### `3_download_history.sh`

`--filename` 可省略。省略时 `filename` 为 JSON `null`，下载全部 `.zip`。用法：`scripts/3_download_history.sh [--filename file.zip]`。

### `3_remove_history.sh`

`--filename` 可省略。省略时删除全部 `.zip`。用法：`scripts/3_remove_history.sh [--filename file.zip]`。

### `3_enable_logs.sh`

无参数。用法：`scripts/3_enable_logs.sh`。

### `3_disable_logs.sh`

无参数。用法：`scripts/3_disable_logs.sh`。

### `3_remove_logs.sh`

无参数。用法：`scripts/3_remove_logs.sh`。
