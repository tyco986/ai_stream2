# FFmpeg API

FastAPI 封装 `ffmpeg` / `ffprobe`：RTSP 推流、探测、截图、抽帧、去 B 帧。默认端口 `8080`。

## 镜像

| 镜像 | 构建 | 容器 | 说明 |
|------|------|------|------|
| `${PROJECT_NAME}_ffmpeg_dev` | `1_build_dev_image.sh` | `${PROJECT_NAME}_ffmpeg` | 挂代码；`python main.py` |
| `${PROJECT_NAME}_ffmpeg_prod` | `1_build_prod_image.sh` | 同上 | Nuitka 编译 `main` + `utils` 为 `.so` |

网络：`${PROJECT_NAME}_default`（推流需同网 MediaMTX）。

## 构建与运行

```bash
# 项目根目录
./servers/ffmpeg/scripts/0_pull_base_image.sh      # 可选
./servers/ffmpeg/scripts/1_build_dev_image.sh      # 或 1_build_prod_image.sh
./servers/ffmpeg/scripts/2_run_dev_container.sh    # 或 2_run_prod_container.sh
./servers/mediamtx/scripts/1_run_container.sh      # publishers 需要
```

健康检查：`http://127.0.0.1:8080/ai_stream2/ffmpeg/health`（前缀随 `${PROJECT_NAME}`）  
Swagger：`http://127.0.0.1:8080/docs`

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/ffmpeg` | `/app` | 仅 dev：Python 代码 |
| `recordings/` | `/root/recordings` | `video/capture` 的本地路径 |
| `outputs/` | `/root/outputs` | 截图 / 抽帧 / nob |
| `logs/` | `/root/logs` | 服务日志 |
| `project.env` | `/project.env` | 项目名等 |

容器内另有 `/root/tmp`（上传临时文件）。

## 接口

前缀：`/{PROJECT_NAME}/ffmpeg`。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查 |
| POST | `/rtsp/publishers` | 上传视频并推 RTSP |
| GET | `/rtsp/publishers` | 活跃推流 |
| DELETE | `/rtsp/publishers` | 停止全部 |
| DELETE | `/rtsp/publishers/{name}` | 停一路 |
| POST | `/rtsp/probe` | 单路探测 |
| POST | `/rtsp/batch/probe` | 批量探测 |
| POST | `/video/capture` | 截图 |
| POST | `/video/extract` | 抽帧（每 N 帧一张） |
| POST | `/video/nob` | 无 B 帧转码，返回 mp4 |

## 约定

JSON 接口外壳：`{"success", "message", "data", "command"}`。`nob` 成功为 `video/mp4`，头含 `X-Command`。

| 接口 | 格式 | 字段 |
|------|------|------|
| `video/capture` | multipart | `input`（容器内路径）、`timestamp`（`HH:MM:SS[.mmm]`） |
| `video/extract` | multipart | 文件 `input`；`interval`（默认 1） |
| `video/nob` | multipart | 文件 `input` |
| `rtsp/publishers` POST | multipart | 文件 `input`；可选 `name`、`loop`（默认 true）、`mediamtx_host` / `mediamtx_port`（默认 `${PROJECT_NAME}_mediamtx:8554`） |
| `rtsp/probe` | JSON | `{"rtsp":"..."}` |
| `rtsp/batch/probe` | JSON | `{"rtsps":["rtsp://..."]}` |

产物：截图 `/root/outputs/ffmpeg/capture`，抽帧 `/root/outputs/ffmpeg/extract/{basename}`，nob `/root/outputs/nob`。

## 命令行

```bash
./servers/ffmpeg/scripts/3_capture.sh --input recordings/video1.mp4 --timestamp 00:00:01
./servers/ffmpeg/scripts/3_extract.sh --input path/to/video.mp4
./servers/ffmpeg/scripts/3_extract.sh --input path/to/video.mp4 --interval 5
./servers/ffmpeg/scripts/3_nob.sh --input path/to/video.mp4 --output ./out_nob.mp4
./servers/ffmpeg/scripts/3_publishers.sh --input path/to/video.mp4
./servers/ffmpeg/scripts/3_publishers.sh --input path/to/video.mp4 --name cam1
./servers/ffmpeg/scripts/3_publishers_info.sh
./servers/ffmpeg/scripts/3_publishers_stop.sh
./servers/ffmpeg/scripts/3_publishers_stop.sh --name cam1
./servers/ffmpeg/scripts/3_rtsp_probe.sh --rtsp rtsp://127.0.0.1:8554/video1
./servers/ffmpeg/scripts/3_rtsp_batch_probe.sh --rtsp rtsp://a --rtsp rtsp://b
```

## 服务参数

| 项 | 默认 |
|----|------|
| `PROJECT_NAME` | 环境 / `project.env` |
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `8080` |
| 日志目录 | `/root/logs/ffmpeg`（硬编码） |
