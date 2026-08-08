<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('streams.publishVideo')"
    width="520px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="form">
      <label class="form__row">
        <span>{{ t('streams.name') }}</span>
        <UiInput
          :model-value="form.name"
          :placeholder="t('streams.publishNameHint')"
          @update:model-value="form.name = $event"
        />
      </label>
      <div class="form__row">
        <span>{{ t('streams.file') }}</span>
        <div class="form__file">
          <UiButton type="primary" @click="onPickFile">
            {{ t('streams.chooseVideo') }}
          </UiButton>
          <span class="form__file-name" :class="{ 'is-empty': !fileName }">
            {{ fileName || t('streams.noFile') }}
          </span>
          <input
            ref="fileInputRef"
            type="file"
            accept="video/*"
            class="form__file-input"
            @change="onFileChange"
          />
        </div>
      </div>
      <div class="form__row">
        <span>{{ t('streams.url') }}</span>
        <div class="form__url">
          <UiInput
            :model-value="form.url"
            disabled
            :clearable="false"
            :placeholder="t('streams.publishUrlHint')"
          />
          <UiButton
            class="form__copy-btn"
            type="primary"
            :disabled="!form.url"
            @click="emit('copy')"
          >
            {{ t('streams.copy') }}
          </UiButton>
        </div>
      </div>
      <div class="form__row form__row--top">
        <span />
        <div class="form__add-stream">
          <UiCheckbox
            :model-value="form.also_add"
            @update:model-value="form.also_add = $event"
          >
            {{ t('streams.alsoAddAsStream') }}
          </UiCheckbox>
          <label v-if="form.also_add" class="form__group">
            <span>{{ t('streams.group') }}</span>
            <UiSelect
              :model-value="form.group_id"
              :options="groupOptions"
              @update:model-value="form.group_id = $event"
            />
          </label>
        </div>
      </div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('streams.cancel') }}</UiButton>
      <UiButton
        class="form__publish-btn"
        type="primary"
        :loading="publishing"
        :disabled="!hasFile"
        @click="emit('publish')"
      >
        <span v-if="!publishing">{{ t('streams.publish') }}</span>
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Group } from '@/api/streams'
import UiButton from '@/shared/ui/Button.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiSelect from '@/shared/ui/Select.vue'

const props = defineProps<{
  modelValue: boolean
  groups: Group[]
  defaultGroupId: string
  publishing: boolean
  resultUrl: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  publish: []
  copy: []
  'update:form': [
    form: {
      name: string
      group_id: string
      also_add: boolean
      url: string
      file: File | null
    },
  ]
}>()

const { t } = useI18n()

const fileInputRef = ref<HTMLInputElement | null>(null)

const form = reactive({
  name: '',
  group_id: '',
  also_add: false,
  url: '',
  file: null as File | null,
})

const groupOptions = computed(() =>
  props.groups.map((group) => ({ label: group.name, value: group.id })),
)

const fileName = computed(() => form.file?.name ?? '')
const hasFile = computed(() => Boolean(form.file))

function onPickFile() {
  fileInputRef.value?.click()
}

function onFileChange(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0] ?? null
  form.file = file
  form.url = ''
  input.value = ''
}

watch(
  () => [props.modelValue, props.defaultGroupId] as const,
  () => {
    if (!props.modelValue) {
      return
    }
    form.name = ''
    form.group_id = props.defaultGroupId
    form.also_add = false
    form.url = ''
    form.file = null
  },
)

watch(
  () => props.resultUrl,
  (value) => {
    form.url = value
  },
)

watch(
  form,
  () => {
    emit('update:form', {
      name: form.name,
      group_id: form.group_id,
      also_add: form.also_add,
      url: form.url,
      file: form.file,
    })
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

.form__row--top {
  align-items: start;
}

.form__file {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
}

.form__file-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 13px;
  color: #303133;
}

.form__file-name.is-empty {
  color: #909399;
}

.form__file-input {
  display: none;
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

.form__copy-btn {
  min-width: 72px;
  flex-shrink: 0;
}

.form__add-stream {
  display: flex;
  flex-direction: column;
  gap: 8px;
  min-width: 0;
}

.form__group {
  display: grid;
  grid-template-columns: 70px 1fr;
  align-items: center;
  gap: 8px;
}

.form__publish-btn {
  min-width: 88px;
}
</style>
