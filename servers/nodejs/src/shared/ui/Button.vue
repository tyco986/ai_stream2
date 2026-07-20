<template>
  <el-button
    :type="type"
    :disabled="disabled"
    :loading="loading"
    @click="emit('click')"
  >
    <slot />
  </el-button>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    type?: 'primary' | 'default' | 'success' | 'warning' | 'danger' | 'info'
    disabled?: boolean
    loading?: boolean
  }>(),
  {
    type: 'default',
    disabled: false,
    loading: false,
  },
)

const emit = defineEmits<{
  click: []
}>()
</script>
