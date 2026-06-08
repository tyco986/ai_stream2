# FFmpeg API 服务

基于 FastAPI 封装 `ffmpeg` / `ffprobe`：RTSP 推流（MediaMTX）、截图、去 B 帧。

**依赖**：Docker（推荐）或 Python 3.12 + 系统 `ffmpeg`；联调需 [MediaMTX](https://github.com/bluenviron/mediamtx)（RTSP **8554**）。

## 快速启动

在项目根目录执行：

```bash
bash servers/mediamtx/scripts/1_build_container.sh   # 可选，RTSP 8554
bash servers/ffmpeg/scripts/0_pull_base_image.sh     # 可选
bash servers/ffmpeg/scripts/1_build_image.sh
bash servers/ffmpeg/scripts/2_build_container.sh
```

HTTP：`http://127.0.0.1:8080`，Swagger：`http://127.0.0.1:8080/docs`。

## 目录与挂载

`2_build_container.sh` 将宿主机目录挂到容器内固定路径（`main.py` 通过环境变量使用容器路径）：

| 宿主机 | 容器 | 用途 |
|--------|------|------|
| `attachments/videos/` | `/app/video` | 测试视频（只读） |
| `outputs/ffmpeg/` | `/app/output` | API 生成文件 |
| `logs/ffmpeg/` | `/app/log` | 滚动日志 `app.log` |
| `media/` | `/media` | 可选外部媒体（只读） |

容器环境变量（见 `Dockerfile`）：`OUTPUT_ROOT`、`LOG_ROOT`、`VIDEO_ROOT`、`MEDIAMTX_HOST=ai_stream2_mediamtx`、`MEDIAMTX_RTSP_PORT=8554`。

## 请求路径写法

`input` / `output` 支持以下形式（会解析为实际文件路径）：

| 写法 | 说明 |
|------|------|
| `/app/video/...` | 容器内路径（脚本、测试默认） |
| `attachments/videos/...` | 相对项目根（宿主机 CLI、curl 脚本默认） |
| `/app/output/...`、`outputs/ffmpeg/...` | 生成目录下的文件 |

默认测试视频：`attachments/videos/video1.mp4` → API `/app/video/video1.mp4`。

## MediaMTX

`configs/mediamtx/mediamtx.yml` 需允许 publisher，例如：

```yaml
paths:
  all_others:
    source: publisher
```

否则 `video2rtsp` 报 `path 'xxx' is not configured`。

### RTSP 地址

推流默认：`rtsp://{MEDIAMTX_HOST}:8554/{stem}`（`stem` 为输入文件名无扩展名）。Docker 内 API 返回 `ai_stream2_mediamtx`；宿主机播放/探测用 `127.0.0.1:8554`，路径相同。容器内勿用 `127.0.0.1` 访问 MediaMTX。

## 本地开发（无 Docker）

```bash
cd servers/ffmpeg
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

未设置环境变量时，默认使用项目根下 `outputs/ffmpeg`、`logs/ffmpeg`、`attachments/videos`（与容器挂载的宿主机路径一致）。

| 参数 | 默认 |
|------|------|
| `--host` / `--port` | `0.0.0.0` / `8080` |
| `--output-root` | `OUTPUT_ROOT` 或 `outputs/ffmpeg` |
| `--log-root` | `LOG_ROOT` 或 `logs/ffmpeg` |
| `--video-root` | `VIDEO_ROOT` 或 `attachments/videos` |
| `--mediamtx-host` / `--mediamtx-rtsp-port` | `MEDIAMTX_HOST` 或 `127.0.0.1` / `8554` |

## API

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ffmpeg/hello_world` | 健康检查 |
| POST | `/ffmpeg/video2rtsp` | 后台 ffmpeg 推 RTSP（`loop` 默认 true） |
| GET | `/ffmpeg/video2rtsp_list` | 活跃推流列表 |
| POST | `/ffmpeg/video2rtsp_stop` | `rtsp` 或 `"all"` |
| POST | `/ffmpeg/frame_extract` | 按 `timestamp` 截图 |
| POST | `/ffmpeg/remove_B_frame` | 重编码去 B 帧 |
| POST | `/ffmpeg/rtsp_info` | `ffprobe` 探测 RTSP |

响应均含 `success`、`message`；失败时为非 2xx，体中无 FastAPI 默认 `detail`。

**`video2rtsp`**：`input` 必填；`rtsp` 可选；同 RTSP 已在推流 → `409`。仅启动进程，不保证 MediaMTX 收流成功。

**`frame_extract`**：`timestamp` 为 `HH:MM:SS` 或 `HH:MM:SS.mmm`；默认输出 `{output-root}/frame_extract/{stem}_{ts}.png`。

**`remove_B_frame`**：默认输出 `{output-root}/remove_B_frame/{stem}_B0.mp4`。

**`rtsp_info`**：`rtsp` 须以 `rtsp://` 开头。

| 状态码 | 场景 |
|--------|------|
| 400 | `rtsp_info` URL 非法 |
| 404 | 输入不存在；`video2rtsp_stop` 未在推流 |
| 409 | `video2rtsp` RTSP 冲突 |
| 422 | 体验证 / `timestamp` 非法 |
| 500 | `ffmpeg` / `ffprobe` 失败 |

请求示例：

```json
{ "input": "/app/video/video1.mp4", "rtsp": null, "loop": true }
{ "input": "/app/video/video1.mp4", "timestamp": "00:01:23", "output": null }
{ "rtsp": "rtsp://127.0.0.1:8554/video1" }
```

## 脚本与测试

`servers/ffmpeg/scripts/`（项目根执行）：`3_video2rtsp.sh`、`3_frame_extract.sh`、`3_remove_B_frame.sh` 等，默认输入 `attachments/videos/video1.mp4`。

`servers/ffmpeg/tests/`：`test_*.py` 默认容器路径 `/app/video/video1.mp4`。

```bash
bash servers/ffmpeg/tests/test_all_on_host.sh
bash servers/ffmpeg/tests/preview_rtsp.sh   # ffplay，默认 rtsp://127.0.0.1:8554/video1
```

`test_all.py` 顺序覆盖 hello → remove_B_frame → video2rtsp → list → rtsp_info → frame_extract → stop。

## 输出与日志

```
outputs/ffmpeg/
  frame_extract/
  remove_B_frame/
logs/ffmpeg/app.log    # 1MB × 10 轮转；请求与异常
```
