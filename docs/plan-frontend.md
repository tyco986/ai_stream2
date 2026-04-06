# 前端 — 实现计划

## 1. 定位与职责

前端是三端架构的**展示与交互层**：

1. **数据展示** — 摄像头状态、检测结果、报警事件、仪表盘统计
2. **操作控制** — 摄像头 CRUD、启停视频流、AI 模型与管道配置、报警确认/解决、规则配置
3. **实时通信** — 通过 WebSocket 接收检测结果推送、摄像头状态变更、报警通知

**不做的事**：直连 DeepStream、直连 Kafka、业务逻辑计算（全部由后端完成）。

---

## 2. 技术栈

| 层面 | 技术 | 说明 |
|------|------|------|
| 框架 | Vue 3 | Composition API + `<script setup>` |
| 语言 | TypeScript | 类型安全，API 对接防 bug |
| 构建工具 | Vite | 开发热更新秒级 |
| UI 组件库 | Element Plus | 中后台标配，组件丰富，中文文档 |
| 路由 | Vue Router 4 | 官方路由 |
| 状态管理 | Pinia | 官方推荐，取代 Vuex |
| HTTP 客户端 | Axios | 拦截器做 JWT 自动 refresh + 统一错误处理 |
| WebSocket | 原生 WebSocket | 轻量封装，自动重连 |
| 图表 | ECharts | Dashboard 检测趋势、统计图 |
| 图标 | @element-plus/icons-vue | Element Plus 配套图标 |
| 按需引入 | unplugin-vue-components + unplugin-auto-import | Element Plus 组件自动注册，减小打包体积 |
| CSS 方案 | Element Plus 内置 + CSS 变量 | 主题通过 CSS 变量定制 |
| 代码规范 | ESLint + Prettier | 统一代码风格 |
| 容器部署 | Nginx | 静态文件托管 + 反向代理 |

---

## 3. 项目结构

```
frontend/
├── public/
│   └── favicon.ico
├── src/
│   ├── api/                         # API 请求模块
│   │   ├── auth.ts                  # 登录、刷新 token
│   │   ├── cameras.ts               # 摄像头 CRUD + 启停
│   │   ├── analytics.ts             # 分析区域 CRUD（ROI/越线/拥挤/方向）
│   │   ├── detections.ts            # 检测结果查询
│   │   ├── alerts.ts                # 报警规则 + 报警记录
│   │   ├── pipelines.ts             # AI 模型 + 管道配置 CRUD + 部署
│   │   ├── recordings.ts            # 录像列表 + 手动录制控制
│   │   ├── screenshots.ts           # 截图列表 + 手动截图
│   │   ├── dashboard.ts             # 仪表盘聚合
│   │   └── deepstream.ts            # DeepStream 代理（状态/预览/录制/截图命令）
│   ├── composables/                 # 可复用逻辑（Composition API）
│   │   ├── useAuth.ts               # JWT 认证状态
│   │   ├── useWebSocket.ts          # WebSocket 连接管理（生命周期绑定）
│   │   ├── usePermission.ts         # 权限判断
│   │   └── useLoading.ts            # 表格/操作 loading 状态管理
│   ├── components/                  # 公共组件
│   │   ├── layout/
│   │   │   ├── AppLayout.vue        # 主布局（侧边栏 + 顶栏 + 内容区）
│   │   │   ├── Sidebar.vue
│   │   │   └── Navbar.vue
│   │   ├── common/
│   │   │   ├── PageHeader.vue       # 页面标题 + 面包屑
│   │   │   ├── StatusTag.vue        # 状态标签（online/offline/error）
│   │   │   └── StaleConfigBanner.vue # analytics_config_stale 持久警告条
│   │   ├── analytics/
│   │   │   └── ZoneDrawer.vue       # Canvas 绘制组件（ROI 多边形 / 越线线段）
│   │   ├── preview/
│   │   │   └── WhepPlayer.vue       # WebRTC WHEP 播放器（对接 MediaMTX）
│   │   └── charts/
│   │       ├── DetectionTrend.vue   # 检测趋势折线图
│   │       └── CameraStatusPie.vue  # 摄像头状态饼图
│   ├── views/                       # 页面组件
│   │   ├── login/
│   │   │   └── LoginView.vue
│   │   ├── dashboard/
│   │   │   └── DashboardView.vue
│   │   ├── cameras/
│   │   │   ├── CameraListView.vue
│   │   │   ├── CameraDetailView.vue
│   │   │   └── CameraPreviewView.vue  # 多画面/单路实时预览
│   │   ├── pipelines/
│   │   │   ├── AIModelListView.vue        # AI 模型注册与管理
│   │   │   └── PipelineProfileView.vue    # 管道配置（模型编排）
│   │   ├── detections/
│   │   │   └── DetectionListView.vue
│   │   ├── recordings/
│   │   │   └── RecordingListView.vue    # 录像列表 + 回放
│   │   ├── screenshots/
│   │   │   └── ScreenshotListView.vue   # 截图列表 + 查看
│   │   ├── alerts/
│   │   │   ├── AlertRuleListView.vue
│   │   │   └── AlertListView.vue
│   │   └── system/
│   │       └── UserListView.vue
│   ├── stores/                      # Pinia 状态管理
│   │   ├── auth.ts                  # 用户 + token
│   │   ├── camera.ts                # 摄像头列表 + 状态
│   │   ├── alert.ts                 # 未处理报警计数 + 铃铛角标
│   │   └── notification.ts          # 实时通知（WebSocket）
│   ├── router/
│   │   └── index.ts                 # 路由配置 + 导航守卫
│   ├── utils/
│   │   ├── request.ts               # Axios 实例 + 拦截器
│   │   ├── websocket.ts             # WebSocket 封装
│   │   └── format.ts                # 日期、数字格式化
│   ├── types/                       # TypeScript 类型定义
│   │   ├── api.ts                   # API 响应类型
│   │   ├── camera.ts
│   │   ├── detection.ts
│   │   ├── alert.ts
│   │   ├── analytics.ts            # AnalyticsZone 类型
│   │   ├── pipeline.ts             # AIModel, PipelineProfile 类型
│   │   ├── recording.ts            # Recording 类型
│   │   └── screenshot.ts           # Screenshot 类型
│   ├── styles/
│   │   ├── variables.css            # Element Plus CSS 变量覆盖
│   │   └── global.css               # 全局样式
│   ├── App.vue
│   └── main.ts
├── Dockerfile
├── nginx.conf
├── index.html
├── vite.config.ts
├── tsconfig.json
├── package.json
└── .eslintrc.cjs
```

