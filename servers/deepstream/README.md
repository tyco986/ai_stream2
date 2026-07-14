# DeepStream API

在 DeepStream 容器内运行的 FastAPI 服务：按模板 build 并后台启动 pipeline。停止由 `docker stop` 接管。

## 运行

```bash
cd /app   # 容器内 WORKDIR
python main.py --host 0.0.0.0 --port 8092
```

API 基址：`http://127.0.0.1:8092`  
Swagger：`http://127.0.0.1:8092/docs`

## 目录挂载（推荐）

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `configs/` | `/root/configs` | pipeline / 模型配置 |
| `models/` | `/root/models` | TensorRT 模型 |
| `logs/` | `/root/logs` | 服务日志 |

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/ai_stream2/deepstream/start_pipeline` | build 并后台启动 pipeline |

### start_pipeline

见 `servers/deepstream/templates/*.yml`（`type` / `name` / `config_dir` / `logger` / `messager` / `drawer`）。

### 响应

```json
{"success": true, "message": ""}
```

失败时 HTTP 400，`success: false`，`message` 为错误详情。

## 典型流程

```bash
# start（build + 后台运行）
./servers/deepstream/scripts/3_start_pipeline.sh --config det_rtsp_pipeline

# stop
docker stop ai_stream2_deepstream
```

## 环境变量

| 变量 | 默认 | 说明 |
|------|------|------|
| `HOST` | `0.0.0.0` | 监听地址 |
| `PORT` | `8092` | 监听端口 |
| `LOG_ROOT` | `/root/logs/deepstream` | 日志目录（`app.log`，10MB × 10 轮转） |

## 日志

- 路径：`{LOG_ROOT}/app.log`
- 每条 HTTP 请求记录 start / 状态码 / 耗时
- pipeline start / 异常写 ERROR 栈
