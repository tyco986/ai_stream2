export interface NavGroup {
  key: string
  label: string
  icon: string
  defaultPath: string
  children: { path: string; label: string; icon: string }[]
}

export const navGroups: NavGroup[] = [
  {
    key: 'monitor',
    label: '监控中心',
    icon: 'DataAnalysis',
    defaultPath: '/dashboard',
    children: [],
  },
  {
    key: 'cameras',
    label: '摄像头',
    icon: 'VideoCamera',
    defaultPath: '/cameras',
    children: [
      { path: '/cameras', label: '摄像头列表', icon: 'List' },
      { path: '/cameras/preview', label: '实时预览', icon: 'Monitor' },
    ],
  },
  {
    key: 'pipelines',
    label: 'AI 管道',
    icon: 'Cpu',
    defaultPath: '/pipelines/models',
    children: [
      { path: '/pipelines/models', label: '模型管理', icon: 'Box' },
      { path: '/pipelines/profiles', label: '管道配置', icon: 'Setting' },
    ],
  },
  {
    key: 'data',
    label: '数据查询',
    icon: 'Search',
    defaultPath: '/detections',
    children: [
      { path: '/detections', label: '检测记录', icon: 'Search' },
      { path: '/recordings', label: '录像回放', icon: 'Film' },
      { path: '/screenshots', label: '截图管理', icon: 'Camera' },
    ],
  },
  {
    key: 'alerts',
    label: '报警',
    icon: 'Bell',
    defaultPath: '/alert-rules',
    children: [
      { path: '/alert-rules', label: '报警规则', icon: 'Setting' },
      { path: '/alerts', label: '报警记录', icon: 'Bell' },
    ],
  },
  {
    key: 'system',
    label: '系统',
    icon: 'UserFilled',
    defaultPath: '/system/users',
    children: [
      { path: '/system/users', label: '用户管理', icon: 'User' },
    ],
  },
]