---

## 4. 页面设计

### 4.1 布局

经典中后台布局：**左侧边栏 + 顶栏 + 内容区**

```
┌──────────────────────────────────────────────┐
│  Logo        搜索      通知铃铛  用户头像 ▼   │  ← 顶栏
├────────┬─────────────────────────────────────┤
│        │                                     │
│ 仪表盘  │          内容区                     │
│ 摄像头  │  ├ 摄像头列表                        │
│        │  └ 实时预览                          │
│ AI管道  │  ├ 模型管理                          │
│        │  └ 管道配置                          │
│ 检测记录│                                     │
│ 录像回放│                                     │
│ 截图管理│                                     │
│ 报警规则│                                     │
│ 报警记录│                                     │
│ 系统管理│                                     │
│        │                                     │
├────────┴─────────────────────────────────────┤
│  侧边栏可折叠                                  │
└──────────────────────────────────────────────┘
```

### 4.2 页面清单

| 页面 | 路由 | 权限 | 主要功能 |
|------|------|------|---------|
| 登录 | `/login` | 公开 | 用户名 + 密码登录 |
| 仪表盘 | `/dashboard` | viewer+ | 在线摄像头数、今日检测数、未处理报警数、检测趋势图 |
| 摄像头列表 | `/cameras` | viewer+ | 表格/卡片视图、状态筛选、启停流操作、**analytics_config_stale 警告条** |
| 摄像头详情 | `/cameras/:id` | viewer+ | **实时预览**、基本信息、当前状态、**关联管道配置**、**分析区域绘制（ROI/越线/拥挤/方向）**、最近检测记录 |
| **实时预览** | `/cameras/preview` | viewer+ | 多画面总览（4×4 tiler）、点击单路放大（show-source 切换）、WebRTC WHEP 播放 |
| **AI 模型管理** | `/pipelines/models` | operator+ | 模型注册/编辑/删除，按类型筛选 |
| **管道配置** | `/pipelines/profiles` | operator+ | 管道配置 CRUD，模型编排，一键部署到 DeepStream |
| 检测记录 | `/detections` | viewer+ | 时间范围筛选、摄像头筛选、分页列表（含分析结果） |
| **录像回放** | `/recordings` | viewer+ | 录像列表（滚动/事件/手动）、按摄像头/时间/类型筛选、在线回放、下载 |
| **截图管理** | `/screenshots` | viewer+ | 截图列表、按摄像头/时间筛选、图片预览、下载 |
| 报警规则 | `/alert-rules` | operator+ | 规则 CRUD（**含分析规则**）、启用/禁用 |
| 报警记录 | `/alerts` | viewer+ | 报警列表、确认/解决操作、状态筛选、**关联事件录像** |
| 用户管理 | `/system/users` | admin | 用户列表、角色分配 |

### 4.3 AI 模型管理页面

**页面结构**：

```
┌─────────────────────────────────────────────────────────────┐
│ AI 模型管理            [类型筛选 ▼]  [+ 注册模型]            │
├─────────────────────────────────────────────────────────────┤
│ 名称         │ 类型       │ 版本  │ 状态   │ 操作           │
│─────────────│───────────│──────│───────│───────────────  │
│ yolov8n     │ 🔍 检测器  │ v1.0 │ ✅ 启用 │ 编辑 | 删除    │
│ NvDCF_perf  │ 📍 跟踪器  │ -    │ ✅ 启用 │ 编辑 | 删除    │
└─────────────────────────────────────────────────────────────┘
```

**注册/编辑模型弹窗**：

- 通用字段：名称、版本、描述、模型文件路径、标签文件路径
- **根据 model_type 动态显示不同配置表单**：
  - `detector`：类别数、缩放因子、cluster_mode、精度
  - `tracker`：跟踪器类型下拉选择（NvDCF_perf / IOU / NvSORT）

> **扩展预留**：后续版本新增 `classifier`（类别数、operate_on_class_ids、精度）和
> `action`（clip_length、stride、输入分辨率）类型表单。

### 4.4 管道配置页面

**页面结构**：

```
┌─────────────────────────────────────────────────────────────┐
│ 管道配置              [+ 新建管道]                           │
├─────────────────────────────────────────────────────────────┤
│ 名称             │ 检测器    │ 跟踪器     │ 视频分析 │ 操作  │
│─────────────────│──────────│───────────│─────────│─────  │
│ 交通监控-标准    │ yolov8n  │ NvDCF_perf│ ✅ 启用  │ 编辑|部署│
│ 安防监控-高级    │ yolov8n  │ NvDCF_acc │ ✅ 启用  │ 编辑|部署│
├─────────────────────────────────────────────────────────────┤
│ [部署到 DeepStream]  ← 选中配置后点击部署                     │
└─────────────────────────────────────────────────────────────┘
```

> **扩展预留**：后续版本在表格中增加"分类器"和"动作识别"列。

**新建/编辑管道配置弹窗**：

```
┌─────────────────────────────────────────┐
│ 管道配置                                 │
│                                          │
│ 管道名称: [________________]             │
│                                          │
│ ① 检测器 (必选):  [yolov8n       ▼]     │
│ ② 跟踪器 (可选):  [NvDCF_perf   ▼]     │
│ ③ 视频分析 (可选): [✓ 启用 nvdsanalytics] │
│                                          │
│ 推理链路预览:                             │
│ YOLO → NvDCF → nvdsanalytics             │
│                                          │
│          [取消]  [保存]                   │
└─────────────────────────────────────────┘
```

> **推理链路预览**：实时显示模型的执行顺序，用户可以直观理解数据流经过哪些模型。
> **扩展预留**：后续版本在 ② 和 ③ 之间添加"分类器(可选)"和"动作识别(可选)"插槽，
> 支持 SGIE 分类器拖拽排序和 SlowFast 等时序模型配置。

### 4.5 摄像头详情页增强

