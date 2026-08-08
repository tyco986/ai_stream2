/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_PROJECT_NAME?: string
  readonly VITE_BACKEND_PROXY_TARGET?: string
  readonly VITE_MEDIAMTX_WEBRTC_BASE?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
