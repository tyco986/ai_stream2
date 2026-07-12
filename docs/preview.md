# Preview 页面

生产端 Preview 页面，用于**实时**预览 RTSP 视频流。本页**不含回放**；仅直播拉流。

**组**与**流**的唯一标识均为 **UUID**（RFC 4122，如 `550e8400-e29b-41d4-a716-446655440000`），由后端生成、不可修改。树与宫格默认展示 **Name**；筛选、槽位绑定、API 请求均以 **id** 为准。

分组树数据结构与 [stream.md](./stream.md) 左侧栏一致，但本页为**只读**（无 **⋮**、无增删改）。流 **URL**（含用户名与密码）由后端返回，用于播放器拉流。

## 主页面

```
+====================================================================================================+
| Preview                                                                                            |
+============+=======================================================================================+
| Groups [▦][🧹][↻]|  [ 2×2 ▼ ] [ Default *              ▼ ]  [💾] [📄] [🧹]              [ ⊞ ◎ ] |
| [Search__]    |+--------------------------------+--------------------------------+              |
| v All      | | cam-01              ● 25fps   | cam-02              ● 15fps   |              |
| v Site A   | |░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|              |
| v Entr.    | +--------------------------------+--------------------------------+              |
|   cam-01 * | | (empty) drop stream here       | (empty) drop stream here       |              |
|   cam-02   |                                                                                       |
+============+=======================================================================================+
```

宫格槽位以**红色框线**（**2px**）标出边界；槽间**无间隙**，画面仍紧挨填满网格。选中槽叠加高亮叠层，框线始终可见。

| 区域 | 说明 |
|------|------|
| 页面标题 | 固定显示 `Preview` |
| **Preset 栏** | 独立一行；**左侧** **Layout** 下拉 + **Preset** 下拉 + **Save** / **Save As** / **Clear** + **Grid / Focus** 图标；见下文 |
| 左侧栏 | 只读分组树 + **Groups 标题行图标** + 搜索框 |
| 右侧栏 | **Preset 栏** + 宫格（或 Focus Mode） |

**布局补充说明**

