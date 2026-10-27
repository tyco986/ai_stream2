# Media

前缀：`/ai_stream2/backend/media`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。`GET /media/{media_id}/download` 与 `POST /media/batch/download` 成功时不套这层外壳，分别直接返回原文件和 zip。

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

根目录 `/root/media`，其下固定三层：`upload/<用户名>/`、`pipeline/<流水线名>/`、`recording/<流名>/`。只收录图片和视频。图片后缀：`.jpg`、`.jpeg`、`.png`、`.gif`、`.webp`、`.bmp`。视频后缀：`.mp4`、`.mkv`、`.avi`、`.mov`、`.webm`、`.ts`、`.m4v`。比较时后缀转成小写。`type`：`image` | `video`。

媒体对象键：`id`、`type`、`name`、`width`、`height`、`duration`、`fps`、`codec`、`size`、`md5`、`folder`、`created_at`、`updated_at`、`user_id`、`stream_id`、`pipeline_id`。空字段为 `null`。`type=image` 时 `duration`、`fps`、`codec` 为 `null`。库内 `size` 为字节、`duration` 为秒；对外 `size` 是 MB 数字（1024×1024，保留一位小数），`duration` 是 `HH:MM:SS`（秒取整，时分秒都补到两位）。`folder` 是文件所在目录的绝对路径。`created_at`、`updated_at` 为 UTC，格式 `YYYY-MM-DDTHH:MM:SSZ`。

