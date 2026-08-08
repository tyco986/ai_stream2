/**
 * Site Config API. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type { SiteConfigVersion } from '@/api/siteConfig/types'

export const listVersions = api.listVersions
export const createVersion = api.createVersion
export const importVersion = api.importVersion
export const exportVersionSync = api.exportVersionSync
export const exportVersion = api.exportVersion
export const applyVersion = api.applyVersion
export const getCurrentVersionNumber = api.getCurrentVersionNumber
