# updator

前缀：`/ai_stream2/updator`。监听 `0.0.0.0:8095`。镜像 `ai_stream2_updator`，容器名相同，端口 `8095:8095`。

JSON 成功外壳：`{success, message, data, file}`。`file` 无附件时为 `null`。没有 history，zip 不随 HTTP 返回。

离线包固定为容器内 `/root/updator/update.zip`（宿主机 `servers/updator/output/update.zip`）。请求日志默认关闭，由 `/logs/enable` 打开。

`pack` 经 `ai_stream2_docker_socket_proxy:2375` 把本机镜像推到 `127.0.0.1:5000`，再从 `ai_stream2_registry2:5000` 按层 digest 做集合差，打成 OCI layout 的 zip。`update` 解该 zip，校验 `from` 层是否仍在仓库，上传差集，写入 tag，再 `docker pull`。`from` 省略或空字符串表示全量。`from` 与 `to` 都可为 `latest`：先记下当前 digest 再 push。

`data` 键：`file_path`，以及每个镜像名下的 `tag`、`from`、`to`。`from` 为全量时是 `null`，否则为旧 manifest digest。`to` 为新 manifest digest。

zip 顶层每个镜像一个目录（名里的 `/` 换成 `_`）。目录内为 `oci-layout`、`index.json`、`blobs/sha256/<digest>`。`index.json` 注解 `org.opencontainers.image.ref.name` 为 `name:tag`，`ai_stream2.from` 为旧 digest（全量没有该项）。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/updator/health` | 无 | 空对象 | 健康检查 |
| `POST` | `/ai_stream2/updator/pack` | 镜像名 → `{from, to}` | `file_path` 与各镜像的 `tag`、`from`、`to` | 差分打成 `update.zip` |
| `POST` | `/ai_stream2/updator/update/offline` | `file_path`（可省略） | 同 `pack` | 用 zip 写入 registry2 并 pull |
| `POST` | `/ai_stream2/updator/logs/enable` | 无 | 空对象 | 打开请求日志 |
| `POST` | `/ai_stream2/updator/logs/disable` | 无 | 空对象 | 关闭请求日志 |
| `DELETE` | `/ai_stream2/updator/logs` | 无 | 空对象 | 清空日志目录下的日志文件 |

### GET `/ai_stream2/updator/health`

入参：无。

出参 `data`：`{}`。`message`：空。`file`：`null`。失败时连接被拒绝。

### POST `/ai_stream2/updator/pack`

JSON。键是仓库名，值是 `{from, to}`。`to` 必填。`from` 可省略。多次镜像打进同一个 zip。zip 写到 `/root/updator/update.zip`，不随响应返回。需要同一 Docker 网络里的 `ai_stream2_registry2` 与 `ai_stream2_docker_socket_proxy`。

入参：镜像名映射。

出参 `data`：`file_path` 与各镜像的 `tag`、`from`、`to`。`message`：空。`file`：`null`。失败抛出运行时错误，连接中断或 `500`。

### POST `/ai_stream2/updator/update/offline`

JSON。`file_path` 可省略，省略时为 `/root/updator/update.zip`。有 `ai_stream2.from` 时先确认旧层都在仓库，缺层则失败。已有 blob 跳过上传。写完 tag 后 pull `127.0.0.1:5000/<name>:<tag>`。

入参：`file_path`。

出参 `data`：与 `pack` 相同形状。`message`：空。`file`：`null`。失败抛出运行时错误，连接中断或 `500`。

### POST `/ai_stream2/updator/logs/enable`

入参：无。之后每个请求写入 `/root/logs/updator` 下的 jsonl。进程重启后恢复为关闭。

出参 `data`：`{}`。`message`：空。`file`：`null`。

### POST `/ai_stream2/updator/logs/disable`

入参：无。之后不再写请求日志。已有日志文件保留。

出参 `data`：`{}`。`message`：空。`file`：`null`。

### DELETE `/ai_stream2/updator/logs`

入参：无。删除容器内 `/root/logs/updator` 下的日志文件，目录保留。

出参 `data`：`{}`。`message`：空。`file`：`null`。

---

## 2. Script

路径相对仓库根。`3_*` 打到 `http://127.0.0.1:${UPDATOR_HOST_PORT:-8095}`，请求头带 `Accept-Language: en`，非 `200` 以非零退出。stdout 是一行 JSON：`request`（`method`、`url`、`headers`、`content`）和 `response`（`code`、`content`）。

| 脚本 | 参数 | 作用 |
|------|------|------|
| `scripts/0_pull_base_image.sh` | 无 | 拉取 `debian:13-slim` |
| `scripts/1_build_image.sh` | 无 | 用本目录 `Dockerfile` 构建 `ai_stream2_updator` |
| `scripts/2_run_container.sh` | 无 | 重建并启动容器，挂载 `output`、`logs` |
| `scripts/3_pack.sh` | JSON 字符串 | `POST /pack` |
| `scripts/3_update.sh` | `--file-path`（可省略） | `POST /update` |

### `0_pull_base_image.sh`

`docker pull debian:13-slim`。

### `1_build_image.sh`

构建上下文是 `servers/updator`，标签 `ai_stream2_updator`。

### `2_run_container.sh`

先建网络 `ai_stream2`，删掉同名旧容器再 `docker run -d`。`-p 8095:8095`，挂载 `/etc/localtime`、`servers/updator/output` 到 `/root/updator`、仓库根 `logs` 到 `/root/logs`。结束后打印健康检查地址。

### `3_pack.sh`

第一个参数是 JSON 正文，必填。用法：`scripts/3_pack.sh '{"backend":{"from":"latest","to":"latest"}}'`。

### `3_update.sh`

`--file-path` 可省略。省略时请求体为 `{}`，服务端用默认 zip 路径。用法：`scripts/3_update.sh [--file-path /root/updator/update.zip]`。
