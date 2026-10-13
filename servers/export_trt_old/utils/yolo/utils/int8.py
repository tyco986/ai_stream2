import ctypes
from pathlib import Path

import numpy as np
import tensorrt as trt
from PIL import Image

from utils.yolo.utils.constants import NET_SCALE_FACTOR, WORKSPACE_MIB


class YoloLetterbox:
    """DeepStream nvinfer maintain-aspect-ratio + symmetric-padding, RGB /255."""

    def __init__(self, height, width):
        self.height = height
        self.width = width

    def apply(self, image_path: Path) -> np.ndarray:
        image = Image.open(image_path).convert("RGB")
        src_w, src_h = image.size
        scale = min(self.width / src_w, self.height / src_h)
        new_w = min(self.width, int(src_w * scale + 0.5))
        new_h = min(self.height, int(src_h * scale + 0.5))
        resized = np.asarray(
            image.resize((new_w, new_h), Image.BILINEAR), dtype=np.float32
        )
        canvas = np.zeros((self.height, self.width, 3), dtype=np.float32)
        pad_x = (self.width - new_w) // 2
        pad_y = (self.height - new_h) // 2
        canvas[pad_y : pad_y + new_h, pad_x : pad_x + new_w] = resized
        canvas *= NET_SCALE_FACTOR
        nchw = np.transpose(canvas, (2, 0, 1))
        return nchw


class CudaRuntime:
    HOST_TO_DEVICE = 1

    def __init__(self, gpu_id):
        self.gpu_id = gpu_id
        self.lib = ctypes.CDLL("libcudart.so")
        self.lib.cudaSetDevice.argtypes = [ctypes.c_int]
        self.lib.cudaSetDevice.restype = ctypes.c_int
        self.lib.cudaMalloc.argtypes = [
            ctypes.POINTER(ctypes.c_void_p),
            ctypes.c_size_t,
        ]
        self.lib.cudaMalloc.restype = ctypes.c_int
        self.lib.cudaFree.argtypes = [ctypes.c_void_p]
        self.lib.cudaFree.restype = ctypes.c_int
        self.lib.cudaMemcpy.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.c_int,
        ]
        self.lib.cudaMemcpy.restype = ctypes.c_int
        status = self.lib.cudaSetDevice(gpu_id)
        if status != 0:
            raise ValueError(f"cudaSetDevice({gpu_id}) failed: {status}")

    def malloc(self, nbytes) -> ctypes.c_void_p:
        ptr = ctypes.c_void_p()
        status = self.lib.cudaMalloc(ctypes.byref(ptr), nbytes)
        if status != 0:
            raise ValueError(f"cudaMalloc({nbytes}) failed: {status}")
        return ptr

    def memcpy_htod(self, device_ptr, host_array) -> None:
        status = self.lib.cudaMemcpy(
            device_ptr,
            host_array.ctypes.data_as(ctypes.c_void_p),
            host_array.nbytes,
            self.HOST_TO_DEVICE,
        )
        if status != 0:
            raise ValueError(f"cudaMemcpy H2D failed: {status}")

    def free(self, device_ptr) -> None:
        if device_ptr and device_ptr.value:
            status = self.lib.cudaFree(device_ptr)
            if status != 0:
                raise ValueError(f"cudaFree failed: {status}")


