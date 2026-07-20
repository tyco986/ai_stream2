/** Product slug from root project.env (injected as VITE_PROJECT_NAME at build/dev). */
export const PROJECT_NAME =
  import.meta.env.VITE_PROJECT_NAME || 'ai_stream2'

export const API_PREFIX = `/${PROJECT_NAME}`
export const BACKEND_PREFIX = `${API_PREFIX}/backend`
export const STORAGE_KEY_PREFIX = `${PROJECT_NAME}.`
