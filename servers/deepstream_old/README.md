# DeepStream API

C++ `deepstream_api`（httplib）：按 generator 配置目录 build 并后台启动 pipeline。默认端口 `8092`，需 GPU。无 HTTP 停止接口。

## 镜像

| 镜像 | 构建 | 容器 | 说明 |
|------|------|------|------|
| `${PROJECT_NAME}_deepstream_dev` | `1_build_dev_image.sh` | `${PROJECT_NAME}_deepstream` | 挂 `/app`；改 C++ / native 后重建镜像 |
| `${PROJECT_NAME}_deepstream_prod` | `1_build_prod_image.sh` | 同上 | 代码打进镜像 |

网络：`${PROJECT_NAME}_default`。`--gpus all`。

## 构建与运行

```bash
# 项目根目录
./servers/deepstream/scripts/0_pull_base_image.sh   # 可选
./servers/deepstream/scripts/1_build_dev_image.sh   # 或 1_build_prod_image.sh
./servers/deepstream/scripts/2_run_dev_container.sh # 或 2_run_prod_container.sh
```

健康检查：`http://127.0.0.1:8092/ai_stream2/deepstream/health`（前缀随 `${PROJECT_NAME}`；进程内 `project_name` 现硬编码为 `ai_stream2`）

无 Swagger。

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/deepstream` | `/app` | 仅 dev：源码（运行仍用镜像内 `deepstream_api`） |
| `configs/` | `/root/configs` | `pipeline.yml` / `params.yml` |
| `models/` | `/root/models` | TensorRT 模型 |
| `attachments/` | `/root/attachments` | 图片 / 视频输入 |
| `outputs/` | `/root/outputs` | pipeline 输出 |
| `logs/` | `/root/logs` | 服务日志 |

## 接口

前缀：`/{PROJECT_NAME}/deepstream`。无 Swagger。三个路由都在 `ApiServer::bindRoutes` 注册，处理函数经 `finish` 包一层：成功 HTTP 200，`ApiError` 用其状态码，YAML 解析失败 400，其它异常 500。响应一律 `Content-Type: application/json`，由 `YamlJson::envelope` 写成：

```json
{"success": true, "message": "", "data": null}
```

`success` 为 false 时 `message` 是错误文本，`data` 为 `null`。请求体用 `YAML::Load` 解析，因此 JSON 对象也可作为入参。

### GET `/health`

无 query、无 body。`handleHealth` 不访问 pipeline，`data` 固定 `null`。

```json
{"success": true, "message": "", "data": null}
```

### GET `/pipeline/status`

无 query、无 body。`PipelineService::status` 读当前子进程：`ChildProcess::running` 用 `waitpid(WNOHANG)` 看 `pipeline_runner` 是否仍在。`name` / `type` 是上次成功 `start` 时从 `params.yml` 记下的值；从未启动则为 JSON `null`。

```json
{"success": true, "message": "", "data": {"pipeline_running": false, "name": null, "type": null}}
```

| 字段 | 类型 | 含义 |
|------|------|------|
| `pipeline_running` | bool | 子进程仍在跑 |
| `name` | string \| null | `params.yml` 的 `pipeline_name` |
| `type` | string \| null | `params.yml` 的 `type` |

### POST `/start_pipeline`

Body 只需一个字段。`handleStart` 读 `config_dir`，空则 400 `missing config_dir`。

```json
{"config_dir": "/root/configs/generator/yolo26n_det_sahi_vis_video"}
```

`PipelineService::start` 再检查该目录、其中的 `pipeline.yml` 与 `params.yml`，以及 `params.yml` 里的 `pipeline_name`、`type`。已有子进程在跑则拒绝。通过后 `fork` + `execl`，把目录作为 `pipeline_runner` 的唯一参数，并记下 name/type。成功时 `data` 只有这两项，没有 pid。

```json
{"success": true, "message": "", "data": {"name": "yolo26n_det_sahi_vis_video", "type": "video"}}
```

| HTTP | `message` | 条件 |
|------|-----------|------|
| 400 | `missing config_dir` | 缺字段或空字符串 |
| 400 | `config_dir is not a directory` | 路径不是目录 |
| 400 | `pipeline is running` | 已有 pipeline 子进程 |
| 400 | `missing pipeline.yml in config_dir` | 目录里没有 `pipeline.yml` |
| 400 | `missing params.yml in config_dir` | 目录里没有 `params.yml` |
| 400 | `params.yml missing pipeline_name` | 没有 `pipeline_name` |
| 400 | `params.yml missing type` | 没有 `type` |
| 400 | YAML 库原文 | body 或 `params.yml` 解析失败 |
| 500 | `fork failed` | `fork` 失败 |
| 500 | `internal error` | 其它异常（含 `execl` 失败后的子进程退出，父进程不在本响应里体现） |

## 约定

`POST /start_pipeline` JSON：`{"config_dir": "/root/configs/generator/..."}`。目录须含 generator 产出的 `pipeline.yml` 与 `params.yml`（`type`、`pipeline_name` 从 `params.yml` 读）。

成功：`{"success": true, "message": "", "data": {"name": "...", "type": "..."}}`。失败 HTTP 400，`success: false`。

停止：`docker stop ${PROJECT_NAME}_deepstream`（杀掉 API 与 pipeline）。

## 命令行

```bash
./servers/deepstream/scripts/3_start_pipeline.sh --config configs/generator/yolo26n_det_sahi_vis_video
docker stop "${PROJECT_NAME}_deepstream"
```

另有 `3_start_all_image_pipeline.sh`、`3_start_all_video_pipeline.sh`、`3_clear_log.sh`。

## 服务参数

进程在 `main.cpp` 硬编码，不读环境变量：

| 项 | 值 |
|----|----|
| `project_name` | `ai_stream2` |
| 监听 | `0.0.0.0:8092` |
| 日志 | `/root/logs/deepstream/app.log` |
| runner | `/usr/local/bin/pipeline_runner` |
