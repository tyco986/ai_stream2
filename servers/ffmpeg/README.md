# FFmpeg API

在 `ai_stream2_ffmpeg` 镜像中运行的 FastAPI 服务，封装 `ffmpeg` / `ffprobe`：RTSP 推流、截图、去 B 帧。

## 构建与运行

```bash
# 项目根目录
./servers/ffmpeg/scripts/0_pull_base_image.sh   # 可选
./servers/ffmpeg/scripts/1_build_image.sh
./servers/ffmpeg/scripts/2_run_container.sh
./servers/mediamtx/scripts/1_run_container.sh # RTSP 推流时需要
```

API：`http://127.0.0.1:8080/ai_stream2/ffmpeg/hello_world`  
Swagger：`http://127.0.0.1:8080/docs`  
容器名：`ai_stream2_ffmpeg`，与 MediaMTX 同网 `ai_stream2_default`（`video2rtsp` 推流到 `ai_stream2_mediamtx:8554`）。

## 目录

| 容器路径 | 用途 |
|----------|------|
| `/root/recordings` | 本地视频（`frame_extract` 的 `input`） |
| `/root/attachments/video` | 上传视频（`video2rtsp` / `remove_B_frame`，同名覆盖） |
| `/root/outputs/ffmpeg/frame_extract` | 截图 PNG（`frame_extract`） |
| `/root/outputs/remove_B_frame` | 去 B 帧 MP4（`remove_B_frame`） |
| `/root/logs/ffmpeg` | 滚动日志 |

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ai_stream2/ffmpeg/hello_world` | 健康检查 |
| POST | `/ai_stream2/ffmpeg/video2rtsp` | 上传视频并推 RTSP |
| GET | `/ai_stream2/ffmpeg/video2rtsp_list` | 活跃推流列表 |
| POST | `/ai_stream2/ffmpeg/video2rtsp_stop` | 停止推流 |
| POST | `/ai_stream2/ffmpeg/frame_extract` | 截图 |
| POST | `/ai_stream2/ffmpeg/remove_B_frame` | 去 B 帧 |
| POST | `/ai_stream2/ffmpeg/rtsp_info` | ffprobe 探测 RTSP |

### 响应格式

JSON 接口成功时 `message` 为字符串、对象或数组（如 `video2rtsp` 返回推流信息对象，`video2rtsp_list` 返回数组）。

```json
{"success": true, "message": {"input": "...", "rtsp": "...", "pid": 12, "loop": true}, "output": null, "command": "ffmpeg ..."}
```

JSON 接口失败：

```json
{"success": false, "message": "...", "output": null, "command": ""}
```

`remove_B_frame` 成功时返回文件（`video/mp4`），响应头含 `X-Command`、`X-Message`；失败返回 JSON。`frame_extract` 成功返回 JSON（`output` 为 `null`，`message` 为 PNG 路径）。

### 上传接口

`multipart/form-data`：

- `frame_extract`：`input`（容器内视频路径）、`timestamp`（见下）
- `remove_B_frame` / `video2rtsp`：`input`（视频文件）
- `video2rtsp` 可选：`rtsp`、`loop`（默认 `true`）、`mediamtx_host`（默认 `ai_stream2_mediamtx`）、`mediamtx_port`（默认 `8554`）

#### `frame_extract` 的 `timestamp`

格式：`HH:MM:SS` 或 `HH:MM:SS` + 小数秒（`.` 后 **1～3 位**）。时分秒均为 **两位数字**。

| 输入示例 | 传给 ffmpeg |
|----------|-------------|
| `00:00:01` | `00:00:01.000` |
| `00:00:01.5` | `00:00:01.500` |
| `00:00:01.50` | `00:00:01.500` |
| `00:00:01.500` | `00:00:01.500` |
| `01:23:45.1` | `01:23:45.100` |

不支持：空字符串、`0:0:1`、超过 3 位小数（如 `00:00:01.1234`）、仅小数点无数字（如 `00:00:01.`）。

`frame_extract` 示例：

```bash
curl -s -X POST http://127.0.0.1:8080/ai_stream2/ffmpeg/frame_extract \
  -F "input=/root/recordings/video1.mp4" \
  -F "timestamp=00:00:01"
```

产物在宿主机 `outputs/ffmpeg/frame_extract/`。

### JSON 请求体

- `video2rtsp_stop`：`{"rtsp": "rtsp://..."}` 或 `{"rtsp": "all"}`
- `rtsp_info`：`{"rtsp": "rtsp://..."}`

## 命令行脚本

```bash
./servers/ffmpeg/scripts/3_frame_extract.sh \
  --input recordings/video1.mp4 \
  --timestamp 00:00:01

./servers/ffmpeg/scripts/3_remove_B_frame.sh \
  --input attachments/videos/video1.mp4 \
  --output ./video1_B0.mp4

./servers/ffmpeg/scripts/3_video2rtsp.sh --input attachments/videos/video1.mp4
./servers/ffmpeg/scripts/3_video2rtsp_list.sh
./servers/ffmpeg/scripts/3_video2rtsp_stop.sh --rtsp all
./servers/ffmpeg/scripts/3_rtsp_info.sh --rtsp rtsp://127.0.0.1:8554/video1
```

JSON 接口与 `frame_extract` 成功时打印响应；`remove_B_frame` 成功时保存到 `--output`；失败时打印 JSON 错误。

## 服务参数

环境变量（Dockerfile 默认）：

| 变量 | 默认值 |
|------|--------|
| `HOST` | `0.0.0.0` |
| `PORT` | `8080` |
| `LOG_ROOT` | `/root/logs/ffmpeg` |

CLI 可覆盖（优先级高于环境变量）：

```bash
python main.py --host 0.0.0.0 --port 8080 --log-root /root/logs/ffmpeg
```

| 参数 | 默认值 |
|------|--------|
| `--host` | `$HOST` 或 `0.0.0.0` |
| `--port` | `$PORT` 或 `8080` |
| `--log-root` | `$LOG_ROOT` 或 `/root/logs/ffmpeg` |

日志：`{log-root}/app.log`（滚动，10×1MB）。