在摄像头详情页增加"实时预览"、"管道配置"和"分析区域"卡片：

```
┌──────────────────────────────────────────────────────┐
│ ⚠ 摄像头集合已变更，分析区域配置可能不准确，请重新部署管道  │  ← analytics_config_stale 警告条
│                                       [重新部署]      │     （仅 analytics_config_stale=true 时显示）
├──────────────────────────────────────────────────────┤
│ 📷 Front Door Camera                                 │
│ 状态: 🟢 在线   RTSP: rtsp://...                     │
├──────────────────────────────────────────────────────┤
│ 实时预览                                              │
│ ┌──────────────────────────────────────────┐         │
│ │  WhepPlayer (WebRTC)                      │         │
│ │  对接 MediaMTX WHEP 端点                   │         │
│ │  摄像头在线时自动播放，离线时显示占位图       │         │
│ └──────────────────────────────────────────┘         │
├──────────────────────────────────────────────────────┤
│ 管道配置                                              │
│ 当前管道: 安防监控-标准                                 │
│ 检测器: yolov8n → 跟踪器: NvDCF → 分析: nvdsanalytics │
│                    [切换管道配置 ▼]                    │
├──────────────────────────────────────────────────────┤
│ 分析区域配置                          [+ 添加区域]     │
│ ┌─────────────────────────────────┐                  │
│ │  Canvas 绘制区域                  │  名称    │ 类型  │
│ │  ┌──────┐                        │  大门入口 │ ROI  │
│ │  │ ROI  │    ──越线──             │  通道A   │ 越线  │
│ │  └──────┘                        │  大厅    │ 拥挤  │
│ │  (1920×1080 坐标系)              │  [部署生效] │
│ └─────────────────────────────────┘                  │
├──────────────────────────────────────────────────────┤
│ 录制与截图                                             │
│ [📹 开始录制] [📹 停止录制]  [📸 截图]                  │
│ 录制状态: ⏺ 录制中 (00:03:45)   ← 仅录制中显示          │
├──────────────────────────────────────────────────────┤
│ 最近检测记录                                           │
│ ┌───────────────────────────────────────────┐        │
│ │ 时间 │ 类型 │ 置信度 │ 分析结果              │        │
│ │ ...  │ ...  │ ...   │ ROI:entrance, 方向:S  │        │
│ └───────────────────────────────────────────┘        │
└──────────────────────────────────────────────────────┘
```

### 4.6 分析区域绘制交互（ZoneDrawer 组件）

**核心交互**：

- **Canvas 叠加层**：在摄像头画面（或静态截图）上方叠加 Canvas，支持绘制多边形和线段
- **绘制模式**：
  - `roi`：点击添加多边形顶点，双击闭合
  - `line_crossing`：点击起点和终点画线
  - `direction`：点击起点和终点画方向向量（带箭头）
  - `overcrowding`：同 ROI 绘制，额外输入阈值
- **已有区域回显**：从 API 加载已保存的 `AnalyticsZone`，用不同颜色/样式在 Canvas 上绘制
- **编辑/删除**：点击已有区域高亮，支持拖拽顶点编辑或删除
- **坐标系**：Canvas 绘制坐标自动映射到 1920×1080 配置坐标系（与 DeepStream 一致）

**组件 Props**：

```typescript
interface ZoneDrawerProps {
  cameraId: string
  zones: AnalyticsZone[]         // 已有区域列表
  mode: 'roi' | 'line_crossing' | 'overcrowding' | 'direction' | null
  width?: number                  // Canvas 显示宽度（默认 960，等比缩放）
}
```

> **部署提示**：区域修改后，页面顶部显示"分析区域已修改，需重新部署管道配置才能生效"，
> 提供快捷"重新部署"按钮。

### 4.7 报警规则创建增强

报警规则创建弹窗根据 `rule_type` 动态切换条件表单：

| rule_type | 条件表单 | 前置条件 |
|-----------|---------|---------|
| `object_count` | 数量阈值输入框 | — |
| `object_type` | 目标类型下拉选择 + 数量阈值 | — |
| `zone_intrusion` | 选择已配置的 ROI 区域名称 + 目标类型 | 摄像头已配置 AnalyticsZone (ROI) |
| `line_crossing` | 选择已配置的越线名称 + 计数阈值 | 摄像头已配置 AnalyticsZone (line_crossing) |
| `overcrowding` | 选择已配置的拥挤区域名称 | 摄像头已配置 AnalyticsZone (overcrowding) |

> `zone_intrusion`、`line_crossing`、`overcrowding` 的区域/线名称从关联摄像头的
> `AnalyticsZone` 列表中动态加载。如果摄像头未配置对应分析区域，该规则类型置灰并提示
> "请先在摄像头详情页配置分析区域"。
>
> **扩展预留**：后续版本新增 `classifier_match`（SGIE 分类匹配）和
> `action_detected`（动作标签 + 置信度阈值）规则类型。

### 4.8 实时预览页面（WebRTC WHEP）

**架构**：DeepStream → RTSP :8554 → MediaMTX → WebRTC WHEP :8889 → 前端 `<video>` 标签。
前端**不直连** DeepStream，连的是 MediaMTX 的 WHEP 端点。预览 URL 从后端 API 获取。

**多画面总览页**（`/cameras/preview`）：

```
┌──────────────────────────────────────────────────────┐
│ 实时预览                              [返回列表]      │
├──────────────────────────────────────────────────────┤
│ ┌────────────┬────────────┬────────────┬───────────┐ │
│ │ cam_001    │ cam_002    │ cam_003    │ cam_004   │ │
│ │ (WhepPlayer)│           │            │           │ │
│ ├────────────┼────────────┼────────────┼───────────┤ │
│ │ cam_005    │ cam_006    │ ...        │           │ │
│ │            │            │            │           │ │
│ └────────────┴────────────┴────────────┴───────────┘ │
│ 点击任一画面 → 发送 switch_preview 命令切换单路全分辨率  │
│ 点击「返回总览」→ switch_preview(source_id=-1) 恢复拼接 │
└──────────────────────────────────────────────────────┘
```

