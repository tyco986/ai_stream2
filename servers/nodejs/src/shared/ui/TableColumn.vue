<template>
  <el-table-column
    v-if="type === 'selection'"
    type="selection"
    width="48"
  />
  <el-table-column
    v-else
    :prop="prop"
    :label="label"
    :width="width"
    :min-width="minWidth"
  >
    <template v-if="$slots.header" #header="scope">
      <slot name="header" v-bind="scope" />
    </template>
    <template v-if="$slots.default" #default="scope">
      <slot v-bind="scope" />
    </template>
  </el-table-column>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    type?: 'selection' | 'default'
    prop?: string
    label?: string
    width?: string | number
    minWidth?: string | number
  }>(),
  {
    type: 'default',
  },
)
</script>
