# 后端页面编写规则

管理端后端（Django + DRF）实现约定。HTTP 契约见 `docs/frontend/<page>_api.md`；UI 规格见对应 `*.md`。本文约束**代码形态**与 **page_based 隔离**，服务后期 **Nuitka + editable compile**（按页独立编译 / 可替换）。

前端约定见 [../frontend/rule.md](../frontend/rule.md)。

---

## 1. 文档与目录

### 文档驱动

| 规则 | 说明 |
|------|------|
| 先文档后代码 | 有 `docs/frontend/<page>_api.md` 才实现该页后端 |
| 字段对齐 | 请求/响应字段名、枚举、路径以前端 `*_api.md` 为准；禁止另造别名 |
| 偏离即改文档 | 实现发现契约不足时**先改 API 文档**，再改代码 |
| 前缀 = 页面 | `/ai_stream2/backend/<page>` 对应一页；无「无页面前缀」的杂项 API |

`PROJECT_NAME` 唯一真相源为仓库根 [`project.env`](../../project.env)；URL 前缀派生自此，禁止业务代码写死旧名。

### 一页一包（page_based）

```
servers/django/                  # 后端根
  manage.py
  config/                        # Django 工程入口（settings、root urls、ASGI）
  pages/
    <page>/                      # 如 streams、events、login、shell、users
      apps.py
      urls.py                    # 仅本页路由；挂到 /backend/<page>/
      models.py                  # 仅本页表；禁止 import 其它 page 的 models
      serializers.py             # 或 serializers/
      views.py                   # 或 views/
      services.py                # 或 services/；本页业务逻辑（类封装）
      permissions.py             # 本页 Permission 校验（可选）
      admin.py                   # 可选
      migrations/
  shared/                        # 跨页唯一合法共享区
    http/                        # 响应外壳、异常 → { success, message, data }
    auth/                        # 会话解析、has_perm 辅助（不写业务 CRUD）
    db/                          # 连接与库级约定（见 §3）；无业务 Model
    pagination/                  # 统一分页形状
    permissions_catalog/         # 产品权限注册（与 user_api PermissionCatalog 对齐）
  scripts/                       # 拉镜像 / 构建 / 启停容器（唯一入口）
  Dockerfile
```

| 规则 | 说明 |
|------|------|
| 跨页复用 | 必须进入 `shared/`，并写明归属；**禁止** A 页 import B 页内部模块 |
| 壳与业务 | `pages/shell/`、`pages/login/` 与业务页同级；会话读/销归 shell，建会话归 login（与前端文档一致） |
| 无上帝 app | 禁止 `common.models` 堆全部业务表；表归属对应 page |
| Nuitka 边界 | 编译/替换单位 = `pages/<page>/`；`shared/` 与 `config/` 为公共底座，单独约定版本 |

跨页**数据**需要时：本页只存对方资源的 **UUID / 稳定 id**（软引用），经对方 API 或只读视图取展示字段；**禁止**跨 page 做 ORM `ForeignKey` 指向另一 page 的 Model（auth 内建 User/Group/Permission 除外，见 §3）。

### 容器化（强制）

所有后端实现、依赖安装、迁移、跑服务、调试与测试均在 **Docker 容器**内完成；**禁止**在宿主机直接 `pip install` / `manage.py` / `runserver` / 连业务库。

| 规则 | 说明 |
|------|------|
| 唯一入口 | 经 `servers/django/scripts/`（对齐其它 `servers/*/scripts`）：拉基镜像、build、run |
| 依赖 | 写入镜像 `requirements.txt` / Dockerfile；不在宿主机装 Django 运行时 |
| 数据与网络 | DB 等同 compose/network 内服务通信；容器名、网络名派生自 `PROJECT_NAME` |
| 宿主机允许 | 仅：编辑源码、调用上述 scripts、`docker` / `docker compose` 本身 |
| 文档与实现 | 页面后端文档默认假定「容器内路径 / 网内主机名」；不写宿主机本机路径当运行前提 |

---

## 2. 路由、视图与契约

### 原则

- 实现与 `*_api.md` / OpenAPI **一致**；可用同一份 OpenAPI 校验或 codegen。
- 统一响应外壳：`{ success, message, data }`；分页 `data` 为 `{ items, total, page, page_size }`。
- 鉴权与 Permission：`401` 未登录、`403` 缺权；codename 与 [user_api.md](../frontend/user_api.md) PermissionCatalog 对齐。

### 页面边界

| 规则 | 说明 |
|------|------|
| 一页只挂本前缀 | `pages/streams/urls.py` 只服务 `/backend/streams` |
| 根 urls 只组装 | `config/urls.py` 仅 `include` 各 page；禁止在 root 写业务视图 |
| 固定段优先 | `options`、`batch`、`map` 等静态路径注册在 `/{id}` 之前（与各 `*_api.md` 一致） |
| Service 层 | 业务逻辑在 `services` 类中；视图薄：解析入参 → 调 Service → 序列化 |

### 与前端

前端经 OpenAPI 生成客户端调用；后端**不**为迁就前端拼装第二套字段名。缺字段改文档与实现，不在后端静默别名。

---

## 3. 数据库

### 结论（默认）

