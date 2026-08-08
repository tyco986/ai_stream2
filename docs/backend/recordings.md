# Recordings（后端）

对应前端契约：[recordings_api.md](../frontend/recordings_api.md)。  
实现包：`servers/django/pages/recordings/`。前缀：`/ai_stream2/backend/recordings`。

---

## 代理的 API

| 本页触发 | 上游 | 用途 |
|----------|------|------|
| `GET /availability`、`/files`、`/playback`、`/download*` | MediaMTX 录像卷（容器内 `/recordings`） | 扫盘索引；单文件 Range / 附件 / zip |

无 HTTP 上游代理。流树 / `recording` 标志由前端调 stream_api。

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/availability` | 月内有录像的日期（按 stream） |
| `GET` | `/files` | 某流某日录像文件列表 |
| `GET` | `/playback` | 可播 URL（须支持 Range） |
| `GET` | `/download` | 单文件附件；支持 `ticket`（无会话） |
| `GET` | `/download/zip` | zip；支持 `ticket` |
| `POST` | `/download/ticket` | 签发短时直链（绕过前端代理） |
| `POST` | `/download/zip` | 兼容：会话下直接打 zip |

媒体：`GET /ai_stream2/media/recordings/<relative>`（Range；鉴权同会话）。

权限：仅 `recordings.view_recording`（列表 / 播放 / 下载）。

**无**布局预设、active_layout、shot、Site Config 切片。

字段 / 错误码以 [recordings_api.md](../frontend/recordings_api.md) 为准。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | **无**录像文件表；**无**布局表 |
| 索引 | 扫 `/recordings` 文件系统 |
| 媒体 | `RecordingMediaView`；`/ai_stream2/media/recordings/...` + Range |
| 打包 | `ZIP_STORED` 流式 zip（mp4 不再 DEFLATE）；单文件勾选直接附件，不打包 |
| 鉴权 | session + `has_perm` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| 录像索引 | **造**（`RecordingIndexService` 扫盘；path = 流 `name`） |
| 回放 URL / Range | **造**（`PlaybackService` + media view） |
| 下载 / zip | **造**（`DownloadService`） |
| MediaMTX 开录 | **复用** streams Enable + `recording`→`record`；本页只读文件 |
| stream 元数据 | 软引用 UUID；`shared` 只读 `resolve_stream_name(stream_id)` |
| Site Config | **不**注册 |

禁止 import `pages.preview` / `pages.streams`。

---

## 目录结构

```
servers/django/pages/recordings/
  apps.py
  urls.py                 # availability / files / playback / playback/media / download*
  views.py
  clients.py              # MediaMTXPlaybackClient
  services.py             # RecordingIndexService, PlaybackService, DownloadService
  permissions.py
  migrations/             # 历史布局表已 drop；无业务表
```

播放：默认窗长 `RECORDINGS_PLAYBACK_WINDOW_SECONDS`（120）；MediaMTX `playback: yes`（`:9996`）。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `RecordingIndexService` | `availability`；`files`（扫盘 + MediaMTX `/list` 校正 `end`） |
| `PlaybackService` | `start`/`duration`（可从 `file` 推导）→ MediaMTX 时间窗 ticket URL |
| `DownloadService` | 单文件 attachment；多文件 zip；media/window ticket |
| `MediaMTXPlaybackClient` | `/list` 时窗；代理 `GET /get` |


文件语义：`start ≤ T < end`（`end` 不含）。`start` 来自文件名；`end` 优先用 Playback `/list` 的 `duration`（截断段亦准），并钳到下一段 `start`；`/list` 不可用时回退邻段/mtime。`id` 由相对 path 稳定派生。

---

## 数据与存储

### 录像文件

**不建表**。文件布局对齐 MediaMTX `recordPath`（如 `{path}/{yyyy-mm-dd}_{time}.mp4`）；`path` = 流 `name`。

约定：索引以 **stream name = MediaMTX path 目录名**；API 入参为 `stream_id`；`RecordingIndexService` 经 `shared` 只读解析 name。

正在写入的末段（mtime 过近）索引时跳过。

### 布局表

已移除（`recordings_layoutpreset` / `recordings_activelayout`）。

---

## 各请求写库明细

本页 REST **不写库**。下载 / zip 只读磁盘。

| 请求 | 持久化 |
|------|--------|
| 全部本前缀 API | 无 |

---

## 配置

| 项 | 说明 |
|----|------|
| `RECORDINGS_ROOT` | 录像根目录（如 `/recordings`） |
| `MEDIAMTX_RECORD_SEGMENT_DURATION` | 默认段长（推断末段 `end`） |
| `RECORDINGS_DOWNLOAD_MAX` | zip 最多文件数（默认如 100） |
