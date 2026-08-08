<template>
  <UiDialog
    :model-value="modelValue"
    :title="mode === 'add' ? t('users.addUser') : t('users.editUser')"
    width="520px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="form">
      <label class="form__row">
        <span>{{ t('users.username') }}</span>
        <UiInput :model-value="form.username" @update:model-value="form.username = $event" />
      </label>
      <div class="form__row">
        <span>{{ t('users.password') }}</span>
        <UiInput
          v-if="mode === 'add'"
          type="password"
          :model-value="form.password"
          @update:model-value="form.password = $event"
        />
        <div v-else class="form__password-edit">
          <UiCheckbox :model-value="resetPassword" @update:model-value="onResetToggle">
            {{ t('users.resetPassword') }}
          </UiCheckbox>
          <UiInput
            v-if="resetPassword"
            type="password"
            :model-value="form.password"
            @update:model-value="form.password = $event"
          />
        </div>
      </div>
      <div class="form__row">
        <span>{{ t('users.active') }}</span>
        <UiRadioGroup
          :model-value="form.is_active"
          :options="onOffOptions"
          @update:model-value="form.is_active = Boolean($event)"
        />
      </div>
      <label class="form__row">
        <span>{{ t('users.groupsField') }}</span>
        <UiSelect
          multiple
          :model-value="form.group_ids"
          :options="groupSelectOptions"
          @update:model-value="form.group_ids = asStringArray($event)"
        />
      </label>
      <div v-if="error" class="form__error">{{ error }}</div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('users.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="emit('save')">
        {{ t('users.save') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { GroupItem, UserItem } from '@/api/users'
import UiButton from '@/shared/ui/Button.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiRadioGroup from '@/shared/ui/RadioGroup.vue'
import UiSelect from '@/shared/ui/Select.vue'

export type UserFormDraft = {
  username: string
  password: string
  is_active: boolean
  group_ids: string[]
}

const props = defineProps<{
  modelValue: boolean
  mode: 'add' | 'edit'
  user: UserItem | null
  groups: GroupItem[]
  saving: boolean
  error: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: []
  'update:form': [form: UserFormDraft]
}>()

const { t } = useI18n()
const resetPassword = ref(false)

const form = reactive<UserFormDraft>({
  username: '',
  password: '',
  is_active: true,
  group_ids: [],
})

const onOffOptions = computed(() => [
  { label: t('users.on'), value: true },
  { label: t('users.off'), value: false },
])

const groupSelectOptions = computed(() =>
  props.groups.map((group) => ({ label: group.name, value: group.id })),
)

watch(
  () => props.modelValue,
  (open) => {
    if (!open) {
      return
    }
    resetPassword.value = false
    if (props.mode === 'edit' && props.user) {
      form.username = props.user.username
      form.password = ''
      form.is_active = props.user.is_active
      form.group_ids = props.user.groups.map((group) => group.id)
      return
    }
    form.username = ''
    form.password = ''
    form.is_active = true
    form.group_ids = []
  },
)

watch(
  form,
  () => {
    emit('update:form', {
      username: form.username,
      password: form.password,
      is_active: form.is_active,
      group_ids: [...form.group_ids],
    })
  },
  { deep: true },
)

function onResetToggle(value: boolean) {
  resetPassword.value = value
  if (!value) {
    form.password = ''
  }
}

function asStringArray(value: string | string[]): string[] {
  return Array.isArray(value) ? value : value ? [value] : []
}
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
  font-size: 13px;
  color: #606266;
}

.form__password-edit {
  display: flex;
  flex-direction: column;
  gap: 6px;
  min-width: 0;
}

.form__error {
  color: #f56c6c;
  font-size: 13px;
}
</style>