| 层 | 选择 | 说明 |
|----|------|------|
| **物理库** | **一个** | 全站共用同一 PostgreSQL（或选定引擎）实例与库名 |
| **连接配置** | **一份** | 仅在 `config/settings`（或 `shared/db`）配置 `DATABASES`；各 page **禁止**私有第二套连接串 |
| **逻辑归属** | **按 page 分 app** | 每个 `pages/<page>` 一个 Django app；表由该 app 的 `migrations` 管理 |
| **多库（multi-db）** | **默认不做** | 不为「页隔离」拆库；Nuitka 要的是 **Python 包隔离**，不是数据库进程隔离 |

### 为何不「一页一库」

- User / Group / Permission、会话与几乎所有页的鉴权共用同一套 auth 表；拆库后跨库事务与 FK 成本高、运维倍增。
- Events ↔ Streams / Pipelines、Pipeline ↔ Model 等存在跨页 id 引用；同库便于一致性与备份。
- 页间禁止 ORM 互引后，**同库 + 软引用 UUID** 已足够支撑 page_based 与按页编译；再拆物理库无额外编译收益。

### 允许的例外（显式文档化后再做）

| 例外 | 何时考虑 |
|------|----------|
| 只读分析副本 | 报表/导出极重读，与主库读写分离 |
| 对象存储 / 文件卷 | 录像、截图、模型文件走对象存储或独立卷，**不是**第二套业务 SQL 库 |
| 超大事件冷热分层 | 仅 events 等写放大表，且迁移与路由方案单独成文 |

例外仍共用同一套**配置入口**（settings 中声明），不在 page 内散落连接串。

### Model 规则

| 规则 | 说明 |
|------|------|
| 表归属 | 业务 Model 只出现在所属 `pages/<page>/` |
| 跨页引用 | UUID 字段 + 文档约定；不建跨 page 的 `ForeignKey` / `ManyToMany` |
| auth 例外 | `pages/users`（及 login/shell 所需）使用 Django auth；其它 page 只 `request.user` / `has_perm`，不复制 User 表 |
| migration | 一页只改本 app migrations；禁止在 A 页 migration 里依赖 B 页模型状态 |

---

## 4. 隔离与 Nuitka / editable

### 目标

- 改 `pages/events` 不迫使重编 `pages/streams`（在依赖图上成立）。
- 交付时可对单页做 Nuitka 编译产物，其余页保持 editable / 源码替换。

### 硬约束

| 规则 | 说明 |
|------|------|
| 禁止跨 page import | `pages.a` 不得 `from pages.b ...`（含 models、services、serializers） |
| 共享只走 shared | 公共类型、HTTP 外壳、鉴权辅助、分页 → `shared/` |
| 无循环依赖 | `config` → `pages.*` → `shared`；`shared` 不 import 任何 `pages.*` |
| 入口可发现 | 新 page：实现包 + 在 `config` 注册 `INSTALLED_APPS` 与 `urls include`；不改其它 page 文件 |

### 编译单位（约定）

```
nuitka 单位建议：
  - shared/          # 底座，版本与主程序绑定
  - pages/<page>/    # 可按页编译为 extension module / 独立产物
  - config/          # 薄入口，组装 include
```

具体 Nuitka 命令与 CI 另文；本文只保证**依赖方向**可编译。

---

## 5. 代码风格（与仓库规则对齐）

完整风格见 `.cursor/rules/code-style.mdc`。后端额外强调：

| 规则 | 说明 |
|------|------|
| Service / Client 用类 | 禁止散落函数集合承载业务 |
| `__init__` | 先全部赋值 `self.*`，再其它逻辑 |
| 有返回值 | 唯一 `return` 在末尾 |
| 禁止 `global`、禁止函数内嵌套 def | — |
| 异常 | 不随意吞；I/O 重试等有策略处才 try |
| 语言 | 标识符与 API key 英文；注释中英均可，文件内一致 |

---

## 6. 鉴权与权限

| 规则 | 说明 |
|------|------|
| 写操作强制 | UI 显隐不算数；后端 `has_perm` / 等价校验 |
| Permission 归属 | catalog 注册在 `shared/permissions_catalog`（或 users 页约定的单一位置）；业务 page 只声明本页需要的 codename 常量，不重复造权 |
| 会话 | Cookie / Authorization 全局一种约定；login 建、shell 读/销 |

---

## 7. 检查清单（开 PR 前）

- [ ] 对应 `*_api.md` 已更新且与实现一致
- [ ] 新代码在 `pages/<page>/` 或正当落入 `shared/`；无跨 page import
- [ ] 无跨 page ORM FK；跨页只用 UUID 软引用（auth 例外已说明）
- [ ] 仅一份 `DATABASES` 配置；本页未私藏连接串
- [ ] 响应外壳与分页形状统一；401/403 与文档一致
- [ ] Service 为类；无 `global` / 无函数内 def / 有返回值单出口
- [ ] 依赖安装 / migrate / 跑服 / 测试均在容器内；未要求宿主机装运行时

---

## 8. 相关文档

| 文档 | 用途 |
|------|------|
| [../frontend/rule.md](../frontend/rule.md) | 前端 page_based 与 OpenAPI |
| 各页 `*_api.md` | HTTP 契约（实现源真相） |
| [../frontend/user_api.md](../frontend/user_api.md) | RBAC / PermissionCatalog |
| [../frontend/login_api.md](../frontend/login_api.md) / [shell_api.md](../frontend/shell_api.md) | 建会话 / 读会话与登出 |
