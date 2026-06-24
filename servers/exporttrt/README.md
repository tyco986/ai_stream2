# Export TRT API

在 `ai_stream2_exporttrt` 镜像中将 dsyolo 产出的 ONNX 目录编译为 TensorRT engine。

## 构建与运行

```bash
# 项目根目录
./servers/exporttrt/scripts/0_pull_base_image.sh   # 可选
./servers/exporttrt/scripts/1_build_image.sh
./servers/exporttrt/scripts/2_run_container.sh
```

API：`http://127.0.0.1:9000/ai_stream2/exporttrt/export_engine`  
Swagger：`http://127.0.0.1:9000/docs`  
容器名：`ai_stream2_exporttrt`（需 `--gpus all`）

## 目录

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
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
4. 返回 JSON，`message` 为 TRT 目录路径

## 接口

`POST /ai_stream2/exporttrt/export_engine`，`multipart/form-data`：

| 字段 | 默认 | 说明 |
|------|------|------|
| `input` | — | ONNX 文件夹路径（必填） |
| `batch_size` | — | 动态模型必填；静态模型勿传 |
| `gpu_id` | `0` | 写入 meta.json |
| `precision` | `fp16` | engine 精度：`fp32` / `fp16` / `int8` |

成功返回 JSON（`message` 为 `/root/models/trt/{name}/`）。失败返回 JSON 错误。

### curl 示例

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/exporttrt/export_engine \
  -F "input=/root/models/onnx/yolo26n"
```

动态 batch：

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/exporttrt/export_engine \
  -F "input=/root/models/onnx/yolo26n" \
  -F "batch_size=4"
```

## 命令行

```bash
./servers/exporttrt/scripts/3_export_engine.sh --input models/onnx/yolo26n
```

`--input` 须为 `models/` 下的 ONNX 目录（映射为容器内 `/root/models/...`）。

## 服务参数

| 变量 / 参数 | 默认值 |
|-------------|--------|
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `9000` |
| `--model-root` | `/root/models`（其下 `onnx/`、`trt/`） |
| `LOG_ROOT` / `--log-root` | `/root/logs/exporttrt` |

日志：`{LOG_ROOT}/app.log`。
