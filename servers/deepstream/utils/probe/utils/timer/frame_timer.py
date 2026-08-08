import ctypes
from pathlib import Path

LATENCY_PROBE_SO = (
    "/opt/nvidia/deepstream/deepstream/service-maker/modules/liblatency_probe.so"
)

# Logical keys from configs/generator/**/sink_path.yml (+ end-to-end latency).
# Indexed element names (rtspclientsink{N}, ...) are normalized to the base key.
AVAILABLE_TIME = (
    "latency",
    "nvurisrcbin",
    "nvstreammux",
    "nvsahipreprocess",
    "nvinfer",
    "nvsahipostprocess",
    "nvtracker",
    "nvdsanalytics",
    "nvstreamdemux",
    "queue_demux",
    "nvvideoconvert",
    "nvvideoconvert_raw",
    "nvvideoconvert_osd",
    "nvvideoconvert_vis",
    "nvosdbin",
    "tee_raw",
    "tee_vis",
    "queue_raw",
    "queue_osd",
    "queue_vis",
    "queue_enc",
    "queue_sahi",
    "capsfilter_raw",
    "capsfilter_osd",
    "capsfilter_vis",
    "nvjpegenc",
    "nvv4l2h264enc",
    "h264parse",
    "mp4mux",
    "filesink",
    "appsink_raw",
    "appsink_vis",
    "rtspclientsink",
)

MAX_COMPONENTS = 64
MAX_COMPONENT_NAME = 64


class Timer:
    """Read selected pipeline timing fields for one frame.

    ``elements`` must be a subset of ``AVAILABLE_TIME``. ``read`` returns a dict
    mapping each requested name to milliseconds or None.
    """

    def __init__(self, elements=AVAILABLE_TIME):
        self.elements = list(elements)
        self.lib = None
        invalid = [name for name in self.elements if name not in AVAILABLE_TIME]
        assert not invalid, f"elements not in AVAILABLE_TIME: {invalid}"
        self.load_lib()

    def load_lib(self):
        assert Path(LATENCY_PROBE_SO).exists(), f"missing latency probe: {LATENCY_PROBE_SO}"
        lib = ctypes.CDLL(LATENCY_PROBE_SO)
        lib.latency_probe_take.argtypes = [
            ctypes.c_uint,
            ctypes.c_uint,
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.latency_probe_take.restype = ctypes.c_int
        self.lib = lib

    def read(self, source_id, frame_number) -> dict:
        result = {name: None for name in self.elements}
        latency_value = ctypes.c_double(-1.0)
        count_value = ctypes.c_int(0)
        name_array = ((ctypes.c_char * MAX_COMPONENT_NAME) * MAX_COMPONENTS)()
        value_array = (ctypes.c_double * MAX_COMPONENTS)()
        ok = self.lib.latency_probe_take(
            ctypes.c_uint(int(source_id)),
            ctypes.c_uint(int(frame_number)),
            ctypes.byref(latency_value),
            ctypes.cast(name_array, ctypes.c_void_p),
            value_array,
            ctypes.c_int(MAX_COMPONENTS),
            ctypes.byref(count_value),
        )
        if ok:
            components = {
                bytes(name_array[index]).split(b"\x00", 1)[0].decode("utf-8", errors="ignore"): round(
                    float(value_array[index]), 2
                )
                for index in range(int(count_value.value))
                if bytes(name_array[index]).split(b"\x00", 1)[0]
            }
            for name in self.elements:
                if name == "latency":
                    if latency_value.value >= 0.0:
                        result[name] = round(float(latency_value.value), 2)
                else:
                    result[name] = components.get(name)
        return result
