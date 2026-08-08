export function formatEventTimestamp(iso: string): string {
  const date = new Date(iso)
  if (Number.isNaN(date.getTime())) {
    return iso.replace('T', ' ').slice(0, 19)
  }
  const y = date.getFullYear()
  const m = String(date.getMonth() + 1).padStart(2, '0')
  const d = String(date.getDate()).padStart(2, '0')
  const hh = String(date.getHours()).padStart(2, '0')
  const mi = String(date.getMinutes()).padStart(2, '0')
  const ss = String(date.getSeconds()).padStart(2, '0')
  return `${y}-${m}-${d} ${hh}:${mi}:${ss}`
}

export function eventDateFromOccurredAt(iso: string): string {
  return iso.slice(0, 10)
}

export function eventTimeQueryFromOccurredAt(iso: string): string {
  const formatted = formatEventTimestamp(iso)
  return formatted.slice(11)
}
