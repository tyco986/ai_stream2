import { shallowRef } from 'vue'
import {
  MediamtxWhepPlayer,
  resolveWebrtcBaseUrl,
  type WhepPlayState,
} from './mediamtxWhep'

type SlotMediaEntry = {
  streamId: string
  pathName: string
  player: MediamtxWhepPlayer
  media: MediaStream | null
  status: WhepPlayState
  paused: boolean
  video: HTMLVideoElement
  mirror: HTMLVideoElement
  ready: Promise<void>
  resolveReady: () => void
}

const PARK_ELEMENT_ID = 'slot-media-park'

const entries = new Map<number, SlotMediaEntry>()

export const slotMediaStatus = shallowRef<Record<number, WhepPlayState>>({})

function mediaPark(): HTMLElement {
  let node = document.getElementById(PARK_ELEMENT_ID)
  if (!node) {
    node = document.createElement('div')
    node.id = PARK_ELEMENT_ID
    node.setAttribute('aria-hidden', 'true')
    node.style.position = 'fixed'
    node.style.width = '1px'
    node.style.height = '1px'
    node.style.overflow = 'hidden'
    node.style.left = '-9999px'
    node.style.top = '0'
    node.style.pointerEvents = 'none'
    document.body.appendChild(node)
  }
  return node
}

function createSlotVideo(): HTMLVideoElement {
  const video = document.createElement('video')
  video.muted = true
  video.playsInline = true
  video.className = 'slot-cell__video'
  mediaPark().appendChild(video)
  return video
}

function createDeferred(): { ready: Promise<void>; resolveReady: () => void } {
  let resolveReady = () => undefined
  const ready = new Promise<void>((resolve) => {
    resolveReady = resolve
  })
  return { ready, resolveReady }
}

function createEntry(streamId: string, pathName: string): SlotMediaEntry {
  const deferred = createDeferred()
  return {
    streamId,
    pathName,
    player: new MediamtxWhepPlayer(resolveWebrtcBaseUrl(), pathName),
    media: null,
    status: 'loading',
    paused: false,
    video: createSlotVideo(),
    mirror: createSlotVideo(),
    ready: deferred.ready,
    resolveReady: deferred.resolveReady,
  }
}

function publishStatus(slotIndex: number, status: WhepPlayState | null) {
  const next = { ...slotMediaStatus.value }
  if (status) {
    next[slotIndex] = status
  } else {
    delete next[slotIndex]
  }
  slotMediaStatus.value = next
}

function releaseVideo(video: HTMLVideoElement) {
  video.pause()
  video.srcObject = null
  video.remove()
}

function waitForVideoFrame(video: HTMLVideoElement): Promise<void> {
  let done = Promise.resolve()
  if (video.readyState < 2) {
    done = new Promise((resolve) => {
      video.requestVideoFrameCallback(() => resolve())
    })
  }
  return done
}

async function applyVideoPaused(video: HTMLVideoElement, paused: boolean) {
  if (video.srcObject && paused && video.readyState >= 2) {
    video.pause()
  } else if (video.srcObject && paused) {
    await video.play().catch(() => undefined)
    if (!video.paused && video.readyState < 2) {
      await waitForVideoFrame(video)
    }
    video.pause()
  } else if (video.srcObject && video.paused) {
    await video.play().catch(() => undefined)
  }
}

function mirrorParked(entry: SlotMediaEntry): boolean {
  const parent = entry.mirror.parentElement
  return !parent || parent.id === PARK_ELEMENT_ID
}

async function applyEntryPaused(entry: SlotMediaEntry) {
  await applyVideoPaused(entry.video, entry.paused)
  if (!mirrorParked(entry)) {
    await applyVideoPaused(entry.mirror, entry.paused)
  } else if (entry.paused && entry.mirror.readyState >= 2) {
    entry.mirror.pause()
  }
}

async function connectSlot(
  slotIndex: number,
  streamId: string,
  pathName: string,
): Promise<WhepPlayState> {
  const entry = createEntry(streamId, pathName)
  entries.set(slotIndex, entry)
  publishStatus(slotIndex, 'loading')
  let result: WhepPlayState = 'failed'
  try {
    const media = await entry.player.play(entry.video)
    const active = entries.get(slotIndex) === entry
    if (active) {
      entry.media = media
      entry.mirror.srcObject = media
      entry.status = 'playing'
      publishStatus(slotIndex, 'playing')
      await applyEntryPaused(entry)
      entry.resolveReady()
      result = 'playing'
    } else {
      result = 'idle'
    }
  } catch {
    if (entries.get(slotIndex) === entry) {
      entry.player.stop()
      releaseVideo(entry.video)
      releaseVideo(entry.mirror)
      entries.delete(slotIndex)
      publishStatus(slotIndex, 'failed')
      entry.resolveReady()
    }
    result = 'failed'
  }
  return result
}

export function mountSlotVideo(slotIndex: number, host: HTMLElement, mirror: boolean) {
  const entry = entries.get(slotIndex)
  const video = entry ? (mirror ? entry.mirror : entry.video) : null
  const current = host.querySelector('video')
  if (current && current !== video) {
    mediaPark().appendChild(current)
  }
  if (video && video.parentElement !== host) {
    host.appendChild(video)
  }
}

export function unmountSlotVideo(host: HTMLElement) {
  const video = host.querySelector('video')
  if (video) {
    mediaPark().appendChild(video)
  }
}

export async function ensureSlotMedia(input: {
  slotIndex: number
  streamId: string
  pathName: string
}): Promise<WhepPlayState> {
  const { slotIndex, streamId, pathName } = input
  const current = entries.get(slotIndex)
  const sameStream = current?.streamId === streamId
  if (current && !sameStream) {
    unbindSlotMedia(slotIndex)
  }
  const existing = sameStream ? current : undefined
  let result: WhepPlayState = 'idle'
  if (existing?.status === 'playing') {
    result = 'playing'
  } else if (existing?.status === 'loading') {
    await existing.ready
    result = entries.get(slotIndex)?.status ?? 'failed'
  } else {
    result = await connectSlot(slotIndex, streamId, pathName)
  }
  return result
}

export async function setSlotPaused(slotIndex: number, paused: boolean) {
  const entry = entries.get(slotIndex)
  if (entry) {
    entry.paused = paused
    await applyEntryPaused(entry)
  }
}

export function unbindSlotMedia(slotIndex: number) {
  const entry = entries.get(slotIndex)
  if (entry) {
    entry.player.stop()
    releaseVideo(entry.video)
    releaseVideo(entry.mirror)
    entries.delete(slotIndex)
    publishStatus(slotIndex, null)
    entry.resolveReady()
  }
}

/** Drop hub entries that no longer match bound slot stream ids. */
export function reconcileSlotMedia(slots: (string | null)[]) {
  for (const index of [...entries.keys()]) {
    const wanted = index < slots.length ? slots[index] : null
    const entry = entries.get(index)
    if (!wanted || !entry || entry.streamId !== wanted) {
      unbindSlotMedia(index)
    }
  }
}
