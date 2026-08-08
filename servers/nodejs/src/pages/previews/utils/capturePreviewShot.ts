/** Capture the live preview stage (video frames only; ignores chrome and selection). */
export function capturePreviewShot(): string {
  let dataUrl = ''
  const stage = document.querySelector('.preview-page__stage')
  if (stage instanceof HTMLElement) {
    const rect = stage.getBoundingClientRect()
    const width = Math.max(1, Math.round(rect.width))
    const height = Math.max(1, Math.round(rect.height))
    const canvas = document.createElement('canvas')
    canvas.width = width
    canvas.height = height
    const ctx = canvas.getContext('2d')
    if (ctx) {
      ctx.fillStyle = '#000'
      ctx.fillRect(0, 0, width, height)
      for (const cell of stage.querySelectorAll<HTMLElement>('.slot-cell')) {
        const cellRect = cell.getBoundingClientRect()
        const x = cellRect.left - rect.left
        const y = cellRect.top - rect.top
        const w = cellRect.width
        const h = cellRect.height
        const video = cell.querySelector('video')
        ctx.fillStyle = '#000'
        ctx.fillRect(x, y, w, h)
        if (
          video instanceof HTMLVideoElement &&
          video.readyState >= 2 &&
          video.videoWidth > 0
        ) {
          const scale = Math.min(w / video.videoWidth, h / video.videoHeight)
          const dw = video.videoWidth * scale
          const dh = video.videoHeight * scale
          ctx.drawImage(video, x + (w - dw) / 2, y + (h - dh) / 2, dw, dh)
        } else {
          const label =
            cell.querySelector('.slot-cell__empty')?.textContent?.trim() ?? ''
          if (label) {
            ctx.fillStyle = '#606266'
            ctx.font = `600 ${Math.max(28, Math.floor(Math.min(w, h) * 0.28))}px sans-serif`
            ctx.textAlign = 'center'
            ctx.textBaseline = 'middle'
            ctx.fillText(label, x + w / 2, y + h / 2)
          }
        }
      }
      dataUrl = canvas.toDataURL('image/png')
    }
  }
  return dataUrl
}
