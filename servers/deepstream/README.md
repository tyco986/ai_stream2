# DeepStream

监听 `0.0.0.0:8094`。JSON 成功外壳：`{success, message, data}`。

状态对象键：`status`（`stopped` | `running`）、`start_time`、`end_time`、`duration`。未开始时时间为 `null`，`duration` 为 `null`。时间为本地时间，格式 `YYYY-MM-DDTHH:MM:SS±HHMM`。`duration` 为秒。

`status=running` 时再启动返回 `409`，`stop the pipeline before start`。未运行时停止返回 `409`，`pipeline is not running`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/deepstream/health` | 无 | `{}` | 健康检查 |
| `POST` | `/start` | multipart：`config`、`model`、`media` | `{}` | 解压后启动管道 |
| `POST` | `/start/local` | `config` | `{}` | 用容器内配置 zip 启动 |
| `GET` | `/status` | 无 | 状态对象 | 读运行状态 |
| `POST` | `/stop` | 无 | `{}` | 结束管道进程 |
| `DELETE` | `/ai_stream2/deepstream/logs` | 无 | `{}` | 清空请求日志文件 |

### GET `/ai_stream2/deepstream/health`

入参：无。

出参 `data`：`{}`。`message`：空字符串。

### POST `/start`

清空 `/root/runtime` 后解压。`config` 解到 `/root/runtime/config`。每个 `model` 按文件名（去掉扩展名）解到 `/root/runtime/models/<stem>`。`media` 非空时按 zip 内相对路径写到容器根目录。然后 fork 子进程跑 `pipeline.yml`，父进程只保留 pid。请求体上限 2 GiB。

入参：multipart。`config` 必填。`model` 可重复。`media` 可省略。

出参 `data`：`{}`。`message`：空字符串。缺 zip 或解压失败时 `500`，消息为异常文本。

### POST `/start/local`

清空 `/root/runtime` 后解压 `config` 指向的 zip 到 `/root/runtime/config`。读其中 `params.yml`，收集全部 `model_zip` 路径并解到 `/root/runtime/models/<stem>`。不处理媒体 zip。

入参：`config`，容器内配置 zip 路径。

出参 `data`：`{}`。`message`：空字符串。缺字段或不是字符串时 `500`，`invalid config`。文件不存在时 `500`，`config zip missing`、`params.yml missing` 或 `model zip missing`。

### GET `/status`

无运行实例时 `status` 为 `stopped`，时间为 `null`，`duration` 为 `null`。运行中 `end_time` 为 `null`，`duration` 为当前时间减去 `start_time`。子进程已退出时写入 `end_time`，`duration` 为结束时间减去 `start_time`。

入参：无。

出参 `data`：状态对象。`message`：空字符串。

### POST `/stop`

向子进程发 `SIGKILL`。`SIGTERM` 会被 GStreamer 接住，直播源上与 `pipeline.stop()` 一样停不下来。

入参：无。

出参 `data`：`{}`。`message`：空字符串。

### DELETE `/ai_stream2/deepstream/logs`

先放开日志文件句柄，再删除 `/root/logs/deepstream` 下全部普通文件，目录保留。

入参：无。

出参 `data`：`{}`。`message`：空字符串。
