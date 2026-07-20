import { fileURLToPath, URL } from 'node:url'
import fs from 'node:fs'
import path from 'node:path'
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import AutoImport from 'unplugin-auto-import/vite'
import Components from 'unplugin-vue-components/vite'
import { ElementPlusResolver } from 'unplugin-vue-components/resolvers'

const configDir = path.dirname(fileURLToPath(import.meta.url))

function readProjectNameFromFile(envPath: string): string | null {
  if (!fs.existsSync(envPath)) {
    return null
  }
  const text = fs.readFileSync(envPath, 'utf8')
  for (const line of text.split('\n')) {
    const trimmed = line.trim()
    if (!trimmed || trimmed.startsWith('#')) {
      continue
    }
    const match = /^PROJECT_NAME=(.*)$/.exec(trimmed)
    if (match) {
      return match[1].trim().replace(/^["']|["']$/g, '')
    }
  }
  return null
}

function readProjectName(): string {
  if (process.env.VITE_PROJECT_NAME) {
    return process.env.VITE_PROJECT_NAME
  }
  if (process.env.PROJECT_NAME) {
    return process.env.PROJECT_NAME
  }
  const candidates = [
    process.env.PROJECT_ENV_FILE,
    path.resolve(configDir, '../../project.env'),
    path.resolve(process.cwd(), 'project.env'),
    '/project.env',
  ].filter((value): value is string => Boolean(value))
  for (const candidate of candidates) {
    const name = readProjectNameFromFile(candidate)
    if (name) {
      return name
    }
  }
  return 'ai_stream2'
}

const projectName = readProjectName()

export default defineConfig({
  plugins: [
    vue(),
    AutoImport({
      resolvers: [ElementPlusResolver()],
    }),
    Components({
      resolvers: [ElementPlusResolver()],
    }),
    {
      name: 'html-project-name',
      transformIndexHtml(html: string) {
        return html.replaceAll('%VITE_PROJECT_NAME%', projectName)
      },
    },
  ],
  define: {
    'import.meta.env.VITE_PROJECT_NAME': JSON.stringify(projectName),
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  server: {
    host: true,
    port: 5173,
    proxy: {
      [`/${projectName}/backend`]: {
        target: 'http://127.0.0.1:8000',
        changeOrigin: true,
      },
    },
  },
})
