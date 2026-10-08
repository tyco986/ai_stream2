# 主页面（App Shell）

管理端全局壳：导航栏 + 内容区。各业务页（Streams / Previews / …）渲染于内容区；本页只定义 **布局 Mode**、**导航项**、**当前用户区**、**Logo / 版本入口** 与 **页面配置弹窗**。

UI 壳（下拉、弹窗）统一用 **Element Plus**。

| 概念 | 说明 |
|------|------|
| **Mode** | `sidebar`（侧栏）\| `topbar`（顶栏）；控制导航栏位置与形态 |
| **导航项** | Streams、Previews、Recordings、Models、Pipelines、Servers、Events、Users |
| **内容区** | 当前选中导航对应的业务页；各页自身标题 / 工具栏见对应 `*.md` |
| **Logo + Version** | 展示当前配置版本；**可点** → [版本管理](#版本管理)。**sidebar**：整页左上角（侧栏顶）。**topbar**：顶栏**最左** |
| **页面配置** | 齿轮弹窗；**仅 Mode**（无 Site Config） |

默认 **Mode** 为 `sidebar`。仅在页面配置弹窗点击 **Apply** 后生效；已登录时经 [shell_api.md](./shell_api.md) **PUT /settings** 按用户持久化，刷新后恢复。

---

## 布局总览

### 侧栏模式（`sidebar`）

```
+----------+========================================================================+
| [Logo] v3|                                                                        |
|          |                                                                        |
| Streams  |  （内容区：当前业务页）                                                   |
| Previews |                                                                        |
| Record.  |                                                                        |
| Models   |                                                                        |
| Pipeline |                                                                        |
| Servers  |                                                                        |
| Events   |                                                                        |
| Users    |                                                                        |
|          |                                                                        |
| -------- |                                                                        |
| alice ⚙  |                                                                        |
+----------+========================================================================+
```

| 区域 | 说明 |
|------|------|
| 左侧栏顶 | **Logo + Version**（整页左上角；版本管理入口） |
| 左侧栏中 | 纵向导航项 |
| 左侧栏底 | 用户名 + 齿轮 |
| 内容区 | 占满剩余宽度；业务页全高 |

### 顶栏模式（`topbar`）

```
+===================================================================================+
| [Logo] v3  Streams  Previews  Recordings  Models  Pipelines  Servers  Events  Users   alice ⚙ |
+===================================================================================+
|                                                                                   |
|  （内容区：当前业务页）                                                              |
|                                                                                   |
+===================================================================================+
```

| 区域 | 说明 |
|------|------|
| 顶栏最左 | **Logo + Version**（版本管理入口） |
| 顶栏中 | 横向导航项 |
| 顶栏最右 | 用户名 + 齿轮 |
| 内容区 | 顶栏下方全宽；业务页全高 |

---

## 导航栏

### 导航项

固定顺序（两种 Mode 相同）：

| 顺序 | 标签 | 对应文档 / 页面 |
|------|------|-----------------|
| 1 | Streams | [stream.md](./stream.md) |
| 2 | Previews | [preview.md](./preview.md) |
| 3 | Recordings | [recordings.md](./recordings.md) |
| 4 | Models | [model.md](./model.md) |
| 5 | Pipelines | [pipeline.md](./pipeline.md) |
| 6 | Servers | [server.md](./server.md) |
| 7 | Events | [event.md](./event.md) |
| 8 | Users | [user.md](./user.md) |

| 行为 | 说明 |
|------|------|
| 点击导航项 | 切换内容区路由 / 页面；当前项高亮 |
| 高亮 | 仅一项；与当前路由一致 |
| 文案 | 固定英文标签如上表；侧栏可完整展示，窄侧栏时允许省略号 |

侧栏模式下导航项**纵向**排列；顶栏模式下**横向**排列，项间距紧凑，过窄时可横向滚动（导航项不换行）。

### Logo + Version（版本入口）

**不**放在 Page Settings / 齿轮旁。

| Mode | 位置 |
|------|------|
| **sidebar** | 整页**左上角**＝侧栏最上方（导航项之上） |
| **topbar** | 顶栏**最左**（导航项之前）：`[Logo] v0 ‖ Streams … alice ⚙`（出厂当前为 `0`） |

| 元素 | 说明 |
|------|------|
| **Logo** | 产品标识（图或短文案）；与 Version 组成同一入口 |
| **Version** | 当前已应用配置版本号（`is_current` 的 `version`）；**dev/prod 镜像构建即带版本 `0`**，故正常不为空 |
| **点击** | Logo+Version **整体为按钮** → 打开 [版本管理](#版本管理) |
| 权限 | 无 export/import 站点配置权时隐藏或 `disabled` |
| 数据 | 进壳与 Versions Apply / Backup / Import 成功后刷新 |

### 用户区（栏尾）

侧栏：**栏底**；顶栏：**最右侧**。

| 元素 | 说明 |
|------|------|
| **用户名** | 当前登录用户显示名（如 `alice`）；只读文本；来自 `GET /me` |
| **齿轮 ⚙** | 用户名**右侧**；点击打开 [页面配置弹窗](#页面配置弹窗)（**仅 Mode**） |
| **Logout** | 登出；`POST /logout` 清会话后跳转 [login.md](./login.md)（位置：用户名旁或齿轮菜单内，实现二选一） |

有效权限（`permissions`）同一次 `GET /me` 返回，驱动导航项显隐。无登录态时用户名占位为 `—`；齿轮仍可用，Mode 仅会话临时（默认 `sidebar`），见 [shell_api.md](./shell_api.md)。未登录访问管理端应跳转 [login.md](./login.md) Login 页。

---

## 页面配置弹窗

点击齿轮打开。标题 **Page Settings**（或中文「页面配置」；实现时与产品文案统一）。

打开时以**当前已应用** Mode 填充草稿；弹窗内修改**不**改变壳布局，直至 **Apply**。

关闭且丢弃草稿：右上角 `×` / 点遮罩 / `Esc` / **Cancel**（与 Cancel 同效）。

**无 Site Config**（Import / Export / Versions 已移除；由 [版本管理](#版本管理) 接替）。

```
+------------------------------------------+
| Page Settings                        [x] |
+------------------------------------------+
| Mode     [ sidebar          ▼ ]          |
|                                          |
|                    [ Cancel ] [ Apply ]  |
+------------------------------------------+
```

| 字段 | 控件 | 选项 | 说明 |
|------|------|------|------|
| **Mode** | 下拉（Element Plus `ElSelect`） | `sidebar` / `topbar` | 展示文案：**侧栏** / **顶栏**（或 Sidebar / Topbar）；仅改弹窗草稿 |

| 按钮 | 样式 | 行为 |
|------|------|------|
| **Cancel** | 次按钮 | 丢弃 Mode 草稿；关弹窗；壳布局与已持久化配置**不变** |
| **Apply** | 主按钮 | 将 Mode 草稿**应用**到壳；已登录则 **PUT /settings**；成功后关弹窗 |

| 行为 | 说明 |
|------|------|
| 改 Mode（下拉） | 只更新弹窗草稿；**不**调 API；**不**切换壳 |
| **Apply** 成功 | 壳按新 Mode 重排；内容区路由与业务页状态**保留** |
| **Apply** 失败 | 壳不切换；弹窗保持打开；Toast 错误（已登录写库失败时） |
| 未登录 **Apply** | 仅会话内应用草稿；**不**调 `PUT /settings` |

**Mode** 仅经本弹窗 **Apply** 提交；配置备份 / 导入 / 回滚见版本管理。

---

## 版本管理

入口：Logo + Version（**不是**齿轮）。配置快照的备份、密文投递、导入与回滚（**仅数据配置，不含代码**）。

**出厂版本 `0`**：**dev / prod** 镜像在**构建时**即带配置版本 `version=0`（`is_current=true`，默认业务配置 payload）。容器首次可用时列表已有该行，Logo 显示 `v0`；Backup 另起 `1`、`2`…，**不能**再占用 `0`。

打开时 `GET` 版本列表（[shell_api.md](./shell_api.md)）。关闭：`×` / 遮罩 / `Esc` / **Cancel**。

```
+------------------------------------------------------------------+
| Versions                                                     [x] |
+------------------------------------------------------------------+
| [ Backup ] [ Import ]                                            |
|                                                                  |
| +--------------------------------------------------------------+ |
| | Version | Description      | Create Time        | Operations | |
| |---------|------------------|--------------------|------------| |
| | 2       | after streams…   | 2026-07-25 10:00   | [ Export ] | |  ← 当前：绿标
| | 1       | manual backup    | 2026-07-20 09:00   | [ Export ] | |  ← 选中：蓝底
| | 0       | image default    | （构建时）         | [ Export ] | |
| +--------------------------------------------------------------+ |
|                                                                  |
|                                         [ Cancel ] [ Apply ]     |
+------------------------------------------------------------------+
```

### 表格

| 列 | 说明 |
|----|------|
| **Version** | 数字，可含一位小数点（如 `0`、`1`、`1.5`）；全库唯一；镜像自带 `0` |
| **Description** | Backup 时填写；可空为 `—` |
| **Create Time** | 创建时间 |
| **Operations** | **Export**：下载「封给厂商」的 `.age` / `.bin` 密文包（无密码、无授权票） |

| 行态 | 样式 |
|------|------|
| 当前版本 | **绿**标（`is_current`；与 Logo 旁 Version 一致） |
| 选中 | **蓝**底；当前且选中时蓝底 + 绿标 |

### 表格外控件

| 位置 | 控件 | 行为 |
|------|------|------|
| 左上 | **Backup** | 开 [备份弹窗](#备份弹窗) |
| Backup 右侧紧挨 | **Import** | 须已选中行 → 选密文包 → 粘贴 **import** JWT → 验票 → `site.key` 解密 → 换 payload → **不**自动 Apply |
| 右下 | **Cancel** | 关弹窗 |
| 右下 | **Apply** | 选中非当前行 → 粘贴 **另一张 apply** JWT → 验票 → 解密本地 payload → 写库；刷新当前 Version |

| 按钮 | 可点 | `disabled` |
|------|------|------------|
| **Import** | 已选中一行 | 无选中 |
| **Apply** | 已选中且非当前 | 无选中或已是当前 |

### 备份弹窗

```
+------------------------------------------+
| Backup                               [x] |
+------------------------------------------+
| Version       [ ________ ]               |
| Description   [ ________ ]               |
|                                          |
|                    [ Cancel ] [ Save ]   |
+------------------------------------------+
```

| 字段 | 必填 | 校验 |
|------|------|------|
| **Version** | 是 | `/^\d+(\.\d+)?$/`；唯一 |
| **Description** | 否 | 文本 |

| 按钮 | 行为 |
|------|------|
| **Cancel** | 关闭 |
| **Save** | 当前运行配置 → 明文 zip → **age 封给本站公钥** → 落盘；`is_current` 不变；无授权票 |

### 配置包加密（age，按收件人）

| 项 | 约定 |
|----|------|
| 库 | **age**（X25519）；实现可调 CLI 或兼容库 |
| 明文 | 内存 zip：`manifest` + 各 page 切片 |
| 产物 | **密文字节**（`.age` / `.bin`），不是可直接打开的 zip |
| 公钥 | 可公开；**不能**解密 |
| 私钥 | 绝密；谁持私钥谁能解对应收件人的包 |

**钥匙（前端只需知道路径与职责；生成 / 挂载见 [backend/shell.md](../backend/shell.md)「钥匙」）**：

| 钥匙 | 谁生成 | 现场落点 |
|------|--------|----------|
| 厂商 age `dev.pub` | 厂商本机；进 **django** 镜像 | `/app/keys/dev.pub` |
| 厂商 JWT `ticket.pub` | 厂商本机；进 **django** 镜像 | `/app/keys/ticket.pub` |
| 本站 age `site.key` / `site.pub` | **django / keys-init** 首次部署写卷 | `/secrets/age/site.key`（及 `.pub`） |
| 厂商 `dev.key` / `ticket.key` | 厂商本机 | **不进**任何现场容器 |

| 动作 | 收件人 | 谁能解密 |
|------|--------|----------|
| Backup Save | 本站公钥 | 本站 `site.key` |
| Export（给厂商） | `dev.pub` | 仅厂商 `dev.key` |
| Import 文件 / Apply 本地 payload | 本站公钥 | 本站 `site.key` |

Export：本地世代先用 `site.key` 解开再重封 `dev.pub`。工程师只搬运密文 + **两张 JWT 票**（无任何私钥）。**nodejs 不持钥。**

### 授权票（JWT，Import / Apply 各一张）

与 age **正交**：加密管机密，票管「允许这次操作」。**两张票**：Import 一张、Apply 一张；`action` 必须与操作一致，不可混用、不可一张两用。

| 项 | 约定 |
|----|------|
| 形态 | **JWT** 字符串（粘贴）；`alg` = **EdDSA**（Ed25519） |
| 签发 | 厂商本机 `ticket.key`（不进现场） |
| 验签 | django 读镜像 `/app/keys/ticket.pub`（与 age 的 `dev.pub` **不是**同一把） |
| 库 | 现场：`PyJWT`；厂商：签发小工具 |

**Claims**：

| Claim | 说明 |
|-------|------|
| `action` | `import` 或 `apply`（与本次操作一致） |
| `pkg_hash` | 密文包 `SHA256` 十六进制：Import = 上传文件；Apply = 该行落盘 payload |
| `site_id` | 可选；有则须与本站一致 |
| `exp` | 过期（Unix / JWT 标准） |
| `jti` | 唯一 id；用过后入库，防重放 |

厂商侧：对 `update.bin` 算 hash → 签 `action=import` 票 → 再签 `action=apply` 且 **同一 `pkg_hash`** 的第二张票（Apply 时 payload 已是该包）。回滚本地旧世代：对该行 payload 的 hash 另签 `action=apply` 票。

### 授权票弹窗

Import 选文件后、以及 Apply 前各弹一次；只收集**本操作**对应的那张 JWT。

```
+------------------------------------------+
| Authorization                        [x] |
+------------------------------------------+
| Ticket [ ____________________________ ]  |
|                                          |
|                    [ Cancel ] [ Verify ] |
+------------------------------------------+
```

| 按钮 | 行为 |
|------|------|
| **Verify** | 带 `X-Auth-Ticket` 继续当前 Import 或 Apply |
| **Cancel** | 中止 |

| Toast | 文案 |
|------|------|
| Backup 成功 | `Version saved` |
| Import 成功 | `Version imported` |
| Apply 成功 | `Version applied` |
| 解密/票/包失败 | 后端 `message` |
| 版本重名 | `Version already exists` |

### 权限与 API

| 操作 | 权限 | 二次授权 |
|------|------|----------|
| 列表 / Backup / Export | `users.export_site_config` | 无 |
| Import | `users.import_site_config` | JWT，`action=import` |
| Apply | `users.import_site_config` | JWT，`action=apply`（另一张） |

---

## Mode 切换行为

| 项 | 说明 |
|----|------|
| 生效时机 | 仅 Page Settings **Apply**；下拉变更本身不生效 |
| 应用瞬间 | 导航从侧栏 ↔ 顶栏重排；内容区路由与业务页状态**保留** |
| 当前高亮 | 应用后仍指向同一导航项 |
| 默认值 | 无已存配置时为 `sidebar`（`DEFAULT_MODE`） |
| 持久化 | 已登录且 **Apply**：后端 `/settings`；未登录 **Apply**：仅会话临时 |

---

## 与业务页关系

| 项 | 说明 |
|----|------|
| 业务页标题 | 各页自有顶栏标题（如 Streams、Preview）；**不属于**本壳导航 |
| Preview 等页内「无齿轮」 | 指业务页工具栏；**壳层**齿轮在导航用户区，二者不冲突 |
| 路由 | 每导航项对应一条前端路由；深链进入时壳按路由高亮对应项 |

---

## 交互流程

```
进入管理端           ──> GET /me + GET /settings + 当前 Version；栏尾与导航显隐；Logo+Version（sidebar 左上）
点击导航项           ──> 切换内容区；高亮该项
Logout               ──> POST /logout → 清会话 → Login 页
点击齿轮             ──> Page Settings（仅 Mode）
Mode Apply           ──> PUT /settings → 切换壳布局
点击 Logo+Version    ──> 打开版本管理；GET /site-config/versions
Backup → Save        ──> Version/Description → zip → age 封本站 → 落盘
行内 Export          ──> （若需）本站私钥解本地包 → age 封厂商公钥 → 下载 .age/.bin
Versions → Import    ──> 选中行 → 选密文包 → 粘贴 import 票 → 验 JWT → 本站私钥解密 → 换 payload
Versions Apply       ──> 选中非当前 → 粘贴 apply 票 → 验 JWT → 本站私钥解密 → 写运行库 → 刷新 Version
```
