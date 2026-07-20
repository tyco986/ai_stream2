<template>
  <div v-if="loaded" class="servers-page">
    <header class="servers-page__title">{{ t('servers.title') }}</header>
    <section class="servers-page__main">
      <ServersToolbar :refreshing="refreshing" @refresh="onRefresh" />
      <ServersTable
        :rows="rows"
        :restarting-ids="restartingIds"
        @restart="openRestart"
        @log="openLog"
      />
    </section>

    <ConfirmRestartDialog
      v-model="confirmOpen"
      :name="confirmName"
      :loading="confirmLoading"
      @confirm="onConfirmRestart"
    />

    <ServerLogDialog v-model="logOpen" :content="logContent" />
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Server } from '@/api/servers'
import { ApiError } from '@/shared/http/client'
import { toast } from '@/shared/ui/toast'
import ConfirmRestartDialog from './components/ConfirmRestartDialog.vue'
import ServerLogDialog from './components/ServerLogDialog.vue'
import ServersTable from './components/ServersTable.vue'
import ServersToolbar from './components/ServersToolbar.vue'
import { useServersPage } from './composables/useServersPage'

const { t } = useI18n()
const pageApi = useServersPage()
const {
  rows,
  loaded,
  refreshing,
  restartingIds,
  bootstrap,
  refreshAll,
  markRestarting,
  applyRefreshResults,
  restartServer,
  getServerLogs,
} = pageApi

const confirmOpen = ref(false)
const confirmLoading = ref(false)
const confirmName = ref('')
const confirmId = ref('')

const logOpen = ref(false)
const logContent = ref('')

onMounted(async () => {
  await bootstrap()
})

async function onRefresh() {
  await refreshAll()
}

function openRestart(row: Server) {
  confirmId.value = row.id
  confirmName.value = row.name
  confirmOpen.value = true
}

async function onConfirmRestart() {
  const id = confirmId.value
  const name = confirmName.value
  confirmLoading.value = true
  markRestarting(id, true)
  try {
    const result = await restartServer(id)
    if (result.refresh) {
      applyRefreshResults([result.refresh])
    }
    toast.success(t('servers.toastRestarted', { name }))
    confirmOpen.value = false
  } catch (err) {
    const message =
      err instanceof ApiError
        ? err.message
        : t('servers.toastRestartFailed', { name })
    toast.error(message)
  }
  markRestarting(id, false)
  confirmLoading.value = false
}

async function openLog(row: Server) {
  const result = await getServerLogs(row.id)
  logContent.value = result.content
  logOpen.value = true
}
</script>

<style scoped>
.servers-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
}

.servers-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.servers-page__main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  background: #fff;
}
</style>
