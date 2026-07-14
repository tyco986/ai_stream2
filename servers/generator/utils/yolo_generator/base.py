from pathlib import Path

import yaml

KAFKA_PROTO_LIB = "/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so"
KAFKA_CONN_STR = "ai_stream2_kafka;9092"
KAFKA_TOPIC = "deepstream-detections"
TRACKER_LL_LIB = "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so"


class DeepstreamGenerator:
    """Base class for building pyservicemaker pipeline YAML configs.

    Subclasses implement add() and link() to populate nodes and edges.
    Each _add_<element> helper returns a properties dict for that GStreamer element.
    """

    def __init__(self) -> None:
        self.pipeline = {
            "deepstream": {
                "nodes": [],
                "edges": {},
            },
        }

    def add(self) -> None:
        pass

    def link(self) -> None:
        pass

    def to_dict(self) -> dict:
        return self.pipeline

    def _append_node(self, element_type: str, name: str, properties: dict) -> None:
        node = {"type": element_type, "name": name}
        if properties:
            node["properties"] = properties
        self.pipeline["deepstream"]["nodes"].append(node)

    def write(self, save_path: Path | str) -> None:
        path = Path(save_path).expanduser().resolve()
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.pipeline,
                handle,
                sort_keys=False,
                default_flow_style=False,
            )

    def _add_nvurisrcbin(
        self,
        uri: str,
        disable_audio: bool = True,
        num_buffers: int | None = None,
        select_rtp_protocol: int = 4,
    ) -> dict:
        """Build properties for ``nvurisrcbin`` (URI decode + demux source bin).

        Args:
            uri: Media source URI (e.g. ``file:///path/video.mp4``, ``rtsp://host/stream``).
            disable_audio: When True, ignore audio tracks and expose only the video pad.
            num_buffers: Cap decoded buffers for finite sources (file/image sequences).
                None leaves the element default (unlimited). Useful for single-pass file tests.
            select_rtp_protocol: RTP transport for RTSP — ``0`` auto, ``4`` TCP.
        """
        properties = {
            "uri": uri,
            "disable-audio": disable_audio,
            "select-rtp-protocol": select_rtp_protocol,
        }
        if num_buffers is not None:
            properties["num-buffers"] = num_buffers
        return properties

    def _add_nvstreammux(
        self,
        batch_size: int,
        width: int,
        height: int,
        live_source: bool = True,
        enable_padding: bool = False,
        batched_push_timeout: int = 40000,
        gpu_id: int = 0,
    ) -> dict:
        """Build properties for ``nvstreammux`` (batch and scale multiple input streams).

        Args:
            batch_size: Maximum number of sources batched per output buffer (equals stream count
                for typical multi-RTSP pipelines).
            width: Output frame width in pixels; all sources are scaled to this width.
            height: Output frame height in pixels; all sources are scaled to this height.
            live_source: True for live inputs (RTSP/RTMP/USB); False for file sources. Affects
                batching timeout and sync behavior.
            enable_padding: When True, preserve aspect ratio with letterbox padding; when False,
                stretch frames to fill width x height.
            batched_push_timeout: Microseconds to wait for a full batch before pushing a partial
                batch (live pipelines). Lower values reduce latency; higher values improve batching.
            gpu_id: GPU device ID used for NVMM batch compositing.
        """
        return {
            "batch-size": batch_size,
            "width": width,
            "height": height,
            "enable-padding": enable_padding,
            "batched-push-timeout": batched_push_timeout,
            "gpu-id": gpu_id,
            "live-source": live_source,
        }

    def _add_nvinfer(
        self,
        config_file_path: str,
        batch_size: int,
        gpu_id: int = 0,
        input_tensor_meta: bool = False,
    ) -> dict:
        """Build properties for ``nvinfer`` (TensorRT primary/secondary inference).

        Args:
            config_file_path: Path to the nvinfer config file (``.txt`` or ``.yml``) defining
                model paths, ``gie-unique-id``, parser, and class attributes.
            batch_size: Runtime inference batch size. For SAHI, must equal tiles per frame
                (slices + optional full-frame tile). Overrides or must match the config file.
            gpu_id: GPU device ID for TensorRT engine execution.
            input_tensor_meta: When True, consume GPU tensors from upstream ``nvsahipreprocess``
                or ``nvdspreprocess`` instead of decoding frames inside nvinfer. Required for
                SAHI pipelines (sets ``input-tensor-meta=true``).
        """
        properties = {
            "config-file-path": config_file_path,
            "batch-size": batch_size,
            "gpu-id": gpu_id,
        }
        if input_tensor_meta:
            properties["input-tensor-meta"] = True
        return properties

    def _add_nvsahipreprocess(
        self,
        config_file: str,
        slice_width: int = 640,
        slice_height: int = 640,
        overlap_width_ratio: float = 0.2,
        overlap_height_ratio: float = 0.2,
        enable_full_frame: bool = True,
        gpu_id: int = 0,
        unique_id: int = 15,
        target_unique_ids: str = "1",
    ) -> dict:
        """Build properties for ``nvsahipreprocess`` (SAHI slice generation and tensor prep).

        Slice geometry is set on this element; tensor layout/normalization live in config_file.

        Args:
            config_file: Path to the preprocess INI (processing dimensions, tensor shape,
                color format, normalization). Must match downstream nvinfer ``infer-dims``.
            slice_width: SAHI tile width in source-frame pixels before scale to network input.
            slice_height: SAHI tile height in source-frame pixels before scale to network input.
            overlap_width_ratio: Horizontal overlap between adjacent tiles, in [0.0, 0.99).
                Higher overlap reduces missed objects at tile edges but increases tile count.
            overlap_height_ratio: Vertical overlap between adjacent tiles, in [0.0, 0.99).
            enable_full_frame: When True, add the entire frame as an extra inference tile
                alongside slices (helps large objects and full-scene context). Increases batch
                size by one.
            gpu_id: GPU used for slice crop/scale via NvBufSurfTransform.
            unique_id: Metadata unique ID assigned to this preprocess element instance.
            target_unique_ids: Semicolon-separated ``gie-unique-id`` values of downstream
                nvinfer instances that will consume the prepared tensors (e.g. ``"1"`` for a
                single PGIE). Must match ``gie-unique-id`` in the pgie config file.
        """
        return {
            "config-file": config_file,
            "slice-width": slice_width,
            "slice-height": slice_height,
            "overlap-width-ratio": overlap_width_ratio,
            "overlap-height-ratio": overlap_height_ratio,
            "enable-full-frame": enable_full_frame,
            "gpu-id": gpu_id,
            "unique-id": unique_id,
            "target-unique-ids": target_unique_ids,
        }

    def _add_nvsahipostprocess(
        self,
        gie_ids: str = "1",
        match_metric: int = 1,
        match_threshold: float = 0.5,
        class_agnostic: bool = False,
        enable_merge: bool = True,
        two_phase_nmm: bool = True,
    ) -> dict:
        """Build properties for ``nvsahipostprocess`` (GreedyNMM merge of slice detections).

        Operates on ``NvDsObjectMeta`` after nvinfer; no tensor access required.

        Args:
            gie_ids: Semicolon-separated ``gie-unique-id`` values whose detections to merge
                (e.g. ``"1"`` or ``"1;3"``). ``"-1"`` processes all GIEs in the batch.
            match_metric: Overlap metric for duplicate detection: ``0`` = IoU (intersection /
                union), ``1`` = IoS (intersection / min area). IoS is recommended for SAHI
                because slice-boundary duplicates often differ greatly in box area.
            match_threshold: Minimum overlap score (0.0–1.0) to treat two boxes as duplicates.
                Lower values suppress more boxes; higher values are more conservative.
            class_agnostic: When False (default), NMM runs per class ID. When True, overlapping
                boxes of different classes can suppress each other (use only when cross-class
                merge is intentional).
            enable_merge: When True, absorbed boxes expand the surviving bbox (GreedyNMM merge).
                When False, perform NMS-only suppression without updating survivor coordinates.
            two_phase_nmm: When True, phase 1 selects candidates on original boxes and phase 2
                re-checks against the expanding merged box (more conservative, fewer chain
                merges). When False, use single-phase merging (more aggressive).
        """
        return {
            "gie-ids": gie_ids,
            "match-metric": match_metric,
            "match-threshold": match_threshold,
            "class-agnostic": class_agnostic,
            "enable-merge": enable_merge,
            "two-phase-nmm": two_phase_nmm,
        }

    def _add_nvtracker(
        self,
        ll_lib_file: str,
        ll_config_file: str,
        tracker_width: int,
        tracker_height: int,
        gpu_id: int = 0,
        operate_on_class_ids: str = "",
    ) -> dict:
        """Build properties for ``nvtracker`` (multi-object tracking across frames).

        Args:
            ll_lib_file: Path to the low-level tracker shared library (``.so``), e.g.
                ``libnvds_nvmultiobjecttracker.so``.
            ll_config_file: Path to the tracker YAML/INI (NvDCF, DeepSORT, etc.).
            tracker_width: Internal processing width; detector boxes are scaled to this
                resolution before tracking. Usually matches pgie input or muxer width.
            tracker_height: Internal processing height; paired with tracker_width.
            gpu_id: GPU device ID for tracker execution when the backend uses GPU.
        """
        return {
            "ll-lib-file": ll_lib_file,
            "ll-config-file": ll_config_file,
            "tracker-width": tracker_width,
            "tracker-height": tracker_height,
            "gpu-id": gpu_id,
            "operate-on-class-ids": operate_on_class_ids,
        }

    def _add_nvdsanalytics(self, config_file: str, gpu_id: int = 0) -> dict:
        """Build properties for ``nvdsanalytics`` (ROI/line/direction analytics on tracks).

        Args:
            config_file: Path to nvdsanalytics config defining ROIs, lines, and rules.
            gpu_id: GPU device ID for analytics processing.
        """
        return {
            "config-file": config_file,
            "gpu-id": gpu_id,
        }

    def _add_tee(self) -> dict:
        """Build properties for ``tee`` (split one stream to multiple downstream branches).

        No GObject properties are set; the element uses GStreamer defaults.
        """
        return {}

    def _add_queue(self, leaky: int = 2, max_size_buffers: int = 4) -> dict:
        """Build properties for ``queue`` (decouple pipeline stages and absorb backpressure).

        Args:
            leaky: Drop policy when the queue is full: ``0`` = no leak, ``1`` = drop oldest
                (upstream), ``2`` = drop newest (downstream).
            max_size_buffers: Maximum queued buffers before blocking or leaking.
        """
        return {
            "leaky": leaky,
            "max-size-buffers": max_size_buffers,
        }

    def _add_nvmsgconv(
        self,
        config: str,
        payload_type: int = 1,
        msg2p_newapi: bool = True,
        frame_interval: int = 1,
    ) -> dict:
        """Build properties for ``nvmsgconv`` (DeepStream metadata to message payload).

        Args:
            config: Path to nvmsgconv config (schema, sensor IDs, payload fields).
            payload_type: Serialization format: ``0`` = custom, ``1`` = DeepStream schema
                (default Kafka/JSON payload).
            msg2p_newapi: When True, use the newer message-to-payload API for object/edge
                metadata conversion.
            frame_interval: Emit one message every N frames (``1`` = every frame).
        """
        return {
            "config": config,
            "payload-type": payload_type,
            "msg2p-newapi": msg2p_newapi,
            "frame-interval": frame_interval,
        }

    def _add_nvmsgbroker(
        self,
        proto_lib: str,
        conn_str: str,
        topic: str,
        broker_config: str,
        sync: bool = False,
        async_: bool = False,
    ) -> dict:
        """Build properties for ``nvmsgbroker`` (publish nvmsgconv payloads to a broker).

        Args:
            proto_lib: Path to the protocol adapter shared library (e.g. Kafka ``libnvds_kafka_proto.so``).
            conn_str: Broker connection string (e.g. ``host;port`` for Kafka).
            topic: Message topic name published to the broker.
            broker_config: Path to broker-specific config file (auth, TLS, client settings).
            sync: Block until the broker acknowledges send.
            async_: Run publish asynchronously through the adapter.
        """
        return {
            "proto-lib": proto_lib,
            "conn-str": conn_str,
            "topic": topic,
            "config": broker_config,
            "sync": sync,
            "async": async_,
        }

    def _add_nvstreamdemux(self) -> dict:
        """Build properties for ``nvstreamdemux`` (split batched mux output per stream).

        No GObject properties are set; demux pads map to source batch indices.
        """
        return {}

    def _add_nvosdbin(
        self,
        gpu_id: int = 0,
        display_bbox: bool = True,
        display_text: bool = True,
        display_mask: bool = False,
    ) -> dict:
        """Build properties for ``nvosdbin`` (on-screen display: boxes, labels, masks).

        Args:
            gpu_id: GPU device ID for OSD compositing.
            display_bbox: Draw bounding boxes around detected/tracked objects.
            display_text: Draw class labels and confidence text on objects.
            display_mask: Draw instance segmentation masks from object metadata.
        """
        return {
            "gpu-id": gpu_id,
            "display-bbox": display_bbox,
            "display-text": display_text,
            "display-mask": display_mask,
        }

    def _add_nvvideoconvert(self, gpu_id: int = 0) -> dict:
        """Build properties for ``nvvideoconvert`` (NVMM color space / format conversion).

        Args:
            gpu_id: GPU device ID for hardware color conversion.
        """
        return {"gpu-id": gpu_id}

    def _add_nvv4l2h264enc(
        self,
        bitrate: int = 4_000_000,
        iframeinterval: int = 25,
        preset_id: int = 1,
        gpu_id: int = 0,
    ) -> dict:
        """Build properties for ``nvv4l2h264enc`` (hardware H.264 encoder).

        Args:
            bitrate: Target encoder bitrate in bits per second (e.g. ``4_000_000`` = 4 Mbps).
            iframeinterval: Number of frames between forced IDR/I-frames (GOP length hint).
            preset_id: Encoder preset balancing speed vs quality (platform-specific; ``1`` is
                a common low-latency default on Jetson/dGPU).
            gpu_id: GPU device ID when encoding via NVENC on discrete GPUs.
        """
        return {
            "bitrate": bitrate,
            "iframeinterval": iframeinterval,
            "preset-id": preset_id,
            "gpu-id": gpu_id,
        }

    def _add_h264parse(self) -> dict:
        """Build properties for ``h264parse`` (align H.264 NAL units for muxing/streaming).

        No GObject properties are set; used between encoder and RTSP sink.
        """
        return {}

    def _add_mp4mux(self) -> dict:
        """Build properties for ``mp4mux`` (mux H.264 into MP4 container).

        No GObject properties are set; used before ``filesink`` for video file output.
        """
        return {}

    def _add_rtspclientsink(
        self,
        location: str,
        sync: bool = False,
        async_: bool = False,
    ) -> dict:
        """Build properties for ``rtspclientsink`` (publish encoded video as RTSP client).

        Args:
            location: RTSP publish URL (e.g. ``rtsp://mediamtx:8554/stream1``).
            sync: When True, synchronize to the pipeline clock (usually False for live inference).
            async_: When True, run the sink state change asynchronously.
        """
        return {
            "location": location,
            "sync": sync,
            "async": async_,
        }

    def _add_fakesink(self, sync: bool = False, async_: bool = False) -> dict:
        """Build properties for ``fakesink`` (discard buffers; headless / benchmark runs).

        Args:
            sync: When True, synchronize to the pipeline clock.
            async_: When True, run the sink state change asynchronously.
        """
        return {
            "sync": sync,
            "async": async_,
        }

    def _add_appsink(
        self,
        emit_signals: bool = True,
        sync: bool = False,
        max_buffers: int = 1,
        drop: bool = True,
        async_: bool = False,
    ) -> dict:
        """Build properties for ``appsink`` (deliver buffers to application via Receiver).

        Args:
            emit_signals: When True, emit ``new-sample`` for BufferRetriever attachment.
            sync: When True, synchronize to the pipeline clock.
            max_buffers: Maximum queued buffers before dropping or blocking.
            drop: When True, drop old buffers when the queue is full.
            async_: When True, run the sink state change asynchronously.
        """
        return {
            "emit-signals": emit_signals,
            "sync": sync,
            "max-buffers": max_buffers,
            "drop": drop,
            "async": async_,
        }

    def _add_nvjpegenc(self, quality: int = 85) -> dict:
        """Build properties for ``nvjpegenc`` (hardware JPEG encoder).

        Args:
            quality: JPEG quality factor in [0, 100].
        """
        return {"quality": quality}

    def _add_filesink(
        self,
        location: str | Path,
        sync: bool = False,
        async_: bool = False,
    ) -> dict:
        """Build properties for ``filesink`` (write encoded media to a local file).

        Args:
            location: Output file path written by the sink.
            sync: When True, synchronize to the pipeline clock.
            async_: When True, run the sink state change asynchronously.
        """
        return {
            "location": str(Path(location).expanduser().resolve()),
            "sync": sync,
            "async": async_,
        }
