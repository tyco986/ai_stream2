<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.gieEditorTitle')"
    width="1200px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="form">
      <label class="form__row">
        <span>{{ t('pipelines.name') }}</span>
        <UiInput
          :model-value="name"
          :invalid="nameInvalid"
          @update:model-value="onNameChange"
        />
      </label>
      <label class="form__row">
        <span>{{ t('pipelines.model') }}</span>
        <UiSelect
          :model-value="modelId"
          :options="modelOptions"
          :placeholder="t('pipelines.selectModel')"
          @update:model-value="onModelChange"
        />
      </label>
      <div class="form__table">
        <UiTable :data="rows" row-key="key">
          <UiTableColumn prop="classSelect" :label="t('pipelines.classIdLabel')" min-width="200">
            <template #default="{ row }">
              <GieClassSelect
                :model-value="row.classSelect"
                :options="optionsForRow(row)"
                :disabled="!modelId"
                :invalid="invalidClassKeys.has(row.key)"
                :placeholder="t('pipelines.selectClass')"
                clearable
                @update:model-value="onClassChange(row, $event)"
              />
            </template>
          </UiTableColumn>
          <UiTableColumn prop="conf" :label="t('pipelines.confidence')" min-width="110">
            <template #default="{ row }">
              <UiInput v-model="row.conf" />
            </template>
          </UiTableColumn>
          <UiTableColumn prop="topk" :label="t('pipelines.topK')" min-width="90">
            <template #default="{ row }">
              <UiInput v-model="row.topk" />
            </template>
          </UiTableColumn>
          <UiTableColumn
            v-for="column in detectedColumns"
            :key="column.field"
            :prop="column.field"
            :label="t(column.labelKey)"
            min-width="80"
          >
            <template #default="{ row }">
              <UiInput
                :model-value="row[column.field]"
                @update:model-value="row[column.field] = $event"
                @blur="onDetectedBlur(row, column.field)"
              />
            </template>
          </UiTableColumn>
          <UiTableColumn :label="t('pipelines.operations')" width="110">
            <template #default="{ $index }">
              <button type="button" class="ops__btn" @click="rows.splice($index, 1)">
                <UiIcon name="delete" :size="16" />
              </button>
            </template>
          </UiTableColumn>
        </UiTable>
      </div>
      <UiButton class="form__add" @click="addRow">+</UiButton>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('pipelines.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="onSave">{{ t('pipelines.save') }}</UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { listModels, type Model } from '@/api/models'
import { createGieTemplate, getGieTemplate, updateGieTemplate } from '@/api/pipelines'
import { ApiError } from '@/shared/http/client'
import {
  ALL_CLASS_SELECT,
  DETECTED_FIELDS,
  GieClassAttrsForm,
  GieClassAttrsFormat,
  type ClassAttrFormRow,
  type ClassSelectOption,
  type DetectedField,
} from '@/shared/gie/classAttrs'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiIcon from '@/shared/ui/Icon.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiSelect from '@/shared/ui/Select.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import { toast } from '@/shared/ui/toast'
import GieClassSelect from './GieClassSelect.vue'

const DETECTED_LABEL_KEYS: Record<DetectedField, string> = {
  detected_min_w: 'pipelines.detectedMinW',
  detected_min_h: 'pipelines.detectedMinH',
  detected_max_w: 'pipelines.detectedMaxW',
  detected_max_h: 'pipelines.detectedMaxH',
}

const props = defineProps<{
  modelValue: boolean
  mode: 'add' | 'edit'
  gieId: string | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  saved: []
}>()

const { t } = useI18n()
const name = ref('')
const modelId = ref('')
const models = ref<Model[]>([])
const rows = ref<ClassAttrFormRow[]>([])
const saving = ref(false)
const nameInvalid = ref(false)
const invalidClassKeys = ref(new Set<string>())
const formatter = new GieClassAttrsFormat()

const detectedColumns = DETECTED_FIELDS.map((field) => ({
  field,
  labelKey: DETECTED_LABEL_KEYS[field],
}))

const modelOptions = computed(() =>
  models.value.map((item) => ({ label: item.name, value: item.id })),
)

const selectedModel = computed(
  () => models.value.find((item) => item.id === modelId.value) ?? null,
)

const classOptions = computed(() => formatter.selectOptions(selectedModel.value))

watch(
  () => props.modelValue,
  async (open) => {
    if (!open) {
      return
    }
    nameInvalid.value = false
    invalidClassKeys.value = new Set()
    const result = await listModels({ page: 1, page_size: 100 })
    models.value = result.items.filter((item) => item.status === 'built')
    if (props.mode === 'edit' && props.gieId) {
      const gie = await getGieTemplate(props.gieId)
      name.value = gie.name
      modelId.value = gie.model_id
      syncFormatterLabels()
      rows.value = gie.class_attrs.map((row, index) =>
        GieClassAttrsForm.fromApi(row, `r-${index}`),
      )
    } else {
      name.value = ''
      modelId.value = models.value[0]?.id ?? ''
      syncFormatterLabels()
      rows.value = [GieClassAttrsForm.createRow('r-0', ALL_CLASS_SELECT)]
    }
  },
)

watch(selectedModel, () => {
  syncFormatterLabels()
})

function syncFormatterLabels() {
  formatter.setClasses(selectedModel.value?.classes ?? [])
}

function optionsForRow(row: ClassAttrFormRow): ClassSelectOption[] {
  const taken = new Set(
    rows.value
      .filter((item) => item.key !== row.key && item.classSelect)
      .map((item) => item.classSelect),
  )
  return classOptions.value.map((option) => ({
    ...option,
    disabled: taken.has(option.value) && option.value !== row.classSelect,
  }))
}

function onClassChange(row: ClassAttrFormRow, value: string) {
  row.classSelect = value
  if (value) {
    const next = new Set(invalidClassKeys.value)
    next.delete(row.key)
    invalidClassKeys.value = next
  }
}

function onModelChange(value: string) {
  modelId.value = value
  syncFormatterLabels()
  const valid = new Set(classOptions.value.map((item) => item.value))
  let cleared = false
  rows.value = rows.value.map((row) => {
    if (!row.classSelect || row.classSelect === ALL_CLASS_SELECT || valid.has(row.classSelect)) {
      return row
    }
    cleared = true
    return { ...row, classSelect: '' }
  })
  if (cleared) {
    toast.warning(t('pipelines.classInvalidForModel'))
  }
}

function onNameChange(value: string) {
  name.value = value
  nameInvalid.value = false
}

function onDetectedBlur(row: ClassAttrFormRow, field: DetectedField) {
  row[field] = GieClassAttrsForm.commitDetectedText(row[field])
}

function addRow() {
  rows.value.push(GieClassAttrsForm.createRow(`r-${Date.now()}`, ''))
}

async function onSave() {
  nameInvalid.value = !name.value.trim()
  invalidClassKeys.value = new Set(
    rows.value.filter((row) => !GieClassAttrsForm.isClassSelected(row)).map((row) => row.key),
  )
  if (nameInvalid.value || invalidClassKeys.value.size > 0 || rows.value.length === 0) {
    return
  }
  saving.value = true
  const body = {
    name: name.value.trim(),
    model_id: modelId.value,
    class_attrs: rows.value.map((row) => GieClassAttrsForm.toApi(row)),
  }
  try {
    if (props.mode === 'edit' && props.gieId) {
      await updateGieTemplate(props.gieId, body)
    } else {
      await createGieTemplate(body)
    }
    emit('saved')
  } catch (err) {
    const message = err instanceof ApiError ? err.message : String(err)
    if (err instanceof ApiError && err.status === 409) {
      nameInvalid.value = true
    }
    toast.error(message)
  }
  saving.value = false
}
</script>

<style scoped>
.form {
  display: flex;
  flex-direction: column;
  gap: 12px;
  width: 100%;
}

.form__row {
  display: grid;
  grid-template-columns: 90px 1fr;
  align-items: center;
  gap: 8px;
}

.form__table {
  width: 100%;
}

.form__table :deep(.el-table) {
  width: 100% !important;
}

.form__add {
  width: 100%;
}

.ops__btn {
  border: none;
  background: transparent;
  color: #f56c6c;
  cursor: pointer;
}
</style>
