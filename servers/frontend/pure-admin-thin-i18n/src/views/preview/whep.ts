export class WhepPlayer {
  pathName: string;
  pc: RTCPeerConnection | null;
  sessionUrl: string | null;
  video: HTMLVideoElement | null;
  resolveIce: (() => void) | null;

  constructor() {
    this.pathName = "";
    this.pc = null;
    this.sessionUrl = null;
    this.video = null;
    this.resolveIce = null;
  }

  open(url: string) {
    const video = this.video ?? this.createVideo();
    this.video = video;
    if (!this.pc) {
      this.start(url);
    }
    return video;
  }

  stop() {
    const session = this.sessionUrl;
    if (session) {
      fetch(session, { method: "DELETE" }).catch(() => undefined);
    }
    this.sessionUrl = null;
    this.pc?.close();
    this.pc = null;
    if (this.video) {
      this.video.srcObject = null;
    }
    this.video = null;
  }

  createVideo() {
    const video = document.createElement("video");
    video.autoplay = true;
    video.muted = true;
    video.playsInline = true;
    video.controls = false;
    return video;
  }

  start(url: string) {
    this.pathName = url;
    const pc = new RTCPeerConnection();
    this.pc = pc;
    pc.addTransceiver("video", { direction: "recvonly" });
    pc.addTransceiver("audio", { direction: "recvonly" });
    pc.ontrack = this.onTrack.bind(this);
    const offer = pc.createOffer().then(description => pc.setLocalDescription(description));
    offer.then(() => this.waitIce()).then(() => this.exchange(pc));
  }

  onTrack(event: RTCTrackEvent) {
    const stream = event.streams[0];
    if (this.video && stream) {
      this.video.srcObject = stream;
      this.video.play();
    }
  }

  waitIce() {
    const pc = this.pc;
    const ready = pc?.iceGatheringState === "complete";
    const pending = new Promise<void>(resolve => {
      this.resolveIce = resolve;
      if (pc) {
        pc.onicegatheringstatechange = this.onIce.bind(this);
      }
    });
    const done = ready ? Promise.resolve() : pending;
    return done;
  }

  onIce() {
    const complete = this.pc?.iceGatheringState === "complete" && this.resolveIce;
    if (complete) {
      this.resolveIce();
      this.resolveIce = null;
    }
  }

  exchange(pc: RTCPeerConnection) {
    return fetch(this.pathName, {
      method: "POST",
      headers: { "Content-Type": "application/sdp" },
      body: pc.localDescription?.sdp || ""
    }).then(response => {
      const location = response.headers.get("location") || "";
      this.sessionUrl = location ? new URL(location, this.pathName).toString() : null;
      return response.text();
    }).then(answer => pc.setRemoteDescription({ type: "answer", sdp: answer }));
  }
}
