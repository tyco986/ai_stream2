# Backend 服务（实现说明）

本目录实现 **Django + DRF** 侧的业务编排与 API 网关：**用户与多租户**、**摄像头与分析区域配置**、**通过 HTTP 代理调用 DeepStream 内置 REST**、**消费 Kafka 写入 PostgreSQL**、**报警规则与通知**、**仪表盘聚合**、以及 **Django Channels WebSocket** 向浏览器推送实时事件。

> 本文档描述**当前仓库代码行为**。项目级架构契约见仓库根目录 `.cursor/rules/architecture.mdc` 与相关设计文档；若与规划文档不一致，以代码为准。

---

## 1. 职责边界

| 本服务负责 | 本服务不负责 |
|-----------|-------------|
| REST API、JWT 认证、组织级数据隔离 | GStreamer / DeepStream 管道、GPU 推理、视频解码 |
| `httpx` 长连接池代理 DeepStream `:9000` 内置 REST（增删流、健康、流信息） | 自建 RTSP 拉流或替代 DeepStream 内置 REST 的流状态机 |
| `confluent-kafka` 消费检测与事件 Topic，批量写入 `Detection`，并驱动报警与推送 | 在容器内运行 nvmsgbroker 或实现检测算法 |
| 报警规则评估（含基于 `nvdsanalytics` 结构的 JSON 条件）、邮件/Webhook（Celery） | 在 Backend 内做 ROI/越线的几何计算（几何在 DeepStream 侧完成） |
| 从 `PipelineProfile` + `AnalyticsZone` 生成配置文件到共享目录，供 DeepStream 挂载 | 直接 `import pyservicemaker` 或操作 GStreamer 元素 |

---

## 2. 技术栈

| 类别 | 选型 |
|------|------|
| 语言 / 运行时 | Python 3.12 |
| Web 框架 | Django 5.x、Django REST Framework |
| 异步 ASGI | uvicorn / gunicorn + `uvicorn.workers.UvicornWorker`（`config.asgi:application`） |
| 认证 | `djangorestframework-simplejwt`（Access + Refresh，黑名单轮换） |
| 数据库 | PostgreSQL（`psycopg2-binary`），`Detection` 设计为高频写入（BigAutoField + 索引；运维上可配合分区，见 §10） |
| 缓存 / Channel layer / Celery broker | 单一 `REDIS_URL`，不同逻辑库号后缀（`/0` 缓存、`/1` Celery broker、`/2` result、`/3` Channels） |
| 消息 | `confluent-kafka`（librdkafka），独立进程 `run_kafka_consumer` |
| HTTP 出站 | `httpx.AsyncClient` 单例连接池 → DeepStream |
| API 文档 | `drf-spectacular`（`/api/schema/`、`/api/docs/`） |
| 日志 | `structlog`（生产 JSON；开发 `development` 设置下为 `ConsoleRenderer`） |
| 配置 | `django-environ`（`.env` / 环境变量） |

---

## 3. 架构总览

```
                         ┌──────────────────────────────────────────┐
                         │  Frontend (Vue)                           │
                         └────────────────────┬─────────────────────┘
                                              │ HTTPS
                                              │ REST + WebSocket (JWT)
                                              ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  Backend :8000 (Django + Channels)                                           │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ DRF ViewSet │→│ Service 层   │→│ Models / DB  │  │ Celery worker/beat   │ │
│  └──────┬──────┘  └──────┬───────┘  └──────────────┘  └──────────────────────┘ │
│         │                │                                                     │
│         │         DeepStreamClient (httpx 异步)                                 │
│         │                └──────────────────────────────┐                       │
│         │                                               ▼                       │
│         │                                    DeepStream :9000 REST              │
│         │                                                                     │
│  ┌──────▼──────────────────────────────────────────────────────────────────┐  │
│  │ DetectionConsumer (management command)                                   │  │
│  │  poll → parse → bulk_create(Detection) → AlertEngine → Alert → notify    │  │
│  │  → channel_layer.group_send → WebSocket 消费者                            │  │
│  └─────────────────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────┬───────────────────────────────┘
                                                │
                    Kafka (bootstrap)           │
                    · deepstream-detections     │
                    · deepstream-events         │
```

