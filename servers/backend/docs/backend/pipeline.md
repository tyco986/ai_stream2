# Pipelines（后端）

## API

### Pipelines


| 方法       | 路径                      | 功能                                |
| -------- | ----------------------- | --------------------------------- |
| `GET`    | `/`                     | 分页列表                              |
| `GET`    | `/types`                | 类型名列表                             |
| `POST`   | `/schema`               | 选定 type → 全量 `TypeSchema`         |
| `GET`    | `/map`                  | id → name                         |
| `GET`    | `/{pipeline_id}`        | 完整 Pipeline                       |
| `POST`   | `/`                     | 新建（`status=stopped`）              |
| `PUT`    | `/{pipeline_id}`        | 全量更新；`running`/`starting` → `409` |
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

固定段：`map`、`batch`、`types`、`schema` 先于 `/{id}`。

---



## 数据库



### `pipeline`


| 列            | 类型          | 约束     | 说明                                                                          |
| ------------ | ----------- | ------ | --------------------------------------------------------------------------- |
| `id`         | UUID        | PK     |                                                                             |
| `name`       | varchar     | UNIQUE |                                                                             |
| `type`       | varchar     | 非空     | 如 `DetVisRTSPPipeline`                                                      |
| `status`     | varchar     | 非空     | 默认 `stopped`                                                                |
| `config`     | JSON        | 非空     | drawer/logger/messager/debouncer/streams/gie_id/interval/tracker/analyzer … |
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

