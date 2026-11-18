# Media

前缀：`/backend/media`。全部接口经 `AuthFilter`。

JSON 成功外壳：`{success, message, data}`。`GET /media/{media_id}/content` 与 `GET /media/{media_id}/download` 成功时直接返回原文件，`POST /media/batch/download` 成功时直接返回 zip，三者都不套这层外壳。

批量结果键：`updated_rows`、`failed_rows`、`skipped_rows`、`updated_count`、`failed_count`、`skipped_count`。

根目录 `/root/media`，其下固定三层：`upload/<用户名>/`、`pipeline/<流水线名>/` 或 `pipeline/<流水线名>-<年月日>-<时分秒>/`、`recording/<流名>/`。只收录图片和视频。图片后缀：`.jpg`、`.jpeg`、`.png`。视频后缀：`.mp4`。比较时后缀转成小写。`type`：`image` | `video`。

媒体对象键：`id`、`type`、`name`、`width`、`height`、`duration`、`fps`、`codec`、`size`、`md5`、`folder`、`created_at`、`updated_at`、`user_id`。空字段为 `null`。`type=image` 时 `duration`、`fps`、`codec` 为 `null`。库内 `size` 为字节、`duration` 为秒；对外 `size` 是 MB 数字（1024×1024，保留一位小数），`duration` 是 `HH:MM:SS`（秒取整，时分秒都补到两位）。`folder` 是文件所在目录的绝对路径。`created_at`、`updated_at` 为 UTC，格式 `YYYY-MM-DDTHH:MM:SSZ`。

列表会先按磁盘同步库：扫描对应目录下的图片、视频，按路径和文件名对齐记录；同一路径有多行时留下 `updated_at` 更新的一条并删除其余行；大小或修改时间变了则重算 md5 并更新探测结果；磁盘上没有的记录删除。录像目录中修改时间距现在不足 2 秒的文件视为仍在写入，不入库。`pipeline`、`recording` 的记录 `user_id` 为用户名 `admin`。

---

## 1. API

| 方法 | 路径 | 入参键 | 出参键（`data`） | 功能 |
|------|------|--------|------------------|------|
| `GET` | `/backend/media/upload/file` | query：`page`、`page_size`、`user_id`、`type`、`search` | `items`、`total`、`page`、`page_size` | 分页列出上传文件 |
| `GET` | `/backend/media/pipeline/file` | query：`page`、`page_size`、`folder`、`type`、`search` | 同上 | 分页列出流水线文件 |
| `GET` | `/backend/media/recording/file` | query：`page`、`page_size`、`folder`、`type`、`search`、`date` | 同上 | 分页列出录像文件 |
| `GET` | `/backend/media/upload/folder` | 无 | `{用户名: user_id}` | 列出 `upload` 下能对上用户的子文件夹 |
| `GET` | `/backend/media/pipeline/folder` | 无 | `{目录名: 绝对路径}` | 同步后列出 `pipeline` 下已入库目录 |
| `GET` | `/backend/media/recording/folder` | 无 | `{目录名: 绝对路径}` | 同步后列出 `recording` 下已入库目录 |
| `GET` | `/backend/media/{media_id}` | path：`media_id` | 媒体对象 | 详情 |
| `GET` | `/backend/media/{media_id}/content` | path：`media_id` | 文件字节 | 预览用的原文件 |
| `GET` | `/backend/media/{media_id}/download` | path：`media_id` | 文件字节 | 下载单个原文件 |
| `POST` | `/backend/media/batch/create` | `items` | 批量结果 | 批量写入记录 |
| `POST` | `/backend/media/batch/upload` | multipart：`user_id`、`file`（可多个） | 批量结果 | 上传到该用户的 upload 目录 |
| `POST` | `/backend/media/batch/remove` | `ids` | 批量结果 | 删文件并删记录 |
| `POST` | `/backend/media/batch/remove/folder` | `folders` | 批量结果 | 删目录及其库记录 |
| `POST` | `/backend/media/batch/download` | `ids` | zip 字节 | 把这些文件打成 zip |
| `GET` | `/backend/media/publish` | 无 | `{name: 发布对象}` | 列出 ffmpeg 当前全部 RTSP 发布 |
| `POST` | `/backend/media/{media_id}/publish` | path：`media_id` | ffmpeg 发布对象 | 把该视频循环发布为 RTSP |
| `DELETE` | `/backend/media/{media_id}/publish` | path：`media_id` | 删除前的发布对象 | 停止该视频的 RTSP 推流 |

### GET `/backend/media/upload/file`

### GET `/backend/media/pipeline/file`

### GET `/backend/media/recording/file`

`page` 默认 `1`，`page_size` 默认 `20`。`upload` 的 `user_id` 为空时不过滤。`pipeline`、`recording` 的 `folder` 为空时列出该分类下全部文件，有值时按 `media.folder` 精确匹配。`type` 为空时图片和视频都返回。`search` 对 `name`、`type`、`codec`、`folder`、`md5`、`width`、`height`、`duration`、`fps`、`size`、`created_at`、`updated_at` 做不区分大小写的包含匹配。按 `created_at` 降序。`date` 只用于录像列表，格式 `YYYY-MM-DD`，按 `created_at` 的 UTC 日历日相等过滤；为空时不按日期过滤。

入参（query）：见上表。

出参 `data`：`items`（媒体对象数组）、`total`、`page`、`page_size`。`message`：`Loaded media list`。失败 `500`，`Failed to load media list`。

### GET `/backend/media/upload/folder`

