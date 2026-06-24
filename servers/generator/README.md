# Generator API

在 `ai_stream2_generator` 镜像中运行的 FastAPI 服务，根据 YOLO 模型与输入源生成 DeepStream pipeline 配置（`pipeline.yml`、`pgie.yml`、`params.yml` 等）。

## 构建与运行

```bash
# 项目根目录
./servers/generator/scripts/1_build_image.sh
./servers/generator/scripts/2_run_container.sh
```

API：`http://127.0.0.1:8091/ai_stream2/generator/generate`  
Swagger：`http://127.0.0.1:8091/docs`（试调入口；完整说明见本文）  
容器名：`ai_stream2_generator`

## 目录

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `models/` | `/root/models` | 读取 TRT 模型（`pgie_model_dir`） |
| `configs/generator/{name}/` | `/root/configs/generator/{name}/` | 模板默认输出目录（`servers/generator/templates/*.yaml` 中 `config_save_dir`） |
| `logs/` | `/root/logs` | 服务日志（`LOG_ROOT` 默认 `/root/logs/generator`） |

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/ai_stream2/generator/generate` | 上传 generator YAML，生成 DeepStream 配置并落盘 |

### 请求

`multipart/form-data`，字段 `input`：generator YAML 文件（与 `servers/generator/templates/*.yaml` 同结构）。

YAML 中相对路径 `models/*`、`configs/*` 会在服务端映射为 `/root/models/*`、`/root/configs/*`。

### 支持的 generator

| generator | 场景 |
|-----------|------|
| `YoloDetImageConfigGenerator` | 单张图片检测 |
| `YoloSegImageConfigGenerator` | 单张图片分割 |
| `YoloPoseImageConfigGenerator` | 单张图片姿态 |
| `YoloDetSahiImageConfigGenerator` | 单张图片 SAHI 检测 |
| `YoloDetVideoConfigGenerator` | 视频文件检测 |
| `YoloSegVideoConfigGenerator` | 视频文件分割 |
| `YoloPoseVideoConfigGenerator` | 视频文件姿态 |
| `YoloDetSahiVideoConfigGenerator` | 视频文件 SAHI 检测 |
| `YoloDetRTSPConfigGenerator` | 多路 RTSP 检测 |
| `YoloSegRTSPConfigGenerator` | 多路 RTSP 分割 |
| `YoloPoseRTSPConfigGenerator` | 多路 RTSP 姿态 |
| `YoloDetSahiConfigGenerator` | 多路 RTSP SAHI 检测 |

### 公共字段

| 字段 | 说明 |
|------|------|
| `generator` | 生成器类名（必填） |
| `config_save_dir` | 配置输出目录（必填；模板见 `servers/generator/templates/`，默认 `/root/configs/generator/{模板名}`） |
| `pgie_model_dir` | 模型目录，含 `meta.json`、`labels.txt`、唯一 `.engine` |
| `pgie_class_attr` | 检测阈值等，默认 `{"all": {"conf": 0.25}}` |
| `pgie_class_on` | 启用的类别 id 列表；省略表示全部类别 |
| `enable_kafka` | 是否输出 Kafka 元数据分支 |

Image / Video 额外字段：`input`、`output`。  
RTSP 额外字段：`streams`、`enable_visualized_rtsp`、`enable_nvtracker`、`interval`、`nvdsanalytics_config`。  
SAHI 额外字段：`sahi_config`。

生成目录中除 pipeline 相关 YAML 外，还会写入 `params.yml`（完整入参快照，含 `generator`）。

`pgie_model_dir` 要求见 `utils/yolo_generator/utils/pgie_config_parser.py`。RTSP 生成时会探测流地址，且多路流分辨率需一致。

### 响应格式

成功：

```json
{"success": true, "message": ""}
```

失败（HTTP 400 / 422）：

```json
{"success": false, "message": "..."}
```

### curl 示例

```bash
curl -s -X POST http://127.0.0.1:8091/ai_stream2/generator/generate \
  -F "input=@servers/generator/templates/yolo26n_det_image.yaml"
```

## 命令行

```bash
./servers/generator/scripts/3_generate.sh --config yolo26n_det_rtsp
./servers/generator/scripts/3_generate.sh --config servers/generator/templates/yolo26n_det_image.yaml
```

`--config` 可为 `servers/generator/templates/` 下 YAML 路径，或模板名（不含 `.yaml`）。路径映射在服务端完成。

旧 DeepStream 容器内生成脚本见 `servers/deepstream/scripts/deepstream/3_generate_pipeline.sh`（已弃用，请用本服务）。

## 服务参数

环境变量（Dockerfile 默认）：

| 变量 | 默认值 |
|------|--------|
| `HOST` | `0.0.0.0` |
| `PORT` | `8091` |
| `LOG_ROOT` | `/root/logs/generator` |

CLI 可覆盖：

```bash
python main.py --host 0.0.0.0 --port 8091 --log-root /root/logs/generator
```

日志：`{LOG_ROOT}/app.log`（滚动，10×1MB）。
