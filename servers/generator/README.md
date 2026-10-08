# Generator API

FastAPI：按 YAML 生成 DeepStream 配置（`pipeline.yml`、`pgie.yml`、`params.yml` 等）。默认端口 `8091`。

## 镜像

| 镜像 | 构建 | 容器 | 说明 |
|------|------|------|------|
| `${PROJECT_NAME}_generator_dev` | `1_build_dev_image.sh` | `${PROJECT_NAME}_generator` | 挂代码；改 pip 后重建 |
| `${PROJECT_NAME}_generator_prod` | `1_build_prod_image.sh` | 同上 | 代码打进镜像 |

网络：`${PROJECT_NAME}_default`。

## 构建与运行

```bash
# 项目根目录
./servers/generator/scripts/0_pull_base_image.sh   # 可选
./servers/generator/scripts/1_build_dev_image.sh   # 或 1_build_prod_image.sh
./servers/generator/scripts/2_run_dev_container.sh # 或 2_run_prod_container.sh
```

健康检查：`http://127.0.0.1:8091/ai_stream2/generator/health`（前缀随 `${PROJECT_NAME}`）  
Swagger：`http://127.0.0.1:8091/docs`

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/generator` | `/app` | 仅 dev：Python 代码 |
| `models/` | `/root/models` | TRT 模型（`pgie.model_dir`） |
| `configs/` | `/root/configs` | 输出 `configs/generator/{name}/` |
| `attachments/` | `/root/attachments` | 模板中的图片 / 视频 |
| `logs/` | `/root/logs` | 服务日志 |

## 接口

前缀：`/{PROJECT_NAME}/generator`。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查 |
| GET | `/types` | 已注册 generator 类名 |
| POST | `/schema` | 按类名返回 schema |
| POST | `/generate` | 上传 YAML，生成配置并落盘 |

## 约定

`GET /types` 的 `data`：`{"items": ["DetImageGenerator", ...]}`（字符串列表）。完整名单以接口为准。

`POST /schema` JSON：`{"generator": "DetImageGenerator"}`。未知类名 HTTP 404。字段形状以 `schemas/*.yaml` 为准。

`POST /generate`：`multipart` 字段 `input`，结构同 `servers/generator/templates/**/*.yaml`。必填 `generator`、`config_save_dir`；`pipeline_name` 缺省时用输出目录名。成功时 `data` 为 `config_save_dir` 路径，目录内含 `params.yml`。

```bash
curl -s http://127.0.0.1:8091/ai_stream2/generator/types

curl -s -X POST http://127.0.0.1:8091/ai_stream2/generator/schema \
  -H 'Content-Type: application/json' \
  -d '{"generator":"DetImageGenerator"}'

curl -s -X POST http://127.0.0.1:8091/ai_stream2/generator/generate \
  -F "input=@servers/generator/templates/yolo/yolo26n_det_image.yaml"
```

## 命令行

```bash
./servers/generator/scripts/3_generate.sh --config yolo26n_det_vis_rtsp
./servers/generator/scripts/3_generate.sh --config servers/generator/templates/yolo/yolo26n_det_image.yaml
./servers/generator/scripts/3_generate_all.sh
```

`--config` 可为模板路径，或 `templates/` 下不含 `.yaml` 的模板名。

## 服务参数

| 项 | 默认 |
|----|------|
| `PROJECT_NAME` | `ai_stream2`（环境） |
| `HOST` / `--host` | `0.0.0.0` |
| `PORT` / `--port` | `8091` |
| 日志目录 | `/root/logs/generator`（硬编码） |