### GET `/backend/media/pipeline/folder`

### GET `/backend/media/recording/folder`

`upload/folder` 只保留能对上用户名的直接子文件夹。`pipeline/folder`、`recording/folder` 先按磁盘同步，再返回该分类下 `media.folder` 去重后的 `{目录名: 绝对路径}`。没有目录时 `data` 为 `{}`。

| 路径 | `data` | 传给文件列表的 query |
|------|--------|----------------------|
| `upload/folder` | `{用户名: user_id}` | `GET .../upload/file` 的 `user_id` |
| `pipeline/folder` | `{目录名: 绝对路径}` | `GET .../pipeline/file` 的 `folder` |
| `recording/folder` | `{目录名: 绝对路径}` | `GET .../recording/file` 的 `folder` |

入参：无。

`message`：`Loaded media list`。失败 `500`，`Failed to load media list`。

### GET `/backend/media/{media_id}`

入参：path `media_id`。

出参 `data`：媒体对象。`message`：`Loaded media`。不存在 `500`，`Failed to load media`。

### GET `/backend/media/{media_id}/content`

按 `folder` 和 `name` 返回磁盘文件，供预览。成功响应为文件本身。不存在 `500`，`Failed to load media`。

### GET `/backend/media/{media_id}/download`

按 `folder` 和 `name` 返回磁盘文件，`Content-Disposition` 附件名为文件名，不打包。成功响应为文件本身。文件不存在 `500`，外壳 `message` 为 `media_download_error`。

### POST `/backend/media/batch/create`

入参 JSON：`items`，元素字段同媒体对象，不含 `id`。缺的字段按空值写入。没有 `items` 时当作空数组。

成功进 `updated_rows`（媒体对象），失败进 `failed_rows`（原请求行）。`message`：`media_batch_create_ok`。

### POST `/backend/media/batch/upload`

multipart。表单字段 `user_id`，文件字段可多个。按用户名建 `/root/media/upload/<用户名>/` 并写入，只保留文件名。落盘后用 MediaInfo 读宽高，视频再读时长（秒）、帧率、编码，并算 md5。按该目录、文件名、`user_id` 找行：没有则插入；已有则更新该行的探测结果、`size`、`md5`、`updated_at`（`created_at` 保持不变）。时间取文件时间，UTC。后缀不属于图片或视频时失败。

入参：`user_id`、文件。

成功进 `updated_rows`（媒体对象），单文件失败进 `failed_rows`（`{name}`）。`message`：`media_batch_upload_ok`。multipart 解析失败 `500`，`media_batch_upload_error`。

### POST `/backend/media/batch/remove`

入参 JSON：`ids`，媒体 id 数组。没有 `ids` 时当作空数组。每个 id 先删磁盘文件（不存在则跳过删文件），再删记录。不存在或删除失败的 id 进 `failed_rows`，其余继续。

成功进 `updated_rows`（删除前的媒体对象），失败进 `failed_rows`（`{id}`）。`message`：`media_batch_remove_ok`。

### POST `/backend/media/batch/remove/folder`

入参 JSON：`folders`，目录绝对路径数组。没有 `folders` 时当作空数组。每个路径必须落在 `/root/media` 之下且不是根目录本身。先删 `folder` 等于该路径、或以该路径加 `/` 为前缀的全部记录，再删除整个目录。

成功进 `updated_rows`（删掉的媒体对象），失败进 `failed_rows`（`{folder}`）。`message`：`media_batch_remove_folder_ok`。

### POST `/backend/media/batch/download`

入参 JSON：`ids`。按 id 取磁盘文件打成 zip，包内用文件名；重名时用 `<media_id>_<文件名>`。下载文件名为 `media-YYYYMMDD-HHMMSS.zip`。

成功响应为文件本身。任一 id 没有对应文件，或打包失败，`500`，外壳 `message` 为 `media_batch_download_error`。

### GET `/backend/media/publish`

转发 ffmpeg `GET /rtsp/publish`，不带 `names`。没有发布时 `data` 为 `{}`。

入参：无。

出参 `data`：`{name: 发布对象}`，对象键与 POST 相同。`message`：`media_publish_list_ok`。ffmpeg 不可达时 `500`，`media_publish_list_error`。

### POST `/backend/media/{media_id}/publish`

只接受 `type=video`。用 `folder` 和 `name` 拼容器内路径，发布名取路径里 `/media/` 之后的部分，调用 ffmpeg `POST /rtsp/publish`。发布成功后按文件名在 All 组新建流：`url` 为 `url_docker`，`mount` 为 `true`，`record` 为 `false`，宽高、fps、`status`、`probed_at` 取发布结果。MediaMTX 挂载或建流失败时 `500`，推流可能已经开始。

入参：path `media_id`。

出参 `data`：该条发布对象，键为 `path`、`url_docker`、`url_host`、`width`、`height`、`fps`、`codec`、`bitrate`、`loop`、`created_at`、`duration`、`status`、`probed_at`。`message`：`media_publish_ok`。记录不存在、不是视频，或 ffmpeg 未发布成功时 `500`，`media_publish_error`。

### DELETE `/backend/media/{media_id}/publish`

发布名与 POST 相同。先 `GET /rtsp/publish?names=` 取该条，再 `DELETE /rtsp/publish?names=` 停掉推流。

入参：path `media_id`。

出参 `data`：删除前的发布对象，键与 POST 相同。`message`：`media_unpublish_ok`。记录不存在、不是视频，或 ffmpeg 里没有这个名字时 `500`，`media_unpublish_error`。