**数据流摘要**：

1. **控制面**：用户调用 `/api/v1/cameras/{id}/start-stream/` → `DeepStreamClient.add_stream` → DeepStream `POST /api/v1/stream/add`（body 结构与 DeepStream 动态源示例一致，`key`/`value` 嵌套）。
2. **数据面**：DeepStream → Kafka `KAFKA_DETECTION_TOPIC` → `DetectionConsumer` 批量入库 → `AlertEngine` → 可选 Celery 通知 + WebSocket 广播。
3. **事件面**：Kafka `KAFKA_EVENT_TOPIC` 中的 JSON（若包含 `event` + `sensorId`/`camera_id`）用于更新 `Camera.status` 并推送 `camera.status` 消息（事件名映射见 §8.2）。

---

## 4. 仓库目录结构（源码）

| 路径 | 作用 |
|------|------|
| `config/` | `settings`（`base` / `development` / `production`）、`urls.py`、`asgi.py`、`wsgi.py`、`celery.py` |
| `common/` | `BaseModel`、统一响应 `success_response`、分页、权限 mixin、DRF 全局异常处理、请求 ID / 访问日志中间件、登录限流 |
| `services/` | `DeepStreamClient`、`DetectionConsumer`、`AlertEngine`、`PipelineDeployer`、`NotificationService` |
| `tasks/` | Celery 任务：`maintenance`（分区清理、摄像头状态同步、死信清理）、`notifications`（报警通知） |
| `websocket/` | JWT query 认证中间件、`DetectionConsumer` / `CameraStatusConsumer` / `AlertConsumer` 路由 |
| `apps/accounts/` | `Organization`、`User`（角色 admin/operator/viewer）、JWT 登录与 `/me/` |
| `apps/cameras/` | `Camera`、`CameraGroup`、`AnalyticsZone`；摄像头 CRUD、启停流、`pipeline` 绑定 |
| `apps/detections/` | `Detection`（高频表）、`KafkaDeadLetter`；只读列表 + `stats`；`run_kafka_consumer` 命令 |
| `apps/alerts/` | `AlertRule`、`Alert`；确认/解决流转 |
| `apps/pipelines/` | `AIModel`、`PipelineProfile`、`CameraModelBinding`；`deploy` 写共享目录配置 |
| `apps/dashboard/` | 概览、检测趋势、摄像头状态分布；`health` 存活/就绪 |
| `tests/` | API / 服务 / 集成测试，`factories.py` + `pytest` |
| `manage.py` | 默认 `DJANGO_SETTINGS_MODULE=config.settings.development` |
| `Dockerfile` | 多阶段：`test`（pytest）、`dev`（uvicorn --reload）、`production`（gunicorn+uvicorn worker） |
| `docker-compose.dev.yml` | Postgres、Redis、Kafka、backend、celery worker/beat、kafka-consumer、可选 test profile |

---

## 5. Django 应用与数据模型

### 5.1 `accounts`

- **`Organization`**：多租户根实体。
- **`User`**（`AUTH_USER_MODEL`）：UUID 主键；`organization` 可空；`role`：`admin` | `operator` | `viewer`。
- **权限惯例**（见 `common/permissions.py`）：
  - **Viewer**：只读（列表/详情类接口）。
  - **Operator 及以上**：创建/更新/删除及启停流、报警确认等写操作。
  - **`OrganizationFilterMixin`**：`get_queryset()` 固定过滤 `organization=request.user.organization`，避免跨租户泄漏。

### 5.2 `cameras`