列表会先按磁盘同步库：扫描对应目录下的图片、视频，按路径和文件名对齐记录；同一路径有多行时留下 `updated_at` 更新的一条并删除其余行；大小或修改时间变了则重算 md5 并更新探测结果；磁盘上没有的记录删除。录像目录中修改时间距现在不足 2 秒的文件视为仍在写入，不入库。`pipeline`、`recording` 的记录 `user_id` 为用户名 `admin`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/ai_stream2/backend/media/upload/file` | query：`page`、`page_size`、`user_id`、`type`、`search` | `items`、`total`、`page`、`page_size` | 分页列出上传文件 |
| `GET` | `/ai_stream2/backend/media/pipeline/file` | query：`page`、`page_size`、`pipeline_id`、`type`、`search` | 同上 | 分页列出流水线文件 |
| `GET` | `/ai_stream2/backend/media/recording/file` | query：`page`、`page_size`、`stream_id`、`type`、`search`、`date` | 同上 | 分页列出录像文件 |
| `GET` | `/ai_stream2/backend/media/upload/folder` | 无 | `{用户名: user_id}` | 列出 `upload` 下能对上用户的子文件夹 |
| `GET` | `/ai_stream2/backend/media/pipeline/folder` | 无 | `{流水线名: pipeline_id}` | 列出 `pipeline` 下能对上流水线的子文件夹 |
| `GET` | `/ai_stream2/backend/media/recording/folder` | 无 | `{流名: stream_id}` | 列出 `recording` 下能对上流的子文件夹 |
| `GET` | `/ai_stream2/backend/media/{media_id}` | path：`media_id` | 媒体对象 | 详情 |
| `GET` | `/ai_stream2/backend/media/{media_id}/content` | path：`media_id` | 文件字节 | 预览用的原文件 |
| `GET` | `/ai_stream2/backend/media/{media_id}/download` | path：`media_id` | 文件字节 | 下载单个原文件 |
| `POST` | `/ai_stream2/backend/media/batch/create` | `items` | 批量结果 | 批量写入记录 |
| `POST` | `/ai_stream2/backend/media/batch/upload` | multipart：`user_id`、`file`（可多个） | 批量结果 | 上传到该用户的 upload 目录 |
| `POST` | `/ai_stream2/backend/media/batch/remove` | `ids` | 批量结果 | 删文件并删记录 |
| `POST` | `/ai_stream2/backend/media/batch/download` | `ids` | zip 字节 | 把这些文件打成 zip |

### GET `/ai_stream2/backend/media/upload/file`

### GET `/ai_stream2/backend/media/pipeline/file`

### GET `/ai_stream2/backend/media/recording/file`

`page` 默认 `1`，`page_size` 默认 `20`。归属 id 为空时不过滤：`upload` 用 `user_id`，`pipeline` 用 `pipeline_id`，`recording` 用 `stream_id`。`type` 为空时图片和视频都返回。`search` 对 `name`、`type`、`codec`、`folder`、`md5`、`width`、`height`、`duration`、`fps`、`size`、`created_at`、`updated_at` 做不区分大小写的包含匹配。按 `created_at` 降序。`date` 只用于录像列表，格式 `YYYY-MM-DD`，按 `created_at` 的 UTC 日历日相等过滤；为空时不按日期过滤。

入参（query）：见上表。

出参 `data`：`items`（媒体对象数组）、`total`、`page`、`page_size`。`message`：`Loaded media list`。失败 `500`，`Failed to load media list`。

### GET `/ai_stream2/backend/media/upload/folder`

### GET `/ai_stream2/backend/media/pipeline/folder`

### GET `/ai_stream2/backend/media/recording/folder`

只保留对应目录下的直接子文件夹，文件夹名须能在表里对上，对不上的目录不返回。目录不存在或没有可映射文件夹时 `data` 为 `{}`。

| 路径 | 文件夹名来源 | `data` | 传给文件列表的 query |
|------|----------------|--------|----------------------|
| `upload/folder` | `"user".username` | `{用户名: user_id}` | `GET .../upload/file` 的 `user_id` |
| `pipeline/folder` | `pipeline.name` | `{流水线名: pipeline_id}` | `GET .../pipeline/file` 的 `pipeline_id` |
| `recording/folder` | `stream.name` | `{流名: stream_id}` | `GET .../recording/file` 的 `stream_id` |

入参：无。

`message`：`Loaded media list`。失败 `500`，`Failed to load media list`。

### GET `/ai_stream2/backend/media/{media_id}`

入参：path `media_id`。

出参 `data`：媒体对象。`message`：`Loaded media`。不存在 `500`，`Failed to load media`。

### GET `/ai_stream2/backend/media/{media_id}/content`

按 `folder` 和 `name` 返回磁盘文件，供预览。成功响应为文件本身。不存在 `500`，`Failed to load media`。

### GET `/ai_stream2/backend/media/{media_id}/download`

按 `folder` 和 `name` 返回磁盘文件，`Content-Disposition` 附件名为文件名，不打包。成功响应为文件本身。文件不存在 `500`，外壳 `message` 为 `media_download_error`。

### POST `/ai_stream2/backend/media/batch/create`

入参 JSON：`items`，元素字段同媒体对象，不含 `id`。缺的字段按空值写入。`stream_id`、`pipeline_id` 为空则存 `null`。没有 `items` 时当作空数组。

成功进 `updated_rows`（媒体对象），失败进 `failed_rows`（原请求行）。`message`：`media_batch_create_ok`。

### POST `/ai_stream2/backend/media/batch/upload`

multipart。表单字段 `user_id`，文件字段可多个。按用户名建 `/root/media/upload/<用户名>/` 并写入，只保留文件名。落盘后用 MediaInfo 读宽高，视频再读时长（秒）、帧率、编码，并算 md5。按该目录、文件名、`user_id` 找行：没有则插入；已有则更新该行的探测结果、`size`、`md5`、`updated_at`（`created_at` 保持不变）。时间取文件时间，UTC。后缀不属于图片或视频时失败。

入参：`user_id`、文件。

成功进 `updated_rows`（媒体对象），单文件失败进 `failed_rows`（`{name}`）。`message`：`media_batch_upload_ok`。multipart 解析失败 `500`，`media_batch_upload_error`。

### POST `/ai_stream2/backend/media/batch/remove`

入参 JSON：`ids`，媒体 id 数组。没有 `ids` 时当作空数组。每个 id 先删磁盘文件（不存在则跳过删文件），再删记录。不存在或删除失败的 id 进 `failed_rows`，其余继续。

成功进 `updated_rows`（删除前的媒体对象），失败进 `failed_rows`（`{id}`）。`message`：`media_batch_remove_ok`。

### POST `/ai_stream2/backend/media/batch/download`

入参 JSON：`ids`。按 id 取磁盘文件打成 zip，包内用文件名；重名时用 `<media_id>_<文件名>`。下载文件名为 `media-YYYYMMDD-HHMMSS.zip`。

成功响应为文件本身。任一 id 没有对应文件，或打包失败，`500`，外壳 `message` 为 `media_batch_download_error`。
