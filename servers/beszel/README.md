# Beszel

轻量多节点监控（Hub + Agent）。官方镜像，不构建项目镜像。

## 脚本

| 脚本 | 作用 |
|------|------|
| `scripts/0_pull_base_image.sh` | 拉取 `henrygd/beszel`、`henrygd/beszel-agent-nvidia` |
| `scripts/1_run_prod_hub.sh` | `docker compose up -d beszel_hub beszel_agent` |
| `scripts/2_run_prod_agent.sh` | `docker compose up -d beszel_agent` |
| `scripts/5_agent_start.sh` | Agent 容器入口：登录 Hub、取 KEY / 长期令牌、探测本机磁盘，再执行 `/agent` |

## 启动

```bash
bash servers/beszel/scripts/0_pull_base_image.sh
bash servers/beszel/scripts/1_run_prod_hub.sh
```

`docker compose up -d beszel_hub beszel_agent` 会拉起 Hub 和 Agent。Agent 的入口是 `5_agent_start.sh`：登录 `admin@example.com` / `88888888`，取公钥和长期令牌，再执行 `/agent`。Hub 数据在 `ai_stream2_beszel_hub`，Agent 数据在 `ai_stream2_beszel_agent`。UI：`http://127.0.0.1:8093`。
