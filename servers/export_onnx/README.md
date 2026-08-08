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

1. `input` 上传 `.pt` 文件，落盘到 `/root/models/pt/{name}.pt`（同名直接覆盖）
2. 清空并创建 `/root/models/onnx/{name}/`，以 `pt/` 下 weights 为 `-w`、`onnx/{name}/` 为 cwd 调用 `utils/yolo_e2e` 或 `utils/yolo_non_e2e` 导出脚本（ONNX 生成在 `pt/` 同目录，`labels.txt` 在 cwd）
3. 将 `{name}.onnx`、`{name}.onnx.data`（若有）从 `pt/` 移到 `onnx/{name}/`，写入 `meta.json`（含 `bundle_sha256`）
4. 返回 JSON，`message` 为 ONNX 目录路径

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ai_stream2/export_onnx/health` | 健康检查 |
| GET | `/ai_stream2/export_onnx/types` | 可用导出类型（label / route / family / task） |
| POST | `/ai_stream2/export_onnx/export_yolo10` | YOLO10 检测 |
| POST | `/ai_stream2/export_onnx/export_yolo26` | YOLO26 检测 |
| POST | `/ai_stream2/export_onnx/export_yolo26_seg` | YOLO26 分割 |
| POST | `/ai_stream2/export_onnx/export_yolo11` | YOLO11 检测 |
| POST | `/ai_stream2/export_onnx/export_yolo11_seg` | YOLO11 分割 |

`GET /types` 的 `data.items` 示例：

```json
[
  {"label": "YOLO10-DET", "route": "export_yolo10", "family": "yolo10", "task": "detect"},
  {"label": "YOLO26-SEG", "route": "export_yolo26_seg", "family": "yolo26", "task": "segment"}
]
```


### 导出参数

`multipart/form-data`：

| 字段 | 默认 | 说明 |
|------|------|------|
| `input` | — | `.pt` 文件（必填） |
| `size` | `640` | 正方形输入分辨率 |
| `opset` | `18` | ONNX opset |
| `batch` | `1` | 静态 batch（`dynamic=false` 时） |
| `dynamic` | `false` | 动态 batch 轴 |
| `simplify` | `false` | ONNX simplify |
| `max_detections` | 按路由 | 每图最大检测数 |
| `conf` | `0.25` | 导出时 NMS 置信度阈值（写入 engine） |
| `iou` | `0.45` | 导出时 NMS IoU 阈值（写入 engine） |

`dynamic=true` 与 `batch>1` 不可同时使用。

成功返回 JSON（`message` 为 `/root/models/onnx/{name}/`）。失败返回 JSON 错误。

### curl 示例

```bash
curl -s -X POST http://127.0.0.1:8090/ai_stream2/export_onnx/export_yolo26 \
  -F "input=@models/pt/yolo26n.pt"
```

产物在宿主机 `models/onnx/yolo26n/`。

## 命令行导出

```bash
./servers/export_onnx/scripts/3_export_yolo26.sh --input models/pt/yolo26n.pt
```

| 脚本 | 模型 |
|------|------|
| `3_export_yolo10.sh` | YOLO10 检测 |
| `3_export_yolo26.sh` | YOLO26 检测 |
| `3_export_yolo26_seg.sh` | YOLO26 分割 |
| `3_export_yolo11.sh` | YOLO11 检测 |
| `3_export_yolo11_seg.sh` | YOLO11 分割 |

## 服务参数

| 变量 / 参数 | 默认值 |
|-------------|--------|
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `8090` |
| `LOG_ROOT` / `--log-root` | `/root/logs/export_onnx` |
| `--model-root` | `/root/models`（其下 `pt/`、`onnx/`） |

日志：`{LOG_ROOT}/app.log`（滚动，10×1MB）。
