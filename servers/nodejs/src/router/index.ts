import { createRouter, createWebHistory } from 'vue-router'
import ShellPage from '@/pages/shell/Page.vue'
import PlaceholderPage from '@/pages/shared/PlaceholderPage.vue'

const placeholder = (title: string) => ({
  component: PlaceholderPage,
  props: { title },
})

export const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      component: ShellPage,
      redirect: '/streams',
      children: [
        {
          path: 'streams',
          name: 'streams',
          component: () => import('@/pages/streams/Page.vue'),
        },
        {
          path: 'previews',
          name: 'previews',
          component: () => import('@/pages/previews/Page.vue'),
        },
        {
          path: 'recordings',
          name: 'recordings',
          component: () => import('@/pages/recordings/Page.vue'),
        },
        {
          path: 'models',
          name: 'models',
          component: () => import('@/pages/models/Page.vue'),
        },
        {
          path: 'pipelines',
          name: 'pipelines',
          component: () => import('@/pages/pipelines/Page.vue'),
        },
        {
          path: 'servers',
          name: 'servers',
          component: () => import('@/pages/servers/Page.vue'),
        },
        { path: 'warnings', name: 'warnings', ...placeholder('Warnings') },
        { path: 'users', name: 'users', ...placeholder('Users') },
      ],
    },
  ],
})
