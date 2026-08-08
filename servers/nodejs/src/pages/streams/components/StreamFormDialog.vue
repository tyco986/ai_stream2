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
        <UiInput
          :model-value="form.name"
          :invalid="nameInvalid"
          @update:model-value="form.name = $event"
        />
      </label>
      <label class="form__row">
        <span>{{ t('streams.group') }}</span>
        <UiSelect
          :model-value="form.group_id"
          :options="groupOptions"
          @update:model-value="form.group_id = $event"
        />
      </label>
      <div class="form__row">
        <span>{{ t('streams.url') }}</span>
        <div class="form__url">
          <UiInput
            :model-value="form.url"
            :invalid="urlInvalid"
            @update:model-value="form.url = $event"
          />
          <UiButton
            class="form__probe-btn"
            type="primary"
            :disabled="!form.url.trim()"
            :loading="probing"
            @click="emit('probe')"
          >
            <span v-if="!probing">{{ t('streams.probe') }}</span>
          </UiButton>
        </div>
      </div>
      <div class="form__row">
        <span>{{ t('streams.info') }}</span>
        <div class="form__info">
          <UiInput
            :model-value="infoWidth"
            :placeholder="t('streams.width')"
            disabled
            :clearable="false"
          />
          <UiInput
            :model-value="infoHeight"
            :placeholder="t('streams.height')"
            disabled
            :clearable="false"
          />
          <UiInput
            :model-value="infoFps"
            :placeholder="t('streams.fps')"
            disabled
            :clearable="false"
          />
        </div>
      </div>
      <div class="form__row">
        <span>{{ t('streams.enabled') }}</span>
        <UiRadioGroup
          :model-value="form.enabled"
          :options="onOffOptions"
          @update:model-value="onEnabledUpdate"
        />
      </div>
      <div class="form__row">
        <span>{{ t('streams.recording') }}</span>
        <UiRadioGroup
          :model-value="form.recording"
          :options="onOffOptions"
          :disabled="!form.enabled"
          @update:model-value="form.recording = Boolean($event)"
        />
      </div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('streams.cancel') }}</UiButton>
      <UiButton
        class="form__save-btn"
        type="primary"
        :loading="saving"
        @click="emit('save')"
      >
        <span v-if="!saving">{{ t('streams.save') }}</span>
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
  probing: boolean
  nameInvalid: boolean
  urlInvalid: boolean
  probe: { resolution: string | null; fps: number | null } | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: []
  probe: []
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

const infoWidth = computed(() => {
  const match = /^(\d+)\s*[x×]\s*(\d+)$/i.exec(props.probe?.resolution?.trim() || '')
  return match ? match[1] : ''
})

const infoHeight = computed(() => {
  const match = /^(\d+)\s*[x×]\s*(\d+)$/i.exec(props.probe?.resolution?.trim() || '')
  return match ? match[2] : ''
})

const infoFps = computed(() => {
  const fps = props.probe?.fps
  return fps == null ? '' : String(fps)
})

function onEnabledUpdate(value: string | boolean | number) {
  form.enabled = Boolean(value)
  if (!form.enabled) {
    form.recording = false
  }
}

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

.form__url {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
}

.form__url .ui-input {
  flex: 1;
  min-width: 0;
}

.form__url :deep(.el-button) {
  flex-shrink: 0;
}

.form__probe-btn {
  min-width: 72px;
}

.form__info {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  min-width: 0;
}

.form__save-btn {
  min-width: 72px;
}
</style>
