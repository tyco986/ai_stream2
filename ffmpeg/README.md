# FFmpeg API 服务

基于 FastAPI 封装 `ffmpeg` / `ffprobe`，提供 RTSP 推流（MediaMTX）、视频截图与 B 帧移除能力。

## 环境要求

- Docker（推荐），或 Python 3.12 + 系统已安装 `ffmpeg`
- [MediaMTX](https://github.com/bluenviron/mediamtx)（RTSP 推流联调，默认端口 **8554**）

## Docker 构建与运行

容器入口为 `python main.py`（内部通过 `uvicorn.run` 启动 FastAPI）。

推荐通过 demo 脚本启动（项目根目录）：

```bash
bash demo/4_ffmpeg/0_pull_base_image.sh   # 可选，预拉 python:3.12-slim
bash demo/1_mediamtx/1_build_container.sh
bash demo/4_ffmpeg/1_build_image.sh
bash demo/4_ffmpeg/2_build_container.sh
```

`2_build_container.sh` 默认挂载：

| 容器路径 | 宿主机 | 说明 |
|---------|--------|------|
| `/app/output` | `ffmpeg/output` | 生成文件 |
| `/app/log` | `ffmpeg/log` | 日志 |
| `/app/video` | `ffmpeg/video` | 内置测试视频（只读） |
| `/media` | `media/` | 可选外部视频（只读） |

容器内环境变量（`2_build_container.sh` 默认）：

- `MEDIAMTX_HOST=ai_stream2_mediamtx`
- `MEDIAMTX_RTSP_PORT=8554`

也可手动构建运行：

```bash
cd ffmpeg
docker build -t ai_stream2_ffmpeg .
docker run -d --name ai_stream2_ffmpeg --network ai_stream2_default \
  -p 8080:8080 \
  -v "$(pwd)/output:/app/output" \
  -v "$(pwd)/log:/app/log" \
  -v "$(pwd)/video:/app/video:ro" \
  -e MEDIAMTX_HOST=ai_stream2_mediamtx \
  ai_stream2_ffmpeg
```

## MediaMTX 前置条件

MediaMTX 需允许 RTSP publisher 推流。`mediamtx/mediamtx.yml` 中除 `preview`（DeepStream UDP）外，需配置：

```yaml
paths:
  all_others:
    source: publisher
```

否则 `video2rtsp` 推流会失败（`path 'xxx' is not configured`）。

## RTSP 地址：容器 vs 宿主机

推流 API 返回的 RTSP 主机名来自 `MEDIAMTX_HOST`。Docker 部署默认为 `ai_stream2_mediamtx`：

```
rtsp://ai_stream2_mediamtx:8554/{stem}
```

路径 `{stem}` 为输入文件名不含扩展名（如 `video1_B0.mp4` → `video1_B0`）。

| 访问方 | RTSP 地址 | 说明 |
|--------|-----------|------|
| 同 Docker 网络内的容器 | `rtsp://ai_stream2_mediamtx:8554/{stem}` | API 返回值，可直接使用 |
| 宿主机（host） | `rtsp://127.0.0.1:8554/{stem}` | 通过端口映射访问，hostname 不同但路径相同 |

容器内 `127.0.0.1` 指向自身，**不能**用于访问 MediaMTX。

## 本地开发

```bash
cd ffmpeg
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python main.py --host 0.0.0.0 --port 8080 --output-root ./output --log-root ./log
```

### 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--host` | `0.0.0.0` | HTTP 监听地址 |
| `--port` | `8080` | HTTP 端口 |
| `--output-root` | 环境变量 `OUTPUT_ROOT` 或 `/app/output` | 生成文件根目录 |
| `--log-root` | 环境变量 `LOG_ROOT` 或 `/app/log` | 滚动日志目录 |
| `--mediamtx-host` | 环境变量 `MEDIAMTX_HOST` 或 `127.0.0.1` | 默认 RTSP 主机名 |
| `--mediamtx-rtsp-port` | 环境变量 `MEDIAMTX_RTSP_PORT` 或 `8554` | 默认 RTSP 端口 |

## API 说明

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ffmpeg/hello_world` | 健康检查 |
| POST | `/ffmpeg/video2rtsp` | 将视频发布为 RTSP（后台 ffmpeg 进程） |
| GET | `/ffmpeg/video2rtsp_list` | 列出当前活跃的 RTSP 推流 |
| POST | `/ffmpeg/video2rtsp_stop` | 按 RTSP 地址停止推流（`all` 停止全部） |
| POST | `/ffmpeg/frame_extract` | 按时间戳截图 |
| POST | `/ffmpeg/remove_B_frame` | 重编码并移除 B 帧 |

### `POST /ffmpeg/video2rtsp`

```json
{
  "input": "/app/output/remove_B_frame/video1_B0.mp4",
  "rtsp": "rtsp://ai_stream2_mediamtx:8554/video1_B0",
  "loop": true
}
```

- `rtsp` 可选；默认为 `rtsp://{mediamtx-host}:{mediamtx-rtsp-port}/{stem}`。
- `loop` 可选，默认 `true`；为 `true` 时循环播放（`-stream_loop -1`），为 `false` 时播完即停。
- 响应字段：`input`、`rtsp`、`pid`、`loop`、`command`。
- 同一 RTSP 地址已在推流时返回 `409`。
- 输入含 B 帧时返回 `400`，需先调用 `remove_B_frame`。

### `GET /ffmpeg/video2rtsp_list`

```json
{
  "publishers": [
    {
      "input": "/app/output/remove_B_frame/video1_B0.mp4",
      "rtsp": "rtsp://ai_stream2_mediamtx:8554/video1_B0",
      "pid": 123,
      "loop": true
    }
  ]
}
```

- 仅包含仍在运行的 ffmpeg 推流进程；已退出的会自动从列表移除。

### `POST /ffmpeg/video2rtsp_stop`

```json
{
  "rtsp": "rtsp://ai_stream2_mediamtx:8554/video1_B0"
}
```

- `rtsp` 为 `"all"` 时停止全部推流。
- `rtsp` 须与推流时使用的地址一致（见上文「RTSP 地址：容器 vs 宿主机」）。
- 响应字段：`stopped`（数组，每项含 `input`、`rtsp`、`pid`、`loop`）。
- 指定 RTSP 未在推流时返回 `404`。

### `POST /ffmpeg/frame_extract`

```json
{
  "input": "/app/video/video1.mp4",
  "timestamp": "00:01:23",
  "output": null
}
```

- `timestamp`：格式为 `HH:MM:SS` 或 `HH:MM:SS.mmm`；省略毫秒时规范化为 **`.000`**。
- 默认输出：`{output-root}/frame_extract/{stem}_{sanitized_ts}.png`（如 `video1_00-01-23-000.png`）。
- 响应字段：`input`、`timestamp`、`output`、`command`。

### `POST /ffmpeg/remove_B_frame`

```json
{
  "input": "/app/video/video1.mp4",
  "output": null
}
```

- 默认输出：`{output-root}/remove_B_frame/{stem}_B0.mp4`
- 响应字段：`input`、`output`、`command`。

### 常见 HTTP 状态码

| 状态码 | 场景 |
|--------|------|
| `400` | `video2rtsp`：输入含 B 帧 |
| `404` | 输入文件不存在；或 `video2rtsp_stop` 指定 RTSP 未在推流 |
| `409` | `video2rtsp`：同一 RTSP 地址已在推流 |
| `422` | `timestamp` 格式无效 |
| `500` | `ffmpeg` / `ffprobe` 执行失败 |

## 测试脚本

`ffmpeg/tests/` 下提供 `requests` + `argparse` 测试脚本，成功时打印完整 JSON 响应。

| 脚本 | 说明 |
|------|------|
| `test_hello_world.py` | 健康检查 |
| `test_remove_B_frame.py` | 去 B 帧（默认 `/app/video/video1.mp4`） |
| `test_frame_extract.py` | 截图 |
| `test_video2rtsp.py` | 推流（默认 `/app/output/remove_B_frame/video1_B0.mp4`） |
| `test_video2rtsp_list.py` | 列出活跃推流 |
| `test_video2rtsp_stop.py` | 停止推流（默认 `rtsp=all`） |
| `test_all.py` | 依次运行以上全部测试 |
| `test_all.sh` | 容器内一键运行 `test_all.py` |
| `test_all_on_host.sh` | 宿主机通过 `docker exec` 运行 `test_all.py` |
| `preview_rtsp.sh` | 宿主机 ffplay 预览 RTSP（默认 `rtsp://127.0.0.1:8554/video1_B0`） |

```bash
# 容器内
docker exec ai_stream2_ffmpeg python3 /app/tests/test_all.py

# 宿主机
bash ffmpeg/tests/test_all_on_host.sh

# 宿主机预览 RTSP
bash ffmpeg/tests/preview_rtsp.sh
```

## Swagger / OpenAPI

- Swagger UI：[http://localhost:8080/docs](http://localhost:8080/docs)
- ReDoc：[http://localhost:8080/redoc](http://localhost:8080/redoc)

### 烟测顺序

1. **GET** `/ffmpeg/hello_world`
2. **POST** `/ffmpeg/remove_B_frame`（源视频含 B 帧时需要）
3. **POST** `/ffmpeg/video2rtsp`
4. **GET** `/ffmpeg/video2rtsp_list`
5. **POST** `/ffmpeg/frame_extract`
6. **POST** `/ffmpeg/video2rtsp_stop` → `{"rtsp":"all"}`

## B 帧处理流程

```
video2rtsp（400，含 B 帧）
    → remove_B_frame
    → video2rtsp（200，开始推流）
```

## 输出目录结构

```
{output-root}/
  frame_extract/
    video1_00-01-23-000.png
  remove_B_frame/
    video1_B0.mp4
```

## 日志

- 滚动文件：`{log-root}/app.log`（单文件最大 1MB，保留 10 个备份）
- 记录每次请求：方法、路径、客户端 IP、状态码、耗时
- 业务 `HTTPException`（400/404/409）记为 warning
- 未捕获异常与 `ffmpeg`/`ffprobe` 失败：完整 traceback（`logger.exception`）