| 项 | 说明 |
|----|------|
| 左侧搜索框 | 过滤分组树节点 Name（组与流） |
| **Groups · 宫格筛选 ▦** | Toggle；仅展示当前 `slots` 已绑流；见下文 |
| **Groups · Clear 🧹** | 与 Preset 栏 **Clear** 相同；清空全部槽位 |
| **Groups · Refresh ↻** | 重新 `GET /groups/tree`；见下文 |
| **Groups 流字色** | `enabled` + `status`；见 [流节点样式](#流节点样式groups-树) |
| 点击左侧组 | 按 **group id** 高亮树节点；**不**自动填充宫格；右侧**无**摘要文案 |
| 点击左侧流 | 将流加入宫格（见下文槽位规则）；树节点高亮 |
| 当前选中槽 | 选中高亮叠层；槽位**红色 2px 框线**始终可见；新点流优先填入该格，无选中格则填下一空槽 |
| 空槽 | 文案 `drop stream here`；可点击设为当前选中槽 |
| **Preset 栏** | **Preset** 下拉展示当前激活预设；未 **Load** 其他预设时为 `Default` |
| **Grid / Focus 图标** | **Preset 栏**最右侧；Grid ↔ Focus 切换；见下文 |

> **无「未分组」节点**；未归属具体组的流等价于 **`All`** 分组。

---

## 左侧栏（Groups）

### 树结构

与 Streams 页相同，但**只读**：

| 节点类型 | 展开时 | Operations |
|----------|--------|------------|
| **组** | 显示子组 + **直属该组的流（Name + 状态字段）** | **无**；节点 key 为 **group id** |
| **流** | — | **无**；节点 key 为 **stream id**；含 `enabled`、`status`、`recording` |

组**折叠**时不显示其下流 Name；**展开**后仅列出**直属该组**的流 **Name** 为子节点。子组内的流显示在对应子组展开后，不在父组节点下展开列出。

```
v Site A
v Entrance :
    cam-01
    cam-02
  v East Gate :
      cam-06
```

### 行为

| 操作 | 说明 |
|------|------|
| 点击**组名** | **单选**（**group id**）；树高亮；右侧宫格**不变** |
| 点击**流 Name** | 按槽位规则加入宫格；左侧流节点可高亮表示已在格中 |
| 展开 / 折叠组 | 仅影响树展示 |
| **`All`** | 展开可显示 Group 为 `All` 的流 Name |
| 搜索 | 过滤树节点 Name；不匹配节点隐藏，父组无可见子节点时一并隐藏 |

### 流节点样式（Groups 树）

与 [stream.md](./stream.md) **流节点样式**相同（`enabled` + `status` 决定字色）。**Offline 且 Enabled** 的流仍为黑色，**可正常点击**绑槽；格内再按 `status` 显示 LIVE / Offline。

| 条件 | 流 Name 字色 |
|------|----------------|
| `enabled=true` 且 `status=online` | **黑色** |
| `enabled=false` | **灰色** |
| `enabled=true` 且 `status=offline` | **黑色** |

### Groups 标题行

`Groups` 文案右侧并排三枚**小图标按钮**（实现自定 glyph）；位于搜索框**之上**。自左向右：**▦** · **🧹** · **↻**。

```
+------------+
| Groups [▦][🧹][↻] |
| [Search__] |
| v All      |
|   cam-01 * |
+------------+
```

#### 宫格筛选（▦）

| 项 | 说明 |
|----|------|
| 图标 | **▦**（小宫格 / 槽位占用）；与 Preset 栏 **Grid ⊞** 区分造型 |
| 类型 | **Toggle**；开启时图标高亮 |
| 行为 | 树仅保留当前 `slots` 非 `null` 的流；保留组层级，隐藏无已绑流的分支 |
| 与搜索 | **叠加**：先宫格筛选，再按 Search 过滤 Name |
| `slots` 全空 | 开启后树为空；或 Toggle **disabled**（实现自定，建议可开但展示空态） |
| `slots` 变更 | 筛选结果随 Preset / 绑流 / **Clear** 自动更新 |
| Tooltip | 建议 `Show streams in layout` |

#### Clear（🧹）

与 [Preset 栏 **Clear**](#clear图标按钮) **同一逻辑**；左侧多提供一个入口。

| 项 | 说明 |
|----|------|
| 图标 | **扫把** 🧹；与 Preset 栏 **Clear** 同款 |
| 行为 | `slots` 全 `null`；**不改** `layout` / `view_mode` |
| 持久化 | **不**调 API，直至 **Save**；Preset 下拉名标 `*` |
| 可用条件 | 至少 1 槽已绑定时可点；全空 **disabled** |
| Focus Mode | **不**退出 Focus；列表仍保留 layout 等量空占位 |
| Tooltip | 建议 `Clear` |

#### Refresh（↻）

重新拉取分组树；用于**多人 / 跨页**场景下同步 Streams 管理端变更，无需离开本页。

| 项 | 说明 |
|----|------|
| 图标 | **↻** / ⟳（refresh） |
| API | **`GET .../streams/groups/tree`**（只读）；**不**调用 Test |
| 树字段 | 同步流节点 `enabled` / `status` / `recording`（库内已有值；**非**当场 Test） |
| **`last_test_at`** | **不**更新 |
| 宫格 / Preset | **不**改 `layout`；**不** Load 预设；未 **Save** 的 `slots` 编辑**保留** |
| 播放 | 已绑槽**继续播放**；不因刷新中断拉流 |
| 树 reconcile | `slots` 中某 **stream id** 在新树中不存在 → 该槽置 `null`；可选 Toast「N stream(s) missing」 |
| 会话态 | 尽量保留组展开 / 折叠、Search、**▦** 筛选开关 |
| 请求中 | 图标 **disabled** 或 loading；防重复点击 |
| Tooltip | 建议 `Refresh` |

### 左侧栏示意

```
+------------+
| Groups [▦][🧹][↻] |
| [Search__] |
| v All      |
|   cam-04   |
| v Site A   |
| v Entr. :  |
|   cam-01 * |
|   cam-02   |
| > Garage:  |
+------------+
```

行末 `*` 表示该流已占用某一宫格槽位。

---

## 右侧宫格

### Preset 栏

位于右侧栏顶、宫格之上；**单行**。自左向右：**Layout** 下拉 → **Preset** 下拉 → **Save** / **Save As** / **Clear** → **Grid / Focus** `[ ⊞ ◎ ]`（行末右对齐）。

```
[ 2×2 ▼ ] [ Default *              ▼ ]  [💾] [📄] [🧹]                        [ ⊞ ◎ ]
```

#### Layout 下拉

展开选项：`1×1` / `2×2` / `3×3` / `4×4`。

| 项 | 说明 |
|----|------|
| 位置 | **Preset 下拉左侧**；收起态如 `2×2 ▼` |
| **Focus Mode** | **始终可用**（与 Grid 相同）；切换规格后右侧列表槽位数同步变化 |

| 布局 | 槽位数 | 默认 |
|------|--------|------|
| `1×1` | 1 | |
| `2×2` | 4 | ✓ |
| `3×3` | 9 | |
| `4×4` | 16 | |

切换 Layout 时：按**槽位序号**保留内容（含 `(empty)`），**不**压缩重排：

| 方向 | 行为 |
|------|------|
| **放大**（如 2×2 → 3×3） | 原 Slot 1 … M 内容不变；新增 Slot M+1 … N 为 **empty** |
| **缩小**（如 3×3 → 2×2） | 保留 Slot 1 … N；超出新槽位数的槽位停止播放并从宫格移除 |

> **Layout** 与 **Preset** 为同行相邻控件：**Layout** 只改宫格规格；**Preset** 切换激活预设。任意变动（含 **Default**）**均不自动持久化**；Preset 下拉名附 `*` 直至 **Save**；或切换预设（未保存时须确认丢弃）。

### 手动保存

| 项 | 说明 |
|----|------|
| 原则 | **无自动保存**；`layout` / `slots` / `view_mode` 变更仅存在于当前会话，直至 **Save** |
| **\*** 脏标记 | 当前宫格与**已加载预设的最近保存内容**不一致时（含 **Grid / Focus** 切换），**Preset** 下拉展示名后附 `*`（**含 Default**） |
| **Save** | 图标按钮；将当前 `layout` + `slots` + `view_mode` 写入下拉所选预设；成功后去除 `*` |
| **Load** | **Preset** 下拉选其他项；应用目标预设已保存内容；若有未 **Save** 变动，须确认丢弃 |
| 刷新 / 重启 | **GET active_layout** → 加载该预设**已保存**快照；未 **Save** 的编辑丢失 |

---

## 布局预设（Preset 下拉）

多条命名布局预设；内置 **`Default`**。**Preset** 下拉切换激活预设；**Save** / **Save As** / **Clear** 图标按钮；下拉底部可进 **Manage**。

### Default 预设

| 项 | 说明 |
|----|------|
| **name** | 固定 **`Default`**；系统内置；**全库唯一** |
| **Preset 下拉** | 激活 **Default** 时收起态显示 `Default` |
| **保存** | 与其它预设相同：仅用户 **Save** 时 **PATCH** 写入；变动后下拉名标 `*` |
| **下拉列表** | **Default** 始终出现；当前激活项标 ✓ |
| **Save As** | 打开 **Save As Layout 弹窗** **POST** 新建；不可用名 **`Default`** |
| 首访 | 后端保证存在 **Default** 记录；默认 `layout: 2×2`、`slots` 全 `null`、`view_mode: grid` |

### 当前激活预设（Active Preset）

后端单独持久化**当前用户激活的 preset id**，服务重启或刷新后无需重新 **Load**。

| 方法 | 路径（示例） | 说明 |
|------|--------------|------|
| **GET** | `.../preview/active_layout` | 返回 `{ "preset_id": "<uuid>" }` |
| **PUT** | `.../preview/active_layout` |  body `{ "preset_id": "<uuid>" }`；**Load** 选中预设后调用 |

**进页流程**：

```
GET active_layout → preset_id
        │
        ▼
GET /layouts/{preset_id}（或列表内嵌）→ layout + slots + view_mode
        │
        ▼
应用宫格与 **Grid / Focus** 视图；Preset 栏显示对应 name（多为 Default 或上次 Load 的命名预设）
```

**Load 某预设** → 应用已保存的 `layout` + `slots` + `view_mode` → **PUT active_layout** → **Preset** 下拉更新；清除 `*`。`view_mode` 为 `focus` 时，`focus_index` 取首个已绑定槽下标，全空则 `0`。

未 **Save** 时切换 **Preset** 或刷新页面：当前编辑不写入后端。

### Preset 下拉

**Preset 栏**内、**Layout 下拉右侧**为 **Preset** 下拉；紧挨其右为 **Save**、**Save As**、**Clear** 三个**图标按钮**（实现自定 glyph，如 💾 / 📄 / 扫把）；行末为 **Grid / Focus** `[ ⊞ ◎ ]`。

```
[ 2×2 ▼ ] [ Default *              ▼ ]  [💾] [📄] [🧹]                        [ ⊞ ◎ ]
```

下拉展开：

```
┌──────────────────┐
│ Default       ✓   │
│ 值班 2×2          │
│ 车库 3×3          │
├──────────────────┤
│ Manage            │
└──────────────────┘
```

| 项 | 行为 |
|----|------|
| 选预设项 | 应用已保存 `layout` + `slots` + `view_mode`；**PUT active_layout**；收起态更新 **name**；清除 `*` |
| 未 **Save** 切换 | 须确认丢弃当前编辑 |
| **\*** | 附于下拉收起态展示名后（如 `Default *`） |
| **Manage** | 位于下拉列表**最下方**（分隔线之下）；打开 **Manage 弹窗** |
| Focus Mode | **Clear** 后**保持**当前 `view_mode`；切换 **Preset** 应用目标预设已保存的 `view_mode` |

### Save（图标按钮）

**Save** 图标紧挨 **Preset** 下拉右侧；**即时保存**，无弹窗。

| 项 | 说明 |
|----|------|
| 写入对象 | 当前**下拉所选预设**（含 **Default**） |
| 写入字段 | `layout`、`slots`、`view_mode`（**不改** `name`） |
| API | **PATCH** `/layouts/{preset_id}`；成功后 **PUT** `.../shot` 上传 Grid 合成图 |
| 成功后 | Toast「Layout saved」；下拉展示名去除 `*` |
| 无变动 | 可 **disabled** 或仍允许点击（幂等；实现自定） |
| Tooltip | 建议 `Save` |

### Clear（图标按钮）

**Clear** 使用**扫把**图标；Preset 栏紧挨 **Save As** 右侧；**Groups 标题行**亦提供同款 **🧹**（逻辑一致）。

| 项 | 说明 |
|----|------|
| 行为 | 清空当前布局全部已绑定流（`slots` 全 `null`）；**不改** `layout` |
| 持久化 | **不**调用 API，直至用户 **Save**；有变动时 **Preset** 下拉名标 `*` |
| 可用条件 | 至少 1 槽已绑定时可点；全空时 **disabled** |
| Focus Mode | **不**退出 Focus；`slots` 全 `null` 后右侧列表仍保留与 layout 等量的**空槽占位**（如 2×2 仍为 4 枚纵向窗口） |
| Tooltip | 建议 `Clear` |

### Shot 合成

Shot 为 **Grid 离屏合成图**，**不是**当前页面 DOM 截图；与 `view_mode` 无关。

| 项 | 说明 |
|----|------|
| 为何不用视口截图 | **Focus** 下截图会带入主区/右侧列表等 **UI 窗框**；右侧列表有**滚动条**且无法完整入镜，不适合作 layout 缩略图 |
| 合成方式 | 按当前 `layout` + `slots`，取各槽**当前画面帧**（空槽用占位）；在 **canvas 离屏**按 Grid **行优先**拼成整张图 |
| 视觉 | 与 **Grid** 宫格一致（含 **2px** 红色槽位框线）；**不含** Focus 侧栏、Preset 栏、滚动条 |
| `view_mode: focus` | 用户可仍在 **Focus** 下 **Save** / **Save As**；**不**切换视图；仅 shot 按上表合成 |
| 上传 | 合成结果在 **Save**（点击时）/ **Save As**（开弹窗时）生成，API 成功后 **PUT** `.../shot` |

### Save As Layout 弹窗（Manage ✏️ 共用）

**Save As** 图标与 **Manage** 行内 **✏️** 共用同一弹窗壳；字段布局一致，按入口区分**新建** / **重命名**。

#### 新建（Save As 图标）

由 **Save As** 图标打开；基于当前宫格**新建**命名预设。**Create** 主按钮为蓝底白字。

```
+--------------------------------------------------+
| Save As Layout                               [x] |
+--------------------------------------------------+
| Name    [_______________________________ ] [🖼]  |
|                                                  |
| Grid layout    2×2  (4 slots)                    |
| View           Focus                             |
| Streams        2 bound                           |
|                                                  |
| Slot        Stream                               |
| 0           cam-01                               |
| 1           cam-02                               |
| 2           (empty)                              |
| 3           (empty)                              |
+--------------------------------------------------+
|            [ Create ]    [ Cancel ]              |
+--------------------------------------------------+
```

| 项 | 说明 |
|----|------|
| **Name** 初值 | 空 |
| 数据来源 | 当前会话 `layout` + `slots` + `view_mode` |
| 持久化 | **POST**；body 含 `name`、`layout`、`slots`、`view_mode`；成功后 **PUT** `.../shot` 上传弹窗内同源截图 |
| 成功后 | **Load** 新预设并 **PUT active_layout**；Toast「Layout created」 |

#### 重命名（Manage ✏️）

由 **Manage → Operations ✏️** 打开；**仅改 `name`**，其余字段展示该行预设**已保存**快照（只读）。

```
+--------------------------------------------------+
| Rename Layout                                [x] |
+--------------------------------------------------+
| Name    [ 值班 2×2_______________________ ] [🖼] |
|                                                  |
| Grid layout    2×2  (4 slots)                    |
| View           Focus                             |
| Streams        2 bound                           |
|                                                  |
| Slot        Stream                               |
| 0           cam-01                               |
| ...                                              |
+--------------------------------------------------+
|              [ Save ]    [ Cancel ]              |
+--------------------------------------------------+
```

| 项 | 说明 |
|----|------|
| **Name** | 可编辑；预填该行预设名；**全库唯一**；不可为 **`Default`** |
| 其余字段 | 只读；来自该行预设已保存的 `layout` / `slots` / `view_mode`；**不**取当前宫格会话态 |
| 持久化 | **PATCH** `/layouts/{preset_id}`；body 仅 `{ "name": "..." }` |
| 成功后 | Toast「Layout renamed」；关闭弹窗；**Manage** 表格与 **Preset** 下拉同步新名 |
| **Default** | **✏️** / **🗑** 展示但 **disabled**；不可重命名、不可删除 |

#### 字段（共用）

| 字段 | 新建 | 重命名 |
|------|------|--------|
| **Name** | 可编辑 | 可编辑 |
| **Shot** 图标 | 可点（Grid 合成预览） | 可点（已存 shot）；无图时 **disabled** |
| Grid layout | 否（当前宫格） | 否（该行已保存） |
| **View**（`view_mode`） | 否（当前 `grid` / `focus`） | 否（该行已保存） |
| Streams | 否 | 否 |
| **Slot** | 否 | 否 |

**Name** 输入框右侧为 **Shot** 图标（实现自定 glyph，如 🖼）；**无**单独 Shot 行。点击图标打开**图片预览窗**（居中大图、`[x]` 或遮罩关闭）；**不**跳转新页。

| 入口 | Shot 图标图源 |
|------|----------------|
| **Save As**（新建） | 打开弹窗时 **Grid 离屏合成**（与即将 **PUT** 的 shot 一致）；见上文 **Shot 合成** |
| **Rename**（✏️） | 该行预设已保存的 `shot_url` / `GET .../shot`；无缓存图时 **disabled** |

**Slot 列表**第一列展示槽位序号 `0` … `N−1`（与 `slots` 下标一致）。

| 项 | 说明 |
|----|------|
| 重名 | 与已有 **name** 冲突 → 校验失败 |
| 不写入 | UUID 不在弹窗展示 |
| Focus Mode | **新建**仍以当前宫格为准（含 `view_mode`）；**重命名**不受 Focus 影响 |

### Manage 弹窗

由 **Preset 下拉 → Manage**（列表最底项）打开；管理全部布局预设。

```
+------------------------------------------------------------------------+
| Manage                                                         [x] |
+------------------------------------------------------------------------+
| [ Delete ]                                 [ Search: ________ ]        |
+---+------------+---------+------+------------------------------+
| ☐ | Name       | Streams | Grid | Operations                   |
+---+------------+---------+------+------------------------------+
| ☐ | Default    | 0       | 2×2  | ✏️ 🗑                        |
| ☐ | 值班 2×2   | 2       | 2×2  | ✏️ 🗑                        |
+---+------------+---------+------+------------------------------+
```

**Delete** 与 **Search** 同处表格上方一行：**Delete** 左对齐（红底白字），**Search** 右对齐；勾选行后点击 **Delete** 批量删除（**Default** 不可勾选 / 不可删）。

**Default** 行 **Operations** 与命名预设相同展示 **✏️**、**🗑**，但均为 **disabled**（灰显、不可点击），不隐藏图标。

| 列 | 说明 |
|----|------|
| **Checkbox** | 多选；**Default** 行 disabled 或不渲染勾选框 |
| Name | 预设名 |
| Streams | 已绑定路数 |
| Grid | `1×1` / `2×2` / `3×3` / `4×4` |
| Operations | **✏️**（重命名，弹窗 **Name** 旁 **Shot** 图标预览）/ **🗑**（删除）；**Default** 行同样展示，**disabled** |

| 操作 | 行为 |
|------|------|
| **✏️** | 打开 **Rename Layout** 弹窗（与 **Save As** 共用壳）；**PATCH** 仅改 `name`；**Default** 行 **disabled** |
| **🗑** | 删除该行预设（二次确认）；若删当前激活项 → **PUT active_layout** 为 **Default** 并应用；**Default** 行 **disabled** |
| **Delete** | 表格上方工具行**左对齐**；红底白字；删除所有已勾选行（二次确认）；同上处理当前激活项 |
| **Search** | 与 **Delete** **同行**；**右对齐**；按 Name 过滤表格 |
| **Close** / `[x]` | 关闭弹窗，不修改当前宫格 |

### Grid / Focus 组合图标

**Preset 栏**行末；**单一控件**内并排两图标（**⊞** Grid、**◎** Focus），**不用文字按钮**。

```
Grid 模式（◎ 可点，⊞ 高亮）          Focus 模式（⊞ 可点，◎ 高亮）
[ ⊞ ◎ ]                              [ ⊞ ◎ ]
  ^^                                        ^^
```

| 项 | 说明 |
|----|------|
| **Grid 图标 ⊞** | 当前为 Focus Mode 时点击 → 退出 Focus，恢复宫格 |
| **Focus 图标 ◎** | 当前为 Grid 时点击 → 以**当前选中槽**的流为焦点，进入 Focus Mode |
| 高亮 | 当前 `view_mode` 对应图标高亮；另一图标默认色 |
| 持久化 | `view_mode`（`grid` \| `focus`）随 **Save** / **Save As** 写入预设；切换图标未 **Save** 时 **Preset** 标 `*` |
| disabled | **Grid** 下：未选中槽 / 选中空槽 / 宫格无已绑定流时，**Focus 图标** disabled（或整控件不可切到 Focus） |
| 单格内 | **无** Focus 按钮；仅此组合图标切换视图 |
| **Clear** / 切换 **Preset** | **Clear** 后保持当前 `view_mode`；切换 **Preset** 应用目标预设已保存的 `view_mode` |

### 预设数据模型

```json
{
  "id": "00000000-0000-4000-8000-000000000002",
  "name": "Default",
  "layout": "2×2",
  "view_mode": "grid",
  "slots": [null, null, null, null]
}
```

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "值班 2×2",
  "layout": "2×2",
  "view_mode": "focus",
  "slots": [
    "7c9e6679-7425-40de-944b-e07fc1f90ae7",
    "a1b2c3d4-e5f6-4789-a012-3456789abcde",
    null,
    null
  ]
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | UUID | 后端生成；**Default** 固定 id 或首启 seed；弹窗**不展示** |
| `name` | string | 展示名；**全库唯一**；`Default` 保留 |
| `layout` | string | `1×1` \| `2×2` \| `3×3` \| `4×4` |
| `view_mode` | string | `grid` \| `focus` |
| `slots` | array | 长度 = 对应 layout 槽位数；元素为 **stream id** 或 `null` |

| 常量 | 值 | 说明 |
|------|-----|------|
| `DEFAULT_LAYOUT_NAME` | `Default` | 内置预设名；不可 **Delete** |
| `DEFAULT_LAYOUT_ID` | UUID（实现 seed） | 内置预设 **id**；进页无 active 记录时可回退 |

存储与恢复：**GET active_layout** 得 `preset_id` → 拉取预设**已保存**内容并应用。全部预设（含 **Default**）仅经用户 **Save** 写入；**Load** 时 **PUT active_layout**。

---

### 槽位规则

| 场景 | 行为 |
|------|------|
| 点击左侧流，**无选中槽** | 填入序号最小的空槽 |
| 点击左侧流，**有选中槽** | 替换选中槽绑定；若该流已在其他槽，原槽置空 |
| 点击空槽 | 设为当前选中槽 |
| 点击已占用槽 | 设为当前选中槽（不停止播放） |
| 槽位已满且无选中槽 | 点击左侧流 → **替换最后一个槽**（序号 `N−1`）为新流；若该流已在其他槽，原槽置空；被替换槽停止播放；当前选中槽设为最后槽 |
| **Clear** | 清空当前布局全部已绑定流（`slots` 全 `null`）；**不改** `layout`；**不**自动保存；有变动时 **Preset 栏** 标 `*` |

### 单格结构

宫格内各槽以**红色 2px 框线**分隔，画面区域**紧挨**填满网格（槽间无额外间隙）；信息层（Name、状态、底栏）叠于画面上或槽内顶/底。

```
┌────────────────────────────┐  ← 红色框线 2px
│ cam-01                 ● LIVE
│ ░░░░░░░░░░░░░░░░░░░░░░░░░░ │   ← 16:9 画面贴边
│ 1920×1080 · 25fps          │
└────────────────────────────┘
```

| 元素 | 说明 |
|------|------|
| 槽位框线 | 红色、**2px**；各槽始终显示；选中时叠加高亮，框线不隐藏 |
| 状态点 `●` | `online` → 绿色 **LIVE**；`offline` → 灰色 **Offline** |
| 画面区 | RTSP 播放器；加载中显示 loading；失败显示 `⊗` |
| 底栏 | `resolution` · `fps`；缺省字段显示 `—` |
| 关闭 `×` | Grid 单格右上角；Focus Mode **主区**右上角；移除该槽绑定并停止播放 |

### 画面状态

| 状态 | 展示 |
|------|------|
| 加载中 | 画面区 loading 动画 |
| 播放中 | 正常画面 + `● LIVE`（`online` 时） |
| Offline | `⊗` 遮罩 + 文案 `Offline` |
| 拉流失败 | `⊗` 遮罩 + 文案 `Connection failed`；可自动重试（间隔实现自定） |
| 空槽 | 占位文案 `drop stream here`；无播放器实例 |

---

## Focus Mode（焦点模式）

自 **Grid** 进入：主区展示**当前焦点槽**；右侧纵向列表与当前 **layout** 槽位**一一对应**（含空槽占位），槽位数 = layout 槽位数（如 2×2 → 4 枚、3×3 → 9 枚）。**Layout** 下拉在 Focus 下**可改**规格，列表枚数随 layout 增减。

**入口在 Preset 栏右侧组合图标**：Grid 下先选中**已占用槽**，再点 **Focus ◎**；单格内无 Focus 按钮。

### 槽位序号（Grid / Focus 共用）

`slots` 为**唯一槽位序列**（下标 `0` … `N−1`）。Grid 与 Focus 共用同一序号：**行优先**，从左到右、从上到下。

| `layout` | 列数 `cols` | `N` |
|----------|-------------|-----|
| `1×1` | 1 | 1 |
| `2×2` | 2 | 4 |
| `3×3` | 3 | 9 |
| `4×4` | 4 | 16 |

Grid 排布为行优先（左→右、上→下）：`index` 对应第 `index` 格，`slots[index]` 即该格绑定。

| 视图 | 对应关系 |
|------|----------|
| **Grid** | 第 `index` 格展示 `slots[index]` |
| **Focus** 右侧列表 | 第 `index` 行展示 `slots[index]`（含空槽占位） |
| **主区** | 展示当前**焦点下标** `focus_index` 对应槽 |
| 弹窗 **Slot** 列 | 序号 `index`（只读） |

| 项 | 说明 |
|----|------|
| 变更 layout | 放大：尾部追加 `null`；缩小：截断 `slots`；序号规则不变 |
| 会话态 | `focus_index`、当前选中槽下标（**不**持久化）；`view_mode` 已写入预设 |
| 持久化 | **Save** 写 `layout` + `slots` + `view_mode` |

### Focus Mode 页面

```
+====================================================================================================+
| Preview                                                                                            |
+============+====================================================++================================+
| Groups [▦][🧹][↻]|  [ 2×2 ▼ ] [ Default              ▼ ]  [💾] [📄] [🧹]              [ ⊞ ◎ ]      |
| [Search__] |+--------------------------------------------------+| +----------------------------+ |
| v Site A   | |                                                  || | (empty)                    | |
| v Entr.    | |              MAIN: cam-01                  [×] || |  cam-02  ●                 | |
|   cam-01 * | |              ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   || |  ░░░░░░░                   | |
|   cam-02   | |                                                  || +----------------------------+ |
|   cam-06   | |  1920×1080 · 25fps                              || | (empty)                    | |
|            | +--------------------------------------------------+| +----------------------------+ |
|            |                                                  | | (empty)                    | |
|            |                                                  | +----------------------------+ |
+============+====================================================++================================+
```

| 区域 | 说明 |
|------|------|
| **Grid / Focus 图标** | Focus Mode 下 **◎** 高亮；点 **⊞** 退出 Focus |
| 主区 | 当前焦点槽；信息层同 Grid 单格（Name、状态、分辨率）；空焦点槽时显示占位 |
| 右侧列表 | 与当前 **layout** 槽位一一对应，**纵向**排列；2×2 为 **4** 行、3×3 为 **9** 行；含空槽占位 |
| 列表项 | 点击切换焦点至对应槽；已绑定显示画面，空槽显示 `drop stream here` |
| **Layout 下拉** | Focus 下**可选**（**Preset 栏**内）；如 2×2 → 3×3 时右侧由 4 行增至 9 行 |

### 进入 / 退出

```
Grid：选中已占用槽（如 cam-01）
        │
        ▼
Preset 栏点 Focus ◎（disabled：未选中槽 / 选中空槽 / 无已绑定流）
        │
        ▼
Focus Mode：主区 = 焦点槽；右侧列表 = layout 全部槽位（含空占位）；◎ 高亮
        │
        ▼
Preset 栏点 Grid ⊞
        │
        ▼
回到 Grid：Layout 与槽位绑定与进入前一致；选中槽仍为原焦点流；⊞ 高亮
```

### Focus Mode 行为

| 操作 | 说明 |
|------|------|
| 点 **Focus ◎** | 以当前选中槽为焦点，进入 Focus Mode |
| 点 **Grid ⊞** | 退出 Focus Mode，恢复 Grid |
| 点击右侧列表项 | 切换焦点槽（含空槽）；主区同步；列表高亮跟随 |
| 点击左侧树中流 | 按槽位规则写入 `slots`；若槽位已有该流则切焦点；空槽则绑定并设为焦点 |
| 主区格内 **×** | 清空该槽绑定（`null`）；焦点切到列表中下一已绑定槽；若全无绑定则主区显示空占位，**仍保持 Focus** |
| **Clear** | `slots` 全 `null`；**保持 Focus**；右侧列表保留 layout 等量空占位（如 2×2 仍 4 行） |
| 切换 **Layout** | Focus 下可用；列表行数随规格变化（如 2×2→3×3：4 行→9 行）；`slots` 保留/截断规则同 Grid |

### 列表项结构

```
+----------------------------+
|  cam-02            ● LIVE  |  ← `slots[1]`；点击切焦点
|  ░░░░░░░░░░░░░░░░░░░░░░░░  |
+----------------------------+
| (empty) drop stream here   |  ← 空槽占位
+----------------------------+
```

列表项展示 Name、状态点、缩略画面；Offline / 失败态与 Grid 单格一致（`⊗` 或灰色遮罩）。

---

## 顶栏控件

顶栏仅页面标题 `Preview`；**无**布局、齿轮或其它操作按钮。

## Preset 栏

| 元素 | 样式 | 说明 |
|------|------|------|
| **Layout** | 下拉；收起态 `2×2 ▼` 等；**Preset 左侧** | **Grid** / **Focus** 下均可选；`1×1` / `2×2` / `3×3` / `4×4` |
| **Preset** | 下拉；收起态 `{name}` 或 `{name} *` | 切换预设即 **Load**；长名 ellipsis |
| **Save** | 图标按钮；Preset 右侧第一枚 | **PATCH** 覆盖当前预设；去 `*` |
| **Save As** | 图标按钮；紧挨 **Save** | 打开 **Save As Layout** 弹窗 |
| **Clear** | 扫把图标按钮；紧挨 **Save As** | 清空全部槽位流绑定；Focus 下**保持** Focus 与列表占位 |
| **Grid / Focus** | 组合图标 `[ ⊞ ◎ ]`；行末右对齐 | 见上文 |

默认 Layout **2×2**；默认视图 **Grid**（**⊞** 高亮）。

---

## 交互流程

```
左侧点击组名        ──> 按 group id 高亮树；宫格不变
左侧点击流 Name     ──> 绑定 stream；满槽无选中时替换最后槽；未 Save 则 Preset 下拉标 *
点击空槽            ──> 设为当前选中槽
点击已占用槽        ──> 设为当前选中槽
点 Focus ◎          ──> view_mode = focus；未 Save 则 Preset 标 *
点 Grid ⊞           ──> view_mode = grid；未 Save 则 Preset 标 *
Focus 下点列表项    ──> 切换焦点槽（含空槽）
Focus 下左侧点流    ──> 按槽位规则绑定；可设焦点
Focus 下 Layout 下拉 ──> 列表行数随规格变；slots 保留/截断；标 *
Clear 图标          ──> 清空 slots（Preset 栏或 Groups 栏 🧹）；不改 layout；未 Save 则标 *；Focus 下保持 Focus 与列表占位
左侧搜索            ──> 过滤树节点 Name
Preset 下拉选预设    ──> 应用已保存 layout + slots + view_mode；PUT active_layout；去 *
Save 图标           ──> PATCH 激活预设 layout + slots + view_mode；去 *
Save As 图标        ──> Save As Layout 弹窗（新建）；POST；Load + PUT active_layout
Manage ✏️           ──> Rename Layout 弹窗（共用壳）；PATCH name
下拉 Manage          ──> Manage 弹窗
Layout 下拉切换规格  ──> 槽位保留/截断；未 Save 则标 *
左侧 Groups ▦      ──> Toggle：仅展示 slots 已绑流（与 Search 叠加）
左侧 Groups 🧹      ──> 同 Preset Clear：清空 slots；未 Save 则标 *
左侧 Groups ↻      ──> GET /groups/tree；同步组/流变更；reconcile slots；不 Test、不 Load 预设
```

---

## Toast 提示

轻量 **Toast**（自动消失）；文案为英文；`{name}`、`{n}` 等为运行时替换。无 Toast 的操作见表中「—」。

| 操作 | 文案 | 说明 |
|------|------|------|
| **Save** 成功 | **`Layout saved`** | Preset 下拉名去除 `*` |
| **Save As** 成功 | **`Layout created`** | Load 新预设并 **PUT active_layout** |
| **Manage ✏️** 重命名成功 | **`Layout renamed`** | **Manage** 表格与 **Preset** 下拉同步 |
| **Groups ↻** / **Load 预设** 后 reconcile | **`{n} stream(s) missing`** | `slots` 中 stream id 已不存在；槽位置 `null` |
| 单槽拉流失败 | **`Connection failed for {name}`** | 格内已有 `⊗` 遮罩；Toast **可选**（建议仅首次失败） |
| **Clear** / 绑槽 / Focus 切换 / ▦ 筛选 | — | 无 Toast |
| API 4xx/5xx（Save 等） | **`{message}`** | 沿用后端 `message` |

> **Confirm** 弹窗（切换 Preset 丢弃未 Save 编辑、Manage 删除预设等）**不是** Toast，见 Preset 栏。

---

## 数据依赖

本页复用 Streams 后端分组树与流字段，详见 [stream_api.md](./stream_api.md)。布局预设 API 见 [preview_api.md](./preview_api.md)。

| 数据 | 来源 | 用途 |
|------|------|------|
| 布局预设列表 | `GET .../layouts` | **Preset** 下拉、**Manage** 表格（含 **Default**） |
| 布局预设 CRUD | `POST/PATCH/DELETE .../layouts/{preset_id}` | **Save**（PATCH layout+slots+view_mode）/ **Save As**（POST）/ **Manage** ✏️（PATCH name）/ **Manage** 删除 |
| Layout shot | `PUT/GET .../layouts/{preset_id}/shot` | **Save** / **Save As** 上传；弹窗 **Shot** 图标预览 |
| **当前激活预设** | `GET/PUT .../preview/active_layout` | 进页恢复、**Load** 后写入；防服务重启丢选中 |
| 分组树 | `GET .../groups/tree` | 左侧栏；进页与 **Groups ↻**；流节点含 `enabled` / `status` / `recording` |
| Groups 宫格筛选 | 纯前端；按 `slots` 剪枝树 |
| 流详情 | 绑槽后 `GET .../{stream_id}` | `url`、`resolution`、`fps`；格内 `status` 优先用树字段，↻ 后随树更新 |
| `url` | Stream 字段 | RTSP 播放地址 |
| `status` | 树节点 / Stream | 树字色 + 格内 LIVE / Offline |
| `enabled` | 树节点 | 树字色（`false` → 灰）；**不**阻止绑槽 |

布局内容**仅**经用户 **Save** 持久化（**含 Default**）；含 `layout`、`slots`、`view_mode`。当前激活 **preset id** 经 **active_layout** 持久化。会话态（不写后端）：左侧树选中、`focus_index`、当前选中槽下标、**Groups ▦ 筛选开关**、未保存的 `layout` / `slots` / `view_mode` 编辑。

---

## 唯一标识

| 实体 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 流 | `id` | UUID | 槽位绑定 key |
| 组 | `id` | UUID | 树节点 key |
| 流 | `name` | `str` | 宫格标题、树节点展示 |
| 组 | `name` | `str` | 树节点展示 |
| 布局预设 | `id` | UUID | 后端生成；Manage 行 key |
| 布局预设 | `name` | `str` | **Preset** 下拉、**Save As** / **Manage ✏️** 弹窗；**全库唯一**；`Default` 保留 |

---

## 边界与校验（建议）

| 规则 | 说明 |
|------|------|
| 只读树 | 无组 **⋮**、无 Add / Edit / Delete |
| 同流多槽 | 同一 **stream id** 同时最多占 1 个槽；再次点击左侧流时迁移或替换 |
| Offline 流 | 允许加入宫格；格内显示 Offline，不阻断其他槽播放 |
| Disabled 流 | 允许加入宫格；拉流结果以实际连接为准 |
| Layout 缩小 | 超出新槽位数的槽位停止播放并从宫格移除 |
| Layout 放大 | 新增槽位为 empty；原槽位序号与内容（含 empty）不变 |
| 页面刷新 / 服务重启 | **GET active_layout** → 加载对应预设**已保存**内容（含 `view_mode`）；无记录时 **Default** |
| 手动保存 | **无自动保存**；变动须 **Save**（**Default** 同为 **PATCH**） |
| Groups ↻ | **不**触发 Test；同步树 `enabled` / `status` / `recording` |
| 树流字色 | `enabled=false` → 灰；`enabled=true` 且 `online` → 黑；`enabled=true` 且 `offline` → 黑 |
| 预设 Name | **Save As** / **Manage ✏️** 不可用 `Default`；**Save** 不改 name；**Default** 行 **✏️** disabled |
| Clear | 清空当前布局全部流；**不**改 `layout`；**不**写入后端，直至 **Save** |
| 未 Save 变动 | **Preset** 下拉名标 `*`；切换预设或离开页面前确认丢弃 |
| Delete 当前激活 | 回退 **Default**；**PUT active_layout** 为 **Default** |
| slots 内流已删 | Load 时该槽置空；Toast 可选提示「N stream(s) missing」 |
| 无回放 | 不提供时间轴、倍速、历史片段 |
| Focus 入口 | 仅 Preset 栏 **Grid / Focus** 组合图标；单格无 Focus 按钮 |
| Focus 前置 | 进入时须选中**已占用**槽（Grid 下）；进入后 **Clear** 全空仍保持 Focus |
| Focus 列表 | 第 `index` 行 = `slots[index]`；含空槽占位 |
| Focus 与 Layout | Focus 下 **Layout** 下拉**可用**；改规格同步增减右侧列表行数 |
| 移除焦点流 | 主区 **×** 仅清空该槽；**Clear** 清空全部槽但保持 Focus 与占位列表 |

---

## 与管理端 / 生产端关系

| 端 | 内容 |
|----|------|
| **Streams（管理端）** | 配置流、分组、启停、录制、测试 |
| **本页（生产端）** | 实时预览 RTSP；暴露 URL、用户名与密码用于播放 |
| **回放（生产端，另页）** | 不在本页实现 |
