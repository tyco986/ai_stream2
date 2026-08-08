# Recordings API

生产端 Recordings 页后端 API。UI 规格见 [recordings.md](./recordings.md)。

分组树与流字段复用 [stream_api.md](./stream_api.md)（`GET /groups/tree` 等）。本前缀提供**日历可用性**、**按日文件列表**、**回放地址**、**单文件下载**与 **zip 打包下载**。

**无**布局预设 / active_layout / shot API。

## 基础信息

| 项 | 值 |
|----|-----|
| 默认 Host | `127.0.0.1` |
| 默认 Port | `8000`（实现时可配置） |
| Base URL | `http://127.0.0.1:8000/ai_stream2/backend/recordings` |
| 路径前缀 | `/ai_stream2/backend/recordings` |

固定段：`availability`、`files`、`playback`、`download`、`download/zip`。

媒体前缀：`/ai_stream2/media/recordings/`（不在本 `backend/recordings` 下）。

---

## 约定

### 响应外壳

JSON 接口：

```json
{
  "success": true,
  "message": "",
  "data": {}
}
```

失败时 `success: false`，HTTP 4xx/5xx，`message` 为说明。

`GET /download`、`POST /download/zip` 成功时直接返回文件流（非 JSON 外壳）。

### 字段语义

| 字段 | 说明 |
|------|------|
| `date` | `YYYY-MM-DD`（本地日历日） |
| `month` | `YYYY-MM` |
| `file` | 相对 `RECORDINGS_ROOT` 的路径，如 `cam-01/2026-07-09_18-00-00-000000.mp4`；前端 opaque |
| `filename` | basename |
| `size_bytes` | 文件字节数 |
| `start` / `end` | 覆盖墙钟；`YYYY-MM-DDTHH:MM:SS.ffffff`；`start ≤ T < end`；`end` 来自 MediaMTX `/list` duration（优先） |
| `id` | 文件稳定 id（可由 `file` 派生）；可代替 `file` 调 playback |

### 鉴权

| 权限 | 能力 |
|------|------|
| `recordings.view_recording` | availability、files、playback、media Range、download、download/zip |

| HTTP | 场景 |
|------|------|
| 401 | 未登录 |
| 403 | 缺 Permission |

---

## 数据模型

### RecordingFile

```json
{
  "id": "a1b2c3d4e5f6789012345678abcdef01",
  "file": "cam-01/2026-07-09_18-00-00-000000.mp4",
  "filename": "2026-07-09_18-00-00-000000.mp4",
  "size_bytes": 134217728,
  "start": "2026-07-09T18:00:00",
  "end": "2026-07-09T19:00:00",
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "stream_name": "cam-01"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | 稳定 id |
| `file` | string | 相对路径 |
| `filename` | string | 文件名 |
| `size_bytes` | number | 字节 |
| `start` / `end` | string | 覆盖区间（`end` 不含） |
| `stream_id` | UUID | |
| `stream_name` | string | |

按 `start` 升序。

### PlaybackResult

```json
{
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "path": "cam-01",
  "file": "cam-01/2026-07-09_18-00-00-000000.mp4",
  "start": "2026-07-09T18:00:00.000000Z",
  "duration": 120.0,
  "url": "http://127.0.0.1:8001/ai_stream2/backend/recordings/playback/media?ticket=...",
  "content_type": "video/mp4"
}
```

`url` 为 `PUBLIC_API_ORIGIN` 上的时间窗流（MediaMTX fMP4）；非整文件 Range。

---

## 接口

### `GET /availability`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/availability`

**Query**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_ids` | string | 是 | 逗号分隔 UUID；UI 单选时传一个 |
| `month` | string | 是 | `YYYY-MM` |

**响应 `data`**

```json
{
  "month": "2026-07",
  "streams": [
    {
      "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
      "dates": ["2026-07-09", "2026-07-10"]
    }
  ]
}
```

无录像 → 对应 `dates: []`（200）。非法 month → 400。

---

### `GET /files`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/files`

