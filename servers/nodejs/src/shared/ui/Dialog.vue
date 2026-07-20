<template>
  <el-dialog
    :model-value="modelValue"
    :title="title"
    :width="width"
    destroy-on-close
    :align-center="alignCenter"
    :before-close="beforeClose"
    :close-on-click-modal="closeOnClickModal"
    @update:model-value="emit('update:modelValue', $event)"
    @close="emit('close')"
  >
    <slot />
    <template v-if="$slots.footer" #footer>
      <slot name="footer" />
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    modelValue: boolean
    title?: string
    width?: string
    alignCenter?: boolean
    beforeClose?: (done: () => void) => void
    closeOnClickModal?: boolean
  }>(),
  {
    title: '',
    width: '420px',
    alignCenter: true,
    closeOnClickModal: true,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  close: []
}>()
</script>
