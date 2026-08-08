# Streams 页面

管理端 Streams 页面，用于配置 RTSP 视频流、分组、启停、测试与录像。生产端预览/回放使用独立页面，暴露 URL、用户名与密码。

**组**与**流**的唯一标识均为 **UUID**（RFC 4122，如 `550e8400-e29b-41d4-a716-446655440000`），由后端生成、不可修改。树与表格默认展示 **Name**；筛选、勾选、API 请求均以 **id** 为准。

## 主页面

```
+===============================================================================================================+
| Streams                                                                                                       |
+============+==================================================================================================+
| Groups [↻] | [ Add ] [ Remove ] [ Enable ] [ Disable ] [ Test ]              [ Search: __________ ]            |
| v All  :   | Filter: cam-01 . 1 stream              Target: 0 selected                                        |
|   cam-04   |+--+------+-------+-------------+-----------+---+--------+-----------+-------------+              |
| v Site A   | |[]|Name  |Group  |URL          |Resolution |FPS|Status  |Last Test  |Operations   |             |
| v Entr. :  |+--+------+-------+-------------+-----------+---+--------+-----------+-------------+              |
|   cam-01   | |[]|cam-01|Site/Ent|rtsp://a@1/s1|1920x1080 |25 |Online  |06-28 10:15|▶🎥↻✏📓🗑|                    |
|   cam-02   | |[]|cam-02|Site/Ent|rtsp://b@1/s2|1280x720  |15 |Online  |06-28 09:40|▶🎥↻✏📓🗑|                    |
| > Garage:  | |[]|cam-04|All    |rtsp://c@2/s1|1920x1080 |30 |Offline |06-27 18:22|■🎥↻✏📓🗑|                     |
|            |+--+------+-------+-------------+-----------+---+--------+-----------+-------------+              |
|            |                                      < 1  2  3 >                                                 |
+============+==================================================================================================+
```

| 区域 | 说明 |
|------|------|
| **右侧搜索框** | 位于右侧工具栏右侧；过滤左侧树与/或右侧表格（实现时固定范围） |
| 左侧栏 | 分组树：节点以 **group id** / **stream id** 标识；展开组时显示**直属流的 Name**；组节点有 **⋮** |
| 右侧栏 | 工具栏 + 搜索框 + 流表格（含 Checkbox 与行内 Operations） |
| 页面标题 | 固定显示 `Streams` |

**布局补充说明**

