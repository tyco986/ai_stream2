# Users 页面

管理端 **Users** 页 UI 与 RBAC 使用约定。基于 Django **User / Group / Permission**；业务写操作仍由后端 `has_perm` 强制校验。

API 契约见 [user_api.md](./user_api.md)。登录页见 [login.md](./login.md)。壳层导航、齿轮、**Versions / 授权票** 见 [shell.md](./shell.md)。

UI 控件统一用 **Element Plus**（经 `shared/ui`）。

| 概念 | 说明 |
|------|------|
| **User** | 登录账号；通过加入 **Group** 获得权限；一般**不**直接挂 Permission |
| **Group** | 权限模板（角色）；M2M 绑定 Django Permission |
| **Permission** | 扁平 `app_label.codename`；前端可按模块分组展示 |
| **有效权限** | 当前用户全部 Group 权限并集（`is_superuser` 视为全权限）；拉取见 [shell_api.md](./shell_api.md) `GET /me` |

---

## 鉴权分层

| 层 | 回答 | 机制 |
|----|------|------|
| 登录会话 | 是谁 | 用户名 + 密码 → 会话 / Token（见 [login.md](./login.md) / [login_api.md](./login_api.md)） |
| RBAC | 能不能做 | Django Group → Permission；UI 显隐 + 后端强制 |
| Step-up（JWT 授权票） | Versions **Import** / **Apply** 各一张票 | 见 [shell.md](./shell.md)；**不**在本页触发；Backup / Export 不要票 |

| 规则 | 说明 |
|------|------|
| 前端权限 | 仅控制导航 / 按钮显隐；**不能**当安全边界 |
| 后端权限 | 每个写接口校验对应 Permission；缺权 → `403` |
| Superuser | `is_superuser=true` 绕过 Permission 检查；**Import** / **Apply** 仍须各自 JWT（壳层）；Backup / Export 不要票 |

---

## Permission 与导航

有效权限驱动：**导航项是否出现**、业务页内写操作按钮是否可用。无 `view_*` 时对应导航隐藏；深链进入无权限页 → 前端拦截并提示。

| 导航 / 模块 | 典型 codename 前缀 | 说明 |
|-------------|-------------------|------|
| Streams | `streams.*` | 见权限目录 |
| Previews | `previews.*` | |
| Recordings | `recordings.*` | |
| Models | `models.*` | |
| Pipelines | `pipelines.*` | |
| Servers | `servers.*` | |
| Events | `events.*` | |
| Users | `users.*` | 本页；改 Group 权限另需 `users.change_group` |

完整 catalog 与 UI 勾选映射见 [user_api.md](./user_api.md) **PermissionCatalog**。

进入管理端时拉取有效权限（[shell_api.md](./shell_api.md) `GET /me`）；刷新 Users / 改自身 Group 后须重新拉取。

---

## Users 主页面

```
+========================================================================================+
| Users                                                                                  |
+========================================================================================+
| [ Add ] [ Remove ] [ Refresh ]                              [ Groups ]                 |
+--+----------+------------------+---------------------+---------------------------------+
|[]| Name     | Groups           | Last Action         | Operations                      |
+--+----------+------------------+---------------------+---------------------------------+
|[]| alice    | admin, operator  | 07-22 10:15  Edit   | ✏ 🗑 📓                         |
|[]| bob      | viewer           | 07-21 18:02  Login  | ✏ 🗑 📓                         |
|[]| carol    | —                | —                   | ✏ 🗑 📓                         |
+--+----------+------------------+---------------------+---------------------------------+
|                                              < 1  2  3 >                               |
+========================================================================================+
```

| 区域 | 说明 |
|------|------|
| 页面标题 | 固定显示 `Users` |
| 工具栏 | 左：**Add** / **Remove** / **Refresh**；右：**Groups** |
| 数据表格 | 全宽；含 Checkbox 与行内 Operations；底部分页 |

**布局补充说明**

| 项 | 说明 |
|----|------|
| 工具栏按钮样式 | Add / Refresh / Groups 为蓝底白字；Remove 为红底白字 |
| 工具栏禁用 | 未勾选行时 **Remove** 为 `disabled`（浅红底白字） |
| 行主键 | 用户 **id**（UUID）；表格默认不展示 id |
| 进入页 | 需有效权限含 `users.view_user`（或等价 view）；否则导航不出现本页 |

