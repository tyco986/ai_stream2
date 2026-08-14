# ExportOnnx Export API

在 `ai_stream2_export_onnx` 镜像中运行的 FastAPI 服务，将 `.pt` 权重导出为 DeepStream 可用的 ONNX 产物。

## 构建与运行

```bash
# 项目根目录
./servers/export_onnx/scripts/0_pull_base_image.sh   # 可选
./servers/export_onnx/scripts/1_build_image.sh
./servers/export_onnx/scripts/2_run_container.sh
```

容器名 `{PROJECT_NAME}_export_onnx`（默认 `ai_stream2_export_onnx`），端口 `8090`。挂载 `models` → `/root/models`，`logs` → `/root/logs`。

API：`http://127.0.0.1:8090/ai_stream2/export_onnx/health`  
Swagger：`http://127.0.0.1:8090/docs`

## 导出流程

1. `input` 上传 `.pt`，`config` 上传 YAML（见 `templates/`）
2. `.pt` 落盘到 `/root/models/pt/{name}.pt`，清空并创建 `/root/models/onnx/{name}/` 后导出
3. 返回 JSON，`data` 为 ONNX 目录路径

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ai_stream2/export_onnx/health` | 健康检查 |
| GET | `/ai_stream2/export_onnx/types` | 可用导出类型 |
| POST | `/ai_stream2/export_onnx/export` | 导出 ONNX |

`POST` 为 `multipart/form-data`：`input` 为 `.pt`，`config` 为 YAML。

YAML 字段：`type`（必填）、`size`、`opset`、`batch`、`dynamic`、`simplify`、`max_det`、`conf`、`iou`。`dynamic=true` 与 `batch>1` 不可同时使用。

```bash
curl -s -X POST http://127.0.0.1:8090/ai_stream2/export_onnx/export \
  -F "input=@models/pt/yolo26n.pt" \
  -F "config=@servers/export_onnx/templates/yolo26_det.yaml"
```

## 命令行导出

```bash
./servers/export_onnx/scripts/3_export.sh --input models/pt/yolo26n.pt --config servers/export_onnx/templates/yolo26_det.yaml
```

## 服务参数

| 变量 / 参数 | 默认值 |
|-------------|--------|
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `8090` |
| `LOG_ROOT` / `--log-root` | `/root/logs/export_onnx` |
| `--model-root` | `/root/models`（其下 `pt/`、`onnx/`） |

日志：`{LOG_ROOT}/app.log`（滚动，10×1MB）。