| 项 | 说明 |
|----|------|
| 工具栏按钮样式 | Add / Enable / Disable / Test 为蓝底白字；Remove 为红底白字 |
| 工具栏禁用 | 未勾选行时，Remove / Enable / Disable / Test 为 `disabled`；Remove 禁用时为浅红底白字 |
| Filter / Target | Filter 为当前筛选摘要；Target 为已勾选行数；位于工具栏与表格之间 |
| 点击左侧流 | 如点 `cam-01`，按 **stream id** 筛选；右侧表格仅显示该一条记录，Filter 为 `cam-01 . 1 stream` |
| 点击左侧组 | 按 **group id** 筛选；右侧显示该组**直属流**及**所有子组**内的流（递归包含） |
| **Groups ↻** | 重新 `GET /groups/tree` + 刷新当前表格；见 [左侧栏](#groups-标题行) |

> **无「未分组」节点**；未归属具体组的流等价于 **`All`** 分组。

---

## 左侧栏（Groups）

### 树结构

| 节点类型 | 展开时 | Operations |
|----------|--------|------------|
| **组** | 显示子组 + **直属该组的流（Name + 状态字段）** | **⋮**；节点 key 为 **group id** |
| **流** | — | 无（操作在右侧表格）；节点 key 为 **stream id**；含 `enabled`、`status`、`recording`（见 [stream_api.md](./stream_api.md) **TreeNode**） |

组**折叠**时不显示其下流 Name；**展开**后仅列出**直属该组**的流 **Name** 为子节点（只读，无 **⋮**）。子组内的流显示在对应子组展开后，不在父组节点下展开列出。

子组与流可并存于同一展开组下，例如：

```
v Site A
v Entrance :
    cam-01
    cam-02
  v East Gate :
      cam-06
```

### Groups 标题行

`Groups` 文案右侧 **↻**（Refresh）；位于树列表**之上**（搜索框在右侧栏，不在此栏）。

```
+------------+
| Groups [↻] |
| v All  :   |
|   cam-04   |
+------------+
```

#### Refresh（↻）

| 项 | 说明 |
|----|------|
| API | **`GET /groups/tree`**；**不**调用 Test |
| 表格 | 用当前 `group_id` / `stream_id` / `search` / 分页 **再 GET 列表**，同步 Status / Enabled / Recording |
| **`last_test_at`** | **不**更新（仅 Test 写入） |
| 树 | 流节点 `enabled` / `status` / `recording` 与库一致；保留展开 / 折叠 |
| 请求中 | 图标 **disabled** 或 loading |
| Tooltip | 建议 `Refresh` |

### 流节点样式（Groups 树）

本页与 [preview.md](./preview.md) **相同**；[recordings.md](./recordings.md) 用 **`recording`** 规则（见该文档）。

| 条件 | 流 Name 字色 |
|------|----------------|
| `enabled=true` 且 `status=online` | **黑色**（默认正文色） |
| `enabled=false` | **灰色** |
| `enabled=true` 且 `status=offline` | **黑色** |

> 灰色仅表示 **Disabled**；**Offline 且仍 Enabled** 的流保持黑色。表格 **Status** 列只展示 **Online** / **Offline**（由 API `status` 决定，与 `enabled` 无关）。

### Operations（⋮）

**仅组节点**显示 **⋮**（三个上下居中的点）：

| 菜单项 | 行为 |
|--------|------|
| **Add Stream** | 打开 **Transfer List 弹窗**，向当前组分配流 |
| **Add Group** | 在**当前组下**新增一条可编辑子组（见下文行内编辑） |
| **Rename** | 当前组 Name 进入行内编辑（√ / ×） |
| **Delete** | **二次确认**后删除组（**级联删除**所有子组）；组内（含子组内）所有流的 Group 置为 **`All`** |

从 **`All`** 节点的 ⋮ 选择 **Add Group** 时，在**顶层**新增分组。

### 行内 Add Group / Rename

**Add Group** 或 **Rename** 时，树中出现可编辑行：

```
  Garage :
  [ new group name____ ]  v  x
```

| 控件 | 行为 |
|------|------|
| **√** | 确认：保存组名并生成 **group id**（UUID）；√ × 消失，该行变为普通组节点（右侧恢复 **⋮**） |
| **×** | 取消：**Add Group** 时删除该草稿行；**Rename** 时恢复原组名 |

### 行为

| 操作 | 说明 |
|------|------|
| 点击**组名** | **单选**（**group id**）；右侧表格显示该组**直属流** + **所有子组**内的流（递归包含） |
| 点击**流 Name**（树内） | **单选**（**stream id**）；右侧表格**仅展示该一条流**（如点 `cam-01` 只显示对应记录） |
| 展开 / 折叠组 | 展开后显示该组**直属**流的 Name 列表 |
| **`All`** | 展开可显示 Group 为 `All` 的流 Name |
| **⋮** | 仅组节点；打开组操作菜单 |

### 左侧栏示意

```
+------------+
| Groups [↻] |
| v All  :   |
|   cam-04   |
| v Site A   |
| v Entr. :  |
|   cam-01   |
|   cam-02   |
| > Garage:  |
| [ name... ] v x |
+------------+
```

行末 `v x` 为 **Add Group** 草稿态；确认后变为普通组节点并恢复 **:**。搜索框位于右侧栏工具栏右侧，不在左侧栏内。

---

## Add Stream（Transfer List 弹窗）

由组 **⋮ → Add Stream** 打开。用于将已有流分配到当前组。

```
+----------------------------------------------------------------+
| Add Stream to Group: Entrance                                [x] |
+----------------------------------------------------------------+
| Available                      | -> | Target Group             |
| +------+--------+--------+     | <- | +--------+--------+      |
| | [ ]  | Name   | Group  |     |    | | Name   | Group  |      |
| +------+--------+--------+     |    | +--------+--------+      |
| | [ ]  | cam-04 | All    |     |    | | cam-01 | Entrance |    |
| | [x]  | cam-05 | Site A |     |    | | cam-02 | Entrance |    |
| +------+--------+--------+     |    | +--------+--------+      |
+----------------------------------------------------------------+
|                           [ Save ]    [ Cancel ]               |
+----------------------------------------------------------------+
```

| 区域 | 列 | 说明 |
|------|-----|------|
| **左侧（候选）** | Checkbox、Name、Group | 可勾选；按 **stream id** 穿梭；`→` 移入右侧 |
| **右侧（已选）** | Name、Group | 表格上方标题 **Target Group**；按 **stream id** 关联目标 **group id**；`←` 移回左侧 |
| **中间** | `→` `←` | 穿梭按钮 |

保存后：右侧列表中的流归入目标 **group id**；原在该组但移出右侧列表的流 **group id** 改为 **`All`**（`ALL_GROUP_ID`）；左侧树在展开该组时显示对应流 **Name**。

---

## 删除组确认

**⋮ → Delete** 时弹出：

```
+--------------------------------+
| Confirm Delete Group       [x] |
+--------------------------------+
| Delete group "Entrance"?       |
| Streams in this group will     |
| move to "All".                 |
+--------------------------------+
|        [ Delete ]  [ Cancel ]  |
+--------------------------------+
```

确认后：按 **group id** 删除该组（**级联删除**所有子组）；其下（含子组内）所有流的 **Group** 设为 **`All`**。

---

## 右侧工具栏

| 元素 | 样式 | 可用条件 | 行为 |
|------|------|----------|------|
| **Add** | 蓝底白字 | 始终 | 打开 Stream Add/Edit 弹窗（新建） |
| **Remove** | 红底白字 | ≥1 行勾选 | **二次确认**后删除勾选流 |
| **Enable** | 蓝底白字 | ≥1 行勾选 | enable 勾选中当前 disabled 的流 |
| **Disable** | 蓝底白字 | ≥1 行勾选 | disable 勾选中当前 enabled 的流 |
| **Test** | 蓝底白字 | ≥1 行勾选 | 测试勾选流；更新 Last Test、Status、Resolution、FPS |
| **Search** | 输入框 | 始终 | 位于工具栏右侧；过滤左侧树与/或右侧表格 |

未勾选时 **Remove / Enable / Disable / Test** 为 `disabled`。

---

## 数据表格

行主键为 **stream id**（UUID，默认不展示）。Checkbox 勾选、批量操作、行内 Operations 均按 **stream id** 关联。

### 列定义

| 列名 | 类型 | 说明 |
|------|------|------|
| Checkbox | 多选框 | 批量 Remove / Enable / Disable / Test；绑定 **stream id** |
| Name | `str` | 展示名；非空；**全库唯一** |
| Group | `str` | 所属组 **name**（展示）；关联 **group id**；默认 **`All`** |
| URL | `str` | 完整展示，含用户名与密码 |
| Resolution | `str` | 只读；格式 `WxH`（如 `1920x1080`） |
| FPS | `int` | 只读 |
| Status | `str` | 只读；仅展示 **`Online`** / **`Offline`**（来自 API `status`；与 `enabled` 无关） |
| Last Test | `datetime` | 最近 Test 成功时间 |
| Operations | 图标组 | 与 Stream 弹窗 **Enabled / Recording** 状态同步 |

---

## 流 Operations 列

图标顺序固定；**🗑 在最右侧**。

| 顺序 | 图标 | 名称 | 状态与交互 |
|:----:|:----:|------|------------|
| 1 | ▶ / ■ | Enable / Disable | **合一图标**；**Enabled**：绿色 ▶；**Disabled**：红色 ■ |
| 2 | 🎥 | Recording | **On**：绿色 🎥；**Off**：灰色 🎥 |
| 3 | ↻ | Test | 带箭头的圆圈；测试中旋转 |
| 4 | ✏ | Edit | **复用**与工具栏 **Add** 相同的 Stream Add/Edit 弹窗；**Name 可编辑** |
| 5 | 📓 | Log | 只读日志弹窗 |
| 6 | 🗑 | Remove | **二次确认**后删除 |

---

## 删除流确认

```
+--------------------------------+
| Confirm Delete             [x] |
+--------------------------------+
| Delete 2 stream(s)?            |
| This cannot be undone.         |
+--------------------------------+
|        [ Delete ]  [ Cancel ]  |
+--------------------------------+
```

---

## Stream Add / Edit 弹窗

工具栏 **Add** 与 Operations **✏ Edit** **复用同一弹窗**；Add 与 Edit 字段布局一致。弹窗**不展示** id（UUID）。URL 内嵌账号密码。

```
+------------------------------------------+
| Add Stream / Edit Stream             [x] |
+------------------------------------------+
| Name          [ cam-01                   ] |
| Group         [ All                    ▼] |
| URL           [ rtsp://user:pass@...   ] |
| Enabled       (●) On    ( ) Off          |
| Recording     ( ) On    (●) Off          |
| [ Test                                   ] |
+------------------------------------------+
|              [ Save ]    [ Cancel ]      |
+------------------------------------------+
```

弹窗内 **Test** 按钮**全宽**铺满内容区；成功时展示 Resolution（`WxH`）、FPS；失败时红色提示；**不阻断 Save**。

| 字段 | Add | Edit |
|------|-----|------|
| Name | 可编辑 | 可编辑（**不禁止修改**） |
| Group | 默认 **`All`** | 可编辑 |
| URL | 可编辑 | 可编辑 |
| Enabled / Recording | 可编辑 | 可编辑 |

**Group 下拉**：列出已有分组（含 **`All`**）。新建分组通过左侧 **⋮ → Add Group**。

---

## Log 弹窗

点击 📓 打开，只读日志文本 + **Close**。

---

## 交互流程

```
左侧点击组名      ──> 按 group id 筛选；右侧展示该组直属流 + 所有子组内的流（递归）
左侧点击流 Name   ──> 按 stream id 筛选；右侧仅展示该一条流（Filter 显示流 Name，条数为 1）

组 ⋮ → Add Stream    ──> Transfer List ──> Save（树展开后可见流 Name）
组 ⋮ → Add Group     ──> 行内可编辑 + √ ×
组 ⋮ → Rename        ──> 行内可编辑 + √ ×
组 ⋮ → Delete        ──> 确认 ──> 组级联删除（含子组），流归 All

右侧 Add             ──> Stream Add/Edit 弹窗（新建）
流 Operations        ──> 启停 / 录制 / Test / Edit / Log / Remove
Operations ✏ Edit    ──> 复用同一 Stream Add/Edit 弹窗（Name 可改）
工具栏               ──> 批量 Remove / Enable / Disable / Test（需勾选）
```

---

## Toast 提示

轻量 **Toast**（自动消失）；文案为英文；`{name}`、`{n}` 等为运行时替换。无 Toast 的操作见表中「—」。

| 操作 | 文案 | 说明 |
|------|------|------|
| Stream 弹窗 **Save**（新建） | **`Stream created`** | 表格新增一行 |
| Stream 弹窗 **Save**（编辑） | **`Stream saved`** | Operations **✏** 或工具栏 **Add** 编辑路径 |
| 行内 **▶ / ■** 启停 | **`Stream enabled`** / **`Stream disabled`** | 单条 **PATCH** |
| 行内 **🎥** 录像开关 | **`Recording enabled`** / **`Recording disabled`** | 单条 **PATCH** |
| 工具栏 **Enable** / **Disable** | **`{n} stream(s) enabled`** / **`{n} stream(s) disabled`** | 批量；`{n}` 为实际更新条数 |
| 行内 / 工具栏 **Test** 完成 | **`Test OK: …`** / **`Test failed: …`** / **`Test finished: {ok} ok, {fail} failed`** | 成功更新 Last Test；失败 Toast 错误原因 |
| 行内 **✏** | **`Edit stream`** | 打开编辑弹窗 |
| 行内 **📓** | **`Log opened`** | 打开日志弹窗 |
| 行内 **🗑** / 工具栏 **Remove** 打开确认 | **`Confirm delete`** | Confirm 弹窗本身不是 Toast；打开时提示 |
| 行内 **🗑** / 工具栏 **Remove** 确认后 | **`Stream deleted`** / **`{n} stream(s) deleted`** | 单条 **DELETE** 或批量 **batch/delete** |
| Transfer List **Save** | **`Group members updated`** | **⋮ → Add Stream** |
| **Add Group** √ 确认 | **`Group created`** | **⋮ → Add Group** |
| **Rename** √ 确认 | **`Group renamed`** | **⋮ → Rename** |
| **Delete** 组确认后 | **`Group deleted`** | 流归入 **All** |
| **Groups ↻ Refresh** | — | 同步树与表格 |
| 弹窗 / 表单校验失败 | — | 错误展示在弹窗内，不 Toast |
| API 4xx/5xx | **`{message}`** | 沿用后端 `message` |

> **Confirm** 弹窗（删除组 / 删除流 / Upload 覆盖确认等）**不是** Toast。

---

## 按钮样式规范

| 元素 | 样式 |
|------|------|
| Add / Save / Enable / Disable / Test | 蓝底白字 |
| Remove / Delete | 红底白字 |
| Cancel | 白底蓝字 |
| Operations ▶ / ■ | 绿 / 红 |
| Operations 🎥 | 绿 / 灰 |
| Operations ↻ | 蓝（行内 Test） |

---

## 与后端约定（概要）

| 操作 | 说明 |
|------|------|
| 列表 | `GET /streams`（`GET /`）；query `group_id` / `stream_id` / `search` / 分页 |
| 新建流 | `POST /streams`（`POST /`） |
| 更新流 | `PATCH /streams/{stream_id}`（启停 / 录制 / 弹窗 Save） |
| 删除流 | 工具栏 `POST /streams/batch/delete`；行内 `DELETE /streams/{stream_id}` |
| 批量启停 / Test | `POST /streams/batch/enable` \| `disable` \| `test` |
| 组树 / 映射 | `GET /groups/tree`、`GET /groups/map` |
| 新建子组 | `POST /groups/{parent_group_id}` |
| 重命名 / 删组 | `PATCH` / `DELETE /groups/{group_id}` |
| Transfer List | `GET .../candidates`、`GET .../members`、`PUT .../members`（body `stream_ids`） |

路径参数 **`stream_id`** / **`group_id`** 与响应字段 **`id`** 同值（UUID）。

具体路径与字段见 [stream_api.md](./stream_api.md)。

---

## 唯一标识

| 实体 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 流 | `id` | UUID | 后端生成；不可改；API 与前端行 key |
| 组 | `id` | UUID | Add Group 时生成；不可改；树节点 key |
| 流 | `name` | `str` | 展示名；表格列 **Name**；**全库唯一** |
| 组 | `name` | `str` | 展示名；树节点显示；**全库唯一** |

---

## 边界与校验（建议）

| 规则 | 说明 |
|------|------|
| id（流） | UUID；后端生成；不可改 |
| id（组） | UUID；Add Group 时生成；不可改 |
| Name（流） | 非空；可编辑；**全库唯一**（不可与其他流重名） |
| Name（组） | 非空；Add Group / Rename 经 √ 确认；**全库唯一**（不可与其他组重名） |
| Group（流） | 默认 **`All`**；无「未分组」；关联组 **id** |
| 删除组 | 二次确认；按 **group id** 删除（**级联删除**子组）；流全部归入 **`All`** |
| 删除流 | 二次确认；按 **stream id** 删除 |
| 弹窗 Test | 不阻断 Save |
| 左侧点组 | 按 **group id** 筛选；右侧表格显示该组直属流及所有子组内的流（递归） |
| 左侧点流 | 按 **stream id** 筛选；右侧表格只显示对应一条记录 |
| Groups ↻ | **不**触发 Test；同步树 `enabled` / `status` / `recording` 与当前表格 |

---

## 与管理端 / 生产端关系

| 端 | 内容 |
|----|------|
| **本页（管理端）** | 配置流、分组、启停、录制、测试；表格完整暴露 URL、用户名与密码 |
| **生产端** | 独立预览/回放；暴露 URL、用户名与密码 |
