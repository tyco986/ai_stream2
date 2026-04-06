# 后端 — 实现计划

## 1. 定位与职责

后端是三端架构的**编排层**和**唯一 API 网关**：

1. **面向前端** — 提供 REST API + WebSocket，处理认证和权限
2. **面向 DeepStream** — 通过 httpx 代理其内置 REST API，管理摄像头/管道生命周期
3. **面向 Kafka** — 消费 DeepStream 推送的检测元数据，入库 + 规则匹配 + 实时推送
4. **业务逻辑** — 报警规则引擎、通知服务、数据统计

**不做的事**：视频解码、AI 推理、流管理（全部由 DeepStream 完成）。

---

## 2. 技术栈

| 层面 | 技术 | 版本/说明 |
|------|------|---------|
| Web 框架 | Django | 5.x |
| API 层 | Django REST Framework | 3.15+ |
| 实时通信 | Django Channels | 4.x，Redis 作 channel layer |
| ASGI 服务器 | Uvicorn | 生产: `gunicorn -k uvicorn.workers.UvicornWorker`（多 worker + 事件循环） |
| 数据库 | PostgreSQL | 15+（Detection 表使用声明式分区） |
| ORM | Django ORM | 内置 |
| 迁移 | Django Migrations | 内置 |
| 缓存 | Redis | 7.x |
| 认证 | djangorestframework-simplejwt | JWT access + refresh token |
| Kafka | confluent-kafka-python | librdkafka 封装 |
| HTTP 客户端 | httpx | async，**长连接池**代理 DeepStream REST API |
| 异步任务 | Celery + Redis | 通知、报表、清理 |
| API 文档 | drf-spectacular | 自动生成 OpenAPI 3.0 |
| API 限流 | DRF Throttling | 登录接口 + 全局限流 |
| 日志 | structlog | 结构化 JSON 日志，携带 request_id |
| 配置解析 | django-environ | 12-factor 环境变量解析 |
| Admin | Django Admin | 内置，前期管理利器 |

### Async/Sync 边界约定

后端存在 async（httpx 代理 DeepStream）和 sync（Django ORM）两种 I/O 模型，必须统一策略避免混用：

| 场景 | 策略 | 说明 |
|------|------|------|
| **API View 调用 DeepStream** | **同步 View + `async_to_sync` 桥接 httpx** | DRF ViewSet 天然同步，避免在 async view 中处理 ORM；`async_to_sync(deepstream_client.add_stream)(...)` 一行完成桥接 |
| **Kafka Consumer** | **纯同步** | confluent-kafka 本身是同步 API，ORM 操作天然同步，无冲突 |
| **Celery 任务调用 DeepStream**（如 `sync_camera_status`） | **`async_to_sync` 桥接** | Celery worker 是同步进程，不能直接 await；通过 `async_to_sync(deepstream_client.get_stream_info)()` 调用 |
| **WebSocket Consumer** | **async Consumer + `database_sync_to_async`** | Django Channels 的 `AsyncJsonWebsocketConsumer` 天然 async，ORM 操作用 `database_sync_to_async` 包装 |

> **总原则**：API 层和 Celery 任务以**同步为主**，通过 `async_to_sync` 桥接 httpx；
> WebSocket 层以**异步为主**，通过 `database_sync_to_async` 桥接 ORM。
> 禁止在同一个函数中混用 `await` 和直接 ORM 调用。

---

## 3. 项目结构

```
backend/
├── config/                          # Django 项目配置
│   ├── __init__.py
│   ├── settings/
│   │   ├── base.py                  # 公共配置
│   │   ├── development.py           # 开发环境
│   │   └── production.py            # 生产环境
│   ├── urls.py                      # 根路由
│   ├── asgi.py                      # ASGI 入口 (Channels)
│   ├── wsgi.py                      # WSGI 入口 (备用)
│   └── celery.py                    # Celery 实例
├── apps/
│   ├── accounts/                    # 用户 & 认证
│   │   ├── models.py                # User, Organization
│   │   ├── serializers.py
│   │   ├── views.py
│   │   ├── urls.py
│   │   └── admin.py
│   ├── cameras/                     # 摄像头管理
│   │   ├── models.py                # Camera, CameraGroup, AnalyticsZone
│   │   ├── serializers.py
│   │   ├── views.py
│   │   ├── urls.py
│   │   └── admin.py
│   ├── detections/                  # 检测结果
│   │   ├── models.py                # Detection, KafkaDeadLetter
│   │   ├── serializers.py
│   │   ├── views.py
│   │   ├── urls.py
│   │   ├── admin.py
│   │   └── management/
│   │       └── commands/
│   │           └── run_kafka_consumer.py
│   ├── alerts/                      # 报警规则 & 记录
│   │   ├── models.py                # AlertRule, Alert
│   │   ├── serializers.py
│   │   ├── views.py
│   │   ├── urls.py
│   │   └── admin.py
│   ├── pipelines/                   # AI 模型 & 推理管道配置
│   │   ├── models.py                # AIModel, PipelineProfile, CameraModelBinding
│   │   ├── serializers.py
│   │   ├── views.py
│   │   ├── urls.py
│   │   └── admin.py
│   └── dashboard/                   # 仪表盘聚合
│       ├── views.py
│       └── urls.py
├── services/                        # 跨 app 业务逻辑
│   ├── deepstream_client.py         # DeepStream REST API 代理
│   ├── kafka_consumer.py            # Kafka 消费者（detections topic）
│   ├── alert_engine.py              # 报警规则引擎
│   ├── pipeline_deployer.py         # 管道配置部署（生成 DeepStream 配置文件 + analytics INI）
│   └── notification.py              # 通知服务 (邮件/Webhook)
├── common/                          # 项目公共模块
│   ├── models.py                    # BaseModel 基类
│   ├── exceptions.py                # ServiceError 等自定义异常
│   ├── response.py                  # 统一响应包装
│   ├── pagination.py                # 自定义分页器
│   ├── permissions.py               # 自定义权限类
│   ├── throttles.py                 # 自定义限流器
│   └── middleware.py                # RequestID 注入中间件
├── websocket/                       # Django Channels
│   ├── consumers.py                 # WebSocket Consumer
│   ├── routing.py                   # WebSocket 路由
│   └── middleware.py                # WebSocket JWT 认证中间件
├── tasks/                           # Celery 异步任务
│   ├── notifications.py             # 发送通知
│   └── maintenance.py               # 数据清理、报表
├── tests/                           # 测试
│   ├── conftest.py                  # pytest fixtures
│   ├── factories.py                 # Model Factory (factory_boy)
│   ├── test_cameras/
│   ├── test_detections/
│   ├── test_alerts/
│   └── test_services/
├── Dockerfile
├── docker-compose.dev.yml
├── requirements.txt
└── manage.py
```

---

## 4. 数据模型

### 4.1 ER 关系总览

```
Organization 1──N User
Organization 1──N CameraGroup
Organization 1──N Camera                ← Camera 直接关联 Organization
Organization 1──N AIModel               ← 模型归属组织
Organization 1──N PipelineProfile       ← 管道配置归属组织
CameraGroup  1──N Camera (optional)
Camera       1──N Detection
Camera       1──N AnalyticsZone         ← 分析区域（ROI/越线/拥挤/方向）
Camera       N──M AIModel               ← 通过 CameraModelBinding 关联
PipelineProfile 1──N CameraModelBinding
AIModel      1──N CameraModelBinding
AlertRule    N──1 Organization
AlertRule    1──N Alert
Alert        N──1 Camera
Alert        N──1 Organization          ← 冗余 FK，加速多租户过滤
Detection    N──1 Camera
```

### 4.2 accounts app

**User** — 继承 `AbstractUser`，扩展组织归属：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| username | CharField | 登录名 |
| email | EmailField | 邮箱 |
| organization | FK → Organization | 所属组织 |
| role | CharField(choices) | admin / operator / viewer |
| is_active | BooleanField | 是否启用 |

**Organization** — 多租户隔离单元：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| name | CharField | 组织名称 |
| created_at | DateTimeField | 创建时间 |
| updated_at | DateTimeField | 更新时间 |

### 4.3 cameras app

**CameraGroup** — 摄像头分组（楼层/区域）：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| name | CharField | 分组名称 |
| organization | FK → Organization | 所属组织 |
| description | TextField | 描述 |

**Camera** — 核心实体，与 DeepStream 流一一对应：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| uid | CharField(unique) | 对应 DeepStream camera_id，全局唯一 |
| name | CharField | 摄像头名称 |
| rtsp_url | CharField | RTSP 地址 |
| organization | FK → Organization | **直接归属组织**（多租户过滤走此字段，避免 JOIN） |
| group | FK → CameraGroup (nullable) | 所属分组（可选，分组为组织内的逻辑分类） |
| status | CharField(choices) | offline / connecting / online / error |
| is_deleted | BooleanField | 软删除 |
| config | JSONField | 扩展配置（分辨率、帧率等） |
| created_at | DateTimeField | 创建时间 |
| updated_at | DateTimeField | 更新时间 |

> **设计决策**：Camera 保留直接 `organization` FK 而非仅通过 `group → organization` 间接关联。
> 原因：多租户 queryset 过滤是最高频操作（每个 API 请求都执行），直连 FK 避免 JOIN，
> 且 group 变为可选后，Camera 不会因未分组而丢失组织归属。

`uid` 是传给 DeepStream REST API 的 `camera_id`，也是 Kafka 消息中 `sensorId` 的对应字段。

**AnalyticsZone** — 摄像头的 nvdsanalytics 分析区域配置（ROI/越线/拥挤/方向）：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| camera | FK → Camera | 关联摄像头 |
| name | CharField | 区域/线名称（如"大门入口"、"通道A"） |
| zone_type | CharField(choices) | `roi` / `line_crossing` / `overcrowding` / `direction` |
| coordinates | JSONField | 坐标点列表 `[[x1,y1], [x2,y2], ...]`（ROI=多边形，越线=两点线段，方向=两点向量） |
| config | JSONField | 类型特定参数 |
| is_enabled | BooleanField | 是否启用 |
| created_at | DateTimeField | 创建时间 |
| updated_at | DateTimeField | 更新时间 |

**AnalyticsZone.config 按 zone_type 的 JSON schema**：

| zone_type | config 字段 | 示例 |
|-----------|-----------|------|
| `roi` | `class_id`（-1=全部）、`inverse`（是否反选） | `{"class_id": -1, "inverse": false}` |
| `line_crossing` | `class_id`、`extended`（是否统计方向） | `{"class_id": 0, "extended": false}` |
| `overcrowding` | `object_threshold`（报警人数阈值） | `{"object_threshold": 5}` |
| `direction` | `direction_name`（如"South"、"North"） | `{"direction_name": "South"}` |

> **设计说明**：`AnalyticsZone` 的坐标基于配置分辨率（默认 1920×1080，与 DeepStream 的
> `nvmultiurisrcbin` 一致）。`PipelineDeployer` 读取摄像头关联的所有 `AnalyticsZone`，
> 生成 `nvdsanalytics` 的 INI 配置文件。变更区域后需通过 deploy 端点重部署。

**状态流转**：

```
新建 → offline
调用 DeepStream add → connecting
DynamicSourceMessage(added) → online
DeepStream 报错/超时 → error
调用 DeepStream remove → offline
软删除 → is_deleted=True
```

### 4.4 detections app

**Detection** — 检测记录（高频写入，数据量最大的表）：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | **BigAutoField** | 自增主键（**不用 UUID**，见下方说明） |
| camera | FK → Camera | 来源摄像头 |
| detected_at | DateTimeField | 检测时间（来自 Kafka 消息 @timestamp，**分区键**） |
| ingested_at | DateTimeField(auto_now_add) | 入库时间（用于排查 Kafka 延迟） |
| frame_number | BigIntegerField | 帧号 |
| object_count | IntegerField | 该帧检测到的对象数 |
| objects | JSONField | 检测对象数组 `[{type, confidence, bbox, object_id, classifier?, analytics?}]` |
| analytics | JSONField (nullable) | 帧级分析结果（越线计数、拥挤状态），来自 nvdsanalytics |

> `objects` 中的 `classifier` 字段为可选数组，由 SGIE 自动填充（如 `[{"type": "vehicle_type", "label": "SUV"}]`）。
> `objects` 中的 `analytics` 字段为可选对象，由 nvdsanalytics 填充（如 `{"roiStatus": ["entrance"], "direction": "South"}`）。
> 帧级 `analytics` 字段包含越线统计和拥挤状态（如 `{"overcrowding": {...}, "lineCrossing": [...]}`）。
> 无相应插件时对应字段不存在。后端原样存储 Kafka 消息结构。

> **为什么不用 UUID 主键**：Detection 表日均写入 140 万行，UUID v4 是随机值，
> 导致 B-tree 索引页分裂和随机 I/O，写入性能下降 30-50%。
> `BigAutoField` 是顺序递增的，对 B-tree 和分区表均友好。
> 其他低频表（Camera、AlertRule 等）继续使用 UUID 主键。

