export type NavItem = {
  label: string
  path: string
  viewPerm: string
}

export const NAV_ITEMS: NavItem[] = [
  { label: 'Streams', path: '/streams', viewPerm: 'streams.view_stream' },
  { label: 'Previews', path: '/previews', viewPerm: 'previews.view_preview' },
  {
    label: 'Recordings',
    path: '/recordings',
    viewPerm: 'recordings.view_recording',
  },
  { label: 'Models', path: '/models', viewPerm: 'models.view_model' },
  {
    label: 'Pipelines',
    path: '/pipelines',
    viewPerm: 'pipelines.view_pipeline',
  },
  { label: 'Servers', path: '/servers', viewPerm: 'servers.view_server' },
  { label: 'Events', path: '/events', viewPerm: 'events.view_event' },
  { label: 'Users', path: '/users', viewPerm: 'users.view_user' },
]
