"""Verify presence JSON user meta propagation for appsink capture.

Run inside the deepstream container:

    python3 scripts/verify_presence_meta.py mux
    python3 scripts/verify_presence_meta.py branch
    python3 scripts/verify_presence_meta.py tee

Results (DeepStream 9.0 / pyservicemaker):
- acquire_user_meta() cannot be appended via frame_meta.append in Python.
- Use acquire_event_message_meta() + set_user_data_json(payload, 8193) + frame_meta.append().
- JSON survives demux -> appsink only when written AFTER demux (e.g. nvvidconv0).
- Writing at mux/pgie/tracker before demux: meta shell reaches appsink but JSON payload is empty.
"""

from pyservicemaker import BatchMetadataOperator, BufferRetriever, Pipeline, Probe, Receiver

VIDEO = "/opt/nvidia/deepstream/deepstream/samples/streams/sample_720p.mp4"
PRESENCE_META_TYPE = 8193
PRESENCE_PAYLOAD = {"presence": 1, "frame_number": 0, "pad_index": 0}


class PresenceMetaWriter(BatchMetadataOperator):
    def __init__(self, source):
        super().__init__()
        self.source = source
        self.batch_count = 0

    def handle_metadata(self, batch_meta):
        self.batch_count += 1
        if self.batch_count > 5:
            return
        for frame_meta in batch_meta.frame_items:
            event_msg = batch_meta.acquire_event_message_meta()
            payload = dict(PRESENCE_PAYLOAD)
            payload["frame_number"] = int(frame_meta.frame_number)
            payload["pad_index"] = int(frame_meta.pad_index)
            payload["source"] = self.source
            event_msg.set_user_data_json(payload, PRESENCE_META_TYPE)
            frame_meta.append(event_msg)


class PresenceMetaReader(BufferRetriever):
    def __init__(self, name, frame_limit):
        super().__init__()
        self.name = name
        self.frame_limit = frame_limit
        self.count = 0
        self.ok_count = 0

    def consume(self, buffer):
        self.count += 1
        for frame_meta in buffer.batch_meta.frame_items:
            for item in frame_meta.user_meta_items(PRESENCE_META_TYPE):
                data = None
                error = None
                try:
                    data = item.get_user_data_json()
                except Exception as exc:
                    error = repr(exc)
                if isinstance(data, dict) and data.get("presence") is not None:
                    self.ok_count += 1
                    print(f"{self.name} fn={frame_meta.frame_number} json={data}")
                else:
                    print(f"{self.name} fn={frame_meta.frame_number} json_err={error}")
        if self.count >= self.frame_limit:
            print(f"{self.name} summary frames={self.count} readable_json={self.ok_count}")
        return 1 if self.count < self.frame_limit else 0


def build_pipeline(mode):
    pipeline = Pipeline(f"verify-presence-{mode}")
    pipeline.add("nvurisrcbin", "src", {"uri": f"file://{VIDEO}"})
    pipeline.add("nvstreammux", "mux", {"batch-size": 1, "width": 1280, "height": 720})
    pipeline.add("nvstreamdemux", "demux")
    pipeline.add("queue", "queue_demux0", {"leaky": 2, "max-size-buffers": 4})
    pipeline.add("nvvideoconvert", "nvvidconv0")
    pipeline.add("tee", "tee_raw0")
    pipeline.add("queue", "queue_raw0", {"leaky": 2, "max-size-buffers": 4})
    pipeline.add("appsink", "appsink_raw0", {"emit-signals": True, "sync": False, "max-buffers": 1, "drop": True})
    pipeline.add("nvosdbin", "osd0")
    pipeline.add("queue", "queue_vis0", {"leaky": 2, "max-size-buffers": 4})
    pipeline.add("appsink", "appsink_vis0", {"emit-signals": True, "sync": False, "max-buffers": 1, "drop": True})
    pipeline.link(("src", "mux"), ("", "sink_%u"))
    pipeline.link("mux", "demux")
    pipeline.link(("demux", "queue_demux0"), ("src_%u", ""))
    pipeline.link("queue_demux0", "nvvidconv0", "tee_raw0")
    pipeline.link(("tee_raw0", "queue_raw0"), ("src_%u", ""))
    pipeline.link("queue_raw0", "appsink_raw0")
    pipeline.link(("tee_raw0", "osd0"), ("src_%u", ""))
    pipeline.link("osd0", "queue_vis0", "appsink_vis0")

    if mode == "mux":
        pipeline.attach("mux", Probe("writer", PresenceMetaWriter("mux")))
    if mode == "branch":
        pipeline.attach("nvvidconv0", Probe("writer", PresenceMetaWriter("branch")))
    if mode == "tee":
        pipeline.attach("nvvidconv0", Probe("writer", PresenceMetaWriter("branch")))

    pipeline.attach(
        "appsink_raw0",
        Receiver("raw", PresenceMetaReader("raw", 5)),
        tips="new-sample",
    )
    if mode == "tee":
        pipeline.attach(
            "appsink_vis0",
            Receiver("vis", PresenceMetaReader("vis", 5)),
            tips="new-sample",
        )
    return pipeline


def main():
    import sys

    mode = sys.argv[1] if len(sys.argv) > 1 else "branch"
    pipeline = build_pipeline(mode)
    pipeline.start().wait()


if __name__ == "__main__":
    main()
