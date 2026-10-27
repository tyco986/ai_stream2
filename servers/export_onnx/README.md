# export_onnx

前缀：`/ai_stream2/export_onnx`。监听 `0.0.0.0:8090`。镜像 `ai_stream2_export_onnx`，容器名相同，端口 `8090:8090`。

JSON 成功外壳：`{success, message, data, file}`。`file` 无附件时为 `null`。`POST /export` 与 `POST /history` 成功时不套这层 JSON，改为 `multipart/form-data`，字段为 `success`、`message`、`data`（JSON）、`file`（zip）。

历史目录为容器内 `/root/history`，只认 `.zip`。

构建结果键：`batch_mode`、`input_tensors`、`output_tensors`、`classes`、`num_class`、`source_path`、`batch_size`、`precision`、`version`、`max_det`、`conf`、`iou`。`version`：`YOLO10-DET`、`YOLO11-DET`、`YOLO11-SEG`、`YOLO26-DET`、`YOLO26-POSE`、`YOLO26-SEG`、`RTMPOSE`、`STGCNPP`。`batch_mode`：`static` | `dynamic`。`precision` 为 `fp32`。`batch_mode` 为 `dynamic` 时 `batch_size` 为 `null`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/export_onnx/health` | 无 | 空对象 | 健康检查 |
| `GET` | `/ai_stream2/export_onnx/types` | 无 | `items` | 可导出的模型类型 |
| `POST` | `/ai_stream2/export_onnx/export` | multipart：`input`、`config`、`label` | 构建结果对象 | 把权重导出为 ONNX zip |
| `GET` | `/ai_stream2/export_onnx/history` | query：`filename` | `{文件名: meta}` | 列出历史包的 meta |
| `POST` | `/ai_stream2/export_onnx/history` | query：`filename` | 每个文件一块 multipart | 下载 zip |
| `DELETE` | `/ai_stream2/export_onnx/history` | query：`filename` | 空对象 | 删除 zip |

### GET `/ai_stream2/export_onnx/health`

入参：无。

出参 `data`：`{}`。`message`：空。`file`：`null`。失败时连接被拒绝。

### GET `/ai_stream2/export_onnx/types`

入参：无。

出参 `data.items`：见上文 `version` 列表。`message`：空。`file`：`null`。

### POST `/ai_stream2/export_onnx/export`

成功时 `Content-Type` 为 `multipart/form-data; boundary=export-onnx-boundary`。zip 内含一个 `.onnx`、`labels.txt`、`meta.json`。文件名是上传文件主名加 `.zip`。同时写入 `/root/history`。

multipart。`input` 是 `.pt` 或 `.onnx`。`config` 是 YAML，至少含 `type`。可省略：`size`（默认 `640`）、`opset`（默认 `18`）、`batch_size`（默认 `1`）、`batch_mode`（默认 `static`）、`simplify`（默认 `false`）、`max_det`（默认 `30`）、`conf`（默认 `0.25`）、`iou`（默认 `0.45`）。`label` 是类别文本，一行一类；`RTMPOSE`、`STGCNPP` 必填，其余类型忽略。

入参：`input`、`config`、`label`。

出参 `data`：构建结果对象。`file`：onnx zip。失败时 JSON，`success` 为 `false`，`message` 为错误文本。

### GET `/ai_stream2/export_onnx/history`

`filename` 可省略。只返回扩展名为 `.zip` 的文件，键为文件名，值为包内 `meta.json`。指定了但不存在时 `data` 为 `{}`。

入参（query）：`filename`。

出参 `data`：`{文件名: meta 对象}`。`message`：空。`file`：`null`。

### POST `/ai_stream2/export_onnx/history`

`filename` 可省略。省略时下载目录下全部 `.zip`。存在时每个命中的 zip 作为 `data` 里的一个 part 返回。外层字段 `success`、`data`、`message`；块内再含 `success`、`data`（该 zip 的 `meta.json`）、`message`、`file`（zip）。指定了但不存在时 `400`，`message` 为 `history file not found: ` 加文件名。

入参（query）：`filename`。

出参：multipart。不存在 `400`。

### DELETE `/ai_stream2/export_onnx/history`

`filename` 可省略。省略时删除目录下全部 `.zip`。指定了但不存在时 `400`，`message` 为 `history file not found: ` 加文件名。

入参（query）：`filename`。

出参 `data`：`{}`。`message`：空。不存在 `400`。

---

## 2. Script

路径相对仓库根。`3_*` 打到 `http://127.0.0.1:${EXPORT_ONNX_HOST_PORT:-8090}`，请求头带 `Accept-Language: en`，非 `200` 以非零退出。stdout 是一行 JSON：`request`（`method`、`url`、`headers`、`content`）和 `response`（`code`、`content`）。带 zip 的响应 `content` 记为 `multipart(zip)`，不打印文件字节。

| 脚本 | 参数 | 作用 |
|------|------|------|
| `scripts/0_pull_base_image.sh` | 无 | 拉取 `ultralytics/ultralytics:latest` |
| `scripts/1_build_image.sh` | 无 | 用本目录 `Dockerfile` 构建 `ai_stream2_export_onnx` |
| `scripts/2_run_container.sh` | 无 | 重建并启动容器，挂载仓库根的 `history`、`logs` |
| `scripts/3_health.sh` | 无 | `GET /health` |
| `scripts/3_export.sh` | `--input`、`--config`、`--label` | `POST /export` |
| `scripts/3_get_history.sh` | `--filename`（可省略） | `GET /history` |
| `scripts/3_download_history.sh` | `--filename`（可省略） | `POST /history` |
| `scripts/3_remove_history.sh` | `--filename`（可省略） | `DELETE /history` |
| `scripts/4_nuitka.sh` | 无 | 镜像构建阶段编译 `main.py`，不单独在宿主机跑 |

### `0_pull_base_image.sh`

`docker pull ultralytics/ultralytics:latest`。

### `1_build_image.sh`

构建上下文是仓库根，标签 `ai_stream2_export_onnx`。构建中会执行 `4_nuitka.sh`。

### `2_run_container.sh`

先建网络 `ai_stream2_default`，删掉同名旧容器再 `docker run -d`。`-p 8090:8090`，挂载 `/etc/localtime` 以及仓库根的 `history`、`logs` 到容器 `/root/history`、`/root/logs`。结束后打印 `http://127.0.0.1:8090/ai_stream2/export_onnx/health`。

### `3_health.sh`

无参数。

### `3_export.sh`

`--input`、`--config` 必填，须是已存在的文件。`--label` 可省略，`RTMPOSE`、`STGCNPP` 要带。用法：`scripts/3_export.sh --input path/to/model.pt --config path/to/config.yaml [--label path/to/labels.txt]`。

### `3_get_history.sh`

`--filename` 可省略。省略时 `filename` 为 JSON `null`，列出全部 `.zip`。用法：`scripts/3_get_history.sh [--filename file.zip]`。

### `3_download_history.sh`

`--filename` 可省略。省略时 `filename` 为 JSON `null`，下载全部 `.zip`。用法：`scripts/3_download_history.sh [--filename file.zip]`。

### `3_remove_history.sh`

`--filename` 可省略。省略时删除全部 `.zip`。用法：`scripts/3_remove_history.sh [--filename file.zip]`。

### `4_nuitka.sh`

在镜像构建阶段把 `main.py` 编成 `/app/export_onnx`，随后删除源码。容器入口是该二进制。