> **初版 SLA**：消费级 GPU 下只支持 tiler 模式（单 RTSP 输出端点），
> 多画面/单路切换通过 tiler 的 `show-source` 属性实现，前端只对接一个 WHEP 端点。
> 预览 URL 通过 `GET /api/v1/cameras/preview-url/` 从后端获取，不硬编码 MediaMTX 地址。

**WhepPlayer 组件 Props**：

```typescript
interface WhepPlayerProps {
  url: string                     // WHEP 端点（如 http://mediamtx:8889/preview/whep）
  autoplay?: boolean              // 默认 true
  muted?: boolean                 // 默认 true（浏览器要求自动播放必须静音）
}
```

**WhepPlayer 核心逻辑**：

```typescript
async function startWhep(url: string, videoEl: HTMLVideoElement) {
  const pc = new RTCPeerConnection()
  pc.ontrack = (event) => {
    videoEl.srcObject = event.streams[0]
  }
  pc.addTransceiver('video', { direction: 'recvonly' })

  const offer = await pc.createOffer()
  await pc.setLocalDescription(offer)

  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/sdp' },
    body: offer.sdp,
  })
  await pc.setRemoteDescription({
    type: 'answer',
    sdp: await res.text(),
  })
  return pc
}
```

> **生命周期管理**：`onUnmounted` 时必须 `pc.close()` 释放 WebRTC 连接，
> 否则 MediaMTX 认为客户端仍在观看，`sourceOnDemand` 不生效。

### 4.9 analytics_config_stale 警告条

**触发条件**：后端 `PipelineProfile.analytics_config_stale = true`（摄像头集合变更后自动标记）。

**展示位置**：摄像头列表页顶部 + 摄像头详情页顶部。

**UI 设计**：

```
┌──────────────────────────────────────────────────────────────┐
│ ⚠ 摄像头集合已变更，分析区域配置可能不准确，请重新部署管道配置    │
│                                              [重新部署]       │
└──────────────────────────────────────────────────────────────┘
```

- 使用 `ElAlert` 组件，`type="warning"`，`closable=false`（持久显示，不可关闭）
- "重新部署"按钮触发 `POST /api/v1/pipeline-profiles/{id}/deploy/`，含二次确认
- 部署成功后 `analytics_config_stale` 标记清除，警告条消失

---

## 5. API 对接层

### Axios 封装

```typescript
import axios from 'axios'
import type { AxiosRequestConfig } from 'axios'
import { useAuthStore } from '@/stores/auth'
import { ElMessage } from 'element-plus'

const request = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL,  // /api/v1
  timeout: 10000,
})

// 请求拦截：自动带 JWT
request.interceptors.request.use((config) => {
  const auth = useAuthStore()
  if (auth.accessToken) {
    config.headers.Authorization = `Bearer ${auth.accessToken}`
  }
  return config
})

// ---- 并发 refresh 保护 ----
// 多个请求同时 401 时，只发一次 refresh，其他请求排队等待结果后重试。
// 后端开启了 BLACKLIST_AFTER_ROTATION，重复 refresh 会使旧 token 失效导致登出。
let isRefreshing = false
let pendingQueue: Array<{
  resolve: (config: AxiosRequestConfig) => void
  reject: (error: any) => void
}> = []

function onRefreshed() {
  pendingQueue.forEach(({ resolve }) => resolve({} as AxiosRequestConfig))
  pendingQueue = []
}

function onRefreshFailed(error: any) {
  pendingQueue.forEach(({ reject }) => reject(error))
  pendingQueue = []
}

// 响应拦截：统一错误处理 + 自动 refresh（含并发保护）
request.interceptors.response.use(
  (response) => response.data,
  async (error) => {
    const originalConfig = error.config
    if (error.response?.status === 401 && !originalConfig._retry) {
      if (isRefreshing) {
        return new Promise((resolve, reject) => {
          pendingQueue.push({ resolve, reject })
        }).then(() => {
          return request(originalConfig)
        })
      }

      originalConfig._retry = true
      isRefreshing = true
      const auth = useAuthStore()
      try {
        const refreshed = await auth.refreshToken()
        if (refreshed) {
          onRefreshed()
          return request(originalConfig)
        }
        onRefreshFailed(error)
        auth.logout()
        return Promise.reject(error)
      } catch (refreshError) {
        onRefreshFailed(refreshError)
        auth.logout()
        return Promise.reject(refreshError)
      } finally {
        isRefreshing = false
      }
    }
    const message = error.response?.data?.message || '请求失败'
    ElMessage.error(message)
    return Promise.reject(error)
  }
)

export default request
```

### API 模块示例

```typescript
// src/api/cameras.ts
import request from '@/utils/request'
import type { Camera, CameraCreate } from '@/types/camera'
import type { ApiResponse, PaginatedResponse } from '@/types/api'

export const cameraApi = {
  list: (params?: Record<string, any>) =>
    request.get<any, ApiResponse<PaginatedResponse<Camera>>>('/cameras/', { params }),

  detail: (id: string) =>
    request.get<any, ApiResponse<Camera>>(`/cameras/${id}/`),

  create: (data: CameraCreate) =>
    request.post<any, ApiResponse<Camera>>('/cameras/', data),

  update: (id: string, data: Partial<CameraCreate>) =>
    request.patch<any, ApiResponse<Camera>>(`/cameras/${id}/`, data),

  delete: (id: string) =>
    request.delete<any, ApiResponse<null>>(`/cameras/${id}/`),

  startStream: (id: string) =>
    request.post<any, ApiResponse<null>>(`/cameras/${id}/start-stream/`),

  stopStream: (id: string) =>
    request.post<any, ApiResponse<null>>(`/cameras/${id}/stop-stream/`),
}
```

### Analytics API 模块

```typescript
// src/api/analytics.ts
import request from '@/utils/request'
import type { AnalyticsZone, AnalyticsZoneCreate } from '@/types/analytics'
import type { ApiResponse } from '@/types/api'

export const analyticsZoneApi = {
  list: (cameraId: string) =>
    request.get<any, ApiResponse<AnalyticsZone[]>>(`/cameras/${cameraId}/analytics-zones/`),

  create: (cameraId: string, data: AnalyticsZoneCreate) =>
    request.post<any, ApiResponse<AnalyticsZone>>(`/cameras/${cameraId}/analytics-zones/`, data),

  update: (cameraId: string, zoneId: string, data: Partial<AnalyticsZoneCreate>) =>
    request.patch<any, ApiResponse<AnalyticsZone>>(`/cameras/${cameraId}/analytics-zones/${zoneId}/`, data),

  delete: (cameraId: string, zoneId: string) =>
    request.delete<any, ApiResponse<null>>(`/cameras/${cameraId}/analytics-zones/${zoneId}/`),
}
```

