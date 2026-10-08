# ExportOnnx API

FastAPI：将 `.pt` / `.pth` 导出为 DeepStream 可用的 ONNX 目录。默认端口 `8090`。

## 镜像

| 镜像 | 构建 | 容器 | 说明 |
|------|------|------|------|
| `${PROJECT_NAME}_export_onnx` | `1_build_image.sh` | `${PROJECT_NAME}_export_onnx` | 代码打进镜像 |

网络：`${PROJECT_NAME}_default`。

## 构建与运行

```bash
# 项目根目录
./servers/export_onnx/scripts/0_pull_base_image.sh   # 可选
./servers/export_onnx/scripts/1_build_image.sh
./servers/export_onnx/scripts/2_run_container.sh
```

健康检查：`http://127.0.0.1:8090/ai_stream2/export_onnx/health`（前缀随 `${PROJECT_NAME}`）  
Swagger：`http://127.0.0.1:8090/docs`

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `models/` | `/root/models` | `pt/` 权重、`onnx/` 产物 |
| `logs/` | `/root/logs` | `{LOG_ROOT}/app.log` |

## 接口

前缀：`/{PROJECT_NAME}/export_onnx`。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查 |
| GET | `/types` | 可用导出类型 |
| POST | `/export` | 导出 ONNX |

## 约定

`POST /export` 为 `multipart/form-data`：`input`（`.pt` 或 `.pth`）、`config`（YAML，见 `templates/`）。

`.pt` 落到 `/root/models/pt/{filename}`，清空并写入 `/root/models/onnx/{stem}/`。成功时 `data` 为该目录路径。

YAML：`type` 必填；其余默认 `size=640`、`opset=18`、`batch=1`、`dynamic=false`、`simplify=false`、`max_det=30`、`conf`、`iou`。`dynamic=true` 时 meta 中 `batch_size` 为 null，`batch` 仍作导出 batch。`iou` 仅 YOLO11 DET/SEG 使用。类型以 `GET /types` 为准。

```bash
curl -s -X POST http://127.0.0.1:8090/ai_stream2/export_onnx/export \
  -F "input=@models/pt/yolo26n.pt" \
  -F "config=@servers/export_onnx/templates/yolo26_det.yaml"
```

## 命令行

```bash
./servers/export_onnx/scripts/3_export.sh --input models/pt/yolo26n.pt --config servers/export_onnx/templates/yolo26_det.yaml
```

## 服务参数

| 项 | 默认 |
|----|------|
| `PROJECT_NAME` | `ai_stream2`（环境） |
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `8090` |
| 日志目录 | `/root/logs/export_onnx`（硬编码） |
| 模型根 | `/root/models`（硬编码） |
