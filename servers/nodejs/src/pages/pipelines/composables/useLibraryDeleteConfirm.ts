import { ref } from 'vue'
import { ApiError } from '@/shared/http/client'
import { toast } from '@/shared/ui/toast'

export function createLibraryDeleteConfirm(options: {
  messageForCount: (n: number) => string
  hint: string
  deleteIds: (ids: string[]) => Promise<void>
}) {
  const confirmOpen = ref(false)
  const confirmLoading = ref(false)
  const confirmMessage = ref('')
  const confirmHint = ref('')
  const confirmIds = ref<string[]>([])

  function requestDelete(ids: string[]) {
    confirmIds.value = [...ids]
    confirmMessage.value = options.messageForCount(ids.length)
    confirmHint.value = options.hint
    confirmOpen.value = true
  }

  async function confirmDelete() {
    confirmLoading.value = true
    try {
      await options.deleteIds([...confirmIds.value])
      confirmIds.value = []
      confirmOpen.value = false
    } catch (err) {
      toast.error(err instanceof ApiError ? err.message : String(err))
    }
    confirmLoading.value = false
  }

  return {
    confirmOpen,
    confirmLoading,
    confirmMessage,
    confirmHint,
    requestDelete,
    confirmDelete,
  }
}