### Analytics TypeScript 类型

```typescript
// src/types/analytics.ts
export type ZoneType = 'roi' | 'line_crossing' | 'overcrowding' | 'direction'

export interface AnalyticsZone {
  id: string
  camera: string
  name: string
  zone_type: ZoneType
  coordinates: number[][]           // [[x1,y1], [x2,y2], ...]
  config: Record<string, any>       // zone_type 特定参数
  is_enabled: boolean
  created_at: string
  updated_at: string
}

export interface AnalyticsZoneCreate {
  name: string
  zone_type: ZoneType
  coordinates: number[][]
  config: Record<string, any>
  is_enabled?: boolean
}
```

### Pipeline API 模块

```typescript
// src/api/pipelines.ts
import request from '@/utils/request'
import type { AIModel, AIModelCreate, PipelineProfile, PipelineProfileCreate } from '@/types/pipeline'
import type { ApiResponse, PaginatedResponse } from '@/types/api'

export const aiModelApi = {
  list: (params?: Record<string, any>) =>
    request.get<any, ApiResponse<PaginatedResponse<AIModel>>>('/ai-models/', { params }),

  detail: (id: string) =>
    request.get<any, ApiResponse<AIModel>>(`/ai-models/${id}/`),

  create: (data: AIModelCreate) =>
    request.post<any, ApiResponse<AIModel>>('/ai-models/', data),

  update: (id: string, data: Partial<AIModelCreate>) =>
    request.patch<any, ApiResponse<AIModel>>(`/ai-models/${id}/`, data),

  delete: (id: string) =>
    request.delete<any, ApiResponse<null>>(`/ai-models/${id}/`),
}

export const pipelineProfileApi = {
  list: (params?: Record<string, any>) =>
    request.get<any, ApiResponse<PaginatedResponse<PipelineProfile>>>('/pipeline-profiles/', { params }),

  detail: (id: string) =>
    request.get<any, ApiResponse<PipelineProfile>>(`/pipeline-profiles/${id}/`),

  create: (data: PipelineProfileCreate) =>
    request.post<any, ApiResponse<PipelineProfile>>('/pipeline-profiles/', data),

  update: (id: string, data: Partial<PipelineProfileCreate>) =>
    request.patch<any, ApiResponse<PipelineProfile>>(`/pipeline-profiles/${id}/`, data),

  delete: (id: string) =>
    request.delete<any, ApiResponse<null>>(`/pipeline-profiles/${id}/`),

  deploy: (id: string) =>
    request.post<any, ApiResponse<null>>(`/pipeline-profiles/${id}/deploy/`),
}

export const cameraPipelineApi = {
  get: (cameraId: string) =>
    request.get<any, ApiResponse<PipelineProfile>>(`/cameras/${cameraId}/pipeline/`),

  set: (cameraId: string, profileId: string) =>
    request.put<any, ApiResponse<null>>(`/cameras/${cameraId}/pipeline/`, { pipeline_profile_id: profileId }),
}
```

### DeepStream 代理 API 模块

```typescript
// src/api/deepstream.ts
import request from '@/utils/request'
import type { ApiResponse } from '@/types/api'

export const deepstreamApi = {
  previewUrl: () =>
    request.get<any, ApiResponse<{ url: string }>>('/deepstream/preview-url/'),

  switchPreview: (sourceId: number) =>
    request.post<any, ApiResponse<null>>('/deepstream/switch-preview/', { source_id: sourceId }),

  status: () =>
    request.get<any, ApiResponse<{ running: boolean; uptime: number }>>('/deepstream/status/'),
}
```

### Pipeline TypeScript 类型

```typescript
// src/types/pipeline.ts

// 初版只支持 detector 和 tracker，扩展预留 classifier / action
export type ModelType = 'detector' | 'tracker'

export interface AIModel {
  id: string
  name: string
  model_type: ModelType
  framework: 'onnx' | 'engine' | 'custom'
  model_file: string
  label_file: string | null
  config: Record<string, any>
  version: string
  description: string
  is_active: boolean
  created_at: string
  updated_at: string
}

export interface AIModelCreate {
  name: string
  model_type: ModelType
  framework: 'onnx' | 'engine' | 'custom'
  model_file: string
  label_file?: string
  config: Record<string, any>
  version: string
  description?: string
}

export interface PipelineProfile {
  id: string
  name: string
  description: string
  detector: AIModel
  tracker: AIModel | null
  analytics_enabled: boolean        // 是否启用 nvdsanalytics 视频分析
  analytics_config_stale: boolean   // 摄像头集合变更后标记为 true，需重部署
  is_active: boolean
  created_at: string
  updated_at: string
}

export interface PipelineProfileCreate {
  name: string
  description?: string
  detector_id: string
  tracker_id?: string
  analytics_enabled?: boolean
}
```

> **扩展预留**：后续版本 `ModelType` 增加 `'classifier' | 'action'`，
> `PipelineProfile` 增加 `classifiers: AIModel[]` 和 `action_model: AIModel | null`，
> `PipelineProfileCreate` 增加 `classifier_ids?: string[]` 和 `action_model_id?: string`。

### TypeScript 类型定义

```typescript
// src/types/api.ts
export interface ApiResponse<T> {
  code: string
  message: string
  data: T
}

export interface PaginatedResponse<T> {
  count: number
  results: T[]
  next: string | null
  previous: string | null
}

// src/types/camera.ts
export interface Camera {
  id: string
  uid: string
  name: string
  rtsp_url: string
  organization: string
  group: string | null
  status: 'offline' | 'connecting' | 'online' | 'error'
  pipeline_profile: string | null   // 关联的 PipelineProfile ID
  config: Record<string, any>
  created_at: string
  updated_at: string
}

export interface CameraCreate {
  name: string
  rtsp_url: string
  group?: string
}
```

---

## 6. WebSocket 实时通信

