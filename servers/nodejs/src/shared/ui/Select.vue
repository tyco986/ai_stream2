<template>
  <div class="ui-select" :class="{ 'ui-select--invalid': invalid }">
    <el-select
      :model-value="modelValue"
      :multiple="multiple"
      :disabled="disabled"
      :placeholder="placeholder"
      :clearable="clearable"
      :collapse-tags="multiple"
      :collapse-tags-tooltip="multiple"
      :popper-class="popperClass"
      style="width: 100%"
      @update:model-value="onUpdate"
      @change="onChange"
    >
      <template v-if="$slots.label" #label="scope">
        <slot name="label" v-bind="scope" />
      </template>
      <template v-if="groups?.length">
        <el-option-group v-for="group in groups" :key="group.label" :label="group.label">
          <el-option
            v-for="opt in group.options"
            :key="String(opt.value)"
            :label="opt.label"
            :value="opt.value"
            :disabled="opt.disabled"
          >
            <slot name="option" :option="opt">
              <span :style="opt.color ? { color: opt.color } : undefined">{{ opt.label }}</span>
            </slot>
          </el-option>
        </el-option-group>
      </template>
      <template v-else>
        <el-option
          v-for="opt in options"
          :key="String(opt.value)"
          :label="opt.label"
          :value="opt.value"
          :disabled="opt.disabled"
        >
          <slot name="option" :option="opt">
            <span :style="opt.color ? { color: opt.color } : undefined">{{ opt.label }}</span>
          </slot>
        </el-option>
      </template>
    </el-select>
  </div>
</template>

<script setup lang="ts">
export type SelectOption = {
  label: string
  value: string
  color?: string
  disabled?: boolean
}

export type SelectOptionGroup = {
  label: string
  options: SelectOption[]
}

const props = withDefaults(
  defineProps<{
    modelValue: string | string[]
    options?: SelectOption[]
    groups?: SelectOptionGroup[]
    disabled?: boolean
    placeholder?: string
    clearable?: boolean
    invalid?: boolean
    popperClass?: string
    multiple?: boolean
  }>(),
  {
    options: () => [],
    disabled: false,
    clearable: false,
    invalid: false,
    popperClass: '',
    multiple: false,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: string | string[]]
  change: [value: string | string[]]
}>()

function onUpdate(value: string | string[] | null | undefined) {
  if (props.multiple) {
    emit('update:modelValue', Array.isArray(value) ? value : [])
    return
  }
  emit('update:modelValue', value ?? '')
}

function onChange(value: string | string[]) {
  emit('change', value)
}
</script>

<style scoped>
.ui-select {
  width: 100%;
}

.ui-select--invalid :deep(.el-select__wrapper),
.ui-select--invalid :deep(.el-select__wrapper.is-focused),
.ui-select--invalid :deep(.el-select__wrapper.is-hovering) {
  box-shadow: 0 0 0 1px var(--el-color-danger) inset;
}
</style>
