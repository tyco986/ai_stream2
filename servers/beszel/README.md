# Beszel

轻量多节点监控（Hub + Agent）。官方镜像，不构建项目镜像。

## 脚本

| 脚本 | 作用 |
|------|------|
| `scripts/0_pull_base_image.sh` | 拉取 `henrygd/beszel`、`henrygd/beszel-agent-nvidia` |
| `scripts/1_run_hub.sh` | 用 compose 启动 Hub（端口 **8093**，数据卷 `ai_stream2_beszel_hub`），用 users 登录、取 KEY / 长期令牌，并拉起本机 Agent（系统名 `localhost`，数据卷 `ai_stream2_beszel_agent`） |
| `scripts/2_run_agent.sh` | 单独启动 Agent（需 `BESZEL_TOKEN` / `BESZEL_KEY`，系统名 `localhost`） |
| `scripts/3_stop_containers.sh` | 停止并删除 Hub + Agent |

## 启动

```bash
bash servers/beszel/scripts/0_pull_base_image.sh
bash servers/beszel/scripts/1_run_hub.sh
```

`1_run_hub.sh` 会拉起 compose 里的 Hub，登录 `users`（`admin@example.com` / `88888888`），取公钥和长期通用令牌，再启动本机 Agent，系统名是 `localhost`。Hub 数据留在 `ai_stream2_beszel_hub`。UI：`http://127.0.0.1:8093`。

单独重拉 Agent：

```bash
export BESZEL_TOKEN='...'
export BESZEL_KEY='ssh-ed25519 ...'
bash servers/beszel/scripts/2_run_agent.sh
```

单独启动 Agent 时可用 `BESZEL_HUB_URL`、`BESZEL_AGENT_IMAGE`。Hub 的端口和账号在 `docker-compose.yml` 的 `beszel` 服务里。