### 封装

```typescript
// src/utils/websocket.ts
import { useAuthStore } from '@/stores/auth'

export class ReconnectingWebSocket {
  private ws: WebSocket | null = null
  private path: string
  private baseReconnectInterval = 1000
  private maxReconnectInterval = 30000
  private maxReconnectAttempts = 10
  private attempts = 0
  private handlers: Map<string, Set<(data: any) => void>> = new Map()
  private closed = false               // 主动 close 后不再重连

  constructor(path: string) {
    this.path = path
  }

  connect() {
    if (this.closed) return

    // 每次连接时动态获取最新 token，避免 refresh 后重连用旧 token
    const auth = useAuthStore()
    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:'
    const url = `${protocol}//${location.host}/ws/${this.path}?token=${auth.accessToken}`

    this.ws = new WebSocket(url)

    this.ws.onopen = () => {
      this.attempts = 0                 // 连接成功重置计数
    }

    this.ws.onmessage = (event) => {
      const message = JSON.parse(event.data)
      const callbacks = this.handlers.get(message.type)
      if (callbacks) {
        callbacks.forEach(cb => cb(message.data))
      }
    }

    this.ws.onclose = () => {
      if (this.closed) return
      if (this.attempts < this.maxReconnectAttempts) {
        const delay = Math.min(
          this.baseReconnectInterval * Math.pow(2, this.attempts),
          this.maxReconnectInterval,
        )
        this.attempts++
        setTimeout(() => this.connect(), delay)
      }
    }
  }

  on(type: string, callback: (data: any) => void) {
    const set = this.handlers.get(type) || new Set()
    set.add(callback)
    this.handlers.set(type, set)
  }

  off(type: string, callback: (data: any) => void) {
    const set = this.handlers.get(type)
    if (set) {
      set.delete(callback)
      if (set.size === 0) this.handlers.delete(type)
    }
  }

  close() {
    this.closed = true
    this.handlers.clear()
    this.ws?.close()
  }
}
```

> **`composables/useWebSocket.ts` 与 `utils/websocket.ts` 的分工**：
> `utils/websocket.ts` 是底层封装（上方代码），只管连接、重连、消息分发。
> `composables/useWebSocket.ts` 是 Vue 生命周期集成层，在 `onMounted` 中 `connect()`，
> `onUnmounted` 中 `close()`，确保组件卸载后自动断开且不泄漏 handler。

### 使用场景

| WebSocket 路径 | 监听事件 | 前端行为 |
|---------------|---------|---------|
| `/ws/detections/` | `detection.new` | 更新仪表盘实时计数 |
| `/ws/cameras/status/` | `camera.status_changed` | 更新摄像头列表状态标签 |
| `/ws/alerts/` | `alert.triggered` | 弹出通知 + 更新报警列表 + 铃铛角标 |

> **扩展预留**：后续版本新增 `/ws/actions/` 端点，监听 `action.detected` 事件。

---

## 7. 路由与权限

### 路由配置

```typescript
const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    component: () => import('@/views/login/LoginView.vue'),
    meta: { public: true },
  },
  {
    path: '/',
    component: () => import('@/components/layout/AppLayout.vue'),
    redirect: '/dashboard',
    children: [
      {
        path: 'dashboard',
        component: () => import('@/views/dashboard/DashboardView.vue'),
        meta: { roles: ['admin', 'operator', 'viewer'] },
      },
      {
        path: 'cameras',
        component: () => import('@/views/cameras/CameraListView.vue'),
        meta: { roles: ['admin', 'operator', 'viewer'] },
      },
      {
        path: 'cameras/preview',
        component: () => import('@/views/cameras/CameraPreviewView.vue'),
        meta: { roles: ['admin', 'operator', 'viewer'] },
      },
      {
        path: 'cameras/:id',
        component: () => import('@/views/cameras/CameraDetailView.vue'),
        meta: { roles: ['admin', 'operator', 'viewer'] },
      },
      {
        path: 'detections',
        component: () => import('@/views/detections/DetectionListView.vue'),
        meta: { roles: ['admin', 'operator', 'viewer'] },
      },
      {
        path: 'pipelines/models',
        component: () => import('@/views/pipelines/AIModelListView.vue'),
        meta: { roles: ['admin', 'operator'] },
      },
      {
        path: 'pipelines/profiles',
        component: () => import('@/views/pipelines/PipelineProfileView.vue'),
        meta: { roles: ['admin', 'operator'] },
      },
      {
        path: 'alert-rules',
        component: () => import('@/views/alerts/AlertRuleListView.vue'),
        meta: { roles: ['admin', 'operator'] },
      },
      {
        path: 'alerts',
        component: () => import('@/views/alerts/AlertListView.vue'),
        meta: { roles: ['admin', 'operator', 'viewer'] },
      },
      {
        path: 'system/users',
        component: () => import('@/views/system/UserListView.vue'),
        meta: { roles: ['admin'] },
      },
    ],
  },
]
```

### 导航守卫

```typescript
router.beforeEach((to, from, next) => {
  const auth = useAuthStore()

  if (to.meta.public) return next()
  if (!auth.isLoggedIn) return next('/login')

  const roles = to.meta.roles as string[] | undefined
  if (roles && !roles.includes(auth.user?.role || '')) {
    return next('/dashboard')  // 无权限跳回首页
  }

  next()
})
```

---

## 8. 状态管理

### auth store

```typescript
export const useAuthStore = defineStore('auth', () => {
  const accessToken = ref(localStorage.getItem('access_token') || '')
  const refreshTokenValue = ref(localStorage.getItem('refresh_token') || '')
  const user = ref<User | null>(null)

  const isLoggedIn = computed(() => !!accessToken.value)

  async function fetchUser() {
    const res = await authApi.me()
    user.value = res.data
  }

  async function login(username: string, password: string) {
    const res = await authApi.login({ username, password })
    accessToken.value = res.data.access
    refreshTokenValue.value = res.data.refresh
    localStorage.setItem('access_token', res.data.access)
    localStorage.setItem('refresh_token', res.data.refresh)
    await fetchUser()
  }

  async function refreshToken(): Promise<boolean> {
    if (!refreshTokenValue.value) return false
    const res = await authApi.refresh({ refresh: refreshTokenValue.value })
    accessToken.value = res.data.access
    localStorage.setItem('access_token', res.data.access)
    return true
  }

  function logout() {
    accessToken.value = ''
    refreshTokenValue.value = ''
    user.value = null
    localStorage.removeItem('access_token')
    localStorage.removeItem('refresh_token')
    router.push('/login')
  }

  return { accessToken, user, isLoggedIn, login, fetchUser, refreshToken, logout }
})
```

### alert store

```typescript
export const useAlertStore = defineStore('alert', () => {
  const unacknowledgedCount = ref(0)

  function increment() {
    unacknowledgedCount.value++
  }

  function setCount(count: number) {
    unacknowledgedCount.value = count
  }

  return { unacknowledgedCount, increment, setCount }
})
```

> `unacknowledgedCount` 用于顶栏铃铛角标显示。
> WebSocket `/ws/alerts/` 收到 `alert.triggered` 时调用 `increment()`，
> 用户进入报警列表页时通过 API 查询真实数量后调用 `setCount()` 校准。

---

## 9. 主题定制

### Element Plus CSS 变量覆盖

```css
/* src/styles/variables.css */
:root {
  --el-color-primary: #409eff;          /* 主色 */
  --el-color-success: #67c23a;
  --el-color-warning: #e6a23c;
  --el-color-danger: #f56c6c;
  --el-border-radius-base: 4px;
  --el-font-size-base: 14px;
}

