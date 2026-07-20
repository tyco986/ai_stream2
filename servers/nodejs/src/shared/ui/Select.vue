<template>
  <div class="ui-select" :class="{ 'ui-select--invalid': invalid }">
    <el-select
      :model-value="modelValue"
      :disabled="disabled"
      :placeholder="placeholder"
      :clearable="clearable"
      :popper-class="popperClass"
      style="width: 100%"
      @update:model-value="emit('update:modelValue', $event)"
      @change="emit('change', $event)"
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

withDefaults(
  defineProps<{
    modelValue: string
    options?: SelectOption[]
    groups?: SelectOptionGroup[]
    disabled?: boolean
    placeholder?: string
    clearable?: boolean
    invalid?: boolean
    popperClass?: string
  }>(),
  {
    options: () => [],
    disabled: false,
    clearable: false,
    invalid: false,
    popperClass: '',
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: string]
  change: [value: string]
}>()
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
