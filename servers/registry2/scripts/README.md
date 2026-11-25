# Registry2

本机 Docker 私有仓库（官方镜像 `registry:2`）。推上去的镜像会再存一份到 `servers/registry2/data`，和本机 Docker 的镜像库分开。

监听 `127.0.0.1:5000`。同一 Docker 网络里的容器用 `ai_stream2_registry2:5000`。

## 脚本

| 脚本 | 作用 |
|------|------|
| `scripts/0_pull_base_image.sh` | 拉取 `registry:2` |
| `scripts/1_run_container.sh` | 启动 `${PROJECT_NAME}_registry2`，数据目录 `servers/registry2/data`，允许按 digest 删除 |
| `scripts/2_list_registry2_image.sh` | 列出仓库里的 `name:tag` |
| `scripts/2_push_registry2_image.sh` | 把本机镜像推入仓库 |
| `scripts/2_pull_registry2_image.sh` | 把仓库镜像拉回本机 |
| `scripts/2_remove_registry2_image.sh` | 按 tag 删除该镜像的全部架构，并回收不再被引用的层 |

```bash
bash servers/registry2/scripts/0_pull_base_image.sh
bash servers/registry2/scripts/1_run_container.sh

bash servers/registry2/scripts/2_push_registry2_image.sh debian:13-slim
bash servers/registry2/scripts/2_push_registry2_image.sh debian:13-slim my/debian:13-slim
bash servers/registry2/scripts/2_list_registry2_image.sh
bash servers/registry2/scripts/2_pull_registry2_image.sh debian:13-slim
bash servers/registry2/scripts/2_remove_registry2_image.sh debian:13-slim
```

只给一个参数时，推送名是本地镜像名去掉最后一个 `/` 之前的部分。`debian:13-slim` 推成 `127.0.0.1:5000/debian:13-slim`。第二个参数用来指定仓库里的 `name:tag`。

删除会去掉该 tag 的 index 以及其中每个架构的 manifest，然后停掉仓库执行 `registry garbage-collect --delete-untagged`，再把仓库拉起来。其它 tag 仍引用的层会留下。已经拉到本机或其它机器上的镜像不受影响。