- **`Camera`**：`uid` 全局唯一，与 DeepStream `camera_id` / Kafka `sensorId` 对齐；`rtsp_url`；`status`：`offline` | `connecting` | `online` | `error`；软删除 `is_deleted`。
- **`CameraGroup`**：组织内分组。
- **`AnalyticsZone`**：`zone_type`（`roi` / `line_crossing` / `overcrowding` / `direction`）；`coordinates` 为 `[[x,y],...]`，校验范围 **0–1920 × 0–1080**；`config` 存类型相关参数（如拥挤阈值的 `object_threshold`）。

### 5.3 `detections`

- **`Detection`**：`BigAutoField` 主键；`detected_at` 索引；`detected_objects` 映射到列名 `objects`；`analytics` 可选，存帧级 nvdsanalytics 摘要。
- **`KafkaDeadLetter`**：解析失败的消息审计（topic/partition/offset/raw/error）。

### 5.4 `alerts`

- **`AlertRule`**：`rule_type` 与 `conditions` JSON 配合（见 §9）；`cameras` 多对多，**空表示匹配组织内全部摄像头**；`cooldown_seconds`；`notify_channels` 字符串列表，如 `["websocket","email","webhook"]`。
- **`Alert`**：状态机 `pending` → `acknowledged` / `resolved`（非法流转抛 `InvalidStateTransitionError`）。

### 5.5 `pipelines`

- **`AIModel`**：`detector` | `tracker`；`model_file` / `label_file` 为字符串路径（通常指向 DeepStream 容器内或共享卷路径）；`config` 内存额外 PGIE/跟踪参数。
- **`PipelineProfile`**：关联 detector 与可选 tracker；`analytics_enabled`；`analytics_config_stale`（部署后清零）。
- **`CameraModelBinding`**：摄像头与 `PipelineProfile` 一对一。

---

## 6. HTTP API 一览

统一成功响应形如（`common/response.py`）：

```json
{"code": "OK", "message": "success", "data": { ... }}
```

业务异常（`ServiceError`）：

```json
{"code": "DEEPSTREAM_UNAVAILABLE", "message": "DeepStream 服务不可用", "data": null}
```

全局前缀 **`/api/v1/`**（健康检查在 **`/api/v1/health/`** 下，见下表）。

| 方法与路径 | 说明 |
|-----------|------|
| **Auth** | |
| `POST /api/v1/auth/login/` | SimpleJWT 获取 access/refresh |
| `POST /api/v1/auth/refresh/` | 刷新令牌 |
| `GET /api/v1/auth/me/` | 当前用户 |
| **Cameras** | |
| `GET/POST /api/v1/cameras/` | 列表、创建（operator+） |
| `GET/PATCH/DELETE /api/v1/cameras/{id}/` | 详情、更新、软删除 |
| `POST /api/v1/cameras/{id}/start-stream/` | 代理 DeepStream 加流，`status`→`connecting` |
| `POST /api/v1/cameras/{id}/stop-stream/` | 代理移除流，`status`→`offline` |
| `GET /api/v1/cameras/{id}/pipeline/` | 当前模型绑定；无绑定时 `data: null` |
| `PUT /api/v1/cameras/{id}/pipeline/` | body：`pipeline_profile_id` |
| `GET/POST /api/v1/camera-groups/` | 分组 CRUD |
| `GET/POST /api/v1/cameras/{camera_pk}/analytics-zones/` | 分析区嵌套资源 |
| `GET/PATCH/DELETE .../analytics-zones/{pk}/` | 单条分析区 |
| **Detections** | |
| `GET /api/v1/detections/` | 游标分页；查询参数：`camera_id`、`start_time`、`end_time`、`object_type` |
| `GET /api/v1/detections/stats/` | 按小时聚合（最多返回 168 桶） |
| **Alerts** | |
| `GET/POST/... /api/v1/alert-rules/` | 规则 CRUD |
| `GET /api/v1/alerts/`、`GET .../{id}/` | 报警列表/详情 |
| `POST /api/v1/alerts/{id}/acknowledge/` | 确认（operator+） |
| `POST /api/v1/alerts/{id}/resolve/` | 解决（operator+） |
| **Pipelines** | |
| `GET/POST /api/v1/ai-models/`、`GET/PATCH/DELETE .../{id}/` | 模型元数据 CRUD |
| `GET/POST /api/v1/pipeline-profiles/`、`GET/PATCH/DELETE .../{id}/` | 管道配置 CRUD |
| `POST /api/v1/pipeline-profiles/{id}/deploy/` | 生成配置文件到 `DS_CONFIG_DEPLOY_DIR` |
| **Dashboard** | |
| `GET /api/v1/dashboard/overview/` | 在线摄像头数、今日检测数、待处理报警等 |
| `GET /api/v1/dashboard/detection-trend/?hours=24` | 按小时趋势 |
| `GET /api/v1/dashboard/camera-status/` | 按 `status` 聚合计数 |
| **DeepStream 代理** | |
| `GET /api/v1/deepstream/health/` | 转发 `GET .../health/get-dsready-state` |
| `GET /api/v1/deepstream/streams/` | 转发 `GET .../stream/get-stream-info` |
| **Health** | |
| `GET /api/v1/health/live/` | 存活（`AllowAny`，无节流） |
| `GET /api/v1/health/ready/` | 就绪：DB + Redis；若 `HEALTH_CHECK_KAFKA=true` 则再探测 Kafka Producer 连通性 |
| **文档** | |
| `GET /api/schema/`、`GET /api/docs/` | OpenAPI Schema、Swagger UI |

