# Preview（后端）

对应前端契约：[preview_api.md](../frontend/preview_api.md)。  
实现包：`servers/django/pages/preview/`。前缀：`/ai_stream2/backend/preview`。

---

## 代理的 API

无。播放 RTSP / WebRTC 由前端经 [stream_api](../frontend/stream_api.md) 取流信息；本页**不**配置 MediaMTX。

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/active_layout` | 当前用户激活预设 id |
| `PUT` | `/active_layout` | 设置激活预设 |
| `GET` | `/layouts` | 全部预设（站点全局） |
| `GET` | `/layouts/map` | name → id |
| `GET` | `/layouts/{preset_id}` | 预设详情 |
| `POST` | `/layouts` | Save As 新建 |
| `PATCH` | `/layouts/{preset_id}` | Save（layout/slots/view_mode）或 Rename |
| `DELETE` | `/layouts/{preset_id}` | 删预设（禁 Default） |
| `POST` | `/layouts/batch/delete` | 批量删；含 Default → `403` |
| `PUT` | `/layouts/{preset_id}/shot` | 上传 Grid 合成缩略图 |
| `GET` | `/layouts/{preset_id}/shot` | 取缩略图 |

字段 / 错误码 / 固定段顺序以 [preview_api.md](../frontend/preview_api.md) 为准。

权限：`previews.view_preview` / `add_preview` / `change_preview` / `delete_preview`。

常量：`DEFAULT_LAYOUT_ID=00000000-0000-4000-8000-000000000002`；`DEFAULT_LAYOUT_NAME=Default`；seed `layout=2x2`、四槽 `null`、`view_mode=grid`。

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | `LayoutPreset`（站点全局）；`ActiveLayout`（按 user） |
| 文件 | shot 落盘媒体卷；`shot_url` 指向本前缀 GET |
| 鉴权 | session + `has_perm` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| 布局 CRUD / slots 校验 | **造**（`LayoutPresetService`） |
| 激活预设 per-user | **造**（对齐 shell `PageSettings`） |
| Default seed | **造**（data migration；不可删/不可改名） |
| 流是否存在 | **软引用**：只存 UUID；响应时可把已删流置 `null` + `missing_stream_ids`；**不** FK 到 streams |
| 分组树 / 流详情 | 前端直调 stream_api；本页不代理 |
| Site Config 切片 | **造**（预设；不含 per-user active） |
| 响应外壳 | **复用** `shared.*` |

---

## 目录结构

```
servers/django/pages/preview/
  apps.py
  urls.py                 # active_layout、map、batch、shot 先于 {preset_id}
  models.py               # LayoutPreset, ActiveLayout
  serializers.py
  views.py
  services.py             # LayoutPresetService, ActiveLayoutService, ShotService
  permissions.py
  migrations/             # Default seed
```

禁止 import `pages.streams` / `pages.recordings`。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `LayoutPreset` | 站点全局预设行 |
| `ActiveLayout` | `user_id` → `preset_id`（1:1） |
| `LayoutPresetService` | list / map / get / create / patch / delete / batch_delete；slots 校验 |
| `ActiveLayoutService` | get（无行 → `DEFAULT_LAYOUT_ID`）/ set |
| `ShotService` | put / get 缩略图文件 |
| Views | 薄封装 |

---

## 数据库

### `preview_layoutpreset`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | Default 固定为 `DEFAULT_LAYOUT_ID` |
| `name` | varchar | UNIQUE，非空 | — | 保留名 `Default` 仅种子行 |
| `layout` | varchar | 非空 | — | `1x1`\|`2x2`\|`3x3`\|`4x4` |
| `view_mode` | varchar | 非空 | `grid` | `grid`\|`focus` |
| `slots` | JSON | 非空 | — | UUID\|null 数组；长度=槽位数 |
| `shot_path` | varchar | 可空 | `NULL` | 磁盘相对路径；响应映射为 `shot_url` |

只读响应字段 `stream_count` = 非 null 槽数，不入库。

种子：`id=DEFAULT_LAYOUT_ID`、`name=Default`、`layout=2x2`、`slots=[null×4]`、`view_mode=grid`。

### `preview_activelayout`

| 列 | 类型 | 约束 | 说明 |
|----|------|------|------|
| `id` | UUID | PK | |
| `user_id` | UUID | UNIQUE，非空 | 当前登录用户 |
| `preset_id` | UUID | 非空 | 指向本页预设（软引用，无跨表 FK 强制） |

无行时 GET 返回 Default，**不**自动 INSERT。

---

## 各请求写库明细

| 请求 | 表 | 写入 / 变更 |
|------|-----|-------------|
| `PUT /active_layout` | `preview_activelayout` | UPSERT `preset_id`（须存在） |
| `POST /layouts` | `preview_layoutpreset` | INSERT；禁 name=`Default`；校验 slots |
| `PATCH /layouts/{id}` | 同左 | 更新 layout/slots/view_mode 和/或 name；Default **禁改名** |
| `DELETE` / `batch/delete` | 同左 | 删行；含 Default → `403`；响应可带 `active_deleted` |
| `PUT .../shot` | 文件 + `shot_path` | 写文件；UPDATE path |
| `GET` 系列 | — | 只读 |

**不**持久化：`focus_index`、选中槽、未 Save 的会话编辑。

Site Config import：重建预设（含 Default 幂等）；**不**导入各用户 `ActiveLayout`。

---

## 调用顺序

```
进页: GET active_layout → GET layouts/{id} →（前端）streams/groups/tree
Save As: POST /layouts → PUT shot → PUT active_layout
Save: PATCH /layouts/{id} → PUT shot
删激活项: DELETE → 前端 PUT active=Default
```

slots：长度匹配 layout；同一 stream_id 至多一次；非法 UUID 在写时 `400`，读时软清理可选。

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| streams | 仅 UUID 软引用；树/播放不经本页 |
| recordings | **独立**预设命名空间（不同 Default id） |
| Site Config | **注册** layouts 切片 |
| MediaMTX | 不本页配置 |
