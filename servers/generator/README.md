# Generator API

在 Generator 容器内运行的 FastAPI 服务，根据 YOLO 模型与输入源生成 DeepStream pipeline 配置（`pipeline.yml`、`pgie.yml`、`params.yml` 等）。

## 镜像分层

| 镜像 | 构建 | 用途 |
|------|------|------|
| `ai_stream2_generator_dev` | `1_build_dev_image.sh` | 开发：运行时依赖 |
| `ai_stream2_generator_prod` | `1_build_prod_image.sh` | 生产：dev + 内置 `servers/generator` 代码 |

## 开发与生产

```bash
# 开发：挂代码，改 Python 后 restart 即可
./servers/generator/scripts/1_build_dev_image.sh   # 改 pip 依赖后
./servers/generator/scripts/2_run_dev_container.sh

# 生产：代码打进镜像
./servers/generator/scripts/1_build_prod_image.sh
./servers/generator/scripts/2_run_prod_container.sh
```

API：`http://127.0.0.1:8091/ai_stream2/generator/generate`  
Swagger：`http://127.0.0.1:8091/docs`（试调入口；完整说明见本文）  
容器名：`ai_stream2_generator`

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/generator` | `/app` | 开发模式：Python 代码 |
| `models/` | `/root/models` | 读取 TRT 模型（`pgie_model_dir`） |
| `configs/generator/{name}/` | `/root/configs/generator/{name}/` | 模板默认输出目录（`servers/generator/templates/**/*.yaml` 中 `config_save_dir`） |
| `logs/` | `/root/logs` | 服务日志（`LOG_ROOT` 默认 `/root/logs/generator`） |

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/ai_stream2/generator/generate` | 上传 generator YAML，生成 DeepStream 配置并落盘 |

### 请求

`multipart/form-data`，字段 `input`：generator YAML 文件（与 `servers/generator/templates/**/*.yaml` 同结构）。

YAML 中相对路径 `models/*`、`configs/*` 会在服务端映射为 `/root/models/*`、`/root/configs/*`。

### 支持的 generator

| generator | 场景 |
|-----------|------|
| `YoloDetImageGenerator` | 单张图片检测 |
| `YoloSegImageGenerator` | 单张图片分割 |
| `YoloDetSahiImageGenerator` | 单张图片 SAHI 检测 |
| `YoloDetVideoGenerator` | 视频文件检测 |
| `YoloSegVideoGenerator` | 视频文件分割 |
| `YoloDetSahiVideoGenerator` | 视频文件 SAHI 检测 |
| `YoloDetRTSPGenerator` | 多路 RTSP 检测 |
| `YoloSegRTSPGenerator` | 多路 RTSP 分割 |
| `YoloDetSahiRTSPGenerator` | 多路 RTSP SAHI 检测 |

### 公共字段

| 字段 | 说明 |
|------|------|
| `generator` | 生成器类名（必填） |
| `config_save_dir` | 配置输出目录（必填；模板见 `servers/generator/templates/`，默认 `/root/configs/generator/{模板名}`） |
| `pgie_model_dir` | 模型目录，含 `meta.json`、`labels.txt`、唯一 `.engine` |
| `pgie.class_attrs` | 检测阈值等，默认 `{"all": {"conf": 0.25}}`；含 `all` 则全开，仅数字 key 则启用这些类 |
| `enable_kafka` | 是否输出 Kafka 元数据分支 |

Image / Video 额外字段：`input`、`output`。  
RTSP 额外字段：`streams`、`enable_nvtracker`、`interval`、`nvdsanalytics_config`。  
SAHI 额外字段：`sahi_config`。

生成目录中除 pipeline 相关 YAML 外，还会写入 `params.yml`（完整入参快照，含 `generator`）。

`pgie_model_dir` 要求见 `utils/subelement_generator/utils/pgie_parser.py`。RTSP 生成时会探测流地址，且多路流分辨率需一致。

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
  -F "input=@servers/generator/templates/yolo/yolo26n_det_image.yaml"
```

## 命令行

```bash
./servers/generator/scripts/3_generate.sh --config yolo26n_det_rtsp
./servers/generator/scripts/3_generate.sh --config servers/generator/templates/yolo/yolo26n_det_image.yaml
```

`--config` 可为 `servers/generator/templates/` 下 YAML 路径，或模板名（不含 `.yaml`，会在子目录中解析）。路径映射在服务端完成。

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