**分页**：默认页码分页 `page` / `page_size`（最大 100）；`Detection` 使用 **游标分页**（`CursorPagination`，ordering `-detected_at`，默认 `page_size=50`）。

**限流**：匿名与用户默认 throttle；登录接口使用 `LoginRateThrottle`（scope `login`，`5/minute`）。

---

## 7. DeepStream 代理契约（`services/deepstream_client.py`）

- **基址**：`DEEPSTREAM_REST_URL`（默认 `http://localhost:9000`）。
- **单例**：模块级 `deepstream_client`，内部 `httpx.AsyncClient` 连接池；视图通过 `async_to_sync` 调用。
- **链路追踪**：若 HTTP 请求经 `RequestIDMiddleware` 设置了上下文，出站会带 `X-Request-ID`。
- **Mock**：`DEEPSTREAM_MOCK=true` 时不发起真实 HTTP，用于纯后端开发（`docker-compose.dev.yml` 默认开启）。

### 7.1 与 DeepStream REST 的 JSON 映射

| 方法 | 后端封装 | DeepStream 路径 |
|------|----------|-----------------|
| 加流 | `add_stream(camera_id, camera_name, rtsp_url)` | `POST /api/v1/stream/add`，body：`{"key":"sensor","value":{"camera_id","camera_name","camera_url","change":"camera_add"}}` |
| 移除 | `remove_stream(camera_id, rtsp_url)` | `POST /api/v1/stream/remove`，`change":"camera_remove"` |
| 流信息 | `get_streams()` / `get_stream_info()` | `GET /api/v1/stream/get-stream-info` |
| 健康 | `health_check()` | `GET /api/v1/health/get-dsready-state` |

任意网络错误在未 Mock 时会抛出，视图层转为 `DeepStreamUnavailableError`（HTTP 503）。

---

## 8. Kafka 消费（`services/kafka_consumer.py`）

### 8.1 进程与配置

