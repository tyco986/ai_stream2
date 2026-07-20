import { createI18n } from 'vue-i18n'
import modelsEn from '@/pages/models/i18n/en'
import modelsZh from '@/pages/models/i18n/zh'
import pipelinesEn from '@/pages/pipelines/i18n/en'
import pipelinesZh from '@/pages/pipelines/i18n/zh'
import previewEn from '@/pages/previews/i18n/en'
import previewZh from '@/pages/previews/i18n/zh'
import recordingsEn from '@/pages/recordings/i18n/en'
import recordingsZh from '@/pages/recordings/i18n/zh'
import serversEn from '@/pages/servers/i18n/en'
import serversZh from '@/pages/servers/i18n/zh'
import shellEn from '@/pages/shell/i18n/en'
import shellZh from '@/pages/shell/i18n/zh'
import streamsEn from '@/pages/streams/i18n/en'
import streamsZh from '@/pages/streams/i18n/zh'
import { STORAGE_KEY_PREFIX } from '@/shared/project'

const LOCALE_KEY = `${STORAGE_KEY_PREFIX}locale`

function readLocale(): 'en' | 'zh' {
  const stored = sessionStorage.getItem(LOCALE_KEY)
  if (stored === 'zh' || stored === 'en') {
    return stored
  }
  return 'en'
}

export const i18n = createI18n({
  legacy: false,
  locale: readLocale(),
  fallbackLocale: 'en',
  messages: {
    en: {
      shell: shellEn,
      preview: previewEn,
      streams: streamsEn,
      recordings: recordingsEn,
      models: modelsEn,
      servers: serversEn,
      pipelines: pipelinesEn,
    },
    zh: {
      shell: shellZh,
      preview: previewZh,
      streams: streamsZh,
      recordings: recordingsZh,
      models: modelsZh,
      servers: serversZh,
      pipelines: pipelinesZh,
    },
  },
})

export function setLocale(locale: 'en' | 'zh') {
  i18n.global.locale.value = locale
  sessionStorage.setItem(LOCALE_KEY, locale)
}
