# FFmpeg

JSON 成功外壳：`{success, message, data}`。

---

## API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/health` | 无 | `{}` | 健康检查 |
| `POST` | `/rtsp/probe` | `urls` | `{url: {status, width, height, fps, probed_at}}` | 探测多路 RTSP，只返回在线的 |
| `POST` | `/rtsp/publish` | `paths` | `{name: {url_docker, url_host, width, height, fps, codec, bitrate, loop, created_at, duration, status, probed_at}}` | 把本地视频循环发布为 RTSP，只返回本次成功的 |
| `GET` | `/rtsp/publish` | `name` 可选 | 同上，按 `name` 筛选 | 列出已发布的 RTSP |
| `DELETE` | `/rtsp/publish` | `name` 可选 | 删除后剩余的发布字典 | 停止对应推流并删除记录 |
| `POST` | `/video/probe` | `path` | `width`、`height`、`fps`、`duration`、`codec`、`bitrate` | 探测本地视频文件 |

### GET `/health`

入参：无。

出参 `data`：`{}`。`message`：空字符串。

### POST `/rtsp/probe`

对 `urls` 逐条用 `ffprobe` 探测第一路视频流，超时 5 秒。在线时 `status` 为 `online`，`width`、`height` 为整数，`fps` 由 `r_frame_rate` 算出。`probed_at` 为 UTC 时间，格式 `YYYY-MM-DDTHH:MM:SSZ`。`status` 为 `offline` 的 URL 不进入返回。

入参：`urls`，字符串数组，RTSP 地址。缺省或不是数组时按空数组处理。

出参 `data`：`{url: {status, width, height, fps, probed_at}}`。全部离线时为 `{}`。`message`：空字符串。

### POST `/rtsp/publish`

对 `paths` 逐条发布。`name` 取该路径的文件名。先用 3 秒超时请求 MediaMTX `GET /v3/info`，不可达时 `503`，`MediaMTX unreachable`。可达后探测该文件，再以循环方式推到 `rtsp://puskas_mediamtx:8554/{name}`。`publishStart` 的记录里 `status` 为 `offline`，`probed_at` 为空字符串。随后每 1 秒请求 MediaMTX `GET /v3/paths/get/{name}`，最多 5 秒。`ready` 为 `true` 时再 `probe` 这条 RTSP，由 `publishEnd` 写入 `status` 和 `probed_at`。超时则停掉推流并删除记录。同名已存在或文件探测失败时不发布，该条不进入返回。

入参：`paths`，字符串数组，容器内视频文件路径。缺省或不是数组时按空数组处理。

出参 `data`：本次发布成功的 `{name: {url_docker, url_host, width, height, fps, codec, bitrate, loop, created_at, duration, status, probed_at}}`。`loop` 恒为 `true`。`created_at` 为发布时刻的 UTC 时间。`duration` 为这条 RTSP 已运行的秒数。`url_host` 为 `rtsp://127.0.0.1:8554/{name}`。一条都没成功时 `400`，`Failed to publish rtsp`。

### GET `/rtsp/publish`

入参：查询参数 `name`，可省略。省略时返回全部。

出参 `data`：与 POST 相同形状的字典。`duration` 按当前时间减去 `created_at` 计算。没有匹配项时为 `{}`。

### DELETE `/rtsp/publish`

按查询参数 `name` 结束对应 `ffmpeg` 进程并删除记录。`name` 省略或为空时删除全部。

入参：`name`，可省略。

出参 `data`：删除后剩余的发布字典，形状与 `GET` 相同。

### POST `/video/probe`

用 `ffprobe` 探测入参路径的第一路视频流。成功时 `width`、`height`、`bitrate` 为整数，`fps`、`duration` 为小数，`codec` 为编码名。失败时这六项均为 `null`。

入参：`path`，字符串，容器内视频文件路径。缺省按空字符串探测。

出参 `data`：`width`、`height`、`fps`、`duration`、`codec`、`bitrate`。`message`：空字符串。