> **为什么去掉 raw_message**：140 万行/天 × 平均 500 字节 = 每天 ~700MB 仅此一字段。
> 调试应依赖 Kafka 消费端日志 + Kafka 本身的 retention（默认 7 天），不入库。

**分区策略（Day 1 设计，不是"后期考虑"）**：

```sql
CREATE TABLE detection (
    id          BIGSERIAL,
    camera_id   UUID NOT NULL,
    detected_at TIMESTAMPTZ NOT NULL,
    ingested_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    frame_number BIGINT,
    object_count INTEGER,
    objects     JSONB,
    analytics   JSONB,                 -- 帧级分析结果（nvdsanalytics 越线/拥挤）
    PRIMARY KEY (id, detected_at)      -- 分区键必须包含在主键中
) PARTITION BY RANGE (detected_at);

-- 自动创建月度分区（pg_partman 扩展或 Celery Beat 任务）
CREATE TABLE detection_2026_04 PARTITION OF detection
    FOR VALUES FROM ('2026-04-01') TO ('2026-05-01');
```

> **Django ORM 迁移与分区表操作顺序**：
>
> Django `makemigrations` 生成的 `CreateModel` 建的是普通表，不含 `PARTITION BY` 子句。
> 且 `PRIMARY KEY (id, detected_at)` 这种复合主键 Django ORM 原生不支持。操作顺序：
>
> 1. 正常 `makemigrations` 生成 Detection 模型迁移（Django 会建普通表）
> 2. 在同一迁移文件末尾追加 `RunSQL`：DROP 普通表 → 执行上方带 `PARTITION BY` 的建表 SQL → 创建初始分区
> 3. 后续 Celery Beat 的 `create_next_partition` 任务负责滚动创建下月分区
> 4. `bulk_create` 写入时 PostgreSQL 自动路由到对应分区，ORM 层无感知
> 5. Django `managed = True` 保留，但后续分区相关的 DDL 变更（加字段等）
>    需确认变更能正确传播到所有子分区（PostgreSQL 声明式分区默认继承父表 DDL）
>
> **实现约束（避免 ORM 认知偏差）**：
> Django 逻辑层仍以 `id` 为对象标识（`pk`），数据库物理层使用 `(id, detected_at)` 复合主键服务分区。
> 团队需固定迁移模板（含 `RunSQL`）并在评审中检查分区 DDL 兼容性，避免把该模型当普通单表处理。
>
> 模型 `Meta` 中不声明 `unique_together` 或 `constraints` 涉及非分区键的列
>（分区表的唯一约束必须包含分区键 `detected_at`）。

**索引策略**（在每个分区上自动创建）：

```python
class Meta:
    indexes = [
        models.Index(fields=["camera", "-detected_at"]),
        models.Index(fields=["-detected_at"]),
    ]
```

**数据量预估与清理**：

- 假设 16 路摄像头，每秒 1 条 → 每天约 140 万条
- 使用 PostgreSQL 声明式分区（按月），清理时直接 `DROP` 过期分区，瞬间完成，无需逐行 DELETE
- Celery Beat 定时任务：创建下月分区 + DROP 超过 `DETECTION_RETENTION_MONTHS` 的旧分区

> **保留策略说明**：按月分区无法精确到"天"。为避免语义歧义，运维参数使用 `DETECTION_RETENTION_MONTHS`（整月粒度）。
> 若业务必须按天精确保留，需改为按日分区（分区数量和维护成本显著上升）。

**KafkaDeadLetter** — Kafka 消费失败的死信记录：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| topic | CharField | Kafka topic |
| partition | IntegerField | 分区号 |
| offset | BigIntegerField | offset |
| raw_message | TextField | 原始消息内容 |
| error_message | TextField | 错误信息 |
| created_at | DateTimeField | 记录时间 |

> **扩展预留 — ActionDetection**（初版不实现，与 DeepStream 端 SGIE/动作识别对齐）：
>
> 后续版本 DeepStream 管道新增 SGIE/SlowFast 后，在此处添加 `ActionDetection` 模型，
> 消费 `deepstream-actions` topic。初版不创建此表、不订阅此 topic。

### 4.5 alerts app

**AlertRule** — 报警规则配置：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| name | CharField | 规则名称 |
| organization | FK → Organization | 所属组织 |
| rule_type | CharField(choices) | object_count / object_type / zone_intrusion / line_crossing / overcrowding |
| conditions | JSONField | 规则条件（schema 因 rule_type 而异，见下方规则类型表） |
| cameras | M2M → Camera | 关联的摄像头（空=全部） |
| is_enabled | BooleanField | 是否启用 |
| cooldown_seconds | IntegerField | 冷却时间，避免重复报警 |
| notify_channels | JSONField | 通知渠道 `["websocket", "email", "webhook"]` |

**Alert** — 报警记录：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| rule | FK → AlertRule | 触发的规则 |
| camera | FK → Camera | 触发的摄像头 |
| organization | FK → Organization | 所属组织（冗余，加速多租户过滤） |
| triggered_at | DateTimeField(db_index) | 触发时间 |
| status | CharField(choices) | pending / acknowledged / resolved |
| snapshot | JSONField | 触发时的检测数据快照 |
| acknowledged_by | FK → User (nullable) | 确认人 |
| acknowledged_at | DateTimeField (nullable) | 确认时间 |
| resolved_by | FK → User (nullable) | 解决人 |
| resolved_at | DateTimeField (nullable) | 解决时间 |

### 4.6 pipelines app

**AIModel** — AI 模型注册表（每个组织独立管理模型）：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| name | CharField(unique per org) | 模型名称（如 `yolov8n`、`slowfast_r50`） |
| organization | FK → Organization | 所属组织 |
| model_type | CharField(choices) | `detector` / `tracker`（扩展预留：`classifier` / `action`） |
| framework | CharField(choices) | `onnx` / `engine` / `custom` |
| model_file | CharField | 模型文件路径（容器内路径或对象存储 URL） |
| label_file | CharField (nullable) | 标签文件路径 |
| config | JSONField | 模型特定配置（因 model_type 而异，见下表） |
| version | CharField | 模型版本号 |
| description | TextField (blank) | 模型描述 |
| is_active | BooleanField | 是否可用 |
| created_at | DateTimeField | 创建时间 |
| updated_at | DateTimeField | 更新时间 |

**AIModel.config 按 model_type 的 JSON schema**：

| model_type | config 必填字段 | 示例 |
|-----------|----------------|------|
| `detector` | `num_classes`, `net_scale_factor`, `cluster_mode`, `network_mode` | `{"num_classes": 80, "net_scale_factor": 0.00392, "cluster_mode": 2, "network_mode": 1}` |
| `classifier`（扩展） | `num_classes`, `operate_on_class_ids`, `network_mode` | `{"num_classes": 6, "operate_on_class_ids": [2, 5, 7], "network_mode": 1}` |
| `tracker` | `tracker_type` | `{"tracker_type": "NvDCF_perf"}` |
| `action`（扩展） | `clip_length`, `stride`, `input_size` | `{"clip_length": 32, "stride": 16, "input_size": [224, 224]}` |

**PipelineProfile** — 推理管道配置（有序的模型组合）：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| name | CharField | 管道名称（如"交通监控-标准"） |
| organization | FK → Organization | 所属组织 |
| description | TextField (blank) | 描述 |
| detector | FK → AIModel | 主检测模型（必须是 detector 类型，恰好 1 个） |
| tracker | FK → AIModel (nullable) | 跟踪配置（默认使用系统内置 NvDCF） |
| analytics_enabled | BooleanField | 是否启用 nvdsanalytics 视频分析 |
| is_active | BooleanField | 是否启用 |
| created_at | DateTimeField | 创建时间 |
| updated_at | DateTimeField | 更新时间 |

> **约束**：`detector` 必须是 `model_type=detector` 的 AIModel，
> `tracker` 必须是 `model_type=tracker`。在 Serializer 层做 `validate()` 校验。
>
> **扩展预留**：后续版本新增 `classifiers`（M2M → AIModel，SGIE 二级分类，0~N 个有序）
> 和 `action_model`（FK → AIModel nullable，动作识别，0~1 个）字段。

**CameraModelBinding** — 摄像头与管道配置的关联：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | UUID | 主键 |
| camera | **OneToOneField** → Camera | 关联摄像头（**唯一**，一个摄像头只能绑定一个管道配置） |
| pipeline_profile | FK → PipelineProfile | 使用的管道配置 |
| is_enabled | BooleanField | 是否启用此绑定 |
| created_at | DateTimeField | 创建时间 |

> **唯一约束**：`camera` 使用 `OneToOneField`（隐含 `unique=True`），确保一个摄像头只能绑定一个 PipelineProfile。
> 这与 DeepStream 的物理约束一致——`nvmultiurisrcbin` 内所有流共享同一管道，一个摄像头不可能同时使用两套推理配置。
>
> **设计说明**：
> - DeepStream 的 `nvmultiurisrcbin` 将所有视频流送入同一管道，因此同一 DeepStream 实例
>   内的所有摄像头共享同一推理链。
> - `CameraModelBinding` 的作用是**后端层面**的逻辑关联：记录每个摄像头期望使用哪个管道配置，
>   用于报警规则过滤、检测结果归因、以及部署时生成 DeepStream 配置文件。
> - 当 PipelineProfile 变更时，需要重启 DeepStream 容器才能生效（管道拓扑是启动时确定的）。
>   后端通过 `PipelineDeployer` 服务生成配置文件并触发重启。

---

## 5. API 端点设计

### 5.1 认证

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/auth/login/` | 登录，返回 access + refresh token |
| POST | `/api/v1/auth/refresh/` | 刷新 access token |
| GET | `/api/v1/auth/me/` | 获取当前用户信息 |

### 5.2 摄像头管理

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/cameras/` | 摄像头列表 |
| POST | `/api/v1/cameras/` | 创建摄像头 |
| GET | `/api/v1/cameras/{id}/` | 摄像头详情 |
| PATCH | `/api/v1/cameras/{id}/` | 更新摄像头 |
| DELETE | `/api/v1/cameras/{id}/` | 软删除摄像头 |
| POST | `/api/v1/cameras/{id}/start-stream/` | 启动视频流（→ DeepStream add） |
| POST | `/api/v1/cameras/{id}/stop-stream/` | 停止视频流（→ DeepStream remove） |
| GET | `/api/v1/camera-groups/` | 摄像头分组列表 |
| POST | `/api/v1/camera-groups/` | 创建分组 |

### 5.3 检测结果

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/detections/` | 检测记录列表（CursorPagination） |
| GET | `/api/v1/detections/stats/` | 检测统计（按时间段/摄像头聚合） |

过滤参数：`?camera_id=xxx&start_time=xxx&end_time=xxx&object_type=person`

### 5.4 报警管理

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/alert-rules/` | 报警规则列表 |
| POST | `/api/v1/alert-rules/` | 创建报警规则 |
| PATCH | `/api/v1/alert-rules/{id}/` | 更新报警规则 |
| DELETE | `/api/v1/alert-rules/{id}/` | 删除报警规则 |
| GET | `/api/v1/alerts/` | 报警记录列表 |
| POST | `/api/v1/alerts/{id}/acknowledge/` | 确认报警 |
| POST | `/api/v1/alerts/{id}/resolve/` | 解决报警 |

### 5.5 AI 模型与管道配置

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/ai-models/` | AI 模型列表 |
| POST | `/api/v1/ai-models/` | 注册 AI 模型 |
| GET | `/api/v1/ai-models/{id}/` | 模型详情 |
| PATCH | `/api/v1/ai-models/{id}/` | 更新模型信息 |
| DELETE | `/api/v1/ai-models/{id}/` | 删除模型 |
| GET | `/api/v1/pipeline-profiles/` | 管道配置列表 |
| POST | `/api/v1/pipeline-profiles/` | 创建管道配置 |
| GET | `/api/v1/pipeline-profiles/{id}/` | 管道配置详情 |
| PATCH | `/api/v1/pipeline-profiles/{id}/` | 更新管道配置 |
| DELETE | `/api/v1/pipeline-profiles/{id}/` | 删除管道配置 |
| POST | `/api/v1/pipeline-profiles/{id}/deploy/` | 部署管道配置到 DeepStream（生成配置 + 重启） |
| GET | `/api/v1/cameras/{id}/pipeline/` | 查看摄像头当前管道配置 |
| PUT | `/api/v1/cameras/{id}/pipeline/` | 设置摄像头管道配置 |

> **`deploy` 端点**：将 PipelineProfile 转换为 DeepStream 配置文件（PGIE YAML、
> tracker YAML、nvdsanalytics INI），推送到 DeepStream 容器的共享卷，
> 然后通过 Docker API 或信号重启 DeepStream 容器。此操作会短暂中断所有视频流。

### 5.6 分析区域配置

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/cameras/{id}/analytics-zones/` | 该摄像头的分析区域列表 |
| POST | `/api/v1/cameras/{id}/analytics-zones/` | 创建分析区域（ROI/越线/拥挤/方向） |
| PATCH | `/api/v1/cameras/{id}/analytics-zones/{zone_id}/` | 更新分析区域 |
| DELETE | `/api/v1/cameras/{id}/analytics-zones/{zone_id}/` | 删除分析区域 |

