<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('users.userLog', { name })"
    width="520px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <ul v-if="entries.length" class="log-list">
      <li v-for="entry in entries" :key="entry.id">
        {{ formatLogAt(entry.at) }}  {{ entry.detail }}
      </li>
    </ul>
    <div v-else class="log-empty">{{ t('users.dash') }}</div>
    <template #footer>
      <UiButton type="primary" @click="emit('update:modelValue', false)">
        {{ t('users.close') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import { formatLogAt, type UserLogEntry } from '@/api/users'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'

defineProps<{
  modelValue: boolean
  name: string
  entries: UserLogEntry[]
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
</script>

<style scoped>
.log-list {
  margin: 0;
  padding: 0;
  list-style: none;
  max-height: 360px;
  overflow: auto;
  font-size: 13px;
  color: #606266;
}

.log-list li {
  padding: 6px 0;
  border-bottom: 1px solid #ebeef5;
}

.log-empty {
  color: #909399;
}
</style>
