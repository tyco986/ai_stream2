# DeepStream API

在 DeepStream 容器内运行的 FastAPI 服务：按模板 build 并后台启动 pipeline。停止由 `docker stop` 接管。

## 镜像分层

| 镜像 | 构建 | 用途 |
|------|------|------|
| `ai_stream2_deepstream_dev` | `1_build_dev_image.sh` | 开发：运行时依赖 + `/opt/ai_stream2/servers/deepstream/libs` |
| `ai_stream2_deepstream_prod` | `1_build_prod_image.sh` | 生产：dev + 内置 `servers/deepstream` 代码 |

原生产物：`libnvds_custom_sequence_preprocess.so`、YOLO-Pose 解析 `libnvds_infer_yolo_pose.so` 在 `/opt/ai_stream2/servers/deepstream/libs`；det/seg YOLO 解析用 deepstream-sahi 的 `libnvds_infer_yolo.so`（`/opt/nvidia/deepstream/deepstream/lib/`）；pose SAHI 插件 `libnvdsgst_sahipostprocess_pose.so` 在 `GST_PLUGIN_PATH`。

## 开发与生产

```bash
# 开发：挂代码，改 Python 后 restart 即可
./servers/deepstream/scripts/1_build_dev_image.sh   # 改 pip / patch / native 后
./servers/deepstream/scripts/2_run_dev_container.sh

# 生产：代码打进镜像
./servers/deepstream/scripts/1_build_prod_image.sh
./servers/deepstream/scripts/2_run_prod_container.sh
```

## 运行

```bash
cd /app   # 容器内 WORKDIR
python main.py --host 0.0.0.0 --port 8092
```

API 基址：`http://127.0.0.1:8092`  
Swagger：`http://127.0.0.1:8092/docs`

## 目录挂载

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `servers/deepstream` | `/app` | 开发模式：Python 代码 |
| `configs/` | `/root/configs` | pipeline / 模型配置 |
| `models/` | `/root/models` | TensorRT 模型 |
| `logs/` | `/root/logs` | 服务日志 |

`schemas/*.yaml`：各 pipeline 的 probe 参数 schema（`available` / `default`）。  
`compile_units/*.yaml`：各 pipeline 逻辑源文件闭包（供后期 prod Nuitka / editable 编译）。

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/ai_stream2/deepstream/types` | 列出已注册 pipeline 类型 |
| POST | `/ai_stream2/deepstream/schema` | 按类型返回 probe 参数 schema |
| POST | `/ai_stream2/deepstream/start_pipeline` | build 并后台启动 pipeline |

### start_pipeline

见 `servers/deepstream/templates/`（含 `yolo/`、`base/` 等子目录下的 `*.yml`：`type` / `name` / `config_dir` / `logger` / `messager` / `drawer`）。

### 响应

```json
{"success": true, "message": ""}
```

失败时 HTTP 400，`success: false`，`message` 为错误详情。

## 典型流程

```bash
./servers/deepstream/scripts/3_start_pipeline.sh --config smoke_fire_pipeline
docker stop ai_stream2_deepstream
```

## 环境变量

| 变量 | 默认 | 说明 |
|------|------|------|
| `HOST` | `0.0.0.0` | 监听地址 |
| `PORT` | `8092` | 监听端口 |
| `LOG_ROOT` | `/root/logs/deepstream` | 日志目录 |

## 日志

- 路径：`{LOG_ROOT}/app.log`
- 每条 HTTP 请求记录 start / 状态码 / 耗时
- pipeline start / 异常写 ERROR 栈
