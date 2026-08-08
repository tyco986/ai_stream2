# FFmpeg API

FastAPI 封装 `ffmpeg` / `ffprobe`：RTSP 推流、探测、截图、去 B 帧。容器内运行。


| 文件                  | 职责                               |
| ------------------- | -------------------------------- |
| `main.py`           | 入口、`create_app`、lifespan、日志、异常映射 |
| `utils/api/constants.py` | 常量                               |
| `utils/api/routes.py`   | 路由                               |
| `utils/api/services.py` | 业务                                |
| `utils/api/schemas.py`  | 请求/响应模型                          |


## 构建与运行

```bash
# 项目根目录
./servers/ffmpeg/scripts/0_pull_base_image.sh      # 可选
./servers/ffmpeg/scripts/1_build_dev_image.sh      # 或 1_build_prod_image.sh
./servers/ffmpeg/scripts/2_run_dev_container.sh    # 或 2_run_prod_container.sh
./servers/mediamtx/scripts/1_run_container.sh      # publishers 需要
```


| 模式   | 镜像                            | 容器名                      | 说明                               |
| ---- | ----------------------------- | ------------------------ | -------------------------------- |
| dev  | `${PROJECT_NAME}_ffmpeg_dev`  | `${PROJECT_NAME}_ffmpeg` | 源码挂载；`python main.py`            |
| prod | `${PROJECT_NAME}_ffmpeg_prod` | 同上                       | Nuitka 编译 `main` + `utils` 为 `.so` |


网络：`${PROJECT_NAME}_default`（与 MediaMTX 等同网）。


|         | URL                                                        |
| ------- | ---------------------------------------------------------- |
| 健康检查    | `http://127.0.0.1:8080/${PROJECT_NAME}/ffmpeg/health` |
| Swagger | `http://127.0.0.1:8080/docs`                               |




## 目录


| 容器路径                           | 用途                                      |
| ------------------------------ | --------------------------------------- |
| `/root/recordings`             | 本地视频（`video/capture` 的 `input`）         |
| `/root/tmp`                    | 上传临时文件（`publishers` / `nob`） |
| `/root/outputs/ffmpeg/capture` | 截图 PNG                                  |
| `/root/outputs/nob`            | 无 B 帧 MP4                               |
| `/root/logs/ffmpeg`            | 滚动日志 `app.log`                          |




## 接口

前缀：`/${PROJECT_NAME}/ffmpeg`。


| 方法   | 路径                         | 说明                    |
| ---- | -------------------------- | --------------------- |
| GET  | `/health`                  | 健康检查                  |
| POST | `/rtsp/publishers`         | 上传视频并推 RTSP           |
| GET  | `/rtsp/publishers`         | 活跃推流列表                |
| DELETE | `/rtsp/publishers`       | 停止全部推流                |
| DELETE | `/rtsp/publishers/{name}` | 按 name 停止一路          |
| POST | `/rtsp/probe`               | 单路 RTSP 探测            |
| POST | `/rtsp/batch/probe`         | 批量探测                  |
| POST | `/video/capture`           | 截图                    |
| POST | `/video/nob`               | 无 B 帧转码（成功返回 mp4 文件） |




### 响应外壳

```json
{"success": true, "message": "", "data": {}, "command": "ffmpeg ..."}
```

失败：`success: false`，`data` 可为 `null`。业务载荷只在 `data`。


| 接口                     | 成功时 `data`                                                   |
| ---------------------- | ------------------------------------------------------------ |
| `rtsp/probe`            | `{"resolution":"1920x1080","fps":25}`（已解析，非 ffprobe 原文）      |
| `rtsp/batch/probe`      | `[{rtsp,success,resolution?,fps?}` 或 `{rtsp,success,error}]` |
| `rtsp/publishers` POST | `{name: rtsp_url}`                                           |
| `rtsp/publishers` GET  | `{name: rtsp_url, ...}`                                      |
| `rtsp/publishers` DELETE | 已停止的 `{name: rtsp_url, ...}`                            |
| `video/capture`        | PNG 路径字符串                                                    |
| `health`               | `null`                                                       |


`nob` 成功返回 `video/mp4` 文件流，响应头含 `X-Command`。

### 请求


| 接口                     | 格式        | 字段                                                                                                           |
| ---------------------- | --------- | ------------------------------------------------------------------------------------------------------------ |
| `video/capture`        | multipart | `input`（容器内路径）、`timestamp`（`HH:MM:SS` 或 `HH:MM:SS.mmm`，1～3 位小数）                                              |
| `video/nob`            | multipart | 文件字段 `input`                                                                                                 |
| `rtsp/publishers` POST | multipart | 文件 `input`；可选 `name`（默认文件名去后缀）、`loop`（默认 true）、`mediamtx_host`（默认 `${PROJECT_NAME}_mediamtx`）、`mediamtx_port`（默认 8554）；推流地址为 `rtsp://{host}:{port}/{name}` |
| `rtsp/probe`            | JSON      | `{"rtsp":"..."}`                                                                                             |
| `rtsp/batch/probe`      | JSON      | `{"rtsps":["rtsp://..."]}`                                                                                   |


`timestamp` 传给 ffmpeg 时不足 3 位小数会补零（如 `00:00:01.5` → `00:00:01.500`）。

## 命令行脚本

```bash
./servers/ffmpeg/scripts/3_capture.sh --input recordings/video1.mp4 --timestamp 00:00:01
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


| 变量 / CLI          | 默认                   |
| ----------------- | -------------------- |
| `HOST` / `--host` | `0.0.0.0`            |
| `PORT` / `--port` | `8080`               |
| `LOG_ROOT`（写死）    | `/root/logs/ffmpeg`  |
| `PROJECT_NAME`    | 来自环境 / `project.env` |


