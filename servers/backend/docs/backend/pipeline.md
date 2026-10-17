# Pipelines（后端）

## API

### Pipelines


| 方法       | 路径                      | 功能                                |
| -------- | ----------------------- | --------------------------------- |
| `GET`    | `/`                     | 分页列表                              |
| `GET`    | `/map`                  | id → name                         |
| `GET`    | `/{pipeline_id}`        | 完整 Pipeline                       |
| `POST`   | `/`                     | 新建（`status=stopped`）              |
| `PATCH`  | `/{pipeline_id}`        | 更新 `input`；`running`/`starting` → `409` |
| `DELETE` | `/{pipeline_id}`        | 删除；运行中 → `409`                    |
| `POST`   | `/batch/delete`         | 批量；失败入 `failed_ids`               |
| `POST`   | `/{pipeline_id}/start`  | 异步启动                              |
| `POST`   | `/{pipeline_id}/stop`   | 停止                                |
| `GET`    | `/{pipeline_id}/status` | 运行态轮询                             |
| `GET`    | `/{pipeline_id}/logs`   | 日志（`tail`/`offset`）               |




### GIE templates（`/gie-templates`）


| 方法                   | 路径              | 功能                       |
| -------------------- | --------------- | ------------------------ |
| `GET`/`POST`         | `/`             | 列表 / 新建                  |
| `GET`/`PUT`/`DELETE` | `/{gie_id}`     | 详情 / 更新 / 删（被引用 → `409`） |
| `POST`               | `/batch/delete` | 批量                       |




### Analyzer templates（`/analyzer-templates`）


| 方法                   | 路径                        | 功能                                   |
| -------------------- | ------------------------- | ------------------------------------ |
| `GET`/`POST`         | `/`                       | 列表 / 新建                              |
| `GET`/`PUT`/`DELETE` | `/{analyzer_template_id}` | 详情 / 全量 annotations / 删（被引用 → `409`） |
| `POST`               | `/batch/delete`           | 批量                                   |
| `POST`               | `/{id}/source/file`       | 上传参考图                                |
| `POST`               | `/{id}/source/stream`     | 从流截帧                                 |


字段 / 枚举 / TypeSchema 形以 [pipeline_api.md](../frontend/pipeline_api.md) 为准。权限：`pipelines.view_pipeline` / `add` / `change` / `delete`（三前缀共用本页权）。

固定段：`map`、`batch` 先于 `/{id}`。

### `POST /`

新建 Pipeline。`multipart/form-data`，字段 `input` 为 YAML（`input.yaml`）。后端用 yaml-cpp 转成 JSON 写入 `input`（`jsonb`），并调用 generator `POST /generate`。generator 返回的 zip 落到 `/root/configs/{name}.zip`，路径写入 `config_path`。`status` 为 `stopped`。`name` 取 JSON 的 `name`，没有则取 `pipeline_name`；`type` 取 `type`，没有则取 `generator`。

入参：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `input` | file | 是 | YAML；结构同 generator 模板 |

`data` 为新建记录：

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | UUID | 服务端生成 |
| `name` | string | |
| `type` | string | |
| `status` | string | `stopped` |
| `input` | object | YAML 转成的 JSON |
| `config_path` | string | `/root/configs/{name}.zip` |
| `updated_at` | datetime | |

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "yolo26n-det-rtsp",
  "type": "DetVisRTSPPipeline",
  "status": "stopped",
  "input": {
    "generator": "DetVisRTSPGenerator",
    "pipeline_name": "yolo26n-det-rtsp"
  },
  "config_path": "/root/configs/yolo26n-det-rtsp.zip",
  "updated_at": "2026-07-18T02:00:00Z"
}
```

### `PATCH /{pipeline_id}`

改已有 Pipeline。入参与 `POST /` 相同，只传新的 YAML。`id`、`status` 不变。`name`、`type`、`input`、`config_path` 按新 YAML 重新生成。`status` 为 `running` 或 `starting` 时返回 `409`。

---



## 数据库



### `pipeline`


| 列            | 类型          | 约束     | 说明                                                                          |
| ------------ | ----------- | ------ | --------------------------------------------------------------------------- |
| `id`         | UUID        | PK     |                                                                             |
| `name`       | varchar     | UNIQUE |                                                                             |
| `type`       | varchar     | 非空     | 如 `DetVisRTSPPipeline`                                                      |
| `status`     | varchar     | 非空     | 默认 `stopped`                                                                |
| `input` | jsonb | | YAML 转成的 JSON |
| `config_path` | text | | zip 落盘路径 `/root/configs/{name}.zip` |
| `updated_at` | timestamptz |        |                                                                             |


`streams` / `gie_id` / `analyzer.template` 均在 JSON 或旁路列；实现可用显式列便于索引：`gie_id` UUID 可空、索引。

### `pipeline_gie`


| 列             | 类型          | 约束     | 说明          |
| ------------- | ----------- | ------ | ----------- |
| `id`          | UUID        | PK     |             |
| `name`        | varchar     | UNIQUE |             |
| `model_id`    | UUID        | 非空     | 软引用；须 built |
| `model_name`  | varchar     | 可空     | 冗余展示        |
| `class_attrs` | JSON        | 非空     | ≥1 行        |
| `updated_at`  | timestamptz |        |             |


`class_attrs_summary`、`pipelines[]` **不**存表，响应时生成。

### `pipeline_analyzer`


| 列             | 类型          | 约束     | 说明      |
| ------------- | ----------- | ------ | ------- |
| `id`          | UUID        | PK     |         |
| `name`        | varchar     | UNIQUE |         |
| `source_path` | varchar     | 可空     |         |
| `annotations` | JSON        | 非空     | 默认 `[]` |
| `updated_at`  | timestamptz |        |         |


---

