import { ApiError } from '@/shared/http/client'
import type { AnalyzerSourceResult } from '@/api/pipelines/types'

function isAllowedImageFileName(name: string): boolean {
  const lowerName = name.toLowerCase()
  return (
    lowerName.endsWith('.jpg') ||
    lowerName.endsWith('.jpeg') ||
    lowerName.endsWith('.png')
  )
}

function readFileAsDataUrl(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(String(reader.result))
    reader.onerror = () => reject(reader.error ?? new Error('Failed to read file'))
    reader.readAsDataURL(file)
  })
}

function loadImageSize(src: string): Promise<{ width: number; height: number }> {
  return new Promise((resolve, reject) => {
    const img = new Image()
    img.onload = () => resolve({ width: img.naturalWidth, height: img.naturalHeight })
    img.onerror = () => reject(new Error('Failed to load image'))
    img.src = src
  })
}

function nowIso(): string {
  return new Date().toISOString()
}

export function createAnalyzerPlaceholderDataUrl(
  width: number,
  height: number,
  label: string,
): string {
  const canvas = document.createElement('canvas')
  const maxEdge = 960
  const scale = Math.min(1, maxEdge / Math.max(width, height, 1))
  canvas.width = Math.max(1, Math.round(width * scale))
  canvas.height = Math.max(1, Math.round(height * scale))
  const ctx = canvas.getContext('2d')
  if (!ctx) {
    return ''
  }
  ctx.fillStyle = '#1f2a37'
  ctx.fillRect(0, 0, canvas.width, canvas.height)
  ctx.strokeStyle = '#3a4a5c'
  const step = 40
  for (let x = 0; x < canvas.width; x += step) {
    ctx.beginPath()
    ctx.moveTo(x, 0)
    ctx.lineTo(x, canvas.height)
    ctx.stroke()
  }
  for (let y = 0; y < canvas.height; y += step) {
    ctx.beginPath()
    ctx.moveTo(0, y)
    ctx.lineTo(canvas.width, y)
    ctx.stroke()
  }
  ctx.fillStyle = '#e5eaf0'
  ctx.font = '20px sans-serif'
  ctx.fillText(label, 24, 40)
  ctx.fillText(`${width}x${height}`, 24, 72)
  return canvas.toDataURL('image/jpeg', 0.85)
}

export async function previewAnalyzerSourceFile(file: File): Promise<AnalyzerSourceResult> {
  if (!isAllowedImageFileName(file.name)) {
    throw new ApiError(400, 'Unsupported image format')
  }
  const dataUrl = await readFileAsDataUrl(file)
  const size = await loadImageSize(dataUrl)
  const capturedAt = nowIso()
  return {
    source_kind: 'file',
    source_stream_id: null,
    source_file_name: file.name,
    source_image_url: dataUrl,
    config_width: size.width,
    config_height: size.height,
    captured_at: capturedAt,
  }
}

export async function previewAnalyzerSourceStream(
  streamId: string,
  streamName: string,
): Promise<AnalyzerSourceResult> {
  const configWidth = 1920
  const configHeight = 1080
  const capturedAt = nowIso()
  return {
    source_kind: 'stream',
    source_stream_id: streamId,
    source_file_name: null,
    source_image_url: createAnalyzerPlaceholderDataUrl(
      configWidth,
      configHeight,
      streamName,
    ),
    config_width: configWidth,
    config_height: configHeight,
    captured_at: capturedAt,
  }
}
