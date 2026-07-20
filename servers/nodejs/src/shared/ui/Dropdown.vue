<template>
  <el-dropdown trigger="click" @command="emit('command', $event)">
    <button type="button" class="ui-dropdown__trigger" @click.stop>
      <slot name="trigger">⋯</slot>
    </button>
    <template #dropdown>
      <el-dropdown-menu>
        <el-dropdown-item
          v-for="item in items"
          :key="item.command"
          :command="item.command"
          :disabled="item.disabled"
        >
          {{ item.label }}
        </el-dropdown-item>
      </el-dropdown-menu>
    </template>
  </el-dropdown>
</template>

<script setup lang="ts">
defineProps<{
  items: { command: string; label: string; disabled?: boolean }[]
}>()

const emit = defineEmits<{
  command: [command: string]
}>()
</script>

<style scoped>
.ui-dropdown__trigger {
  border: none;
  background: transparent;
  cursor: pointer;
  padding: 2px 6px;
  color: #606266;
  font-size: 16px;
  line-height: 1;
}
</style>