/* 暗色主题（后期可选） */
html.dark {
  --el-bg-color: #141414;
  --el-bg-color-overlay: #1d1d1d;
  --el-text-color-primary: rgba(255, 255, 255, 0.85);
}
```

---

## 10. Docker 容器化

### frontend/Dockerfile

```dockerfile
# ---- Build Stage ----
FROM node:20-alpine AS builder

RUN npm config set registry https://registry.npmmirror.com

WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci
COPY . .
RUN npm run build

# ---- Production Stage ----
FROM nginx:alpine AS production

COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

### frontend/nginx.conf

```nginx
server {
    listen 80;
    server_name _;

    root /usr/share/nginx/html;
    index index.html;

    # SPA history mode — 所有路由回退到 index.html
    location / {
        try_files $uri $uri/ /index.html;
    }

    # API 反向代理到后端
    location /api/ {
        proxy_pass http://backend:8000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Request-ID $request_id;
    }

    # WebSocket 反向代理
    location /ws/ {
        proxy_pass http://backend:8000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_read_timeout 86400s;     # WebSocket 长连接不超时
    }

    # 静态资源缓存
    location /assets/ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }

    # 禁止访问 . 开头的隐藏文件
    location ~ /\. {
        deny all;
    }
}
```

### 独立构建

```bash
# 独立构建
docker build -t ai-stream-frontend ./frontend

# 独立运行（需要后端可达）
docker run --rm -p 80:80 ai-stream-frontend
```

### 项目根目录 docker-compose.yml 中

```yaml
frontend:
  build:
    context: ./frontend
  ports:
    - "80:80"
  depends_on:
    - backend
  restart: unless-stopped
```

---

## 11. 环境变量

通过 Vite 的 `.env` 文件管理：

```bash
# .env.development
VITE_API_BASE_URL=/api/v1

# .env.production
VITE_API_BASE_URL=/api/v1
```

> 生产环境中，前端 Nginx 反向代理 `/api/` 到后端，所以 `VITE_API_BASE_URL` 统一为 `/api/v1`。
> 开发环境使用 Vite 的 `proxy` 配置代理到本地后端。

### Vite 开发代理

```typescript
// vite.config.ts
export default defineConfig({
  server: {
    proxy: {
      '/api': {
        target: 'http://localhost:8000',
        changeOrigin: true,
      },
      '/ws': {
        target: 'ws://localhost:8000',
        ws: true,
      },
    },
  },
})
```

---

## 12. 关键交互流程

### 12.1 登录

```
用户输入用户名密码 → POST /api/v1/auth/login/
  → 成功：存 token 到 localStorage + Pinia，跳转 /dashboard
  → 失败：显示错误信息
```

### 12.2 摄像头启动视频流

```
用户点击「启动」按钮
  → 按钮 loading 状态
  → POST /api/v1/cameras/{id}/start-stream/
  → 成功：摄像头状态变为 connecting（本地立即更新）
  → WebSocket 收到 camera.status_changed(online) → 状态更新为 online
  → 失败：ElMessage.error 提示，按钮恢复
```

### 12.3 报警实时通知

```
WebSocket /ws/alerts/ 收到 alert.triggered
  → ElNotification 弹出报警通知（右上角）
  → 顶栏铃铛图标角标 +1
  → 如果当前在报警列表页，自动刷新列表
```

### 12.4 Token 自动续期

```
任意 API 请求返回 401
  → Axios 拦截器检测 isRefreshing 标志
    → 已有 refresh 在飞：当前请求进入 pendingQueue 等待
    → 无 refresh 在飞：发起 POST /api/v1/auth/refresh/
  → refresh 成功：存新 token，pendingQueue 中所有请求用新 token 重试（用户无感知）
  → refresh 失败：清除 token，pendingQueue 全部 reject，跳转登录页
```

> **为什么需要并发保护**：后端开启了 `BLACKLIST_AFTER_ROTATION`，
> 第二个并发 refresh 请求使用的旧 refresh token 已被拉黑，导致所有请求失败 → 登出。
> `isRefreshing` + Promise 队列确保只发一次 refresh。

### 12.5 分析区域配置

```
用户进入摄像头详情页 → "分析区域配置" 卡片
  → 已有区域在 Canvas 上回显（从 GET /cameras/{id}/analytics-zones/ 加载）

用户点击「+ 添加区域」→ 选择区域类型（ROI/越线/拥挤/方向）
  → Canvas 进入绘制模式
  → ROI: 点击添加多边形顶点 → 双击闭合 → 弹出名称+参数输入框
  → 越线: 点击起点和终点 → 弹出名称输入框
  → 拥挤: 同 ROI 绘制 → 弹出名称+阈值输入框
  → 确认 → POST /cameras/{id}/analytics-zones/
  → 成功：Canvas 更新，区域列表刷新

页面顶部提示："分析区域已修改，需重新部署管道配置才能生效"
  → 用户点击「重新部署」→ POST /pipeline-profiles/{id}/deploy/
  → 同管道配置部署流程（含二次确认）
```

