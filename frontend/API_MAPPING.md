# API 调用映射表

所有 `@API:XXX` 标记与后端 API 端点的对应关系。在页面源码中搜索标记即可定位调用点。

## 认证

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:AUTH_LOGIN | POST | /api/v1/auth/login/ | 用户登录 | LoginView.vue |

## 仪表盘

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:DASH_OVERVIEW | GET | /api/v1/dashboard/overview/ | 总览统计（在线摄像头数、检测数、报警数） | DashboardView.vue |
| @API:DASH_TREND | GET | /api/v1/dashboard/detection-trend/ | 检测趋势（时间序列） | DashboardView.vue |
| @API:DASH_STATUS | GET | /api/v1/dashboard/camera-status/ | 摄像头状态统计 | DashboardView.vue |

## 摄像头

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:CAM_LIST | GET | /api/v1/cameras/ | 摄像头列表 | CameraListView.vue |
| @API:CAM_CREATE | POST | /api/v1/cameras/ | 创建摄像头 | CameraListView.vue |
| @API:CAM_START | POST | /api/v1/cameras/{id}/start-stream/ | 启动视频流 | CameraListView.vue |
| @API:CAM_STOP | POST | /api/v1/cameras/{id}/stop-stream/ | 停止视频流 | CameraListView.vue |
| @API:CAM_DETAIL | GET | /api/v1/cameras/{id}/ | 摄像头详情 | CameraDetailView.vue |
| @API:CAM_PIPELINE_GET | GET | /api/v1/cameras/{id}/pipeline/ | 查看摄像头管道配置 | CameraDetailView.vue |
| @API:CAM_PIPELINE_SET | PUT | /api/v1/cameras/{id}/pipeline/ | 设置摄像头管道配置 | CameraDetailView.vue |

## 分析区域

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:ZONE_LIST | GET | /api/v1/cameras/{id}/analytics-zones/ | 分析区域列表 | CameraDetailView.vue |
| @API:ZONE_CREATE | POST | /api/v1/cameras/{id}/analytics-zones/ | 创建分析区域 | CameraDetailView.vue |

## 检测记录

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:DET_LIST | GET | /api/v1/detections/ | 检测记录列表 | DetectionListView.vue |
| @API:DET_RECENT | GET | /api/v1/detections/?camera_id={id} | 某摄像头最近检测 | CameraDetailView.vue |

## AI 模型

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:MODEL_LIST | GET | /api/v1/ai-models/ | AI 模型列表 | AIModelListView.vue |
| @API:MODEL_CREATE | POST | /api/v1/ai-models/ | 注册 AI 模型 | AIModelListView.vue |
| @API:MODEL_UPDATE | PATCH | /api/v1/ai-models/{id}/ | 更新模型信息 | AIModelListView.vue |
| @API:MODEL_DELETE | DELETE | /api/v1/ai-models/{id}/ | 删除模型 | AIModelListView.vue |

## 管道配置

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:PIPE_LIST | GET | /api/v1/pipeline-profiles/ | 管道配置列表 | PipelineProfileView.vue |
| @API:PIPE_CREATE | POST | /api/v1/pipeline-profiles/ | 创建管道配置 | PipelineProfileView.vue |
| @API:PIPE_UPDATE | PATCH | /api/v1/pipeline-profiles/{id}/ | 更新管道配置 | PipelineProfileView.vue |
| @API:PIPE_DELETE | DELETE | /api/v1/pipeline-profiles/{id}/ | 删除管道配置 | PipelineProfileView.vue |
| @API:PIPE_DEPLOY | POST | /api/v1/pipeline-profiles/{id}/deploy/ | 部署到 DeepStream | PipelineProfileView.vue |

## 录像

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:REC_LIST | GET | /api/v1/recordings/ | 录像列表 | RecordingListView.vue |
| @API:REC_DOWNLOAD | GET | /api/v1/recordings/{id}/download/ | 下载录像文件 | RecordingListView.vue |
| @API:REC_STREAM | GET | /api/v1/recordings/{id}/stream/ | 流式播放录像 | RecordingListView.vue |

## 截图

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:SS_LIST | GET | /api/v1/screenshots/ | 截图列表 | ScreenshotListView.vue |
| @API:SS_DOWNLOAD | GET | /api/v1/screenshots/{id}/download/ | 下载截图 | ScreenshotListView.vue |

## 报警规则

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:RULE_LIST | GET | /api/v1/alert-rules/ | 报警规则列表 | AlertRuleListView.vue |
| @API:RULE_CREATE | POST | /api/v1/alert-rules/ | 创建报警规则 | AlertRuleListView.vue |
| @API:RULE_UPDATE | PATCH | /api/v1/alert-rules/{id}/ | 更新报警规则 | AlertRuleListView.vue |
| @API:RULE_DELETE | DELETE | /api/v1/alert-rules/{id}/ | 删除报警规则 | AlertRuleListView.vue |
| @API:RULE_TOGGLE | PATCH | /api/v1/alert-rules/{id}/ | 启用/禁用规则 | AlertRuleListView.vue |

## 报警记录

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:ALERT_LIST | GET | /api/v1/alerts/ | 报警记录列表 | AlertListView.vue |
| @API:ALERT_ACK | POST | /api/v1/alerts/{id}/acknowledge/ | 确认报警 | AlertListView.vue |
| @API:ALERT_RESOLVE | POST | /api/v1/alerts/{id}/resolve/ | 解决报警 | AlertListView.vue |

## DeepStream 代理

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:DS_PREVIEW_URL | GET | /api/v1/deepstream/preview-url/ | 获取预览 WHEP 端点 | CameraPreviewView.vue |
| @API:DS_SWITCH_PREVIEW | POST | /api/v1/deepstream/switch-preview/ | 切换单路/总览预览 | CameraPreviewView.vue |
| @API:DS_START_REC | POST | /api/v1/deepstream/start-recording/ | 开始手动录制 | CameraDetailView.vue |
| @API:DS_STOP_REC | POST | /api/v1/deepstream/stop-recording/ | 停止手动录制 | CameraDetailView.vue |
| @API:DS_SCREENSHOT | POST | /api/v1/deepstream/screenshot/ | 手动截图 | CameraDetailView.vue |

## 用户管理

| 标记 | 方法 | 端点 | 说明 | 所在文件 |
|------|------|------|------|---------|
| @API:USER_LIST | GET | /api/v1/users/ | 用户列表 | UserListView.vue |
| @API:USER_UPDATE | PATCH | /api/v1/users/{id}/ | 更新用户角色 | UserListView.vue |
