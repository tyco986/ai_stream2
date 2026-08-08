export type WhepPlayState = 'idle' | 'loading' | 'playing' | 'failed'

export class MediamtxWhepPlayer {
  baseUrl: string
  pathName: string
  pc: RTCPeerConnection | null
  sessionUrl: string | null

  constructor(baseUrl: string, pathName: string) {
    this.baseUrl = baseUrl.replace(/\/$/, '')
    this.pathName = pathName
    this.pc = null
    this.sessionUrl = null
  }

  async play(videoEl: HTMLVideoElement): Promise<MediaStream> {
    this.stop()
    const pc = new RTCPeerConnection({
      iceServers: [{ urls: 'stun:stun.l.google.com:19302' }],
    })
    this.pc = pc
    pc.addTransceiver('video', { direction: 'recvonly' })
    pc.addTransceiver('audio', { direction: 'recvonly' })
    const mediaPromise = new Promise<MediaStream>((resolve, reject) => {
      const timer = window.setTimeout(() => {
        reject(new Error('WHEP track timeout'))
      }, 15000)
      pc.ontrack = (event) => {
        const media = event.streams[0]
        if (!media) {
          return
        }
        window.clearTimeout(timer)
        videoEl.srcObject = media
        resolve(media)
      }
    })
    const offer = await pc.createOffer()
    await pc.setLocalDescription(offer)
    await waitIceGathering(pc)
    const whepUrl = `${this.baseUrl}/${encodeURIComponent(this.pathName)}/whep`
    const response = await fetch(whepUrl, {
      method: 'POST',
      headers: { 'Content-Type': 'application/sdp' },
      body: offer.sdp || '',
    })
    if (!response.ok) {
      const text = await response.text()
      throw new Error(text || `WHEP HTTP ${response.status}`)
    }
    const location = response.headers.get('location')
    if (location) {
      this.sessionUrl = new URL(location, whepUrl).toString()
    }
    const answer = await response.text()
    await pc.setRemoteDescription({ type: 'answer', sdp: answer })
    return mediaPromise
  }

  stop() {
    if (this.sessionUrl) {
      fetch(this.sessionUrl, { method: 'DELETE' }).catch(() => undefined)
      this.sessionUrl = null
    }
    if (this.pc) {
      this.pc.close()
      this.pc = null
    }
  }
}

function waitIceGathering(pc: RTCPeerConnection) {
  let done: Promise<void>
  if (pc.iceGatheringState === 'complete') {
    done = Promise.resolve()
  } else {
    done = new Promise((resolve) => {
      pc.addEventListener('icegatheringstatechange', function onChange() {
        if (pc.iceGatheringState === 'complete') {
          pc.removeEventListener('icegatheringstatechange', onChange)
          resolve()
        }
      })
    })
  }
  return done
}

export function resolveWebrtcBaseUrl() {
  const configured = import.meta.env.VITE_MEDIAMTX_WEBRTC_BASE as string | undefined
  return (configured || 'http://127.0.0.1:8889').replace(/\/$/, '')
}
