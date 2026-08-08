# Netdata

宿主机 / Docker 容器资源监控（CPU、内存、网络；有 NVIDIA 时采集 GPU）。官方镜像，不构建项目镜像。

## 脚本

| 脚本 | 作用 |
|------|------|
| `scripts/0_pull_base_image.sh` | 拉取 `netdata/netdata:stable` |
| `scripts/1_run_container.sh` | 启动 `${PROJECT_NAME}_netdata`（项目网络 + `-p 19999:19999`，挂载 docker.sock；检测到 `nvidia-smi` 时加 `--gpus`） |
| `scripts/2_stop_container.sh` | 停止并删除容器 |

```bash
bash servers/netdata/scripts/0_pull_base_image.sh
bash servers/netdata/scripts/1_run_container.sh
```

UI：`http://127.0.0.1:19999/v3`（本地看板；v2 根路径可能弹出 Cloud 登录引导，直接用 `/v3` 或设 `NETDATA_DISABLE_CLOUD=1`）。

可选环境变量：`NETDATA_CONTAINER_NAME`、`NETDATA_IMAGE`、`NETDATA_HOST_PORT`。