- **入口**：`python manage.py run_kafka_consumer` → `DetectionConsumer.run()`。
- **订阅**：`KAFKA_DETECTION_TOPIC` + `KAFKA_EVENT_TOPIC`（同一消费者循环内分支处理）。
- **消费组**：`KAFKA_CONSUMER_GROUP`；`auto.offset.reset=latest`；**手动提交**（`enable.auto.commit=false`），批量刷盘成功后 `commit`。
- **批量**：缓冲至 `KAFKA_BATCH_SIZE` 条或超过 `KAFKA_FLUSH_INTERVAL` 秒触发 `_flush_detections`。
- **容错**：连续刷盘失败达到 `MAX_CONSECUTIVE_FAILURES`（5）会 **主动崩溃** 以便编排器重启；解析失败写入 `KafkaDeadLetter` 并跳过该条。
- **相机映射**：内存缓存 `Camera.uid` → `Camera`（TTL 300s），避免每条 Kafka 消息打 DB。

### 8.2 检测消息解析（`deepstream-detections`）

期望 JSON 至少包含：

- `sensorId`：对应 `Camera.uid`
- `@timestamp`：ISO8601，解析为 `detected_at`
- 可选 `frame_number`、`objects`（数组 → `detected_objects` 与 `object_count`）、`analytics`

若库中无对应 `Camera`（含已软删），该条 **静默跳过**（不入库）。

### 8.3 事件消息（`deepstream-events`）

`_handle_event` 解析 JSON 后调用 `_update_camera_status`：

- 读取 `event`、`sensorId` 或 `camera_id`
- **映射**（存在则更新 `Camera.status` 并 WebSocket 推送 `camera_status_{org_id}` 组）：

| event（代码中） | Camera.status |
|----------------|---------------|
| `camera_online`, `stream_started` | `online` |
| `camera_offline`, `stream_removed` | `offline` |
| `camera_error`, `stream_error` | `error` |

> 若 DeepStream 侧实际发出的 `event` 字符串与上表不一致，状态不会更新。对接时需与 DeepStream 事件生产者对齐字段名。

### 8.4 刷盘后的副作用顺序

1. `Detection.objects.bulk_create`
2. 每 50 次 flush 调用 `AlertEngine.prune_cooldown_cache()`
3. `AlertEngine.evaluate_detection` → `Alert.objects.bulk_create`
4. WebSocket：`detections_{organization_id}`、`alerts_{organization_id}`
5. Celery：`send_alert_notification.delay(alert_id)`（当 `notify_channels` 非空）

WebSocket 推送失败 **仅打日志**，不阻塞提交 offset。

---

## 9. 报警规则引擎（`services/alert_engine.py`）

规则从 DB 加载，**30s TTL** 缓存；冷却键 `(rule_id, camera_id)` 存内存，`86400s` 窗口清理。

| rule_type | conditions 要点 | 数据来源 |
|-----------|-----------------|----------|
| `object_count` | `min_count` | `detection.object_count` |
| `object_type` | `object_type`, `min_count` | `detected_objects[].type` |
| `zone_intrusion` | `zone_name`, 可选 `object_type` | 对象上 `analytics.roiStatus` 列表是否包含 `zone_name` |
| `line_crossing` | `line_name`, `min_count` | `detection.analytics.lineCrossing[]` 匹配 `name`，`in+out` 与阈值比较 |
| `overcrowding` | `zone_name` | `detection.analytics.overcrowding` 的 `roi_name` 与 `triggered` |

**摄像头范围**：`AlertRule.cameras` 为空 → 全部摄像头；否则仅匹配 M2M 中的摄像头。

---

## 10. Celery 定时任务（`config/settings/base.py` → `CELERY_BEAT_SCHEDULE`）

| 任务 | 周期 | 说明 |
|------|------|------|
| `tasks.maintenance.cleanup_old_detections` | 每天 | 按 `DETECTION_RETENTION_MONTHS` **删除旧分区表**（表名匹配 `detections_detection_*`） |
| `tasks.maintenance.create_next_partition` | 每天 | `CREATE TABLE ... PARTITION OF detections_detection` 创建下月分区 |
| `tasks.maintenance.sync_camera_status` | 每 60s | 拉取 DeepStream `get-stream-info`，修正 DB 与运行时不一致（未在活跃列表且仍为 online 类 → `error`；标记 offline 但实际有流 → `online`） |
| `tasks.maintenance.cleanup_dead_letters` | 每天 | 删除超过 `DEAD_LETTER_RETENTION_DAYS` 的死信记录 |

