import { computed, ref } from 'vue'
import {
  applyGuestMode,
  getMe,
  getSettings,
  putSettings,
  readGuestMode,
  type PageSettings,
  type ShellMode,
} from '@/api/shell'
import {
  getCurrentVersionNumber,
  listVersions,
} from '@/api/siteConfig'
import { ApiError } from '@/shared/http/client'

const DEFAULT_MODE: ShellMode = 'sidebar'

const mode = ref<ShellMode>(DEFAULT_MODE)
const username = ref<string | null>(null)
const userId = ref<string | null>(null)
const isSuperuser = ref(false)
const permissions = ref<string[]>([])
const loggedIn = ref(false)
const loaded = ref(false)
const currentVersion = ref('0')

export function useShellSettings() {
  function clearSession() {
    username.value = null
    userId.value = null
    isSuperuser.value = false
    permissions.value = []
  }

  function hasPerm(codename: string): boolean {
    if (isSuperuser.value) {
      return true
    }
    if (permissions.value.includes('*')) {
      return true
    }
    return permissions.value.includes(codename)
  }

  const canOpenVersions = computed(
    () =>
      loggedIn.value &&
      (hasPerm('users.export_site_config') ||
        hasPerm('users.import_site_config')),
  )

  async function refreshCurrentVersion() {
    try {
      const data = await listVersions()
      currentVersion.value = getCurrentVersionNumber(data.items)
    } catch {
      if (!loggedIn.value) {
        currentVersion.value = '0'
      }
    }
  }

  async function bootstrap() {
    try {
      const [user, settings] = await Promise.all([getMe(), getSettings()])
      username.value = user.username
      userId.value = user.id
      isSuperuser.value = user.is_superuser
      permissions.value = [...user.permissions]
      mode.value = settings.mode
      loggedIn.value = true
      await refreshCurrentVersion()
    } catch (err) {
      if (err instanceof ApiError && err.status === 401) {
        clearSession()
        loggedIn.value = false
        mode.value = readGuestMode()
        currentVersion.value = '0'
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
    userId,
    isSuperuser,
    permissions,
    hasPerm,
    canOpenVersions,
    currentVersion,
    loggedIn,
    loaded,
    bootstrap,
    applySettings,
    refreshCurrentVersion,
  }
}
