import { ref } from 'vue'
import {
  applyGuestMode,
  getMe,
  getMockLoggedIn,
  getSettings,
  putSettings,
  readGuestMode,
  type PageSettings,
  type ShellMode,
} from '@/api/shell'
import { ApiError } from '@/shared/http/client'

const DEFAULT_MODE: ShellMode = 'sidebar'

const mode = ref<ShellMode>(DEFAULT_MODE)
const username = ref<string | null>(null)
const loggedIn = ref(false)
const loaded = ref(false)

export function useShellSettings() {
  async function bootstrap() {
    loggedIn.value = getMockLoggedIn()
    if (!loggedIn.value) {
      username.value = null
      mode.value = readGuestMode()
      loaded.value = true
      return
    }
    try {
      const [user, settings] = await Promise.all([getMe(), getSettings()])
      username.value = user.username
      mode.value = settings.mode
      loggedIn.value = true
    } catch (err) {
      if (err instanceof ApiError && err.status === 401) {
        username.value = null
        loggedIn.value = false
        mode.value = readGuestMode()
      } else {
        throw err
      }
    }
    loaded.value = true
  }

  async function applySettings(draft: PageSettings) {
    if (loggedIn.value) {
      const saved = await putSettings(draft)
      mode.value = saved.mode
      return
    }
    const guest = applyGuestMode(draft.mode)
    mode.value = guest.mode
  }

  return {
    mode,
    username,
    loggedIn,
    loaded,
    bootstrap,
    applySettings,
  }
}
