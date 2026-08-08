# Beszel

轻量多节点监控（Hub + Agent）。官方镜像，不构建项目镜像。

## 脚本

| 脚本 | 作用 |
|------|------|
| `scripts/0_pull_base_image.sh` | 拉取 `henrygd/beszel`、`henrygd/beszel-agent` |
| `scripts/1_run_hub.sh` | 启动 Hub `${PROJECT_NAME}_beszel`（默认宿主机端口 **8093**，避免与 export_onnx:8090 冲突） |
| `scripts/2_run_agent.sh` | 启动 Agent（需 `BESZEL_TOKEN` / `BESZEL_KEY`；与 Hub 共享 unix socket） |
| `scripts/3_stop_containers.sh` | 停止并删除 Hub + Agent |

## 启动

```bash
bash servers/beszel/scripts/0_pull_base_image.sh
bash servers/beszel/scripts/1_run_hub.sh
```

打开 `http://127.0.0.1:8093` → 创建管理员 → **Add System**：

- Host/IP 填：`/beszel_socket/beszel.sock`
- 复制 TOKEN、KEY，再启动 Agent：

```bash
export BESZEL_TOKEN='...'
export BESZEL_KEY='ssh-ed25519 ...'
bash servers/beszel/scripts/2_run_agent.sh
```

可选环境变量：`BESZEL_HOST_PORT`、`BESZEL_APP_URL`、`BESZEL_HUB_URL`、`BESZEL_HUB_IMAGE`、`BESZEL_AGENT_IMAGE`。
