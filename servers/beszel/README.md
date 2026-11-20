# Beszel

轻量多节点监控（Hub + Agent）。官方镜像，不构建项目镜像。

## 脚本

| 脚本 | 作用 |
|------|------|
| `scripts/0_pull_base_image.sh` | 拉取 `henrygd/beszel`、`henrygd/beszel-agent-nvidia` |
| `scripts/1_run_hub.sh` | 启动 Hub（默认端口 **8093**），用 users 登录、取 KEY / 长期令牌，并拉起本机 Agent（系统名 `localhost`） |
| `scripts/2_run_agent.sh` | 单独启动 Agent（需 `BESZEL_TOKEN` / `BESZEL_KEY`，系统名 `localhost`） |
| `scripts/3_stop_containers.sh` | 停止并删除 Hub + Agent |

## 启动

```bash
bash servers/beszel/scripts/0_pull_base_image.sh
bash servers/beszel/scripts/1_run_hub.sh
```

`1_run_hub.sh` 会登录 `users`（`admin@example.com` / `88888888`），取公钥和长期通用令牌，再启动本机 Agent，系统名是 `localhost`。UI：`http://127.0.0.1:8093`。

单独重拉 Agent：

```bash
export BESZEL_TOKEN='...'
export BESZEL_KEY='ssh-ed25519 ...'
bash servers/beszel/scripts/2_run_agent.sh
```

可选环境变量：`BESZEL_HOST_PORT`、`BESZEL_APP_URL`、`BESZEL_HUB_URL`、`BESZEL_HUB_IMAGE`、`BESZEL_AGENT_IMAGE`。