---

## 工具栏

| 元素 | 样式 | 可用条件 | 行为 |
|------|------|----------|------|
| **Add** | 蓝底白字 | 有 `users.add_user` | 打开 [User 弹窗](#user-弹窗add--edit)（创建） |
| **Remove** | 红底白字 | ≥1 行勾选且有 `users.delete_user` | **二次确认**后批量删除勾选用户 |
| **Refresh** | 蓝底白字 | 始终（有 view） | 按当前分页 / 筛选重新 `GET` 用户列表 |
| **Groups** | 蓝底白字 | 有 `users.view_group` | 打开 [Groups 弹窗](#groups-弹窗) |

**Remove** 不可删除当前登录用户自身；若勾选含自身，确认后跳过自身并 Toast 说明（或确认前禁用含自身的提交——实现二选一，文档以**跳过自身并提示**为准）。

### Remove 确认

```
+--------------------------------+
| Confirm Remove             [x] |
+--------------------------------+
| Remove {n} selected user(s)? |
+--------------------------------+
|          [ Remove ]  [ Cancel ]|
+--------------------------------+
```

---

## 数据表格

### 列定义

| 列名 | 类型 | 说明 |
|------|------|------|
| Checkbox | — | 行多选；供工具栏 Remove |
| Name | `str` | `username`；只读列，编辑在弹窗 |
| Groups | `str` | 所属 Group **name** 列表，逗号分隔；无 Group 时为 `—` |
| Last Action | `str` | 最近一条审计摘要；无记录为 `—`；展示格式见下 |
| Operations | 图标组 | Edit / Remove / Log |

### Last Action 展示

| API 字段 | UI |
|----------|-----|
| `last_action_at` + `last_action_label` | `{MM-DD HH:mm}  {label}`，如 `07-22 10:15  Edit` |
| 均为空 | `—` |

`label` 为短英文动词（如 `Login` / `Edit` / `Export`），由后端审计写入，前端不翻译枚举以外的自由文本时可原样展示。

### 行样式

| 条件 | 说明 |
|------|------|
| `is_active=false` | Name 列灰色；表示已停用 |
| `is_superuser=true` | 不强制特殊色；Edit 弹窗内可见开关 |

---

## Operations 列

图标顺序固定：**Edit → Remove → Log**。

| 顺序 | 图标 | 名称 | 权限 | 行为 |
|:----:|:----:|------|------|------|
| 1 | ✏ | Edit | `users.change_user` | 打开 User 弹窗（编辑） |
| 2 | 🗑 | Remove | `users.delete_user` | **二次确认**后删除该用户（不可删自身） |
| 3 | 📓 | Log | `users.view_user` | 打开该用户 [Log 弹窗](#log-弹窗) |

无对应写权限时图标隐藏或 disabled（实现统一：无权限则**隐藏**）。

---

## User 弹窗（Add / Edit）

工具栏 **Add** 与 Operations **✏** **复用同一弹窗**。弹窗**不展示**用户 id。

```
+------------------------------------------+
| Add User / Edit User                 [x] |
+------------------------------------------+
| Username   [ ________________ ]          |
| Password   [ ________________ ]          |  ← 仅 Add；Edit 为可选「Reset password」
| Active     [✓]                           |
| Superuser  [ ]                           |
| Groups     [ admin ▼ ]  （多选）          |
|                                          |
|                    [ Cancel ] [ Save ]   |
+------------------------------------------+
```

| 字段 | 控件 | Add | Edit | 说明 |
|------|------|-----|------|------|
| **Username** | 文本 | 必填 | 可改（若产品禁止改则只读） | 对应 Django `username`；唯一 |
| **Password** | 密码 | 必填 | 默认不显示；提供 **Reset password** 时填写新密码 | 不明文回显 |
| **Active** | 开关 | 默认开 | 可改 | `is_active`；关则无法登录 |
| **Superuser** | 开关 | 默认关 | 可改 | `is_superuser`；仅已是 superuser 的操作者可打开（后端强制） |
| **Groups** | 多选下拉 | 可选 | 可改 | 选项来自 Group 列表；存 Group id |

| 按钮 | 行为 |
|------|------|
| **Cancel** | 关弹窗；不调写 API |
| **Save** | Add → `POST`；Edit → `PATCH`；成功后关弹窗并 Refresh 表格 |

**不**在本弹窗勾选 Permission（权限只经 Groups 模板配置）。

---

## Groups 弹窗

点击工具栏 **Groups** 打开。用于维护 Django Group 及其 Permission 集合（权限模板）。

```
+==================================================================================+
| Groups                                                                       [x] |
+========================+=========================================================+
| [ + ]                  | Permissions                                             |
|------------------------|---------------------------------------------------------|
| admin            ✏ 🗑  | Module      view   add   change   delete                |
| operator         ✏ 🗑  | Streams     [✓]    [✓]   [✓]      [ ]                   |
| viewer           ✏ 🗑  | Previews    [✓]    [ ]   [ ]      [ ]                   |
| * (selected)           | ...                                                     |
|                        | Users       [✓]    [✓]   [✓]      [✓]                   |
|                        |                                                         |
|                        |                              [ Cancel ] [ Save ]        |
+========================+=========================================================+
```

| 区域 | 说明 |
|------|------|
| 左侧 | Group 列表；**+** 新建；行内 ✏ 改名、🗑 删除（二次确认） |
| 右侧 | 当前选中 Group 的 Permission 勾选表；按**模块**分行、按动作列勾选 |
| 打开时 | `GET` Group 列表 + Permission catalog + 当前选中 Group 的已授权 ids |
| **Save** | 将右侧勾选写回当前 Group 的 permission 集合（`PUT`）；**不**自动关弹窗（可继续切 Group 编辑）；实现也可 Save 后保持打开并 Toast |

| 规则 | 说明 |
|------|------|
| 存储 | 后端只存 Django Permission id / codename 列表；嵌套「页面树」**不是**持久化格式 |
| UI 映射 | catalog 将扁平 permission 映射为模块 × 动作矩阵；未映射的 permission 可归入 **Other** 行 |
| 删除 Group | 用户仅解除与该 Group 的 M2M，**不**级联删用户 |
| 权限 | 查看需 `users.view_group`；改名/改权限需 `users.change_group`；新建需 `users.add_group`；删除需 `users.delete_group` |

左侧新建 Group 时默认名为可编辑草稿；确认后出现在列表并选中，右侧全不勾选直至 Save。

---

## Log 弹窗

点击 📓 打开，只读审计列表 + **Close**。

```
+------------------------------------------+
| User Log: alice                      [x] |
+------------------------------------------+
| 07-22 10:15  Edit user                   |
| 07-22 09:01  Login                       |
| 07-21 18:02  Export site config          |
| ...                                      |
|                              [ Close ]   |
+------------------------------------------+
```

数据来自该用户审计日志 API；分页或滚动加载由实现选择，首屏足够即可。

---

## 交互流程

```
进入 Users                 ──> GET 用户列表（分页）
Add / ✏ Save               ──> POST 或 PATCH 用户；刷新表
Remove（工具栏 / 行内）    ──> 确认后 DELETE / batch delete
Refresh                    ──> 再 GET 列表
Groups                     ──> GET groups + permissions；Save → PUT group permissions
📓 Log                     ──> GET 该用户审计日志
```

---

## Toast（建议文案）

| 场景 | 文案 |
|------|------|
| 用户已保存 | `User saved` |
| 用户已删除 | `User deleted` / `{n} user(s) deleted` |
| 跳过删除自身 | `Cannot delete the current user` |
| Group 已保存 | `Group saved` |
| 无权限 | 沿用全局 `403` 映射 |

---

## 与其它文档

| 文档 | 关系 |
|------|------|
| [shell.md](./shell.md) / [shell_api.md](./shell_api.md) | 导航 **Users**；`GET /me` / **Logout**；齿轮 Mode；Versions / age / 授权票 |
| [login.md](./login.md) / [login_api.md](./login_api.md) | 登录页；建会话 |
| [user_api.md](./user_api.md) | 本页 User / Group / Permission / 用户审计 |
| 各业务 `*_api.md` | 各写接口自行声明所需 Permission codename |
