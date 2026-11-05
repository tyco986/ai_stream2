# DeepStream 9.0 GStreamer 插件列表

来源：DeepStream 容器内 `gst-inspect-1.0`。
合计：**68** 个可 inspect 的 element。

| 名称 | 说明 | 类型 | 来源 |
| --- | --- | --- | --- |
| `dsexample` | NVIDIA 示例插件，用于在 DGPU 上与 DeepStream 集成 | Transform | official-example |
| `nv3dsink` | NVIDIA 视频 Sink 插件 | Sink | official |
| `nvanalyticsbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvblender` | NVIDIA 合成/混合插件，用于 DGPU/Jetson 上的 DeepStream | Compositor | official |
| `nvcamerasrcbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvdewarper` | 360° 画面去畸变 GStreamer 插件 | Transform | official |
| `nvdewarperbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvds3dbridge` | NVIDIA DeepStream 3D bridge 自定义插件 | Transform | official |
| `nvds3dfilter` | NVIDIA DeepStream 3D filter 自定义插件 | Transform | official |
| `nvds3dmixer` | NVIDIA 3D Mixer 插件 | Mux | official |
| `nvdsA2Vtemplate` | NVIDIA nvdsA2Vtemplate 插件，处理音频输入到视频输出相关场景 | Element | official |
| `nvds_text_to_speech` | NVIDIA DeepStream 语音合成插件 | Transform | official |
| `nvdsanalytics` | NVIDIA dsanalytics 插件，用于 DGPU/Jetson 上与 DeepStream 集成 | Transform | official |
| `nvdsasr` | NVIDIA DeepStream 自动语音识别插件 | Transform | official |
| `nvdsaudiotemplate` | NVIDIA 示例模板插件，用于 DGPU/Jetson 上与 DeepStream 集成 | Transform | official |
| `nvdsbuffersyncbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvdsdynamicsrcbin` | 读取多个文件的 GStreamer 插件 | Bin | official |
| `nvdslogger` | 日志 GStreamer 插件 | Transform | official |
| `nvdsmetaextract` | DeepStream META 插入与提取元件 | Transform | official |
| `nvdsmetainsert` | DeepStream META 插入与提取元件 | Transform | official |
| `nvdsmetamux` | NVIDIA batch meta mux 插件，用于 DGPU/Jetson 上与 DeepStream 集成 | Mux | official |
| `nvdsosd` | 绘制矩形与文字的 GStreamer 插件 | Transform | official |
| `nvdspostprocess` | NVIDIA nvdspostprocess 插件，解析 nvdsinfer/nvdsinferserver 的推理输出（DGPU/Jetson） | Transform | official |
| `nvdspreprocess` | NVIDIA 自定义预处理插件，用于 DGPU/Jetson 上与 DeepStream 集成 | Transform | official |
| `nvdsucxclientsink` | NVIDIA UCX sink/source 插件 | Sink | official |
| `nvdsucxclientsrc` | NVIDIA UCX sink/source 插件 | Source | official |
| `nvdsucxserversink` | NVIDIA UCX sink/source 插件 | Sink | official |
| `nvdsucxserversrc` | NVIDIA UCX sink/source 插件 | Source | official |
| `nvdsvideotemplate` | NVIDIA nvdsvideotemplate 插件，用于 DGPU/Jetson 上与 DeepStream 集成 | Transform | official |
| `nvdsvisionencoder` | NVIDIA DeepStream 视觉编码器插件 | Transform | official |
| `nvdsxfer` | NVIDIA NVDSXFER 插件 | Transform | official |
| `nveglglessink` | EGL/GLES sink | Sink | official |
| `nvimagedec` | NVIDIA 图像解码/编码插件 | Decoder | official |
| `nvimageenc` | NVIDIA 图像解码/编码插件 | Encoder | official |
| `nvinfer` | NVIDIA DeepStreamSDK TensorRT 插件 | Transform | official |
| `nvinferaudio` | NVIDIA DeepStreamSDK 音频推理插件 | Transform | official |
| `nvinferbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvinferserver` | NVIDIA DeepStreamSDK TensorRT Inference Server 插件 | Transform | official |
| `nvinferserverbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvjpegdec` | NVIDIA 加速 JPEG 插件库 | Decoder | official |
| `nvjpegenc` | NVIDIA 加速 JPEG 插件库 | Encoder | official |
| `nvmsgbroker` | 消息 broker | Sink | official |
| `nvmsgbrokersinkbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvmsgconv` | 元数据转换 | Transform | official |
| `nvmultistreamtiler` | NVIDIA 多路拼接 Tiler 插件 | Compositor | official |
| `nvmultiurisrcbin` | DeepStream SDK nvmultiurisrcbin Bin | Bin | official |
| `nvof` | NVIDIA 光流插件，用于 DGPU 上与 DeepStream 集成 | Transform | official |
| `nvofvisual` | NVIDIA 光流可视化插件，用于 DGPU 上与 DeepStream 集成 | Transform | official |
| `nvosdbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvreplay` | 加载 ground truth 标签用于评估的 GStreamer 插件 | Transform | official |
| `nvrtspoutsinkbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvsegvisual` | NVIDIA 分割可视化插件，用于 DGPU 上与 DeepStream 集成 | Transform | official |
| `nvstreamdemux` | NVIDIA 多路 mux/demux 插件 | Demux | official |
| `nvstreammux` | NVIDIA 多路 mux/demux 插件 | Mux | official |
| `nvtilerbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvtracker` | 目标跟踪 GStreamer 插件 | Transform | official |
| `nvtrackerbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvunixfdsink` | Unix 文件描述符 sink/source | Sink | official |
| `nvunixfdsrc` | Unix 文件描述符 sink/source | Source | official |
| `nvurisrcbin` | DeepStream SDK nvurisrcbin Bin | Bin | official |
| `nvv4l2av1enc` | NVIDIA Video 4 Linux 元件 | Encoder | official |
| `nvv4l2decoder` | NVIDIA Video 4 Linux 元件 | Decoder | official |
| `nvv4l2h264enc` | NVIDIA Video 4 Linux 元件 | Encoder | official |
| `nvv4l2h265enc` | NVIDIA Video 4 Linux 元件 | Encoder | official |
| `nvvideoconvert` | 视频色彩空间转换与缩放 | Transform | official |
| `nvvideoencfilesinkbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvvideorenderersinkbin` | DeepStream SDK 通用 Gst Bin | Bin | official |
| `nvvideotestsrc` | 使用 CUDA 生成填有测试图案的 NVIDIA GPU buffer（memory:NVMM） | Source | official |
