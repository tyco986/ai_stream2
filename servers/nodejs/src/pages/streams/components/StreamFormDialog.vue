<template>
  <UiDialog
    :model-value="modelValue"
    :title="title"
    width="520px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="form">
      <label class="form__row">
        <span>{{ t('streams.name') }}</span>
        <UiInput :model-value="form.name" @update:model-value="form.name = $event" />
      </label>
      <label class="form__row">
        <span>{{ t('streams.group') }}</span>
        <UiSelect
          :model-value="form.group_id"
          :options="groupOptions"
          @update:model-value="form.group_id = $event"
        />
      </label>
      <label class="form__row">
        <span>{{ t('streams.url') }}</span>
        <UiInput :model-value="form.url" @update:model-value="form.url = $event" />
      </label>
      <div class="form__row">
        <span>{{ t('streams.enabled') }}</span>
        <UiRadioGroup
          :model-value="form.enabled"
          :options="onOffOptions"
          @update:model-value="form.enabled = Boolean($event)"
        />
      </div>
      <div class="form__row">
        <span>{{ t('streams.recording') }}</span>
        <UiRadioGroup
          :model-value="form.recording"
          :options="onOffOptions"
          @update:model-value="form.recording = Boolean($event)"
        />
      </div>
      <div class="form__test">
        <UiButton type="primary" :loading="testing" @click="emit('test')">
          {{ t('streams.test') }}
        </UiButton>
      </div>
      <div v-if="testInfo" class="form__test-info">{{ testInfo }}</div>
      <div v-if="error" class="form__error">{{ error }}</div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('streams.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="emit('save')">
        {{ t('streams.save') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, reactive, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Group, Stream } from '@/api/streams'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiRadioGroup from '@/shared/ui/RadioGroup.vue'
import UiSelect from '@/shared/ui/Select.vue'

const props = defineProps<{
  modelValue: boolean
  mode: 'add' | 'edit'
  stream: Stream | null
  groups: Group[]
  defaultGroupId: string
  saving: boolean
  testing: boolean
  testInfo: string
  error: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: []
  test: []
  'update:form': [
    form: {
      name: string
      group_id: string
      url: string
      enabled: boolean
      recording: boolean
    },
  ]
}>()

const { t } = useI18n()

const form = reactive({
  name: '',
  group_id: '',
  url: '',
  enabled: true,
  recording: false,
})

const title = computed(() =>
  props.mode === 'add' ? t('streams.addStream') : t('streams.editStream'),
)

const groupOptions = computed(() =>
  props.groups.map((group) => ({ label: group.name, value: group.id })),
)

const onOffOptions = computed(() => [
  { label: t('streams.on'), value: true },
  { label: t('streams.off'), value: false },
])

watch(
  () => [props.modelValue, props.stream, props.mode, props.defaultGroupId] as const,
  () => {
    if (!props.modelValue) {
      return
    }
    if (props.mode === 'edit' && props.stream) {
      form.name = props.stream.name
      form.group_id = props.stream.group_id
      form.url = props.stream.url
      form.enabled = props.stream.enabled
      form.recording = props.stream.recording
      return
    }
    form.name = ''
    form.group_id = props.defaultGroupId
    form.url = ''
    form.enabled = true
    form.recording = false
  },
)

watch(
  form,
  () => {
    emit('update:form', { ...form })
  },
  { deep: true },
)
</script>

<style scoped>
.form {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.form__row {
  display: grid;
  grid-template-columns: 90px 1fr;
  align-items: center;
  gap: 8px;
}

.form__test :deep(.el-button) {
  width: 100%;
}

.form__test-info {
  color: #67c23a;
  font-size: 13px;
}

.form__error {
  color: #f56c6c;
  font-size: 13px;
}
</style>
