"""Compare probe overhead: main path (batch) vs branch path (per-stream).

Run inside ai_stream2_deepstream container:

    python3 scripts/probe_path_benchmark.py main 2 300
    python3 scripts/probe_path_benchmark.py branch 2 300
"""

import statistics
import sys
import time

from pyservicemaker import BatchMetadataOperator, Pipeline, Probe

VIDEO = "/opt/nvidia/deepstream/deepstream/samples/streams/sample_720p.mp4"
PGIE_CONFIG = "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_infer_primary.txt"


def parse_batch(batch_meta):
    results = []
    for frame_meta in batch_meta.frame_items:
        objects = []
        for obj in frame_meta.object_items:
            rect = obj.rect_params
            objects.append(
                {
                    "cls": int(obj.class_id),
                    "conf": float(obj.confidence),
                    "box": [
                        int(round(float(rect.left))),
                        int(round(float(rect.top))),
                        int(round(float(rect.left) + float(rect.width))),
                        int(round(float(rect.top) + float(rect.height))),
                    ],
                }
            )
        results.append(
            {
                "pad_index": int(frame_meta.pad_index),
                "frame_number": int(frame_meta.frame_number),
                "source_id": int(frame_meta.source_id),
                "objects": objects,
            }
        )
    return results


class BenchmarkProbe(BatchMetadataOperator):
    def __init__(self, name, frame_limit):
        super().__init__()
        self.name = name
        self.frame_limit = frame_limit
        self.callback_count = 0
        self.frame_count = 0
        self.elapsed_seconds = []

    def handle_metadata(self, batch_meta):
        start = time.perf_counter()
        results = parse_batch(batch_meta)
        object_count = sum(len(frame_result["objects"]) for frame_result in results)
        elapsed = time.perf_counter() - start
        self.elapsed_seconds.append(elapsed)
        self.callback_count += 1
        self.frame_count += len(results)
        if object_count == 0 and self.callback_count <= 3:
            print(f"{self.name} warn: no objects in callback {self.callback_count}")
        if self.callback_count >= self.frame_limit:
            self.print_summary()
            return 0
        return 1

    def print_summary(self):
        samples = self.elapsed_seconds
        total = sum(samples)
        print(f"\n=== {self.name} ===")
        print(f"callbacks={self.callback_count} frames={self.frame_count}")
        print(f"total_probe_s={total:.4f} avg_ms={statistics.mean(samples) * 1000:.3f}")
        if len(samples) > 1:
            print(f"p50_ms={statistics.median(samples) * 1000:.3f} p95_ms={sorted(samples)[int(len(samples) * 0.95) - 1] * 1000:.3f}")
        print(f"probe_fps={self.callback_count / total:.1f}" if total > 0 else "probe_fps=inf")


def add_sources(pipeline, stream_count):
    for index in range(stream_count):
        pipeline.add(
            "nvurisrcbin",
            f"src{index}",
            {"uri": f"file://{VIDEO}", "disable-audio": True},
        )


def link_sources_to_mux(pipeline, stream_count):
    for index in range(stream_count):
        pipeline.link((f"src{index}", "mux"), ("", "sink_%u"))


def build_main_pipeline(stream_count, frame_limit):
    pipeline = Pipeline("probe-bench-main")
    add_sources(pipeline, stream_count)
    pipeline.add(
        "nvstreammux",
        "mux",
        {"batch-size": stream_count, "width": 1280, "height": 720, "batched-push-timeout": 40000},
    )
    pipeline.add(
        "nvinfer",
        "pgie",
        {"config-file-path": PGIE_CONFIG, "batch-size": stream_count},
    )
    pipeline.add(
        "nvtracker",
        "tracker",
        {"ll-lib-file": "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so"},
    )
    pipeline.add("fakesink", "sink", {"sync": False})
    link_sources_to_mux(pipeline, stream_count)
    pipeline.link("mux", "pgie", "tracker", "sink")
    pipeline.attach("tracker", Probe("bench_main", BenchmarkProbe("main_tracker", frame_limit)))
    return pipeline


def build_branch_pipeline(stream_count, frame_limit):
    pipeline = Pipeline("probe-bench-branch")
    branch_probes = []
    add_sources(pipeline, stream_count)
    pipeline.add(
        "nvstreammux",
        "mux",
        {"batch-size": stream_count, "width": 1280, "height": 720, "batched-push-timeout": 40000},
    )
    pipeline.add(
        "nvinfer",
        "pgie",
        {"config-file-path": PGIE_CONFIG, "batch-size": stream_count},
    )
    pipeline.add(
        "nvtracker",
        "tracker",
        {"ll-lib-file": "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so"},
    )
    pipeline.add("nvstreamdemux", "demux")
    link_sources_to_mux(pipeline, stream_count)
    pipeline.link("mux", "pgie", "tracker", "demux")
    for index in range(stream_count):
        pipeline.add("queue", f"queue_demux{index}", {"leaky": 2, "max-size-buffers": 4})
        pipeline.add("nvvideoconvert", f"nvvidconv{index}")
        pipeline.add("fakesink", f"sink{index}", {"sync": False})
        pipeline.link(("demux", f"queue_demux{index}"), (f"src_%u", ""))
        pipeline.link(f"queue_demux{index}", f"nvvidconv{index}", f"sink{index}")
        probe = BenchmarkProbe(f"branch_nvvidconv{index}", frame_limit)
        branch_probes.append(probe)
        pipeline.attach(f"nvvidconv{index}", Probe(f"bench_branch{index}", probe))
    return pipeline, branch_probes


def print_branch_aggregate(probes):
    all_samples = []
    total_callbacks = 0
    total_frames = 0
    for probe in probes:
        all_samples.extend(probe.elapsed_seconds)
        total_callbacks += probe.callback_count
        total_frames += probe.frame_count
    total = sum(all_samples)
    print("\n=== branch_aggregate ===")
    print(f"callbacks={total_callbacks} frames={total_frames} probes={len(probes)}")
    print(f"total_probe_s={total:.4f} avg_ms={statistics.mean(all_samples) * 1000:.3f}")
    if len(all_samples) > 1:
        print(f"p50_ms={statistics.median(all_samples) * 1000:.3f} p95_ms={sorted(all_samples)[int(len(all_samples) * 0.95) - 1] * 1000:.3f}")
    print(f"probe_fps={total_callbacks / total:.1f}" if total > 0 else "probe_fps=inf")


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "main"
    stream_count = int(sys.argv[2]) if len(sys.argv) > 2 else 2
    frame_limit = int(sys.argv[3]) if len(sys.argv) > 3 else 300
    if mode == "main":
        pipeline = build_main_pipeline(stream_count, frame_limit)
        branch_probes = []
    elif mode == "branch":
        pipeline, branch_probes = build_branch_pipeline(stream_count, frame_limit)
    else:
        raise SystemExit("mode must be main or branch")
    print(f"mode={mode} streams={stream_count} frame_limit={frame_limit}")
    pipeline.start().wait()
    if mode == "branch":
        print_branch_aggregate(branch_probes)


if __name__ == "__main__":
    main()