> 分析区域与摄像头绑定。坐标基于 1920×1080 分辨率。
> 变更后需通过 `deploy` 端点重部署管道配置，`PipelineDeployer` 会读取所有关联摄像头的
> `AnalyticsZone` 自动生成 `nvdsanalytics` 的 `analytics_config.txt`。
>
> **stream-id 漂移防护**：摄像头增删后，`PipelineDeployer` 自动将当前部署标记为
> `analytics_config_stale=True`（PipelineProfile 字段）。前端检测到此标记时在
> 摄像头列表/详情页显示持久警告条：**"摄像头集合已变更，分析区域配置可能不准确，请重新部署管道"**。
> 用户确认重部署后标记清除。避免运维忘记重部署导致 nvdsanalytics 的 `stream-N` 规则
> 作用于错误的摄像头。

### 5.7 仪表盘

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/dashboard/overview/` | 总览（在线摄像头数、今日检测数、未处理报警数） |
| GET | `/api/v1/dashboard/detection-trend/` | 检测趋势（时间序列） |
| GET | `/api/v1/dashboard/camera-status/` | 各摄像头状态统计 |

### 5.8 系统状态

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/api/v1/health/live/` | **Liveness**（进程存活，固定 200） |
| GET | `/api/v1/health/ready/` | **Readiness**（DB + Redis + Kafka 连通性） |
| GET | `/api/v1/deepstream/health/` | DeepStream 健康检查（代理） |
| GET | `/api/v1/deepstream/streams/` | 当前流信息（代理） |

> 健康检查拆分为 Liveness / Readiness 两个端点（详见第 15 节）。
> backend API 容器的 Docker HEALTHCHECK 指向 `/api/v1/health/ready/`，K8s livenessProbe 指向 `/live/`。
> 两个端点均为公开接口（`AllowAny`）。

### 5.9 详细 API 文档编写规范（OpenAPI + 业务语义）

为保证前后端联调、测试脚本编写、线上排障的一致性，每个 API 端点都必须补齐“可执行文档”，不仅有路径，还要有业务语义。

**每个端点文档必须包含**：

1. 接口基础信息：`summary`、`description`、标签（tag）、鉴权方式（JWT/AllowAny）
2. 请求定义：Path 参数、Query 参数、Request Body schema、字段必填性、默认值、枚举值
3. 响应定义：成功响应 schema（200/201）+ 常见失败响应（400/401/403/404/429/503）
4. 业务错误码：与 `ServiceError.code` 对齐（如 `CAMERA_NOT_FOUND`、`DEEPSTREAM_UNAVAILABLE`）
5. 权限与多租户说明：哪个角色可调用、跨组织访问的预期结果（一般为 404）
6. 副作用说明：是否触发外部调用（DeepStream/Kafka/Celery）、是否幂等、是否异步最终一致
7. 示例：至少 1 个请求示例 + 1 个成功响应示例 + 1 个失败响应示例

**推荐实现方式（文档层约束）**：

- DRF ViewSet/View 上使用 `drf-spectacular` 的 `@extend_schema` / `@extend_schema_view`
- 公共错误响应抽成可复用 `OpenApiResponse` 片段，避免各端点重复且不一致
- 对 `start-stream`、`stop-stream`、`deploy`、`acknowledge`、`resolve` 这类动作型端点，必须单独写清幂等性和状态流转

**完成标准（DoD）**：

- `openapi.json` 中所有业务端点都有 `summary + description + response schema`
- Swagger UI 中每个端点都能看见参数说明和示例
- 业务错误码在文档与实际响应一致，不允许“文档写 A、返回 B”

---

## 6. DeepStream 代理服务

### 设计

封装为 Service 单例，**必须复用 `httpx.AsyncClient` 连接池**：

```python
import httpx
from django.conf import settings

class DeepStreamClient:
    """长连接池代理 DeepStream REST API。

    httpx.AsyncClient 内部维护连接池（默认 100 连接），
    复用 TCP 连接避免每次请求的握手开销。禁止每次请求新建 AsyncClient。
    """

    def __init__(self):
        self._client = httpx.AsyncClient(
            base_url=settings.DEEPSTREAM_REST_URL,
            timeout=httpx.Timeout(connect=5.0, read=10.0, write=5.0, pool=5.0),
        )

    async def add_stream(self, camera_id, camera_name, rtsp_url):
        return await self._client.post("/api/v1/stream/add", json={
            "key": "sensor",
            "value": {
                "camera_id": camera_id,
                "camera_name": camera_name,
                "camera_url": rtsp_url,
                "change": "camera_add",
            }
        })

    async def remove_stream(self, camera_id, rtsp_url):
        return await self._client.post("/api/v1/stream/remove", json={
            "key": "sensor",
            "value": {
                "camera_id": camera_id,
                "camera_url": rtsp_url,
                "change": "camera_remove",
            }
        })

    async def get_stream_info(self):
        return await self._client.get("/api/v1/stream/get-stream-info")

    async def health_check(self):
        return await self._client.get("/api/v1/health/get-dsready-state")

    async def close(self):
        await self._client.aclose()

# 模块级单例
deepstream_client = DeepStreamClient()
```

