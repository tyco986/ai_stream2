from pathlib import Path

import cupy as cp
import tensorrt as trt


class TrtRunner:
    def __init__(self, engine_path):
        self.engine_path = Path(engine_path)
        logger = trt.Logger(trt.Logger.ERROR)
        runtime = trt.Runtime(logger)
        engine = runtime.deserialize_cuda_engine(self.engine_path.read_bytes())
        names = [engine.get_tensor_name(i) for i in range(engine.num_io_tensors)]
        inputs = [n for n in names if engine.get_tensor_mode(n) == trt.TensorIOMode.INPUT]
        outputs = [n for n in names if engine.get_tensor_mode(n) == trt.TensorIOMode.OUTPUT]
        self.runtime = runtime
        self.engine = engine
        self.context = engine.create_execution_context()
        self.stream = cp.cuda.Stream()
        self.input_name = inputs[0]
        self.output_name = outputs[0]
        self.batch_size = self.resolve_batch_size()

    @property
    def input_shape(self):
        return tuple(self.engine.get_tensor_shape(self.input_name))

    def resolve_batch_size(self) -> int:
        shape = tuple(self.engine.get_tensor_shape(self.input_name))
        batch = int(shape[0])
        if batch < 0:
            batch = int(self.engine.get_tensor_profile_shape(self.input_name, 0)[2][0])
        return batch

    def infer(self, batch):
        context = self.context
        engine = self.engine
        engine_shape = tuple(engine.get_tensor_shape(self.input_name))
        dtype = cp.dtype(trt.nptype(engine.get_tensor_dtype(self.input_name)))
        used = batch.shape[0]
        feed = cp.ascontiguousarray(cp.asarray(batch, dtype=dtype))
        if engine_shape[0] > 0 and used != engine_shape[0]:
            padded = cp.zeros((engine_shape[0], *batch.shape[1:]), dtype=dtype)
            padded[:used] = feed
            feed = padded
        if any(dim < 0 for dim in engine_shape):
            context.set_input_shape(self.input_name, tuple(int(dim) for dim in feed.shape))
        buffers = {self.input_name: feed}
        for i in range(engine.num_io_tensors):
            name = engine.get_tensor_name(i)
            if name != self.input_name:
                out_dtype = cp.dtype(trt.nptype(engine.get_tensor_dtype(name)))
                buffers[name] = cp.empty(tuple(context.get_tensor_shape(name)), dtype=out_dtype)
            context.set_tensor_address(name, int(buffers[name].data.ptr))
        with self.stream:
            ok = context.execute_async_v3(int(self.stream.ptr))
        self.stream.synchronize()
        if not ok:
            raise ValueError(f"TensorRT execute failed: {self.engine_path}")
        result = buffers[self.output_name][:used]
        return result

    def __call__(self, batch):
        size = self.batch_size
        result = cp.concatenate(
            [
                self.infer(batch[index : index + size])
                for index in range(0, batch.shape[0], size)
            ],
            axis=0,
        )
        return result
