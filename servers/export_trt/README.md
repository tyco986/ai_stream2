# Export TRT API

FastAPI：将 export_onnx 产出的 ONNX 目录编译为 TensorRT engine。默认端口 `9000`，需 GPU。

## 镜像

| 镜像 | 构建 | 容器 | 说明 |
|------|------|------|------|
| `${PROJECT_NAME}_export_trt_dev` | `1_build_dev_image.sh` | `${PROJECT_NAME}_export_trt` | 挂代码；改 pip / native 后重建 |
| `${PROJECT_NAME}_export_trt_prod` | `1_build_prod_image.sh` | 同上 | 代码打进镜像 |

网络：`${PROJECT_NAME}_default`。`--gpus all`。

## 构建与运行

```bash
# 项目根目录
./servers/export_trt/scripts/0_pull_base_image.sh   # 可选
./servers/export_trt/scripts/1_build_dev_image.sh   # 或 1_build_prod_image.sh
./servers/export_trt/scripts/2_run_dev_container.sh # 或 2_run_prod_container.sh
```

健康检查：`http://127.0.0.1:9000/ai_stream2/export_trt/health`（前缀随 `${PROJECT_NAME}`）  
Swagger：`http://127.0.0.1:9000/docs`

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/export_trt` | `/app` | 仅 dev：Python 代码 |
| `models/` | `/root/models` | `onnx/` 输入、`trt/` 产物 |
| `logs/` | `/root/logs` | 服务日志 |
| `outputs/` | `/root/outputs` | tester 输出 |
| `attachments/` | `/root/attachments` | INT8 校准 zip、tester 输入 |

## 接口

前缀：`/{PROJECT_NAME}/export_trt`。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查 |
| GET | `/types` | 可用导出类型 |
| POST | `/export` | 导出 TensorRT engine |
| POST | `/test` | 用 YAML 跑 tester |

## 约定

`POST /export`：`input` 为 ONNX 目录 zip，`config` 为 `templates/export/*.yaml`。解压到 `/root/models/onnx/{zip_stem}/`，写入 `/root/models/trt/{stem}/`。成功时 `data` 为 TRT 目录路径。

导出 YAML：`type` 必填；`batch_size`、`gpu_id`、`precision`、`opt_level`。动态 ONNX 必须填 `batch_size`；静态 ONNX 必须不填（从 meta 解析）。类型以 `GET /types` 为准。

YOLO INT8：`precision: int8`，解析后的 batch ≥ 8；校准图 `/root/attachments/int8_calib.zip`（500 张，校准 batch=8）。每次导出清空 TRT 目录后重校准。RTMPose / ST-GCN++ 的 INT8 走 `trtexec --int8`，无 MinMax 校准。

`POST /test`：只传 `config`（`templates/test/*.yaml`：`type`、`engine`、`input`）。

```bash
curl -s -X POST http://127.0.0.1:9000/ai_stream2/export_trt/export \
  -F "input=@yolo26n.zip" \
  -F "config=@servers/export_trt/templates/export/yolo26_det.yaml"
```

## 命令行

```bash
./servers/export_trt/scripts/3_export.sh --input models/onnx/yolo26n --config servers/export_trt/templates/export/yolo26_det.yaml
./servers/export_trt/scripts/3_test.sh --config servers/export_trt/templates/test/rtmpose_image.yaml
```

`3_export.sh` 的 `--input` 为 ONNX 目录，脚本打包成 zip 再上传。

## 服务参数

| 项 | 默认 |
|----|------|
| `PROJECT_NAME` | `ai_stream2`（环境） |
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `9000` |
| 日志目录 | `/root/logs/export_trt`（硬编码） |
| 模型根 | `/root/models`（硬编码） |
