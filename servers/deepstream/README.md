# DeepStream API

在 DeepStream 容器内运行的 FastAPI 服务：加载 generator 产出的静态 pipeline YAML，attach `YoloDetDrawer`，后台启停 pipeline。

## 运行

```bash
cd /app   # 容器内 WORKDIR
python main.py --host 0.0.0.0 --port 8092 --log-root /root/logs/deepstream
```

API 基址：`http://127.0.0.1:8092`  
Swagger：`http://127.0.0.1:8092/docs`

## 目录挂载（推荐）

| 宿主机 | 容器路径 | 用途 |
|--------|----------|------|
| `configs/` | `/root/configs` | pipeline YAML（`build_pipeline` 的 `input`） |
| `models/` | `/root/models` | TensorRT 模型 |
| `logs/` | `/root/logs` | 服务日志 |

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/ai_stream2/deepstream/build_pipeline` | 加载 YAML 并 build（attach drawer） |
| POST | `/ai_stream2/deepstream/start_pipeline` | 后台启动 pipeline |
| POST | `/ai_stream2/deepstream/stop_pipeline` | 停止运行中的 pipeline |

### build_pipeline

```json
{
  "input": "/root/configs/generator/yolo26n_det_rtsp/pipeline.yml",
  "name": "yolo26n-det"
}
```

### start_pipeline / stop_pipeline

```json
{
  "name": "yolo26n-det"
}
```

### 响应

```json
{"success": true, "message": ""}
```

失败时 HTTP 400，`success: false`，`message` 为错误详情。

## 典型流程

```bash
# 1. generator 生成配置（generator 服务）
curl -s -X POST http://127.0.0.1:8091/ai_stream2/generator/generate \
  -F "input=@configs/generator/yolo26n_det_rtsp/params.yml"

# 2. build
curl -s -X POST http://127.0.0.1:8092/ai_stream2/deepstream/build_pipeline \
  -H "Content-Type: application/json" \
  -d '{"input":"/root/configs/generator/yolo26n_det_rtsp/pipeline.yml","name":"yolo26n-det"}'

# 3. start
curl -s -X POST http://127.0.0.1:8092/ai_stream2/deepstream/start_pipeline \
  -H "Content-Type: application/json" \
  -d '{"name":"yolo26n-det"}'

# 4. stop
curl -s -X POST http://127.0.0.1:8092/ai_stream2/deepstream/stop_pipeline \
  -H "Content-Type: application/json" \
  -d '{"name":"yolo26n-det"}'
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
- pipeline build / start / stop / 异常写 ERROR 栈