> **分区说明**：`Detection` 模型注释声明分区表设计；**初始 Django migration 创建的是普通表**。若要在生产使用 `create_next_partition` / `cleanup_old_detections`，需要 DBA 将 `detections_detection` 迁移为 **PostgreSQL 声明式分区父表** 并创建首月分区，与运维脚本一致。否则这两个任务会报错或无效。

---

## 11. 管道部署（`services/pipeline_deployer.py`）

- **输出目录**：环境变量 `DS_CONFIG_DEPLOY_DIR`（默认 `/shared/deepstream-config`）。
- **生成文件**：
  - `pgie_config.txt`：来自 `AIModel(detector)` 的 `model_file`、`label_file` 与 `config`（如 `num_classes`、`net_scale_factor` 等）。
  - `tracker_config.yml`：若配置了 tracker。
  - `analytics_config.txt`：若 `analytics_enabled`，按绑定到该 profile 的摄像头（按 `uid` 排序分配 stream 索引）展开 `AnalyticsZone`，写入 nvdsanalytics 风格的段（ROI / 越线 / 拥挤 / 方向）。
- **部署后**：`pipeline_profile.analytics_config_stale = False`。

DeepStream 容器需挂载同一共享卷，并将 `DS_ANALYTICS_CONFIG` 等指向生成文件；具体文件名与 DeepStream 镜像内约定需与运维一致。

---

## 12. WebSocket（`websocket/`）

- **认证**：`JWTAuthMiddleware` 从 query 读 `token=<access_token>`（浏览器无法自定义 WS Header）。
- **路径与分组**：

| URL | 组名 | 服务端事件 type → 客户端 type |
|-----|------|--------------------------------|
| `/ws/detections/` | `detections_{organization_id}` | `detection.new` |
| `/ws/cameras/status/` | `camera_status_{organization_id}` | `camera.status` |
| `/ws/alerts/` | `alerts_{organization_id}` | `alert.new` |

- 用户无 `organization_id` 时连接关闭码 **4001**。

---

## 13. 通知（`services/notification.py`）

由 Celery `tasks.notifications.send_alert_notification` 触发：

- **`email`**：`django.core.mail.send_mail`，收件人为**同组织 `role=admin` 且邮箱非空**的用户；`DEFAULT_FROM_EMAIL` 若未配置则使用占位地址。
- **`webhook`**：`settings.ALERT_WEBHOOK_URL` 若未设置则跳过并打日志。

异常会触发 Celery 重试（`max_retries=3` 等）。

---

## 14. 环境变量参考

| 变量 | 默认 / 说明 |
|------|-------------|
| `DJANGO_SETTINGS_MODULE` | 开发：`config.settings.development`；生产：`config.settings.production` |
| `SECRET_KEY` | 生产必须替换 |
| `DEBUG` | 布尔 |
| `ALLOWED_HOSTS` | 列表 |
| `DATABASE_URL` | `postgres://...` |
| `DB_CONN_MAX_AGE` | 默认 `600` |
| `REDIS_URL` | 无 DB 后缀的基础 URL，代码中拼接 `/0`…`/3` |
| `DEEPSTREAM_REST_URL` | DeepStream REST 根 URL |
| `DEEPSTREAM_MOCK` | 是否 Mock DeepStream |
| `KAFKA_BOOTSTRAP_SERVERS` | Kafka 地址 |
| `KAFKA_DETECTION_TOPIC` | 默认 `deepstream-detections` |
| `KAFKA_EVENT_TOPIC` | 默认 `deepstream-events` |
| `KAFKA_COMMAND_TOPIC` | 默认 `deepstream-commands`（Backend **不消费**命令 Topic，仅配置占位/对齐运维） |
| `KAFKA_CONSUMER_GROUP` | 默认 `backend-consumer` |
| `KAFKA_BATCH_SIZE` | 默认 `100` |
| `KAFKA_FLUSH_INTERVAL` | 默认 `2.0`（秒） |
| `ACCESS_TOKEN_LIFETIME_MINUTES` | 默认 `30` |
| `DETECTION_RETENTION_MONTHS` | 检测分区保留月数 |
| `DEAD_LETTER_RETENTION_DAYS` | 死信保留天数 |
| `HEALTH_CHECK_KAFKA` | 就绪探针是否检查 Kafka |
| `CORS_ALLOWED_ORIGINS` | 生产环境 CORS 列表 |

