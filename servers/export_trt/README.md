# export_trt

前缀：`/ai_stream2/export_trt`。监听 `0.0.0.0:8080`。镜像 `ai_stream2_export_trt`，容器名相同，端口 `8080:8080`。

JSON 成功外壳：`{success, message, data, file}`。`file` 无附件时为 `null`。`POST /export` 与 `POST /history` 成功时不套这层 JSON，改为 `multipart/form-data`，字段为 `success`、`message`、`data`（JSON）、`file`（zip）。

历史目录为容器内 `/root/history`，只认 `.zip`。

构建结果键：`type`、`batch_size`、`batch_mode`、`input_tensors`、`output_tensors`、`precision`、`opt_level`、`device`、`workspace`、`static_plugin`、`max_det`、`conf`、`iou`、`num_class`、`classes`、`status`、`built_at`、`trt_version`。`type`：`YOLO10-DET`、`YOLO11-DET`、`YOLO11-SEG`、`YOLO26-DET`、`YOLO26-POSE`、`YOLO26-SEG`、`RTMPOSE`、`STGCNPP`。`batch_mode`：`static` | `dynamic`。`precision`、`batch_size`、`device` 取自 config。`workspace` 默认 `8192`。`status` 成功为 `built`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/export_trt/health` | 无 | 空对象 | 健康检查 |
| `GET` | `/ai_stream2/export_trt/types` | 无 | `items` | 可导出的模型类型 |
| `POST` | `/ai_stream2/export_trt/export` | multipart：`input`、`config` | 构建结果对象 | 把 ONNX 包编成 TensorRT engine zip |
| `GET` | `/ai_stream2/export_trt/history` | query：`filename` | `{文件名: meta}` | 列出历史包的 meta |
| `POST` | `/ai_stream2/export_trt/history` | query：`filename` | 每个文件一块 multipart | 下载 zip |
| `DELETE` | `/ai_stream2/export_trt/history` | query：`filename` | 空对象 | 删除 zip |

### GET `/ai_stream2/export_trt/health`

入参：无。

出参 `data`：`{}`。`message`：空。`file`：`null`。失败时连接被拒绝。

### GET `/ai_stream2/export_trt/types`

入参：无。

出参 `data.items`：见上文 `type` 列表。`message`：空。`file`：`null`。

### POST `/ai_stream2/export_trt/export`

成功时 `Content-Type` 为 `multipart/form-data; boundary=ai_stream2_export_trt`。zip 内含一个 `.engine`、`labels.txt`、`meta.json`。文件名是上传文件主名加 `.zip`。同时写入 `/root/history`。

multipart。`input` 是 zip 文件，内含一个 `.onnx`、`labels.txt`、`meta.json`。`config` 是 YAML 文件，至少含 `type`、`precision`、`batch_size`、`device`。`batch_mode`、张量、`max_det`、`conf`、`iou`、`num_class`、`classes` 取自 zip 内 `meta.json`。`opt_level` 可省略，省略时为 `null`。`device` 传给 `cudaSetDevice`。`batch_size` 在 `batch_mode` 为 `dynamic` 时作为 optimization profile 的 opt/max。

`YOLO11-SEG`、`YOLO26-SEG` 的 `static_plugin` 为 `/opt/ai_stream2/servers/export_trt/libs/libnvdsinfer_custom_impl_Yolo_seg.so`，其余类型为 `null`。

入参：`input`、`config`。

出参 `data`：构建结果对象。`file`：engine zip。构建失败抛出运行时错误，连接中断。

### GET `/ai_stream2/export_trt/history`

`filename` 可省略。只返回扩展名为 `.zip` 的文件，键为文件名，值为包内 `meta.json`；没有 `meta.json` 时值为 `{}`。

入参（query）：`filename`。

出参 `data`：`{文件名: meta 对象}`。`message`：空。`file`：`null`。

### POST `/ai_stream2/export_trt/history`

`filename` 可省略。省略时下载目录下全部 `.zip`。存在时每个命中的 zip 作为 `data` 里的一个 part 返回。外层字段 `success`、`data`、`message`；块内再含 `success`、`data`（该 zip 的 `meta.json`）、`message`、`file`（zip）。指定了但不存在时 `400`，`message` 为 `history file not found: ` 加文件名。

入参（query）：`filename`。

出参：multipart。不存在 `400`。

### DELETE `/ai_stream2/export_trt/history`

`filename` 可省略。省略时删除目录下全部 `.zip`。指定了但不存在时 `400`，`message` 为 `history file not found: ` 加文件名。

入参（query）：`filename`。

出参 `data`：`{}`。`message`：空。不存在 `400`。

---

## 2. Script

路径相对仓库根。`3_*` 打到 `http://127.0.0.1:${EXPORT_TRT_HOST_PORT:-8080}`，请求头带 `Accept-Language: en`，非 `200` 以非零退出。stdout 是一行 JSON：`request`（`method`、`url`、`headers`、`content`）和 `response`（`code`、`content`）。带 zip 的响应 `content` 记为 `multipart(zip)`，不打印文件字节。

