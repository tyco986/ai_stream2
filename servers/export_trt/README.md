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

API：`http://127.0.0.1:9000/ai_stream2/export_trt/health`  
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

1. `input` 上传 ONNX 目录的 zip，`config` 上传 YAML（见 `templates/`）
2. 解压到 `/root/models/onnx/{zip_stem}/`，调用 `trtexec` 写入 `trt/{name}/`
3. 返回 JSON，`data` 为 TRT 目录路径

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ai_stream2/export_trt/health` | 健康检查 |
| GET | `/ai_stream2/export_trt/types` | 可用导出类型 |
| POST | `/ai_stream2/export_trt/export` | 导出 TensorRT engine |

`POST` 为 `multipart/form-data`：`input` 为 zip，`config` 为 YAML。

YAML 字段：`type`（必填）、`batch_size`（动态模型必填）、`gpu_id`、`precision`、`opt_level`。

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/export_trt/export \
  -F "input=@yolo26n.zip" \
  -F "config=@servers/export_trt/templates/yolo26_det.yaml"
```

## 命令行

```bash
./servers/export_trt/scripts/3_export.sh --input models/onnx/yolo26n --config servers/export_trt/templates/yolo26_det.yaml
./servers/export_trt/scripts/3_export.sh --input models/onnx/yolov10x-smoke-fire --config servers/export_trt/templates/yolo10x_smoke_fire.yaml
./servers/export_trt/scripts/3_export.sh --input models/onnx/rtmpose-s-aic --config servers/export_trt/templates/rtmpose.yaml
```

`--input` 为 ONNX 目录路径；`--config` 为 YAML 路径。脚本会打包成 zip 再上传。

## 服务参数

| 变量 / 参数 | 默认值 |
|-------------|--------|
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `9000` |
| `--model-root` | `/root/models`（其下 `onnx/`、`trt/`） |
| `LOG_ROOT` / `--log-root` | `/root/logs/export_trt` |

日志：`{LOG_ROOT}/app.log`。