生产环境还通过 `production.py` 开启 cookie secure、XSS/内容类型防护等。

---

## 15. 本地与 Docker 开发

### 15.1 依赖安装（主机）

```bash
cd backend
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
```

创建数据库并执行迁移：

```bash
export DATABASE_URL=postgres://postgres:postgres@localhost:5432/ai_stream
export DJANGO_SETTINGS_MODULE=config.settings.development
python manage.py migrate
python manage.py createsuperuser
```

### 15.2 运行服务

- **仅 API（开发）**：`uvicorn config.asgi:application --reload --host 0.0.0.0 --port 8000`
- **Kafka 消费者**（需可连 Kafka 与 DB）：`python manage.py run_kafka_consumer`
- **Celery**：`celery -A config worker -l info`、`celery -A config beat -l info`

### 15.3 `docker-compose.dev.yml`

提供 **postgres、redis、kafka、backend、celery worker/beat、kafka-consumer**。默认 `DEEPSTREAM_MOCK=true`、`HEALTH_CHECK_KAFKA=false`（避免 dev 就绪探针强依赖 Kafka）。需要联调真实 DeepStream 时关闭 Mock 并设置 `DEEPSTREAM_REST_URL`。

---

## 16. 测试

| 内容 | 位置 |
|------|------|
| API：认证、摄像头、检测、报警、管道、DeepStream 代理、仪表盘、限流等 | `tests/test_api/` |
| 服务：Kafka 消费、AlertEngine、序列化 | `tests/test_services/` |
| 集成：WebSocket、Kafka 管道 | `tests/test_integration/` |

运行（示例）：

```bash
cd backend
pytest -q
```

`pytest.ini` 已设置 `DJANGO_SETTINGS_MODULE`；`conftest.py` 将 Channels 换为 **内存 channel layer** 便于测试。

---

## 17. 运维与排障提示

- **DeepStream 不可用**：检查网络、`DEEPSTREAM_REST_URL`、DeepStream 容器是否监听 9000；Mock 模式下不会发现连接问题。
- **Kafka 消费滞后**：调大 `KAFKA_BATCH_SIZE` / 缩短 `KAFKA_FLUSH_INTERVAL` 需权衡 DB 写入频率；确保消费者进程数与分区策略匹配运维要求。
- **检测不入库**：确认 Kafka 消息中 `sensorId` 与 `Camera.uid` 一致；相机是否已创建；消息是否被死信（查 `KafkaDeadLetter`）。
- **摄像头状态不更新**：依赖 `KAFKA_EVENT_TOPIC` 事件字段与 §8.3 映射一致；否则仅靠定时任务 `sync_camera_status` 纠偏。
- **报警不触发**：检查规则 `is_enabled`、`conditions` 是否与 DeepStream 输出的 JSON 结构一致（尤其 `analytics` 嵌套路径）。
- **分区任务失败**：确认已按 §10 将 `Detection` 表运维为分区表；否则仅依赖非分区表 + 手动清理历史数据。

---

## 18. 许可证与第三方

Django、DRF、Channels、confluent-kafka、PostgreSQL、Redis 等遵循各自开源许可证；生产部署请遵守依赖项许可与组织安全策略。