| 脚本 | 参数 | 作用 |
|------|------|------|
| `scripts/0_pull_base_image.sh` | 无 | 拉取 `nvcr.io/nvidia/deepstream:9.0-triton-multiarch` |
| `scripts/1_build_image.sh` | 无 | 用本目录 `Dockerfile` 构建 `ai_stream2_export_trt` |
| `scripts/2_run_container.sh` | 无 | 重建并启动容器，挂载仓库根的 `models`、`history`、`logs` |
| `scripts/3_health.sh` | 无 | `GET /health` |
| `scripts/3_export.sh` | `--input`、`--config` | `POST /export` |
| `scripts/3_get_history.sh` | `--filename`（可省略） | `GET /history` |
| `scripts/3_download_history.sh` | `--filename`（可省略） | `POST /history` |
| `scripts/3_remove_history.sh` | `--filename`（可省略） | `DELETE /history` |
| `scripts/4_rebuild_src.sh` | 无 | 在运行中的容器里编译并覆盖 `/app/export_trt`，然后 `docker restart` |

### `0_pull_base_image.sh`

`docker pull nvcr.io/nvidia/deepstream:9.0-triton-multiarch`。

### `1_build_image.sh`

构建上下文是 `servers/export_trt`，标签 `ai_stream2_export_trt`。

### `2_run_container.sh`

先建网络 `ai_stream2_default`，删掉同名旧容器再 `docker run -d`。`--gpus all`，`-p 8080:8080`，挂载 `/etc/localtime` 以及仓库根的 `models`、`history`、`logs` 到容器 `/root/models`、`/root/history`、`/root/logs`。结束后打印 `http://127.0.0.1:8080/ai_stream2/export_trt/health`。

### `3_health.sh`

无参数。

### `3_export.sh`

`--input`、`--config` 都必填，须是已存在的文件。用法：`scripts/3_export.sh --input path/to/model.zip --config path/to/config.yaml`。

### `3_get_history.sh`

`--filename` 可省略。省略时 `filename` 为 JSON `null`，列出全部 `.zip`。用法：`scripts/3_get_history.sh [--filename file.zip]`。

### `3_download_history.sh`

`--filename` 可省略。省略时 `filename` 为 JSON `null`，下载全部 `.zip`。用法：`scripts/3_download_history.sh [--filename file.zip]`。

### `3_remove_history.sh`

`--filename` 可省略。省略时删除全部 `.zip`。用法：`scripts/3_remove_history.sh [--filename file.zip]`。

### `4_rebuild_src.sh`

对容器 `ai_stream2_export_trt` 执行 `cmake --build /tmp/build`，把产物拷到 `/app/export_trt`，然后重启该容器。
