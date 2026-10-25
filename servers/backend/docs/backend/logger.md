# Logger

前缀：`/ai_stream2/backend/logs`。全部接口经 `AuthFilter`。读日志和查开关要 `event` 的读权限，删除要 `event` 的删权限，开关要 `event` 的改权限。

JSON 成功外壳：`{success, message, data}`。

默认关闭。`POST /enable` 后，各模块请求写入 `/root/logs/backend/{page}/{page}_{YYYY-MM-DD}.jsonl`，每天 `00:00` 换新文件，保留 90 天。每行一条紧凑 JSON。

行字段：`ts`、`method`、`url`、`code`、`input`、`output`、`latency`、`ip`、`in_bytes`、`out_bytes`、`error`、`header`。`input`、`output` 正文是 JSON 时嵌成值，否则为字符串。`header` 为请求头对象，只含 `authorization`、`accept-language`。`ts` 为本地时间，毫秒三位。`latency` 为毫秒，保留两位小数。中文按 UTF-8 写出。

`page`：`stream`、`stream_group`、`model`、`pipeline`、`pipeline_gie`、`pipeline_analyzer`、`server`、`preview`、`user_group`、`user`、`event`、`notice`、`media`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/logs` | query：`page`、`date`，均可空 | `{page: {date: {content}}}` | 读取调试日志 |
| `DELETE` | `/ai_stream2/backend/logs` | query：`page`，可空 | `{page: 删除文件数}` | 删除调试日志 |
| `POST` | `/ai_stream2/backend/logs/enable` | 无 | `{enabled: true}` | 打开调试日志 |
| `POST` | `/ai_stream2/backend/logs/disable` | 无 | `{enabled: false}` | 关闭调试日志 |
| `GET` | `/ai_stream2/backend/logs/status` | 无 | `{enabled}` | 查询写入开关 |

### GET `/ai_stream2/backend/logs`

`page`、`date` 都可省略。`date` 若填写须为 `YYYY-MM-DD`。`page` 若填写须为上面的模块名。

入参（query）：`page`、`date`。

出参 `data`：`page` 到日期对象。每个日期的值是 `{content}`，`content` 为该日日志行解析后的对象数组，空行和无法解析的行不包含在内。数组里的 `latency` 是两位小数字符串。没有文件的 `page` 是 `{}`。都不填返回全部模块；只填 `page` 返回该模块全部日期；只填 `date` 返回全部模块的这一天；都填只返回该模块这一天。`message`：`logger_get_ok`。`page` 或 `date` 非法时 `400`，`message` 为 `logger_get_error`。

### DELETE `/ai_stream2/backend/logs`

`page` 可省略。填写时须为上面的模块名。先释放已打开的 logger，再删除对应目录。

入参（query）：`page`。

出参 `data`：`page` 到删除的文件数。不填 `page` 时覆盖全部模块，目录不存在时数量为 `0`。`message`：`logger_remove_ok`。`page` 非法时 `400`，`message` 为 `logger_remove_error`。

### POST `/ai_stream2/backend/logs/enable`

进程内打开写入开关，无请求体。

入参：无。

出参 `data`：`{enabled: true}`。`message`：`logger_enable_ok`。

### POST `/ai_stream2/backend/logs/disable`

进程内关闭写入开关，无请求体。已打开的 logger 保留，下次开启后继续写。

入参：无。

出参 `data`：`{enabled: false}`。`message`：`logger_disable_ok`。

### GET `/ai_stream2/backend/logs/status`

返回进程内写入开关，无请求体。

入参：无。

出参 `data`：`{enabled}`，布尔值。`message`：`logger_status_ok`。