> **为什么不能每次 `async with httpx.AsyncClient():`**：
> 每次新建 Client = 新 TCP 连接 + DNS 解析。高频调用时产生延迟和 FD 泄漏风险。
> 工业实践：长驻 Client 实例 + 分级超时（connect 短、read 长）。
>
> **生命周期管理**：ASGI 服务器关闭时需调用 `deepstream_client.close()` 释放连接池。
> 在 `config/asgi.py` 中通过 ASGI lifespan 协议的 `lifespan.shutdown` 事件处理。
> 实现时参考 [ASGI Lifespan Spec](https://asgi.readthedocs.io/en/latest/specs/lifespan.html)
> 或使用 Starlette 的 `@app.on_event("shutdown")` 等高层封装。

### 调用链路

```
前端点击"启动视频流"
  → POST /api/v1/cameras/{id}/start-stream/
  → CameraViewSet.start_stream()
  → CameraService.start_stream(camera)
      → deepstream_client.add_stream(camera.uid, camera.name, camera.rtsp_url)
      → 成功：camera.status = "connecting"，保存
      → 失败：异常抛出，DRF 全局异常处理器返回错误
```

### start-stream 幂等策略

**已在线（`online`/`connecting`）的摄像头再次调用 `start-stream`：返回 200 + 当前状态，不重复调 DeepStream。**

理由：前端断线重连、运维批量脚本、用户重复点击都可能重复调用。返回 400 会制造不必要的错误处理分支。

```python
def start_stream(self, camera):
    if camera.status in ("online", "connecting"):
        return camera  # 幂等：已在线直接返回
    # ... 正常调用 DeepStream add_stream
```

> **边界情况——DB 为 online 但 DeepStream 实际已掉线**：幂等检查依据的是 DB 状态，
> 不会主动探测 DeepStream。此时用户点击"启动"会被拦截直接返回，不触发重连。
> 这是有意为之——`sync_camera_status` Celery Beat 任务（每分钟）会从 DeepStream
> 查询实际流状态并将 DB 修正为 `error`/`offline`，之后用户重试即可正常启动。
> 如需立即恢复，运维可调用 `stop-stream` + `start-stream` 强制重连。

### 注意事项

- DeepStream 不可达时，让 `httpx.ConnectError` 自然抛出
- **操作顺序：先调外部再写库**——先调 DeepStream `add_stream`，成功后再用短事务写 `camera.status = "connecting"`。DeepStream 失败时 DB 无脏数据，无需回滚，也不存在长事务问题
- **禁止在 `transaction.atomic()` 内做网络 I/O**：httpx 调用耗时不可控（网络抖动、DeepStream 重启），若包裹在事务内会拉长数据库事务持有时间，高并发下导致连接池耗尽和行锁竞争
- `stop-stream` 同理：先调 DeepStream `remove_stream`，成功后再短事务写 `camera.status = "offline"`
- Camera.status 的最终更新（connecting → online）由 Kafka Consumer 异步完成
- 这意味着 DB 与 DeepStream 之间是**最终一致性**，不是分布式事务

> **边界情况——DeepStream 成功但写库失败**：极低概率（DB 连接瞬断等）。此时 DeepStream
> 已添加流但 Camera.status 仍为旧状态。`sync_camera_status` Celery Beat 任务（每分钟）
> 会从 DeepStream 查询实际流状态并同步到 DB，自动修复不一致。无需引入分布式事务。

---

## 7. Kafka Consumer

### 架构

```
                                     ┌─────────────────────────┐
Kafka topic                          │   DetectionConsumer      │
deepstream-detections ──poll()──►    │                          │
                                     │  1. 解析检测消息          │
                                     │  2. 批量入库 Detection     │
                                     │  3. AlertEngine 规则匹配   │
                                     │  4. channel_layer 推送     │
                                     │  5. commit offset         │
                                     └─────────────────────────┘
```

后端 Consumer 订阅 `deepstream-detections` topic，批量入库并触发报警规则。
DeepStream 端 `EmptyFrameFilter` 已过滤空帧，实际消息量降低 50-80%。

### 运行方式

Django management command，Docker 中独立容器/进程：

```bash
python manage.py run_kafka_consumer
```

### 批量入库策略（含 Graceful Shutdown + Camera 缓存）

```python
import signal
import time
import structlog
from confluent_kafka import Consumer
from django.conf import settings
from django.db import transaction

logger = structlog.get_logger(__name__)

class DetectionConsumer:
    BATCH_SIZE = 100
    FLUSH_INTERVAL = 2.0  # seconds

    def __init__(self):
        self._shutdown = False
        self._camera_cache = {}        # uid → Camera instance
        self._cache_ttl = 300          # 缓存 5 分钟
        self._cache_loaded_at = 0
        signal.signal(signal.SIGTERM, self._handle_signal)
        signal.signal(signal.SIGINT, self._handle_signal)

    def _handle_signal(self, signum, frame):
        logger.info("Received signal %s, shutting down gracefully...", signum)
        self._shutdown = True

    def _get_camera(self, sensor_id):
        """内存缓存 sensorId → Camera 映射，避免每条消息查库。"""
        now = time.time()
        if now - self._cache_loaded_at > self._cache_ttl:
            from apps.cameras.models import Camera
            self._camera_cache = {
                c.uid: c for c in Camera.objects.filter(is_deleted=False)
            }
            self._cache_loaded_at = now
        return self._camera_cache.get(sensor_id)

    def run(self):
        consumer = Consumer(self._kafka_config())
        consumer.subscribe([settings.KAFKA_DETECTION_TOPIC])
        detection_buffer = []
        last_flush = time.time()

        while not self._shutdown:
            msg = consumer.poll(timeout=1.0)
            if msg and not msg.error():
                detection_buffer.append(self._parse_message(msg))

            should_flush = (
                len(detection_buffer) >= self.BATCH_SIZE
                or (detection_buffer
                    and time.time() - last_flush >= self.FLUSH_INTERVAL)
            )
            if should_flush:
                self._flush_detections(detection_buffer)
                consumer.commit(asynchronous=False)
                detection_buffer.clear()
                last_flush = time.time()

        # Graceful shutdown: flush remaining buffers before exit
        if detection_buffer:
            logger.info("Flushing %d detections before shutdown",
                        len(detection_buffer))
            self._flush_detections(detection_buffer)
            consumer.commit(asynchronous=False)
        consumer.close()
        logger.info("Kafka consumer closed cleanly")
```

> **三个关键改进**：
> 1. **Graceful Shutdown**：捕获 SIGTERM/SIGINT，退出循环前 flush 剩余 buffer 并 commit offset，Docker 停容器时不丢消息。
> 2. **Camera 缓存**：`sensorId → Camera` 映射缓存在内存中（TTL 5分钟），避免每条消息查一次 DB。16 路摄像头 × 1 FPS = 每秒 16 次 DB 查询 → 每 5 分钟 1 次。
> 3. **信号处理**：在 management command 进程中直接注册信号，不依赖外部机制。
>
> **注意**：DeepStream 端 `EmptyFrameFilter` 过滤空帧后，实际 Kafka 消息量降低 50-80%，
> Consumer 压力相应降低。Dashboard 帧率统计需改用后端定期从 DeepStream PerfMonitor 获取的数据。
>
> **扩展预留**：后续版本新增 `deepstream-actions` topic 后，在此处添加 `action_buffer`
> 和 `_flush_actions()` 逻辑，`consumer.subscribe` 中增加 `KAFKA_ACTION_TOPIC`。

### Kafka 提交语义与幂等

**提交语义：至少一次（at-least-once）**。`consumer.commit(asynchronous=False)` 在 `_flush_detections()` 成功后同步提交。
显式 `asynchronous=False` 确保 commit 完成后才继续下一轮 poll，避免依赖默认值。
`commit()` 无参数时提交所有已分配 partition 的 stored offsets（即 `poll()` 返回的最后位置）。
初版单 partition 场景下与"整批成功才 commit"语义完全一致；扩展到多 partition 时，
一次 `poll()` 可能返回多个 partition 的消息，flush 成功后统一 commit 所有 partition 的 offset。
如果 flush 成功但 commit 前进程崩溃，重启后会重复消费已入库的消息。

**去重策略：接受少量重复，不做显式去重**。理由：
- Detection 是 `BigAutoField` 自增主键，没有天然业务去重键
- 构造去重键（如 `camera_id + detected_at + frame_number`）需要唯一索引，在每秒 16 次写入下增加写入开销
- 重复的 Detection 行对业务影响极小（报警规则有 cooldown 保护，Dashboard 统计用聚合查询，个位数重复在百万级数据中不可见）
- 进程崩溃是低频事件，正常运行时 commit 紧跟 flush，重复窗口仅为单个 batch（≤100 条）

> **如果未来需要严格去重**，可在 `_flush_detections()` 中对 `bulk_create` 加
> `update_conflicts=True`（Django 4.1+ `ignore_conflicts` 或 `update_conflicts`），
> 基于 `(camera_id, detected_at, frame_number)` 联合唯一约束。但初版不需要。

### 消息解析与错误处理

**`_parse_message` 错误处理流程**：

```python
def _parse_message(self, msg):
    """解析 Kafka 消息，失败写入死信表。"""
    raw = msg.value()
    data = json.loads(raw.decode("utf-8"))
    camera = self._get_camera(data["sensorId"])
    if camera is None:
        return None
    detected_at = parse_datetime(data["@timestamp"])
    if detected_at is None:
        raise ValueError("Invalid @timestamp")
    return Detection(
        camera=camera,
        detected_at=detected_at,
        frame_number=data.get("frame_number"),
        object_count=len(data.get("objects", [])),
        objects=data.get("objects", []),
        analytics=data.get("analytics"),
    )

def _flush_detections(self, buffer):
    valid = [d for d in buffer if d is not None]
    if valid:
        with transaction.atomic():
            Detection.objects.bulk_create(valid)
        # PostgreSQL bulk_create 通过 RETURNING 子句回填自增 id 到实例上，
        # 后续 AlertEngine 和 WebSocket 推送可直接使用 detection.id。
        # 注意：ignore_conflicts=True 或 MySQL 后端不回填 id，如有迁移需调整。
    # AlertEngine + WebSocket 推送 ...
```

**异常处理在 `run()` 循环中**：

```python
if msg and not msg.error():
    parsed = self._safe_parse(msg)
    if parsed is not None:
        detection_buffer.append(parsed)

def _safe_parse(self, msg):
    """解析失败写死信，不中断消费循环。"""
    raw = msg.value()
    try:
        return self._parse_message(msg)
    except (json.JSONDecodeError, KeyError, ValueError) as e:
        KafkaDeadLetter.objects.create(
            topic=msg.topic(),
            partition=msg.partition(),
            offset=msg.offset(),
            raw_message=raw.decode("utf-8", errors="replace")[:10000],
            error_message=str(e),
        )
        return None
```

> **原则**：单条消息解析失败不影响整个 batch，写入死信表后继续消费。
> 死信表供运维排查 DeepStream 消息格式变更或版本不兼容问题。
> `@timestamp` 解析失败（`parse_datetime` 返回 `None`）也按坏消息处理，进入死信，不允许带 `NULL detected_at` 进入 batch。
> `_flush_detections` 需包裹 `transaction.atomic()`，确保批量写入失败时整批回滚，与"整批成功才 commit"语义一致。

### bulk_create 失败时的 commit 语义

`_flush_detections` 中 `bulk_create` 可能因约束冲突、磁盘满、连接断开等原因抛异常。
`run()` 循环中 `consumer.commit()` 紧跟 `_flush_detections()` 之后——
如果 flush 抛异常，commit 不会执行，整批不提交 offset。

**语义决策**：
- **整批成功才 commit**：`bulk_create` 在 `transaction.atomic()` 内，要么全写入要么全回滚，不存在"部分成功"
- **失败不 clear buffer**：保留当前 batch，下一个 flush 周期重试同一批
- **连续失败熔断**：连续 N 次 flush 失败后主动崩溃退出，由 Docker `restart: always` 重启；重启后从未 commit 的 offset 重新消费，配合 at-least-once 语义恢复

### 消息格式

**检测消息**（topic: `deepstream-detections`，来自 nvmsgbroker，含 nvdsanalytics 分析结果）：

```json
{
  "messageid": "uuid",
  "mdsversion": "1.0",
  "@timestamp": "2026-04-05T10:30:00.000Z",
  "sensorId": "cam_001",
  "analytics": {
    "overcrowding": {"roi_name": "entrance", "count": 7, "threshold": 5, "triggered": true},
    "lineCrossing": [{"name": "Entry", "in": 23, "out": 18}]
  },
  "objects": [
    {"id": "1", "type": "person", "confidence": 0.92,
     "bbox": {"topleftx": 100, "toplefty": 200, "bottomrightx": 300, "bottomrighty": 400},
     "analytics": {"roiStatus": ["entrance"], "direction": "South"}},
    {"id": "2", "type": "car", "confidence": 0.87,
     "bbox": {"topleftx": 400, "toplefty": 300, "bottomrightx": 600, "bottomrighty": 500},
     "classifier": [{"type": "vehicle_type", "label": "SUV", "confidence": 0.91}]}
  ]
}
```

字段映射（Detection）：

| Kafka 消息字段 | → Detection Model 字段 |
|---------------|----------------------|
| `sensorId` | → `camera`（通过 `_get_camera()` 缓存查找） |
| `@timestamp` | → `detected_at` |
| `objects` | → `objects` (JSONField，含 SGIE classifier 和 nvdsanalytics 目标级分析) |
| `analytics` | → `analytics` (JSONField，帧级分析结果：越线/拥挤) |
| `len(objects)` | → `object_count` |

> **扩展预留 — 动作识别消息**（初版不实现）：
> 后续版本 DeepStream 管道新增 SlowFast 等动作识别后，在此处添加
> `deepstream-actions` topic 的消息解析和 `ActionDetection` 入库逻辑。

### 事件消息消费（`deepstream-events` topic）

除了 `deepstream-detections` 的检测消息外，后端还需消费 `deepstream-events` topic：

| 事件类型 | 来源 | 后端处理 |
|---------|------|---------|
| 录制完成（`recording_done`） | SmartRecordConfig 原生 Kafka 通知 | 更新录像记录、通知前端 |
| 截图完成（`screenshot_done`） | ScreenshotRetriever Python Producer | 更新截图记录、通知前端 |
| 录制错误（`recording_error`） | DeepStream Python Producer | 记录错误日志、通知运维 |

事件消费可与 `DetectionConsumer` 共用进程（通过 `msg.topic()` 路由），
或独立为轻量 consumer（事件频率远低于检测消息）。

---

## 8. 报警规则引擎

### 设计

```python
from datetime import timedelta

from django.utils.timezone import now

class AlertEngine:
    """规则引擎：内存缓存冷却状态，避免热路径上的 DB 查询。
    初版支持检测规则和分析规则（nvdsanalytics）。
    """

    DETECTION_RULE_TYPES = {"object_count", "object_type"}
    ANALYTICS_RULE_TYPES = {"zone_intrusion", "line_crossing", "overcrowding"}
    # 扩展预留（初版不实现，与 DeepStream SGIE/Action 对齐）：
    # CLASSIFIER_RULE_TYPES = {"classifier_match"}
    # ACTION_RULE_TYPES = {"action_detected"}

    def __init__(self):
        self._last_triggered = {}   # (rule_id, camera_id) → datetime
        self._cache_ttl_seconds = 86400

    def evaluate_detection(self, detection, active_rules):
        """评估 Detection（PGIE + nvdsanalytics）触发的规则。
        检测规则和分析规则都从同一条 Detection 消息中读取数据。
        """
        rule_types = self.DETECTION_RULE_TYPES | self.ANALYTICS_RULE_TYPES
        rules = [r for r in active_rules if r.rule_type in rule_types]
        return self._evaluate(detection, rules)

    def _evaluate(self, record, rules):
        triggered_alerts = []
        for rule in rules:
            if not self._camera_matches(record.camera, rule):
                continue
            if not self._cooldown_passed(rule, record.camera):
                continue
            if self._conditions_match(record, rule):
                alert = self._create_alert(record, rule)
                triggered_alerts.append(alert)
                self._last_triggered[(rule.id, record.camera_id)] = now()
        return triggered_alerts

    def _cooldown_passed(self, rule, camera):
        """内存缓存冷却判断，O(1) 查找，不查数据库。"""
        key = (rule.id, camera.id)
        last_time = self._last_triggered.get(key)
        if not last_time:
            return True
        elapsed = (now() - last_time).total_seconds()
        return elapsed >= rule.cooldown_seconds

    def _prune_cooldown_cache(self):
        """定期清理过旧 key，避免内存字典无限增长。"""
        threshold = now() - timedelta(seconds=self._cache_ttl_seconds)
        self._last_triggered = {
            key: ts for key, ts in self._last_triggered.items() if ts >= threshold
        }
```

> **为什么不能每次查数据库**：原版 `_cooldown_passed` 对每条检测 × 每条规则执行
> `Alert.objects.filter(...).order_by(...).first()`。16 路摄像头 × 5 条规则 × 1 FPS
> = 每秒 80 次 DB 查询，全部在 Kafka Consumer 热路径上。
> 改为内存字典后变为 O(1) 字典查找。进程重启时缓存冷启动，最多多触发一次报警（可接受）。
>
> **多实例注意**：`_last_triggered` 是进程内字典，单 consumer 进程下合理。
> 若按踩坑清单 #4 扩展为多 partition + 多 consumer 实例，每个实例各持一份字典，
> 同一 `(rule_id, camera_id)` 可能在多个实例各触发一次。届时有两条迁移路径：
> 1. **分区亲和**（推荐初期）：Kafka topic 按 `sensorId`（即 `camera_id`）做 key，
>    同一摄像头的消息始终落到同一 partition → 同一 consumer 实例，内存冷却仍然有效。
> 2. **Redis 冷却**（多实例 + 无分区保证时）：`_cooldown_passed` 改用
>    `Redis SET NX EX` 做分布式锁式冷却，代价是每次规则匹配多一次 Redis RTT（~0.5ms）。
>
> 初版单 consumer 不需改动；扩展时优先走路径 1。
>
> **缓存清理建议**：在 consumer 主循环每 N 轮调用一次 `_prune_cooldown_cache()`（如每 1000 条消息或每 5 分钟），
> 并在规则删除/禁用后按 `rule_id` 清理对应 key，避免 `_last_triggered` 随历史规则无限增长。

### 支持的规则类型（初版）

| rule_type | conditions 示例 | 含义 | 数据来源 |
|-----------|----------------|------|---------|
| `object_count` | `{"min_count": 5}` | 单帧检测对象超过阈值 | 当前 Detection 记录的 objects 字段（PGIE 检测结果） |
| `object_type` | `{"object_type": "person", "min_count": 1}` | 检测到指定类型 | 当前 Detection 记录的 objects 字段（PGIE 检测结果） |
| `zone_intrusion` | `{"zone_name": "entrance", "object_type": "person"}` | 目标进入指定区域 | Detection.**objects**[].analytics.roiStatus（**目标级**，nvdsanalytics） |
| `line_crossing` | `{"line_name": "Entry", "min_count": 10}` | 越线计数超阈值 | Detection.**analytics**.lineCrossing（**帧级**，nvdsanalytics） |
| `overcrowding` | `{"zone_name": "entrance"}` | 区域拥挤（nvdsanalytics 判定） | Detection.**analytics**.overcrowding（**帧级**，nvdsanalytics） |

> **扩展预留**（初版不实现，与 DeepStream SGIE/Action 对齐）：
> | `classifier_match` | `{"classifier_type": "vehicle_type", "label": "truck"}` | SGIE 分类匹配 | Detection.objects[].classifier (SGIE) |
> | `action_detected` | `{"action_label": "fighting", "min_confidence": 0.7}` | 检测到指定动作 | ActionDetection |

> **zone_intrusion / line_crossing / overcrowding** 的计算由 DeepStream 的 `nvdsanalytics` 插件在管道内完成，
> 后端只需读取 Kafka 消息中的 `analytics` 字段即可，**不做几何运算**。
> 这确保了全帧率（30 FPS）分析精度，且后端无额外 CPU 负载。

### 报警触发后

1. 写入 `Alert` 表
2. 通过 `channel_layer.group_send(f"alerts_{org_id}", ...)` 推送到对应组织的前端 WebSocket
3. 通过 Celery 异步发送通知（邮件/Webhook）

---

## 9. WebSocket 实时推送

### WebSocket 端点

| 路由 | group 名模板 | 推送时机 |
|------|-------------|---------|
| `/ws/detections/` | `detections_{organization_id}` | Kafka Consumer flush 检测结果后 |
| `/ws/cameras/status/` | `camera_status_{organization_id}` | Camera.status 变更时 |
| `/ws/alerts/` | `alerts_{organization_id}` | 报警触发时 |

> **多租户隔离**：group 名包含 `organization_id`，与 REST 端的 `OrganizationFilterMixin` 对齐。
> Consumer connect 时从 `scope["user"].organization_id` 构造 group 名并加入；
> Kafka Consumer / Celery 任务推送时通过 `camera.organization_id` 路由到对应 group。
> 这确保 Org A 用户只收到 Org A 的数据推送，不会跨租户泄漏。
>
> **扩展预留**：后续版本新增 `/ws/actions/` 端点，推送动作识别实时结果。

### WebSocket JWT 认证方式

浏览器 WebSocket API 不支持自定义 HTTP Header，JWT 通过 **query parameter** 传递：

```
ws://host:8000/ws/detections/?token=<access_token>
```

`websocket/middleware.py` 中的认证中间件从 query string 提取 token 并验证：

```python
from channels.db import database_sync_to_async
from rest_framework_simplejwt.exceptions import TokenError
from rest_framework_simplejwt.tokens import AccessToken

from apps.accounts.models import User

class JWTAuthMiddleware:
    def __init__(self, app):
        self.app = app

    async def __call__(self, scope, receive, send):
        query_string = scope.get("query_string", b"").decode()
        token_str = dict(
            pair.split("=", 1) for pair in query_string.split("&") if "=" in pair
        ).get("token")
        if not token_str:
            await send({"type": "websocket.close", "code": 4001})
            return
        user = await self._get_user(token_str)
        if user is None:
            await send({"type": "websocket.close", "code": 4001})
            return
        scope["user"] = user
        await self.app(scope, receive, send)

    @database_sync_to_async
    def _get_user(self, token_str):
        try:
            token = AccessToken(token_str)
            return User.objects.get(id=token["user_id"])
        except (TokenError, User.DoesNotExist, KeyError, ValueError):
            return None
```

> **安全说明**：token 在 URL 中可能被日志记录。生产环境确保 Nginx/反向代理不记录 query string，
> 或在 Nginx 层剥离 token 后通过 header 转发给 ASGI。
>
> **生产加固（可选）— WS Ticket 方案**：如安全基线要求更高，可用短期一次性 ticket 替代
> 长期 JWT 出现在 URL 中。流程：前端先调 `POST /api/v1/auth/ws-ticket/` 获取 ticket
>（后端生成随机 token，`Redis SET NX EX 30` 存储，有效期 30 秒，一次使用后即删除），
> 再以 `ws://host/ws/detections/?ticket=<ticket>` 握手。中间件从 Redis 取出并删除 ticket，
> 验证通过后按正常流程设置 `scope["user"]`。此方案消除了长期 JWT 在 URL 中的泄漏面
>（浏览器历史、Referer、服务端访问日志），代价是多一次 HTTP 往返 + 一次 Redis 操作。
> 初版使用 query string JWT + Nginx 不记录即可，后续按安全评审结论决定是否迁移。

### 消息流

```
Kafka Consumer flush
  → 按 camera.organization_id 分组
  → channel_layer.group_send(f"detections_{org_id}", {type, data})
  → DetectionConsumer.detection_new(event)
  → self.send_json(event["data"])
  → 前端 WebSocket onmessage（仅收到本组织数据）
```

### 数据量控制

检测结果高频（每秒多次），前端不需要逐帧数据。策略：

- WebSocket 推送**聚合摘要**（每 N 秒一次），不推送逐条 Detection
- 或只推送有对象的帧，空帧不推送
- 具体策略由 Kafka Consumer 在 flush 时控制

---

## 10. 认证与权限

### JWT 配置

```python
SIMPLE_JWT = {
    "ACCESS_TOKEN_LIFETIME": timedelta(minutes=30),
    "REFRESH_TOKEN_LIFETIME": timedelta(days=7),
    "ROTATE_REFRESH_TOKENS": True,
    "BLACKLIST_AFTER_ROTATION": True,
}
```

### API 限流

```python
REST_FRAMEWORK = {
    "DEFAULT_THROTTLE_CLASSES": [
        "rest_framework.throttling.AnonRateThrottle",
        "rest_framework.throttling.UserRateThrottle",
    ],
    "DEFAULT_THROTTLE_RATES": {
        "anon": "20/minute",       # 未认证（含登录接口）
        "user": "200/minute",      # 已认证用户
        "login": "5/minute",       # 登录接口独立限流，防暴力破解
    },
}
```

> **`login` scope 生效条件**：`DEFAULT_THROTTLE_RATES` 中的 `"login"` 是一个自定义 scope，
> 仅靠全局 `AnonRateThrottle` / `UserRateThrottle` **不会**自动应用。
> 登录视图必须显式挂载 `ScopedRateThrottle` 并设置 `throttle_scope`：
>
> ```python
> class LoginView(TokenObtainPairView):
>     throttle_classes = [ScopedRateThrottle]
>     throttle_scope = "login"
> ```
>
> 否则登录接口只受 `anon` 限流（20/min），无法达到 5/min 的暴力破解防护目标。

### 权限层级

| 角色 | 摄像头管理 | 规则配置 | 查看检测/报警 | 用户管理 |
|------|----------|---------|-------------|---------|
| admin | ✅ | ✅ | ✅ | ✅ |
| operator | ✅ | ✅ | ✅ | ❌ |
| viewer | ❌ | ❌ | ✅ | ❌ |

### 数据隔离

同一组织（Organization）下的用户只能看到自己组织的数据。通过自定义 permission + queryset 过滤：

```python
class OrganizationFilterMixin:
    """所有涉及多租户数据的 ViewSet 必须混入此 Mixin。

    安全警告：遗漏此 Mixin 会导致跨租户数据泄漏。
    在 Code Review 中将此作为必检项。
    """

    def get_queryset(self):
        return super().get_queryset().filter(
            organization=self.request.user.organization
        )
```

---

## 11. Celery 异步任务

### 任务列表

| 任务 | 触发方式 | 说明 |
|------|---------|------|
| `send_alert_notification` | 报警触发时 | 发送邮件/Webhook 通知 |
| `cleanup_old_detections` | Celery Beat 每日 | DROP 超过 `DETECTION_RETENTION_MONTHS` 的 Detection 分区（整月粒度） |
| `cleanup_dead_letters` | Celery Beat 每日 | 删除超过 `DEAD_LETTER_RETENTION_DAYS` 的 KafkaDeadLetter 记录 |
| `create_next_partition` | Celery Beat 每月25日 | 创建下月 Detection 分区 |
| `sync_camera_status` | Celery Beat 每分钟 | 向 DeepStream 查询流状态，同步到数据库（通过 `async_to_sync` 桥接 httpx） |
| `generate_daily_report` | Celery Beat 每日 | 生成每日检测统计（可选） |

### Redis DB 分配约定

`REDIS_URL` 环境变量不带 DB 编号（如 `redis://redis:6379`），各组件在代码中追加自己的 DB：

| Redis DB | 用途 | 配置位置 |
|----------|------|---------|
| `/0` | Django Cache（`django.core.cache`） | `base.py` CACHES |
| `/1` | Celery Broker | `base.py` CELERY_BROKER_URL |
| `/2` | Celery Result Backend | `base.py` CELERY_RESULT_BACKEND |
| `/3` | Django Channels Layer | `base.py` CHANNEL_LAYERS |

> docker-compose 环境变量只需传 `REDIS_URL=redis://redis:6379`（不带 `/N`），
> settings 中各组件各自追加 DB 编号。避免"一处改 URL 忘改其他"的漂移问题。

### Celery 配置

```python
from celery.schedules import crontab

# CELERY_BROKER_URL / CELERY_RESULT_BACKEND 已在 base.py 中通过 _redis_url 拼接（见第 13 节）
CELERY_RESULT_EXPIRES = 3600             # 结果 1 小时后过期，不让 Redis 无限膨胀
CELERY_TASK_ACKS_LATE = True             # 任务执行完才 ack，worker 崩溃时自动重投
CELERY_TASK_REJECT_ON_WORKER_LOST = True # worker 被 kill 时拒绝任务（触发重投）

CELERY_BEAT_SCHEDULE = {
    "cleanup-detections": {
        "task": "tasks.maintenance.cleanup_old_detections",
        "schedule": crontab(hour=3, minute=0),
    },
    "create-next-partition": {
        "task": "tasks.maintenance.create_next_partition",
        "schedule": crontab(day_of_month=25, hour=2, minute=0),
    },
    "sync-camera-status": {
        "task": "tasks.maintenance.sync_camera_status",
        "schedule": 60.0,
    },
    "cleanup-dead-letters": {
        "task": "tasks.maintenance.cleanup_dead_letters",
        "schedule": crontab(hour=3, minute=30),
    },
}
```

### 任务重试策略

```python
from celery import shared_task

@shared_task(
    bind=True,
    max_retries=3,
    default_retry_delay=60,
    retry_backoff=True,          # 指数退避：60s → 120s → 240s
    retry_jitter=True,           # 添加随机抖动，避免重试风暴
)
def send_alert_notification(self, alert_id):
    ...
```

> **录像磁盘管理已移至 DeepStream 端**：DeepStream 的 `DiskGuard` 守护线程以行车记录仪模型
> 自主管理磁盘（`rolling/` 按使用率循环覆盖，`locked/` 按超龄清理）。
> 后端**不参与录像文件清理**，仅通过 Kafka 事件消费录像完成通知并写入数据库做元数据记录。
> 详见 DeepStream plan 的「磁盘空间管理（行车记录仪模型）」章节。

---

## 12. Docker 容器化

### 设计原则

- 后端有**独立的 Dockerfile**，构建上下文为 `backend/` 目录
- `docker build ./backend` 即可独立构建，不依赖项目根目录
- Dockerfile 含四个 target：`builder`（编译依赖）、`test`（跑测试）、`dev`（本地热重载开发）、`production`（生产运行）
- 后端有独立的 `docker-compose.dev.yml`，只含后端 + 基础设施（无 DeepStream）

### backend/Dockerfile（多阶段：base → build → test → dev → production）

```dockerfile
# ---- Base Stage（国内镜像，所有阶段共用） ----
FROM python:3.12-slim AS base
RUN sed -i 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list.d/debian.sources
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

# ---- Build Stage ----
FROM base AS builder

RUN apt-get update && apt-get install -y \
    libpq-dev gcc \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt /tmp/
RUN pip install --no-cache-dir --prefix=/install -r /tmp/requirements.txt

# ---- Test Stage ----
FROM base AS test

RUN apt-get update && apt-get install -y \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /install /usr/local
WORKDIR /app
COPY . .

CMD ["pytest", "--tb=short", "-q"]

# ---- Dev Stage ----
FROM base AS dev
RUN apt-get update && apt-get install -y \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /install /usr/local
WORKDIR /app
COPY . .
CMD ["uvicorn", "config.asgi:application", "--reload", "--host", "0.0.0.0", "--port", "8000"]

# ---- Production Stage ----
FROM base AS production

RUN apt-get update && apt-get install -y \
    libpq5 curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /install /usr/local

RUN addgroup --system app && adduser --system --ingroup app app
WORKDIR /app
COPY . .
RUN chown -R app:app /app
USER app

EXPOSE 8000

HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD curl -f http://localhost:8000/api/v1/health/ready/ || exit 1

CMD ["gunicorn", "config.asgi:application", \
     "-k", "uvicorn.workers.UvicornWorker", \
     "-b", "0.0.0.0:8000", \
     "--workers", "4", \
     "--access-logfile", "-"]
```

> **健康检查边界**：上述 `HEALTHCHECK` 仅适用于 `backend` API 容器。
> `celery-worker` / `celery-beat` / `kafka-consumer` 不暴露 HTTP 8000，不能复用该检查。
> 在 compose 中需对这些容器关闭 Dockerfile 继承的 healthcheck（`healthcheck: { disable: true }`）
> 或覆盖为各自进程级检查命令。

### 独立构建 & 测试命令

```bash
# 独立构建生产镜像（默认 target = 最后一个 stage = production）
docker build -t ai-stream-backend ./backend

# 独立构建并运行测试（target = test）
docker build --target test -t ai-stream-backend-test ./backend
docker run --rm \
    -e DATABASE_URL=postgres://user:pass@host:5432/test_db \
    -e REDIS_URL=redis://host:6379 \
    -e DJANGO_SETTINGS_MODULE=config.settings.development \
    ai-stream-backend-test

# 或一行搞定（CI 用）
docker build --target test -t ai-stream-backend-test ./backend \
    && docker run --rm --network backend_default \
       -e DATABASE_URL=postgres://user:pass@postgres:5432/test_db \
       -e REDIS_URL=redis://redis:6379 \
       ai-stream-backend-test
```

### backend/docker-compose.dev.yml（独立开发/测试）

```yaml
# 后端独立开发环境，无需 DeepStream
# 用法：cd backend && docker compose -f docker-compose.dev.yml up

x-backend-env: &backend-env
  DJANGO_SETTINGS_MODULE: config.settings.development
  DATABASE_URL: postgres://postgres:postgres@postgres:5432/ai_stream
  REDIS_URL: redis://redis:6379
  HEALTH_CHECK_KAFKA: "false"          # API / Worker / Beat 不依赖 Kafka readiness
  DEEPSTREAM_MOCK: "true"
  KAFKA_BOOTSTRAP_SERVERS: kafka:9092
  KAFKA_DETECTION_TOPIC: deepstream-detections
  KAFKA_EVENT_TOPIC: deepstream-events
  KAFKA_COMMAND_TOPIC: deepstream-commands
  SECRET_KEY: dev-insecure-secret-key

services:
  # ---- 基础设施 ----
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: ai_stream
      POSTGRES_PASSWORD: postgres
    ports:
      - "5432:5432"
    volumes:
      - pgdata:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 5s
      retries: 5

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    healthcheck:
      test: ["CMD", "redis-cli", "ping"]
      interval: 5s
      retries: 5

  kafka:
    image: bitnami/kafka:3.7
    environment:
      KAFKA_CFG_NODE_ID: 1
      KAFKA_CFG_PROCESS_ROLES: controller,broker
      KAFKA_CFG_CONTROLLER_QUORUM_VOTERS: 1@kafka:9093
      KAFKA_CFG_LISTENERS: PLAINTEXT://:9092,CONTROLLER://:9093
      KAFKA_CFG_ADVERTISED_LISTENERS: PLAINTEXT://kafka:9092
      KAFKA_CFG_CONTROLLER_LISTENER_NAMES: CONTROLLER
      KAFKA_CFG_LISTENER_SECURITY_PROTOCOL_MAP: CONTROLLER:PLAINTEXT,PLAINTEXT:PLAINTEXT
    ports:
      - "9092:9092"

  # ---- 后端服务 ----
  backend:
    build:
      context: .
      target: dev
    ports:
      - "8000:8000"
    environment:
      <<: *backend-env
    volumes:
      - .:/app
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    restart: unless-stopped

  celery-worker:
    build:
      context: .
      target: production
    command: celery -A config worker -l info --concurrency=2
    environment:
      <<: *backend-env
    volumes:
      - screenshots:/app/screenshots  # 共享卷：截图下载 API 需访问
    depends_on:
      - backend
    restart: unless-stopped
    healthcheck:
      disable: true

  celery-beat:
    build:
      context: .
      target: production
    command: celery -A config beat -l info
    environment:
      <<: *backend-env
    depends_on:
      - backend
    restart: unless-stopped
    healthcheck:
      disable: true

  kafka-consumer:
    build:
      context: .
      target: production
    command: python manage.py run_kafka_consumer
    environment:
      <<: *backend-env
      HEALTH_CHECK_KAFKA: "true"       # Consumer 必须对 Kafka 强校验
    depends_on:
      - backend
      - kafka
    restart: always
    stop_grace_period: 30s
    healthcheck:
      disable: true

  # ---- 测试 runner（按需启动） ----
  test:
    build:
      context: .
      target: test
    environment:
      <<: *backend-env
      DATABASE_URL: postgres://postgres:postgres@postgres:5432/test_ai_stream
      REDIS_URL: redis://redis:6379
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    profiles:
      - test                           # 只在 --profile test 时启动

volumes:
  pgdata:
```

### 使用方式

```bash
# ---- 独立开发 ----
cd backend
docker compose -f docker-compose.dev.yml up           # 启动后端 + 基础设施
docker compose -f docker-compose.dev.yml up backend    # 只启动 API 服务

# ---- 独立跑测试 ----
cd backend
docker compose -f docker-compose.dev.yml --profile test run --rm test

# ---- CI 流水线 ----
cd backend
docker build --target test -t backend-test .
docker compose -f docker-compose.dev.yml up -d postgres redis
docker run --rm --network backend_default \
    -e DATABASE_URL=postgres://postgres:postgres@postgres:5432/test_db \
    backend-test
```

### 项目根目录 docker-compose.yml（全栈编排）

项目根目录的 `docker-compose.yml` 编排三端 + 所有基础设施：

```yaml
x-backend-env: &backend-env
  DJANGO_SETTINGS_MODULE: config.settings.production
  DATABASE_URL: postgres://user:pass@postgres:5432/ai_stream
  REDIS_URL: redis://redis:6379
  HEALTH_CHECK_KAFKA: "false"          # API / Worker / Beat 不依赖 Kafka readiness
  DEEPSTREAM_REST_URL: http://deepstream:9000
  KAFKA_BOOTSTRAP_SERVERS: kafka:9092
  KAFKA_DETECTION_TOPIC: deepstream-detections
  KAFKA_EVENT_TOPIC: deepstream-events
  KAFKA_COMMAND_TOPIC: deepstream-commands
  SECRET_KEY: ${SECRET_KEY}

services:
  backend:
    build:
      context: ./backend
      target: production
    ports:
      - "8000:8000"
    environment:
      <<: *backend-env
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    restart: unless-stopped

  celery-worker:
    build:
      context: ./backend
      target: production
    command: celery -A config worker -l info --concurrency=4
    environment:
      <<: *backend-env
    depends_on:
      - backend
    restart: unless-stopped
    healthcheck:
      disable: true

  celery-beat:
    build:
      context: ./backend
      target: production
    command: celery -A config beat -l info
    environment:
      <<: *backend-env
    depends_on:
      - backend
    restart: unless-stopped
    healthcheck:
      disable: true

  kafka-consumer:
    build:
      context: ./backend
      target: production
    command: python manage.py run_kafka_consumer
    environment:
      <<: *backend-env
      HEALTH_CHECK_KAFKA: "true"       # Consumer 必须对 Kafka 强校验
    depends_on:
      - backend
      - kafka
    restart: always
    stop_grace_period: 30s
    healthcheck:
      disable: true
```

> 项目根目录的 compose 里 `context: ./backend` 指向后端目录，
> 后端自己的 `docker-compose.dev.yml` 里 `context: .` 指向自身。
> **同一个 Dockerfile，两种入口，构建结果完全一致。**
> 开发 compose 的 `backend` 走 `dev` target + 代码挂载；生产 compose 走 `production` target（不可变镜像）。

### 进程清单

| 进程 | 容器 | 作用 |
|------|------|------|
| Gunicorn + Uvicorn Worker (ASGI) | backend | HTTP API + WebSocket |
| Celery Worker | celery-worker | 异步任务执行 |
| Celery Beat | celery-beat | 定时任务调度 |
| Kafka Consumer | kafka-consumer | 消费 DeepStream 检测结果 |

---

## 13. 配置管理

### 环境变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `DJANGO_SETTINGS_MODULE` | `config.settings.development` | settings 模块 |
| `SECRET_KEY` | — | Django secret key（必须设置） |
| `DATABASE_URL` | `postgres://localhost:5432/ai_stream` | PostgreSQL 连接 |
| `REDIS_URL` | `redis://localhost:6379` | Redis 连接（**不带 `/N`**，各组件在代码中追加 DB 编号，见第 11 节） |
| `DEEPSTREAM_REST_URL` | `http://localhost:9000` | DeepStream REST API 地址 |
| `DEEPSTREAM_MOCK` | `false` | 开发环境 mock DeepStream |
| `KAFKA_BOOTSTRAP_SERVERS` | `localhost:9092` | Kafka 地址 |
| `KAFKA_DETECTION_TOPIC` | `deepstream-detections` | 检测结果 Kafka topic |
| `KAFKA_EVENT_TOPIC` | `deepstream-events` | DeepStream 事件 topic（录制完成/截图完成） |
| `KAFKA_COMMAND_TOPIC` | `deepstream-commands` | 后端→DeepStream 命令 topic |
| `KAFKA_CONSUMER_GROUP` | `backend-consumer` | Kafka consumer group |
| `HEALTH_CHECK_KAFKA` | `true` | Readiness 是否检查 Kafka（API 建议设 `false`，仅 kafka-consumer 设 `true`） |
| `DETECTION_RETENTION_MONTHS` | `1` | Detection 分区保留月数（按月分区时建议使用该参数） |
| `ACCESS_TOKEN_LIFETIME_MINUTES` | `30` | JWT access token 有效期 |
| `DEAD_LETTER_RETENTION_DAYS` | `90` | Kafka 死信记录保留天数（排障用，应长于检测数据；死信量极小，多留无存储压力） |

### 环境变量解析

使用 `django-environ` 解析，支持类型转换和默认值：

```python
import environ

env = environ.Env(
    DEBUG=(bool, False),
    DEEPSTREAM_MOCK=(bool, False),
    ACCESS_TOKEN_LIFETIME_MINUTES=(int, 30),
    DETECTION_RETENTION_MONTHS=(int, 1),
    DEAD_LETTER_RETENTION_DAYS=(int, 90),
)

DATABASES = {"default": env.db("DATABASE_URL")}
DATABASES["default"]["CONN_MAX_AGE"] = env.int("DB_CONN_MAX_AGE", default=600)

_redis_url = env("REDIS_URL")   # redis://redis:6379（不带 /N）
CACHES = {"default": env.cache_url_config(f"{_redis_url}/0")}
CELERY_BROKER_URL = f"{_redis_url}/1"
CELERY_RESULT_BACKEND = f"{_redis_url}/2"

CHANNEL_LAYERS = {
    "default": {
        "BACKEND": "channels_redis.core.RedisChannelLayer",
        "CONFIG": {
            "hosts": [f"{_redis_url}/3"],
        },
    },
}
```

> **数据库连接池策略**：`CONN_MAX_AGE=600`（10 分钟）复用 TCP 连接。
> 4 Gunicorn worker + Celery worker + Celery Beat + Kafka Consumer = 至少 7 个进程同时连 PostgreSQL。
> Django 默认 `CONN_MAX_AGE=0`（每次请求开关连接），在 Detection 高频写入下连接开销不可忽视。
> 如需更精细控制（如连接数上限），后续可引入 pgbouncer 作为连接池代理。

### Settings 分离

```
config/settings/
├── base.py          # INSTALLED_APPS, MIDDLEWARE, REST_FRAMEWORK, CHANNEL_LAYERS 等公共配置
├── development.py   # DEBUG=True, CORS 全开, 控制台日志, uvicorn --reload
└── production.py    # DEBUG=False, ALLOWED_HOSTS, 安全中间件, JSON 文件日志
```

---

## 14. 错误处理

### 自定义异常体系（业务码 ≠ HTTP 状态码）

```python
class ServiceError(Exception):
    """业务异常基类。

    code 是业务错误码（字符串），前端用于精确匹配错误类型。
    http_status 是 HTTP 状态码。二者独立。
    """

    def __init__(self, message, code="UNKNOWN_ERROR", http_status=400):
        self.message = message
        self.code = code
        self.http_status = http_status

class DeepStreamUnavailableError(ServiceError):
    def __init__(self):
        super().__init__(
            "DeepStream 服务不可用",
            code="DEEPSTREAM_UNAVAILABLE",
            http_status=503,
        )

class CameraNotFoundError(ServiceError):
    def __init__(self, camera_id):
        super().__init__(
            f"摄像头 {camera_id} 不存在",
            code="CAMERA_NOT_FOUND",
            http_status=404,
        )
```

> **为什么分离业务码和 HTTP 状态码**：
> 原版 `code=400` 与 HTTP status 重复，前端无法区分"参数错误"和"摄像头不存在"。
> 业务码用字符串（如 `CAMERA_NOT_FOUND`），前端用 switch-case 精确处理。

### DRF 全局异常处理器

```python
def custom_exception_handler(exc, context):
    status_code_to_code = {
        400: "VALIDATION_ERROR",
        401: "UNAUTHORIZED",
        403: "FORBIDDEN",
        404: "NOT_FOUND",
        429: "THROTTLED",
        503: "SERVICE_UNAVAILABLE",
    }

    if isinstance(exc, ServiceError):
        return Response(
            {"code": exc.code, "message": exc.message, "data": None},
            status=exc.http_status,
        )
    response = exception_handler(exc, context)
    if response:
        return Response(
            {"code": status_code_to_code.get(response.status_code, "SERVER_ERROR"),
             "message": str(exc),
             "data": response.data if response.status_code in (400, 401, 403, 404, 429) else None},
            status=response.status_code,
        )
    return None
```

---

## 15. 日志与可观测性

### 结构化日志

使用 `structlog` 输出 JSON 格式日志，每条日志携带 `request_id`：

```python
import structlog

structlog.configure(
    processors=[
        structlog.contextvars.merge_contextvars,
        structlog.stdlib.add_log_level,
        structlog.processors.TimeStamper(fmt="iso"),
        structlog.processors.JSONRenderer(),        # 生产用 JSON，开发用 ConsoleRenderer
    ],
    wrapper_class=structlog.stdlib.BoundLogger,
    logger_factory=structlog.stdlib.LoggerFactory(),
)
```

### Request ID 中间件

```python
import uuid
import structlog
from structlog.contextvars import clear_contextvars

class RequestIDMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        request_id = request.headers.get("X-Request-ID", str(uuid.uuid4()))
        request.request_id = request_id
        structlog.contextvars.bind_contextvars(request_id=request_id)
        try:
            response = self.get_response(request)
            response["X-Request-ID"] = request_id
            return response
        finally:
            clear_contextvars()
```

> **用途**：前端/运维拿到 `X-Request-ID` 可在日志系统中快速定位整条请求链路。
> DeepStream 代理调用时将 request_id 传递到 httpx header，实现跨服务追踪。

### 健康检查端点（Liveness / Readiness 分离）

K8s 和负载均衡器的高频探针（每 5-10 秒）如果每次都探测 DB + Redis + Kafka，会放大负载。
拆分为两个端点：

| 端点 | 用途 | 检查内容 | 频率 |
|------|------|---------|------|
| `/api/v1/health/live/` | **Liveness**（进程存活） | 固定返回 200 | 高频（K8s livenessProbe） |
| `/api/v1/health/ready/` | **Readiness**（服务就绪） | DB + Redis + Kafka 连通性 | 低频（K8s readinessProbe / 负载均衡） |

> backend API 容器的 Docker HEALTHCHECK 指向 `/api/v1/health/ready/`（每 30 秒一次，可接受）。
> 如果上 K8s，livenessProbe 指向 `/live/`，readinessProbe 指向 `/ready/`。
> Kafka 检查可通过环境变量 `HEALTH_CHECK_KAFKA=true/false` 控制是否启用。
>
> **生产编排策略**：`backend`（API + WebSocket）容器设 `HEALTH_CHECK_KAFKA=false`，
> `kafka-consumer` 容器设 `HEALTH_CHECK_KAFKA=true`。
> 理由：API 容器不直接依赖 Kafka，Kafka 短暂不可用时 REST API 和 WebSocket 仍可正常服务；
> 若 API 的 readiness 包含 Kafka 检查，Kafka 故障会导致 API 返回 503 / 被编排器摘除，
> 误杀本可正常工作的 API Pod。仅 kafka-consumer 容器需要 Kafka 强校验。
> 开发环境可统一设为 `false` 简化启动。

```python
class LivenessView(APIView):
    permission_classes = [AllowAny]
    throttle_classes = []

    def get(self, request):
        return Response({"status": "alive"})


class ReadinessView(APIView):
    permission_classes = [AllowAny]
    throttle_classes = []

    def get(self, request):
        checks = {
            "database": self._check_db(),
            "redis": self._check_redis(),
        }
        if env.bool("HEALTH_CHECK_KAFKA", default=True):
            checks["kafka"] = self._check_kafka()
        healthy = all(checks.values())
        return Response(
            {"status": "healthy" if healthy else "degraded", "checks": checks},
            status=200 if healthy else 503,
        )
```

---

## 16. 测试策略

### 工具链

| 工具 | 用途 |
|------|------|
| pytest + pytest-django | 测试运行器 |
| factory_boy | Model 工厂，替代 fixtures |
| pytest-asyncio | 仅用于 **Channels WebSocket Consumer** 和少数 async 路径（如直接测试 `DeepStreamClient` 的 async 方法）；DRF API View 以同步为主（见第 2 节），测试用标准 pytest-django 的 `APIClient` |
| respx | mock httpx 请求（测试 DeepStreamClient） |
| pytest-cov | 覆盖率报告 |

### 测试分层

| 层级 | 测试内容 | 数量占比 |
|------|---------|---------|
| **单元测试** | AlertEngine 规则匹配、Service 层逻辑、序列化器 | ~60% |
| **API 测试** | DRF ViewSet 端到端（含认证、权限、数据隔离） | ~30% |
| **集成测试** | Kafka Consumer 消费链路、WebSocket 推送 | ~10% |

### 测试场景清单

#### 单元测试

| 模块 | 场景 | 验证 |
|------|------|------|
| AlertEngine | 规则条件匹配 | `object_count >= 5` 触发，`< 5` 不触发 |
| AlertEngine | 冷却机制 | 冷却期内同一规则+摄像头不重复触发 |
| AlertEngine | 冷启动 | 进程重启后缓存为空，首次检测正常触发 |
| Kafka 消息解析 | 正常消息 | 正确映射为 Detection 字段 |
| Kafka 消息解析 | 畸形 JSON | 不崩溃，返回 None 或进死信 |
| Kafka 消息解析 | 缺失字段 | sensorId 不存在时跳过，不抛异常 |
| Serializer | Camera 必填字段 | rtsp_url / name 缺失 → 400 |
| Serializer | 枚举字段 | status 传入非法值 → 400 |
| ServiceError | 业务码分离 | `code="CAMERA_NOT_FOUND"`, `http_status=404` 各自独立 |

#### API 测试

| 模块 | 场景 | 验证 |
|------|------|------|
| 认证 | 无 token | 返回 401 |
| 认证 | 过期 token | 返回 401 |
| 认证 | refresh 换 token | 返回新 access token，旧 refresh 加入黑名单 |
| 权限 | viewer 创建摄像头 | 返回 403 |
| 权限 | operator 管理用户 | 返回 403 |
| 多租户隔离 | Org A 用户查 Org B 摄像头 | 列表为空，detail 返回 404 |
| 多租户隔离 | Org A 用户操作 Org B 报警 | acknowledge 返回 404 |
| Camera CRUD | 创建 → 列表 → 改名 → 软删除 | 每步状态正确，删后列表不可见 |
| Camera start-stream | DeepStream 正常 | 返回 200，camera.status → connecting |
| Camera start-stream | DeepStream 不可达 | 返回 503，code=DEEPSTREAM_UNAVAILABLE，camera.status 不变 |
| Camera start-stream | 已在线的摄像头再次启动 | **幂等：返回 200 + 当前状态**，不重复调 DeepStream |
| Alert 流转 | pending → acknowledge → resolve | 每步校验状态、操作人、时间戳 |
| Alert 流转 | 非 pending 状态再 acknowledge | 返回 400 |
| Detection 列表 | CursorPagination | 前后翻页结果正确，无重复无遗漏 |
| Detection 过滤 | camera_id + 时间范围 | 只返回匹配的记录 |
| 限流 | 登录接口 6 次/分钟 | 第 6 次返回 429 |
| 健康检查 | DB + Redis 正常 | 返回 200，status=healthy |
| 健康检查 | Redis 不可达 | 返回 503，status=degraded，checks.redis=false |

#### 管道配置测试

| 模块 | 场景 | 验证 |
|------|------|------|
| AIModel CRUD | 创建 detector 类型模型 | config 按 detector schema 校验通过 |
| AIModel CRUD | model_type 与 config schema 不匹配 | 返回 400，config 校验失败 |
| PipelineProfile | 创建管道配置 | detector 必须是 detector 类型，tracker 必须是 tracker 类型 |
| PipelineProfile | detector 关联 tracker 类型模型 | 返回 400 |
| CameraModelBinding | 绑定摄像头到管道 | 绑定成功，查询摄像头返回关联的 pipeline |
| Deploy | 部署管道配置 | 生成正确的 PGIE YAML + tracker YAML + analytics INI，触发重启 |
| AnalyticsZone CRUD | 创建 ROI 分析区域 | coordinates 校验通过，zone_type 正确 |
| AnalyticsZone CRUD | 坐标超范围 (>1920 或 <0) | 返回 400 |
| AnalyticsZone | overcrowding zone 无 object_threshold | config 校验返回 400 |
| Deploy (analytics) | 部署含分析区域的配置 | 生成正确的 analytics_config.txt，stream-id 按 uid 排序 |
| AlertRule | zone_intrusion 规则读 analytics 结果 | Kafka 消息含 `roiStatus` 时触发 |
| AlertRule | overcrowding 规则读 analytics 结果 | `analytics.overcrowding.triggered=true` 时触发 |
| AlertRule | line_crossing 规则 | 越线计数超阈值触发，未超不触发 |
| 多租户 | Org A 模型不可被 Org B 使用 | PipelineProfile 关联跨组织模型返回 400 |

#### 集成测试

| 链路 | 场景 | 验证 |
|------|------|------|
| Kafka → DB | 正常消息消费 | Detection 表有记录，字段映射正确（含 analytics 字段） |
| Kafka → DB | 批量入库 | 100 条消息 → 1 次 bulk_create |
| Kafka → 死信 | 畸形消息 | KafkaDeadLetter 有记录，后续消息不受影响 |
| Kafka → Alert | 检测触发规则 | Alert 表有记录，snapshot 正确 |
| Kafka → WebSocket | 检测结果推送 | WebSocket client 收到 `detection.new` 消息 |
| Kafka (analytics) → Alert | 含 overcrowding 的检测消息 | overcrowding 规则正确触发，analytics 字段正确入库 |
| Kafka (analytics) → Alert | 含 lineCrossing 的检测消息 | line_crossing 规则正确触发 |
| Kafka graceful shutdown | SIGTERM | buffer 中剩余消息 flush 入库，offset 已 commit |
| Camera 全生命周期 | 创建 → start → Kafka 状态消息 → online → stop → offline | 每步 DB 状态正确 |

#### WebSocket 测试

| 场景 | 验证 |
|------|------|
| 无 token 连接 | 返回 4001 关闭码 |
| 过期 token 连接 | 返回 4001 关闭码 |
| 正常连接 | 接受连接，加入对应 group |
| 收到 group_send | 客户端收到正确格式的 JSON `{type, data}` |
| 断开连接 | 自动移出所有 group |

### Swagger UI（开发辅助，非正式测试）

drf-spectacular 自动生成的 Swagger UI 用于：

- 开发期手动调试接口参数
- 前端查看 API 文档和交互式示例
- 验证 OpenAPI schema 是否完整

> **不依赖 Swagger 做回归测试**——改了代码不可能每次手动把所有接口点一遍。
> 回归测试由上述 pytest 自动化覆盖。

### Swagger 测试规范（必须执行的文档验收）

Swagger 不替代自动化回归，但必须作为**接口文档完整性 + 基础可用性**验收入口。

#### 测试目标

- 验证 OpenAPI schema 完整：参数、响应、错误码、鉴权标注齐全
- 验证关键端点可在 Swagger 中完成最小闭环调用（登录 → 鉴权 → 读写）
- 验证文档与真实行为一致（状态码、字段名、必填规则）

#### 最小 Swagger 测试清单

1. 文档加载：Swagger UI 可访问，`openapi.json` 可导出
2. 鉴权流程：`/auth/login/` 获取 token，`Authorize` 后调用受保护接口成功
3. 参数校验：至少抽查 5 个端点，验证必填字段/枚举校验与文档一致
4. 错误码校验：抽查 `400/401/403/404/429/503` 响应是否符合统一响应结构
5. 动作型端点：抽查 `start-stream`、`deploy`、`acknowledge` 等端点的响应与幂等描述一致
6. 多租户校验：在 Swagger 下用不同组织账号验证隔离行为（越权访问应返回 404/403）

#### 与脚本测试的关系

- Swagger 测试用于“文档质量 + 人工抽样验证”
- `backend/test/test_{api_name}.py` + `test_all.py` 用于“可重复自动回归”
- 发布前要求二者都通过：**Swagger 抽样通过 + 脚本全量通过**

### 脚本化全 API 回归测试计划（`requests + argparse`，暂不实现）

> **当前阶段约束**：本节仅定义落地规范与目录结构，**不开始实现代码**。

#### 目录与命名约定

- 所有 API 测试脚本放在 `backend/test/`
- 每个 API 一份脚本，命名统一为 `test_{api_name}.py`
- 全量入口脚本为 `backend/test/test_all.py`
- 测试数据文件放在 `backend/example_data/`

#### 单脚本统一接口约定

每个 `test_{api_name}.py` 脚本统一实现：

- 使用 `argparse` 解析参数：`--base-url`、`--username`、`--password`、`--timeout`、`--verbose`、`--strict`
- 使用 `requests.Session` 复用连接和认证头
- 输出结构化结果：`PASS` / `FAIL`、HTTP 状态码、关键断言信息
- 脚本内自生成最小必要数据（例如随机名称、时间戳），不依赖人工预置数据库
- 出错时返回非 0 退出码，便于被 `test_all.py` 汇总

#### 全量入口脚本约定（`test_all.py`）

- 自动发现 `backend/test/` 下所有 `test_*.py`（排除 `test_all.py`）
- 支持按标签或名称过滤：`--include` / `--exclude`
- 串行执行并收集结果，最后输出汇总（总数、通过、失败、耗时、失败清单）
- 默认执行全量 API 脚本；`--fail-fast` 可在首个失败时中断

#### API 脚本清单（按端点拆分）

| API 端点 | 脚本文件 |
|---|---|
| `POST /api/v1/auth/login/` | `test_auth_login.py` |
| `POST /api/v1/auth/refresh/` | `test_auth_refresh.py` |
| `GET /api/v1/auth/me/` | `test_auth_me.py` |
| `GET /api/v1/cameras/` | `test_cameras_list.py` |
| `POST /api/v1/cameras/` | `test_cameras_create.py` |
| `GET /api/v1/cameras/{id}/` | `test_cameras_detail.py` |
| `PATCH /api/v1/cameras/{id}/` | `test_cameras_update.py` |
| `DELETE /api/v1/cameras/{id}/` | `test_cameras_delete.py` |
| `POST /api/v1/cameras/{id}/start-stream/` | `test_cameras_start_stream.py` |
| `POST /api/v1/cameras/{id}/stop-stream/` | `test_cameras_stop_stream.py` |
| `GET /api/v1/camera-groups/` | `test_camera_groups_list.py` |
| `POST /api/v1/camera-groups/` | `test_camera_groups_create.py` |
| `GET /api/v1/detections/` | `test_detections_list.py` |
| `GET /api/v1/detections/stats/` | `test_detections_stats.py` |
| `GET /api/v1/alert-rules/` | `test_alert_rules_list.py` |
| `POST /api/v1/alert-rules/` | `test_alert_rules_create.py` |
| `PATCH /api/v1/alert-rules/{id}/` | `test_alert_rules_update.py` |
| `DELETE /api/v1/alert-rules/{id}/` | `test_alert_rules_delete.py` |
| `GET /api/v1/alerts/` | `test_alerts_list.py` |
| `POST /api/v1/alerts/{id}/acknowledge/` | `test_alerts_acknowledge.py` |
| `POST /api/v1/alerts/{id}/resolve/` | `test_alerts_resolve.py` |
| `GET /api/v1/ai-models/` | `test_ai_models_list.py` |
| `POST /api/v1/ai-models/` | `test_ai_models_create.py` |
| `GET /api/v1/ai-models/{id}/` | `test_ai_models_detail.py` |
| `PATCH /api/v1/ai-models/{id}/` | `test_ai_models_update.py` |
| `DELETE /api/v1/ai-models/{id}/` | `test_ai_models_delete.py` |
| `GET /api/v1/pipeline-profiles/` | `test_pipeline_profiles_list.py` |
| `POST /api/v1/pipeline-profiles/` | `test_pipeline_profiles_create.py` |
| `GET /api/v1/pipeline-profiles/{id}/` | `test_pipeline_profiles_detail.py` |
| `PATCH /api/v1/pipeline-profiles/{id}/` | `test_pipeline_profiles_update.py` |
| `DELETE /api/v1/pipeline-profiles/{id}/` | `test_pipeline_profiles_delete.py` |
| `POST /api/v1/pipeline-profiles/{id}/deploy/` | `test_pipeline_profiles_deploy.py` |
| `GET /api/v1/cameras/{id}/pipeline/` | `test_cameras_pipeline_get.py` |
| `PUT /api/v1/cameras/{id}/pipeline/` | `test_cameras_pipeline_put.py` |
| `GET /api/v1/cameras/{id}/analytics-zones/` | `test_analytics_zones_list.py` |
| `POST /api/v1/cameras/{id}/analytics-zones/` | `test_analytics_zones_create.py` |
| `PATCH /api/v1/cameras/{id}/analytics-zones/{zone_id}/` | `test_analytics_zones_update.py` |
| `DELETE /api/v1/cameras/{id}/analytics-zones/{zone_id}/` | `test_analytics_zones_delete.py` |
| `GET /api/v1/dashboard/overview/` | `test_dashboard_overview.py` |
| `GET /api/v1/dashboard/detection-trend/` | `test_dashboard_detection_trend.py` |
| `GET /api/v1/dashboard/camera-status/` | `test_dashboard_camera_status.py` |
| `GET /api/v1/health/live/` | `test_health_live.py` |
| `GET /api/v1/health/ready/` | `test_health_ready.py` |
| `GET /api/v1/deepstream/health/` | `test_deepstream_health.py` |
| `GET /api/v1/deepstream/streams/` | `test_deepstream_streams.py` |

#### `backend/example_data/` 数据规划（用于脚本输入）

| 文件 | 用途 |
|---|---|
| `users.json` | 测试账号（admin/operator/viewer）与组织信息 |
| `camera_groups.json` | 摄像头分组模板 |
| `cameras.json` | 摄像头模板（使用可替换 rtsp 示例） |
| `analytics_zones.json` | ROI/越线/拥挤/方向区域模板 |
| `ai_models.json` | detector/tracker 模型模板 |
| `pipeline_profiles.json` | 管道配置模板 |
| `alert_rules.json` | 报警规则模板 |
| `detections_query.json` | 检测查询参数模板（时间范围/过滤） |
| `dashboard_query.json` | 仪表盘查询参数模板 |

#### 测试数据生成原则

- 固定模板 + 运行时动态字段（随机后缀、当前时间、UUID）
- 先创建依赖，再测目标 API，再按需清理（避免污染）
- 对幂等 API（如 `start-stream`）至少覆盖一次重复调用断言
- 对多租户 API 默认生成双组织数据（Org A / Org B）验证隔离

#### 需与你确认的特殊数据/模型（实现前讨论）

1. **DeepStream 可用性模式**：默认走 `DEEPSTREAM_MOCK=true`，还是提供一套接真实 DeepStream 的冒烟参数。
2. **RTSP 测试源**：是否统一使用 `mediamtx + 本地样例视频` 作为稳定输入源。
3. **AI 模型样例**：`ai_models.json` 使用占位路径，还是绑定一套可实际部署的最小模型文件。
4. **数据清理策略**：脚本执行后是“保留现场便于排障”还是“自动清理恢复”。

### API 文档与 Swagger 测试计划（暂不实现）

#### 详细 API 文档编写要求

- 每个端点需补齐：功能说明、权限要求、请求参数、请求示例、响应示例、错误码说明
- 文档结构按资源拆分：认证、摄像头、检测、报警、模型、管道、仪表盘、系统状态
- 每个端点必须声明多租户边界（按 `organization` 隔离）与幂等语义（如 `start-stream`）
- 统一错误码词典：`code`、HTTP 状态码、前端处理建议
- 对高风险接口补充操作注意事项（例如 `deploy` 会触发重启、短暂中断）

#### Swagger（drf-spectacular）增强要求

- 为所有 ViewSet/Action 补齐 `summary`、`description`、`tags`
- 使用 `extend_schema` 明确请求体与响应体 schema（含 4xx/5xx 错误响应）
- 为分页列表、过滤参数、枚举字段补齐 OpenAPI 参数说明
- 保证 Swagger UI 中每个端点都可直接调试（开发环境）
- OpenAPI 文档变更纳入评审：接口变化必须同步更新 schema

#### Swagger 测试纳入策略

- 在 `backend/test/` 增加 `test_swagger_schema.py`（校验关键路径与 schema 完整性）
- 在 `test_all.py` 中加入 Swagger 测试步骤（默认启用，可通过参数关闭）
- 最低校验项：文档可访问、核心端点存在、请求/响应字段与实现一致
- 对新增 API 设门禁：未补 Swagger 描述和示例时不通过测试

### 编写详细 README（暂不实现）

> 要求在实现阶段同步编写并维护 `backend/README.md`，作为后端单体入口文档。

#### README 章节结构要求

1. 项目简介与架构定位（Backend 作为唯一 API 网关）
2. 环境依赖与本地启动（含 Docker 与非 Docker）
3. 配置说明（环境变量清单 + 示例）
4. 数据库迁移与初始化数据
5. API 文档入口（Swagger/OpenAPI 链接）
6. 测试说明（单脚本、`test_all.py`、Swagger 测试）
7. 常见问题与排障（Kafka、DeepStream、Redis、权限）
8. 开发规范与提交流程

#### README 质量标准

- 新同学 30 分钟内可按文档跑起后端并完成一次 API 调试
- 所有命令可直接复制执行，避免省略关键参数
- 与 `docs/plan-backend.md` 保持一致，出现冲突以最新实现为准并及时回写

---

## 17. 开发流程

### 启动顺序

```bash
# 1. 基础设施
docker compose up -d postgres redis kafka

# 2. 数据库迁移
python manage.py migrate

# 3. 创建超级用户
python manage.py createsuperuser

# 4. 启动开发服务器（带热重载）
uvicorn config.asgi:application --reload --host 0.0.0.0 --port 8000

# 5. 启动 Kafka Consumer (另一个终端)
python manage.py run_kafka_consumer

# 6. 启动 Celery Worker (另一个终端)
celery -A config worker -l info

# 7. 启动 Celery Beat (另一个终端)
celery -A config beat -l info
```

> **开发用 `uvicorn --reload`**，不用 Daphne（无热重载）也不用 `runserver`（不支持完整 ASGI）。
> 生产用 `gunicorn -k uvicorn.workers.UvicornWorker`（多 worker）。

### 开发无 DeepStream 时的 Mock

开发阶段可能没有 DeepStream 容器。策略：

- 设置 `DEEPSTREAM_MOCK=true`，`DeepStreamClient` 返回预设成功响应
- Kafka Consumer 可以手动往 topic 推测试消息验证消费链路
- 使用 `respx` 在测试中 mock httpx 请求

---

## 18. 踩坑预防清单

| # | 坑 | 现象 | 解决 |
|---|-----|------|------|
| 1 | N+1 查询 | 列表接口响应慢 | `select_related` / `prefetch_related` |
| 2 | Detection 表膨胀 | 磁盘爆满 | **Day 1 分区** + 按 `DETECTION_RETENTION_MONTHS` DROP 过期分区 |
| 3 | Detection UUID 主键 | 高频写入性能差 | 改用 `BigAutoField` |
| 4 | Kafka Consumer 单线程瓶颈 | 消息积压 | 增加 Kafka partition 数 + 多 consumer 实例 |
| 5 | WebSocket 推送风暴 | 前端卡死 | 聚合摘要推送，不逐帧推 |
| 6 | JWT token 过期 | 前端 401 | 前端拦截 401 自动 refresh |
| 7 | 开发环境无 DeepStream | 摄像头功能不可用 | `DEEPSTREAM_MOCK=true` |
| 8 | Celery Worker 未启动 | 通知发不出去 | Docker `restart: unless-stopped` + 进程日志告警（worker/beat/consumer 不走 HTTP healthcheck） |
| 9 | Camera.status 不一致 | 前端状态与实际不符 | Celery Beat 定期 sync + Kafka 事件驱动更新 |
| 10 | 时区问题 | 时间显示错乱 | Django `USE_TZ=True`，Kafka 时间戳统一 UTC |
| 11 | httpx 连接泄漏 | DeepStream 代理响应变慢 | 复用 AsyncClient 单例，禁止每次新建 |
| 12 | AlertEngine 热路径查库 | Kafka Consumer 变慢 | 内存缓存冷却时间，不查 DB |
| 13 | Kafka Consumer 异常退出丢消息 | 重启后重复消费或丢失 | Graceful shutdown + 手动 commit |
| 14 | Docker Compose YAML 语法错误 | 容器启动失败 | 使用 `x-` 扩展字段定义锚点 |
| 15 | 多租户数据泄漏 | Org A 看到 Org B 数据 | `OrganizationFilterMixin` 必检项 + 测试覆盖 |
| 16 | Celery 任务无限重试 | 队列阻塞 | `max_retries=3` + `retry_backoff=True` |
| 17 | 容器以 root 运行 | 安全隐患 | Dockerfile 多阶段构建 + 非 root 用户 |
| 18 | 跨服务排障困难 | 无法追踪请求链路 | `RequestIDMiddleware` + structlog |
| 19 | PipelineProfile 变更不生效 | 改了管道配置但 DeepStream 没变 | 必须通过 `deploy` 端点触发配置部署 + 容器重启 |
| 20 | 空帧过滤后 Dashboard 帧率统计失真 | 显示"每秒帧数"远低于实际 | 帧率改用 PerfMonitor 数据，不依赖 Kafka 消息计数 |
| 21 | AIModel.config 校验缺失 | 错误的模型配置导致 DeepStream 启动失败 | Serializer `validate()` 按 model_type 校验 config schema |
| 22 | AnalyticsZone 坐标超范围 | nvdsanalytics 静默忽略无效区域 | Serializer 校验坐标在 [0, config-width]×[0, config-height] 范围内 |
| 23 | analytics 配置未随 deploy 更新 | 修改了 ROI 但 DeepStream 还用旧配置 | `PipelineDeployer` 每次 deploy 都重新生成 `analytics_config.txt` |
| 24 | stream-id 映射漂移 | 摄像头增删后分析区域作用于错误摄像头 | 按 Camera.uid 排序分配 stream-id，添加流时保持同一排序顺序 |
| 25 | Detection.analytics 为 null 时规则匹配崩溃 | AlertEngine 报 KeyError | analytics 规则先检查 `detection.analytics` 是否存在，不存在则跳过 |
