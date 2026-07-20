<template>
  <div v-if="loaded" class="shell" :class="`shell--${mode}`">
    <aside v-if="mode === 'sidebar'" class="shell__sidebar">
      <div class="shell__logo">{{ projectName }}</div>
      <AppNav layout="sidebar" />
      <UserArea :username="username" @open-settings="settingsOpen = true" />
    </aside>

    <header v-else class="shell__topbar">
      <AppNav layout="topbar" />
      <UserArea :username="username" @open-settings="settingsOpen = true" />
    </header>

    <main class="shell__content">
      <RouterView />
    </main>

    <PageSettingsDialog
      v-model="settingsOpen"
      :applied-mode="mode"
      :applying="applying"
      @apply="onApply"
      @cancel="settingsOpen = false"
    />
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { RouterView } from 'vue-router'
import { useI18n } from 'vue-i18n'
import type { ShellMode } from '@/api/shell'
import { ApiError } from '@/shared/http/client'
import { PROJECT_NAME } from '@/shared/project'
import { toast } from '@/shared/ui/toast'
import AppNav from './components/AppNav.vue'
import PageSettingsDialog from './components/PageSettingsDialog.vue'
import UserArea from './components/UserArea.vue'
import { useShellSettings } from './composables/useShellSettings'

const { t } = useI18n()
const projectName = PROJECT_NAME
const { mode, username, loaded, bootstrap, applySettings } = useShellSettings()
const settingsOpen = ref(false)
const applying = ref(false)

onMounted(() => {
  bootstrap()
})

async function onApply(draftMode: ShellMode) {
  applying.value = true
  try {
    await applySettings({ mode: draftMode })
    settingsOpen.value = false
  } catch (err) {
    const message =
      err instanceof ApiError ? err.message : t('shell.applyFailed')
    toast.error(message || t('shell.applyFailed'))
  }
  applying.value = false
}
</script>

<style scoped>
.shell {
  display: flex;
  height: 100%;
  min-height: 100vh;
  background: #f5f7fa;
}

.shell--sidebar {
  flex-direction: row;
}

.shell--topbar {
  flex-direction: column;
}

.shell__sidebar {
  width: 200px;
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  background: #fff;
  border-right: 1px solid #e4e7ed;
}

.shell__logo {
  padding: 16px 12px;
  font-weight: 700;
  font-size: 16px;
  color: #303133;
  border-bottom: 1px solid #ebeef5;
}

.shell__topbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 0 8px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  min-height: 52px;
  flex-shrink: 0;
}

.shell__content {
  flex: 1;
  min-width: 0;
  min-height: 0;
  overflow: auto;
  background: #fff;
}
</style>
