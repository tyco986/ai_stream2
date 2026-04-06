import { createRouter, createWebHistory } from 'vue-router'
import type { RouteRecordRaw } from 'vue-router'

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
        meta: { title: '仪表盘' },
      },
      {
        path: 'cameras',
        component: () => import('@/views/cameras/CameraListView.vue'),
        meta: { title: '摄像头列表' },
      },
      {
        path: 'cameras/preview',
        component: () => import('@/views/cameras/CameraPreviewView.vue'),
        meta: { title: '实时预览' },
      },
      {
        path: 'cameras/:id',
        component: () => import('@/views/cameras/CameraDetailView.vue'),
        meta: { title: '摄像头详情' },
      },
      {
        path: 'detections',
        component: () => import('@/views/detections/DetectionListView.vue'),
        meta: { title: '检测记录' },
      },
      {
        path: 'recordings',
        component: () => import('@/views/recordings/RecordingListView.vue'),
        meta: { title: '录像回放' },
      },
      {
        path: 'screenshots',
        component: () => import('@/views/screenshots/ScreenshotListView.vue'),
        meta: { title: '截图管理' },
      },
      {
        path: 'pipelines/models',
        component: () => import('@/views/pipelines/AIModelListView.vue'),
        meta: { title: 'AI 模型管理' },
      },
      {
        path: 'pipelines/profiles',
        component: () => import('@/views/pipelines/PipelineProfileView.vue'),
        meta: { title: '管道配置' },
      },
      {
        path: 'alert-rules',
        component: () => import('@/views/alerts/AlertRuleListView.vue'),
        meta: { title: '报警规则' },
      },
      {
        path: 'alerts',
        component: () => import('@/views/alerts/AlertListView.vue'),
        meta: { title: '报警记录' },
      },
      {
        path: 'system/users',
        component: () => import('@/views/system/UserListView.vue'),
        meta: { title: '用户管理' },
      },
    ],
  },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