**Query**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_id` | UUID | 是 | |
| `date` | string | 是 | `YYYY-MM-DD` |

**响应 `data`**

```json
{
  "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "stream_name": "cam-01",
  "date": "2026-07-09",
  "files": [ /* RecordingFile */ ]
}
```

无文件 → `files: []`（200）。流不存在 / 无法解析 name → 404。

---

### `GET /playback`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/playback`

**说明**：签发可播 URL（默认约 120s 时间窗），由 MediaMTX Playback 按 `path`+`start`+`duration` 拼 fMP4；`url` 指向 `PUBLIC_API_ORIGIN` 上的 `GET /playback/media?ticket=`，绕过 Vite。

**Query**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_id` | UUID | 是 | |
| `start` | string | 与 `file`/`id` 可组合 | RFC3339 / 本地 ISO；缺省时从 `file` 文件名解析 |
| `end` | string | 否 | 用于钳制 `duration` 不超过本文件剩余长度 |
| `duration` | number | 否 | 秒；默认 `RECORDINGS_PLAYBACK_WINDOW_SECONDS`（120） |
| `file` | string | 否 | `/files` 相对路径；可代替 `start` |
| `id` | string | 否 | `/files` 的 `id`（亦接受 `segment_id`） |

**响应 `data`**：`PlaybackResult`（含 `url`、`start`、`duration`、`path`、`file`、`content_type`）

缺 `start` 且无法从 file 推导 → 400；流不存在 → 404。

---

### `GET /playback/media`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/playback/media?ticket=`

**说明**：校验 window ticket 后流式代理 MediaMTX `GET /get?path=&start=&duration=&format=fmp4`。供 `<video src>`；ticket 鉴权（可无会话）。

---

### `GET /download`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/download`

**Query**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `stream_id` | UUID | 是 | |
| `file` | string | 是 | 相对路径 |

**成功**：`200`，浏览器原生下载（`Content-Disposition: attachment`）。前端应直接导航/锚点触发，**不要**先把整文件读入内存 Blob。

文件不存在 / 路径非法 → 404；缺参 → 400。

---

### `POST /download/zip`

**完整地址**：`http://127.0.0.1:8000/ai_stream2/backend/recordings/download/zip`

**Body**（JSON 或 form 字段 `items` = 同结构 JSON 字符串，便于浏览器 form 提交）：

```json
{
  "items": [
    {
      "stream_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
      "file": "cam-01/2026-07-09_18-00-00-000000.mp4"
    }
  ]
}
```

| 字段 | 说明 |
|------|------|
| `items` | 非空数组；每项 `stream_id` + `file` |
| 上限 | 服务端 `RECORDINGS_DOWNLOAD_MAX`（如 100）；超出 → 400 |
| 单文件 | **不**打 zip，直接返回该 mp4 附件 |
| 多文件 | `application/zip`，**STORED** 流式打包（不对 mp4 再压缩） |

**成功**：`200` + `Content-Disposition: attachment`。前端宜用 form/iframe 交给浏览器下载，避免整包 `blob()` 卡住页面。

---

### 媒体 `GET /ai_stream2/media/recordings/<path:rel>`

鉴权会话或 media ticket；支持 Range；路径须落在 `RECORDINGS_ROOT` 内。整文件直链（下载/调试）；**播放**改走 `/playback/media` 时间窗代理。

---

## 前端调用顺序（摘要）

| 时机 | 请求 |
|------|------|
| 选中流 / 翻月 | `GET /availability?stream_ids={id}&month=` |
| 选日 | `GET /files?stream_id=&date=` |
| 播放 | `GET /playback?stream_id=&file=&start=&end=` → `<video src=url>` |
| 行下载 | `POST /download/ticket` → 浏览器打开 ticket URL |
| 顶栏 Download | `POST /download/ticket` body 勾选项 |

树数据来自 stream_api，不经本前缀。

---

## 错误码摘要

| HTTP | message 示例 | 场景 |
|------|--------------|------|
| 400 | `Invalid month` / `Invalid date` | 日期格式 |
| 400 | `start is required` | playback 缺 start / 无法从 file 推导 |
| 400 | `No files to download` | zip 空 items |
| 400 | `Too many files to download` | 超上限 |
| 404 | `Stream not found` | 无法解析 stream |
| 404 | `Recording not found` | 文件不存在 |