class YoloMinMaxCalibrator(trt.IInt8MinMaxCalibrator):
    def __init__(
        self,
        image_paths,
        cache_path,
        batch_size,
        channels,
        height,
        width,
        gpu_id,
    ):
        self.image_paths = list(image_paths)
        self.cache_path = Path(cache_path)
        self.batch_size = batch_size
        self.channels = channels
        self.height = height
        self.width = width
        self.gpu_id = gpu_id
        super().__init__()
        self.letterbox = YoloLetterbox(height, width)
        self.cuda = CudaRuntime(gpu_id)
        self.offset = 0
        nbytes = batch_size * channels * height * width * 4
        self.device_ptr = self.cuda.malloc(nbytes)

    def get_batch_size(self) -> int:
        size = self.batch_size
        return size

    def get_batch(self, names) -> list[int] | None:
        del names
        pointers = None
        if self.offset < len(self.image_paths):
            chunk = self.image_paths[self.offset : self.offset + self.batch_size]
            self.offset += self.batch_size
            while len(chunk) < self.batch_size:
                chunk.append(chunk[-1])
            batch = np.stack([self.letterbox.apply(path) for path in chunk], axis=0)
            batch = np.ascontiguousarray(batch, dtype=np.float32)
            self.cuda.memcpy_htod(self.device_ptr, batch)
            pointers = [int(self.device_ptr.value)]
        return pointers

    def read_calibration_cache(self) -> bytes | None:
        cache = None
        if self.cache_path.is_file():
            cache = self.cache_path.read_bytes()
        return cache

    def write_calibration_cache(self, cache) -> None:
        self.cache_path.write_bytes(bytes(cache))

    def release(self) -> None:
        self.cuda.free(self.device_ptr)
        self.device_ptr = ctypes.c_void_p()


class YoloInt8EngineBuilder:
    def __init__(
        self,
        onnx_path,
        engine_path,
        cache_path,
        image_paths,
        batch_size,
        calib_batch,
        gpu_id,
        opt_level,
        input_name,
        channels,
        height,
        width,
        plugin_path,
        dynamic,
    ):
        self.onnx_path = Path(onnx_path)
        self.engine_path = Path(engine_path)
        self.cache_path = Path(cache_path)
        self.image_paths = image_paths
        self.batch_size = batch_size
        self.calib_batch = calib_batch
        self.gpu_id = gpu_id
        self.opt_level = opt_level
        self.input_name = input_name
        self.channels = channels
        self.height = height
        self.width = width
        self.plugin_path = plugin_path
        self.dynamic = dynamic

    def parse_errors(self, parser) -> str:
        errors = [str(parser.get_error(index)) for index in range(parser.num_errors)]
        text = "; ".join(errors) if errors else "onnx parse failed"
        return text

    def build(self) -> None:
        logger = trt.Logger(trt.Logger.WARNING)
        if self.plugin_path is not None:
            ctypes.CDLL(str(self.plugin_path), mode=ctypes.RTLD_GLOBAL)
        trt.init_libnvinfer_plugins(logger, "")
        CudaRuntime(self.gpu_id)

        builder = trt.Builder(logger)
        network = builder.create_network(
            1 << int(trt.NetworkDefinitionCreationFlag.EXPLICIT_BATCH)
        )
        parser = trt.OnnxParser(network, logger)
        parsed = parser.parse_from_file(str(self.onnx_path))
        if not parsed:
            raise ValueError(self.parse_errors(parser))

        config = builder.create_builder_config()
        config.set_memory_pool_limit(
            trt.MemoryPoolType.WORKSPACE, WORKSPACE_MIB * 1024 * 1024
        )
        if self.opt_level is not None:
            config.builder_optimization_level = self.opt_level
        config.set_flag(trt.BuilderFlag.INT8)
        config.set_flag(trt.BuilderFlag.FP16)

        if self.dynamic:
            profile = builder.create_optimization_profile()
            min_shape = (1, self.channels, self.height, self.width)
            opt_shape = (self.batch_size, self.channels, self.height, self.width)
            profile.set_shape(self.input_name, min_shape, opt_shape, opt_shape)
            config.add_optimization_profile(profile)
            config.set_calibration_profile(profile)

        calibrator = YoloMinMaxCalibrator(
            self.image_paths,
            self.cache_path,
            self.calib_batch,
            self.channels,
            self.height,
            self.width,
            self.gpu_id,
        )
        config.int8_calibrator = calibrator
        serialized = builder.build_serialized_network(network, config)
        calibrator.release()
        if serialized is None:
            raise ValueError("int8 engine build failed")
        self.engine_path.write_bytes(bytes(serialized))
