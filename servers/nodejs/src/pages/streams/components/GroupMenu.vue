<template>
  <UiDropdown :items="items" @command="emit('command', $event)">
    <template #trigger>⋮</template>
  </UiDropdown>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import UiDropdown from '@/shared/ui/Dropdown.vue'

const props = defineProps<{
  canDelete: boolean
  canRename: boolean
}>()

const emit = defineEmits<{
  command: [command: string]
}>()

const { t } = useI18n()

const items = computed(() => [
  { command: 'add-stream', label: t('streams.addStream') },
  { command: 'add-group', label: t('streams.addGroup') },
  { command: 'rename', label: t('streams.rename'), disabled: !props.canRename },
  { command: 'delete', label: t('streams.delete'), disabled: !props.canDelete },
])
</script>
