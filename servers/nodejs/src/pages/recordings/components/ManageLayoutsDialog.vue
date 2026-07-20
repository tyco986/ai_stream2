<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('recordings.edit')"
    width="720px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="manage__toolbar">
      <UiButton
        type="danger"
        :disabled="selectedIds.length === 0"
        @click="emit('batch-delete', selectedIds)"
      >
        {{ t('recordings.delete') }}
      </UiButton>
      <div class="manage__search">
        <UiInput
          :model-value="search"
          :placeholder="t('recordings.search')"
          @update:model-value="emit('update:search', $event)"
        />
      </div>
    </div>
    <UiTable :data="rows" row-key="id" @selection-change="onSelectionChange">
      <UiTableColumn type="selection" />
      <UiTableColumn prop="name" :label="t('recordings.name')" min-width="140" />
      <UiTableColumn prop="stream_count" :label="t('recordings.streams')" width="90" />
      <UiTableColumn :label="t('recordings.grid')" width="80">
        <template #default="{ row }">
          {{ row.layout.replace('x', '×') }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('recordings.operations')" width="100">
        <template #default="{ row }">
          <div class="manage__ops">
            <button
              type="button"
              class="manage__btn"
              :disabled="row.id === defaultId"
              :title="t('recordings.rename')"
              @click="emit('rename', row)"
            >
              <UiIcon name="edit" :size="16" />
            </button>
            <button
              type="button"
              class="manage__btn is-danger"
              :disabled="row.id === defaultId"
              :title="t('recordings.delete')"
              @click="emit('delete', row)"
            >
              <UiIcon name="delete" :size="16" />
            </button>
          </div>
        </template>
      </UiTableColumn>
    </UiTable>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import {
  DEFAULT_LAYOUT_ID,
  type LayoutPresetSummary,
} from '@/api/recordings'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiIcon from '@/shared/ui/Icon.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'

const props = defineProps<{
  modelValue: boolean
  items: LayoutPresetSummary[]
  search: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'update:search': [value: string]
  rename: [row: LayoutPresetSummary]
  delete: [row: LayoutPresetSummary]
  'batch-delete': [ids: string[]]
}>()

const { t } = useI18n()
const selectedIds = ref<string[]>([])
const defaultId = DEFAULT_LAYOUT_ID

const rows = computed(() => props.items)

watch(
  () => props.modelValue,
  () => {
    selectedIds.value = []
  },
)

function onSelectionChange(rowsSelected: LayoutPresetSummary[]) {
  selectedIds.value = rowsSelected
    .filter((row) => row.id !== DEFAULT_LAYOUT_ID)
    .map((row) => row.id)
}
</script>

<style scoped>
.manage__toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
}

.manage__search {
  width: 220px;
}

.manage__ops {
  display: flex;
  gap: 4px;
}

.manage__btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  border: none;
  border-radius: 4px;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.manage__btn:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.manage__btn.is-danger {
  color: #f56c6c;
}
</style>
