<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('recordings.shotPreview')"
    width="640px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="shot">
      <img v-if="url" :src="url" alt="shot" class="shot__img" />
    </div>
    <template #footer>
      <UiButton type="primary" @click="emit('update:modelValue', false)">
        {{ t('recordings.close') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'

defineProps<{
  modelValue: boolean
  url: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
</script>

<style scoped>
.shot {
  display: flex;
  justify-content: center;
  background: #000;
  min-height: 240px;
}

.shot__img {
  max-width: 100%;
  display: block;
}
</style>
