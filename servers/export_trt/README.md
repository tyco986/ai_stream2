# Export TRT API

在 Export TRT 容器内将 export_onnx 产出的 ONNX 目录编译为 TensorRT engine。

## 镜像分层

| 镜像 | 构建 | 用途 |
|------|------|------|
| `ai_stream2_export_trt_dev` | `1_build_dev_image.sh` | 开发：运行时依赖 + seg static plugin |
| `ai_stream2_export_trt_prod` | `1_build_prod_image.sh` | 生产：dev + 内置 `servers/export_trt` 代码 |

原生产物：`libnvdsinfer_custom_impl_Yolo_seg.so`（来自 `attachments/deepstream-sahi/nvdsinfer_custom_impl_Yolo_seg.zip`）在 `/opt/ai_stream2/servers/export_trt/libs`（不随 `/app` 挂载覆盖）。segment 导出时经 `--staticPlugins` 注入。

## 开发与生产

```bash
# 开发：挂代码，改 Python 后 restart 即可
./servers/export_trt/scripts/1_build_dev_image.sh   # 改 pip / native plugin 后
./servers/export_trt/scripts/2_run_dev_container.sh

# 生产：代码打进镜像
./servers/export_trt/scripts/1_build_prod_image.sh
./servers/export_trt/scripts/2_run_prod_container.sh
```

API：`http://127.0.0.1:9000/ai_stream2/export_trt/export_engine`  
Swagger：`http://127.0.0.1:9000/docs`  
容器名：`ai_stream2_export_trt`（需 `--gpus all`）

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/export_trt` | `/app` | 开发模式：Python 代码 |
| `models/` | `/root/models` | ONNX 输入目录、TRT 产物 |
| `logs/` | `/root/logs` | 服务日志 |

| 路径 | 内容 |
|------|------|
| `/root/models/onnx/{name}/` | `.onnx`、`.onnx.data`、`labels.txt`、`meta.json` |
| `/root/models/trt/{name}/` | `.engine`、`labels.txt`、`meta.json` |

## 导出流程

1. `input` 传入容器内 ONNX 文件夹路径；目录不存在则报错
2. 校验目录含 `*.onnx`、`labels.txt`、`meta.json`
3. 调用镜像内 `trtexec` 构建 engine，写入 `trt/{name}/`
4. 返回 JSON，`data` 为 TRT 目录路径

## 接口

`POST /ai_stream2/export_trt/export_engine`，`multipart/form-data`：

| 字段 | 默认 | 说明 |
|------|------|------|
| `input` | — | ONNX 文件夹路径（必填） |
| `batch_size` | — | 动态模型必填；静态模型勿传 |
| `gpu_id` | `0` | 写入 meta.json |
| `precision` | `fp16` | engine 精度：`fp32` / `fp16` / `int8` |
| `opt_level` | — | `trtexec --builderOptimizationLevel`（可选） |

成功返回 JSON（`data` 为 `/root/models/trt/{name}/`）。失败返回 JSON 错误。

### curl 示例

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/export_trt/export_engine \
  -F "input=/root/models/onnx/yolo26n"
```

动态 batch：

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/export_trt/export_engine \
  -F "input=/root/models/onnx/yolo26n" \
  -F "batch_size=4"
```

YOLO10 smoke-fire 等需固定 opt_level：

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/export_trt/export_engine \
  -F "input=/root/models/onnx/yolov10x-smoke-fire" \
  -F "batch_size=1" \
  -F "opt_level=0"
```

## 命令行

```bash
./servers/export_trt/scripts/3_export_engine.sh --input models/onnx/yolo26n
./servers/export_trt/scripts/3_export_engine.sh --input models/onnx/yolov10x-smoke-fire --batch-size 1 --opt-level 0
```

`--input` 须为 `models/` 下的 ONNX 目录（映射为容器内 `/root/models/...`）。

## 服务参数

| 变量 / 参数 | 默认值 |
|-------------|--------|
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `9000` |
| `--model-root` | `/root/models`（其下 `onnx/`、`trt/`） |
| `LOG_ROOT` / `--log-root` | `/root/logs/export_trt` |

日志：`{LOG_ROOT}/app.log`。
