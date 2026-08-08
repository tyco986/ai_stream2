import { createRouter, createWebHistory } from 'vue-router'
import { getMe, isLoggedIn } from '@/api/shell'
import ShellPage from '@/pages/shell/Page.vue'

export const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/login',
      name: 'login',
      component: () => import('@/pages/login/Page.vue'),
      meta: { public: true },
    },
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
        { path: 'events', name: 'events', component: () => import('@/pages/events/Page.vue') },
        { path: 'users', name: 'users', component: () => import('@/pages/users/Page.vue') },
      ],
    },
  ],
})

router.beforeEach(async (to) => {
  if (to.meta.public) {
    return true
  }
  // Avoid awaiting /shell/me while media Range requests saturate the host
  // connection pool (blocks navigation from Recordings playback).
  if (isLoggedIn()) {
    return true
  }
  try {
    await getMe()
    return true
  } catch {
    return {
      name: 'login',
      query: { redirect: to.fullPath },
    }
  }
})