### 12.6 管道配置部署

```
用户在「管道配置」页面创建/编辑管道配置
  → 选择检测器（必选）、跟踪器、视频分析开关
  → 保存 → POST /api/v1/pipeline-profiles/
  → 关联摄像头 → PUT /api/v1/cameras/{id}/pipeline/

用户点击「部署到 DeepStream」
  → 按钮 loading + ElMessageBox.confirm("部署将重启 DeepStream，所有视频流短暂中断，确认？")
  → 确认 → POST /api/v1/pipeline-profiles/{id}/deploy/
  → 成功：ElMessage.success("管道配置已部署")
          → 所有摄像头状态临时变为 connecting
          → WebSocket 收到 camera.status_changed(online) → 状态恢复
  → 失败：ElMessage.error("部署失败: ...") + 按钮恢复
```

### 12.7 实时预览

```
用户进入「实时预览」页面 (/cameras/preview)
  → GET /api/v1/cameras/preview-url/ 获取 WHEP 端点 URL
  → WhepPlayer 组件发起 WebRTC WHEP 协商
  → 默认显示 4×4 多画面总览（tiler show-source=-1）

用户点击某路画面 → 切换单路全分辨率
  → POST /api/v1/deepstream/switch-preview/ { source_id: N }
    → 后端发送 Kafka 命令 { action: "switch_preview", source_id: N }
  → tiler 切换到该路全分辨率，WhepPlayer 画面自动更新

用户点击「返回总览」
  → POST /api/v1/deepstream/switch-preview/ { source_id: -1 }
  → 恢复 4×4 拼接画面
```

> 预览切换延迟约 ~200ms（Kafka 传输 + tiler 切换），低于人眼反应时间，体感无延迟。

---

## 13. 踩坑预防清单

| # | 坑 | 现象 | 解决 |
|---|-----|------|------|
| 1 | WebSocket 断线不重连 | 切换网络后实时数据停止 | `ReconnectingWebSocket` 自动重连 + 指数退避（1s → 2s → 4s → ...，上限 30s） |
| 2 | JWT 过期导致白屏 | 用户操作中突然跳登录页 | Axios 拦截 401 自动 refresh，无感续期 |
| 3 | 大列表渲染卡顿 | 检测记录几千条时页面卡 | 后端分页 + 前端不缓存全量数据 |
| 4 | WebSocket 推送风暴 | 检测结果高频刷新导致 UI 卡顿 | 后端已做聚合推送，前端用 `requestAnimationFrame` 节流渲染 |
| 5 | Nginx SPA 路由 404 | 刷新页面 404 | `try_files $uri $uri/ /index.html` |
| 6 | 开发环境跨域 | API 请求被浏览器拦截 | Vite `proxy` 配置代理 |
| 7 | Element Plus 按需引入 | 打包体积过大 | `unplugin-vue-components` + `unplugin-auto-import`（已在技术栈列入） |
| 8 | 多租户数据串扰 | 前端缓存了 A 组织数据，切换到 B | 切换用户时清空所有 Pinia store |
| 9 | ECharts 容器 resize | 窗口缩放后图表不自适应 | `ResizeObserver` 监听容器变化调用 `chart.resize()` |
| 10 | TypeScript 类型与后端不一致 | API 返回字段改了前端没同步 | 类型定义集中在 `types/` 目录，API 变更时同步更新 |
| 11 | 管道部署未确认就执行 | 用户误点导致所有视频流中断 | `ElMessageBox.confirm` 二次确认 + 明确提示"将重启 DeepStream" |
| 12 | 模型配置表单不随类型切换 | 用户切换 model_type 后表单字段没变 | `watch(modelType)` 动态切换表单 schema，清空旧值 |
| 13 | Canvas 坐标超出范围 | 用户拖拽到 Canvas 外导致坐标为负或超 1920×1080 | `clamp` 坐标到有效范围，绘制时限制在 Canvas 边界内 |
| 14 | 并发 401 触发多次 refresh | 后端开启 `BLACKLIST_AFTER_ROTATION`，第二次 refresh 用旧 token 被拒 → 全量登出 | `isRefreshing` 标志 + Promise 队列，确保只发一次 refresh，其他请求排队等待后重试 |
| 15 | Canvas 坐标系不匹配 | 绘制的 ROI 在 DeepStream 中位置偏移 | Canvas 显示宽度与 1920×1080 配置坐标系做等比映射 |
| 16 | 分析区域修改后忘记重部署 | 修改了 ROI 但 DeepStream 仍用旧配置 | 区域变更后显示持久提示条，强调"需重新部署" |
| 17 | 报警规则选择不存在的区域名 | 规则永远不触发 | 区域名称从摄像头的 AnalyticsZone 动态加载，不允许手动输入 |
| 18 | ZoneDrawer 未销毁 Canvas 事件 | 切换页面后内存泄漏 | `onUnmounted` 中移除所有 Canvas 事件监听 |
| 19 | WebSocket 重连用旧 token | JWT refresh 后 WebSocket 重连仍带旧 token → 4001 关闭 → 永不恢复 | `connect()` 中每次动态获取最新 `accessToken`，不在构造函数缓存 URL |
| 20 | analytics_config_stale 未展示 | 摄像头增删后分析区域作用于错误摄像头，用户不知道需要重部署 | 摄像头列表/详情页检测 `analytics_config_stale=true` 时显示 `ElAlert` 持久警告条 |
| 21 | WebRTC ICE 协商失败 | 内网/NAT 环境下预览无画面 | MediaMTX 配置 STUN/TURN 服务器，或确保前端与 MediaMTX 在同一网络 |
| 22 | WhepPlayer 未释放 PeerConnection | 切换页面后 MediaMTX 仍认为有人观看，`sourceOnDemand` 不生效 | `onUnmounted` 中 `pc.close()` 释放 WebRTC 连接 |
| 23 | 首次加载无 loading 指示 | 页面白屏数秒，用户以为卡死 | 表格/卡片列表统一使用 `v-loading` 指令 + 骨架屏占位 |
