# Models 页面

管理端 Models 页面，用于注册、编译与管理推理模型（`.pt` 源文件 → TensorRT engine）。

每条模型的唯一标识为 **UUID**（RFC 4122，如 `550e8400-e29b-41d4-a716-446655440000`），字段名 **`id`**（下文称 **model id**），由后端生成、不可修改。表格默认展示 **Name**；勾选、批量删除、行内 Operations、API 请求均以 **id** 为准。

## 主页面

```
+==========================================================================================================================+
| Models                                                                                                                   |
+==========================================================================================================================+
| [ Add ] [ Remove ] [ Refresh ]                                    [ Search: ________ ] |
| Target: 0 selected                                                                                                       |
+--+--------+---------+----------+--------+-----------+------+-----------+---------+---------------+----------+------------+---------------------+
|[]| Name   | Version | BatchSize| Type   | Precision | Task | Num Class | Classes | Source File   | Status   | Last Build | Operations          |
+--+--------+---------+----------+--------+-----------+------+-----------+---------+---------------+----------+------------+---------------------+
|[]| yolo11n| v1.0    | 1        | Static | FP16      | det  | 80        | View     | /models/y11.pt| Built    | 06-28 10:15| ⬆ 🔨 📓            |
|[]| yolo11s| —       | 4        | Dynamic| FP16      | —    | —         | —        | /models/y11s.pt| Building | —          | ⬆ 🔨 📓           |
|[]| custom | —       | 1        | Static | FP16      | —    | —         | —       | —             | Not Built| —          | ⬆ 🔨 📓            |
+--+--------+---------+----------+--------+-----------+------+-----------+---------+---------------+----------+------------+---------------------+
|                                              < 1  2  3 >                                                                 |
+==========================================================================================================================+
```

| 区域 | 说明 |
|------|------|
| 页面标题 | 固定显示 `Models` |
| 工具栏 | **Add** / **Remove** / **Refresh**（左）；**Search**（右对齐）；位于表格上方 |
| Target | 已勾选行数摘要；位于工具栏与表格之间 |
| 数据表格 | 全宽；含 Checkbox 与行内 Operations |

**布局补充说明**

| 项 | 说明 |
|----|------|
| 工具栏按钮样式 | **Add**、**Refresh** 为蓝底白字 / 绿底白字（见下表）；**Remove** 为红底白字 |
| 工具栏禁用 | 未勾选行时 **Remove** 为 `disabled`；禁用时为**浅红底白字** |
| Refresh | 始终可用；重新拉取表格数据（Status、Last Build、Version、Task 等只读列） |
| **Search** | 位于工具栏**右对齐**；按 **Name** 或 **Source File** 子串过滤；输入变更时 `GET /models?search=…`（页码重置为 1） |
| 分页 | 表格底部分页；默认每页条数与 Streams 页一致 |

---

## 右侧工具栏

| 元素 | 样式 | 可用条件 | 行为 |
|------|------|----------|------|
| **Add** | 蓝底白字 | 始终 | 打开 **Add Model** 弹窗（新建） |
| **Remove** | 红底白字 | ≥1 行勾选 | **二次确认**后删除勾选模型及其关联 engine 文件（若存在） |
| **Refresh** | 绿底白字 | 始终 | 刷新表格；不丢失当前勾选状态（实现时按 **model id** 恢复勾选） |
| **Search** | 文本输入 | 始终 | 右对齐；按 Name / Source File 过滤表格；见「布局补充说明」 |

未勾选时 **Remove** 为 `disabled`（浅红底白字）。

---

## 数据表格

行主键为 **model id**（UUID，默认不展示）。Checkbox 勾选、批量 Remove、行内 Operations 均按 **model id** 关联。

### 列定义

| 列名 | 类型 | 说明 |
|------|------|------|
| Checkbox | 多选框 | 批量 Remove；绑定 **model id** |
| Name | `str` | 展示名；非空；**全库唯一**；对应字段 `name`，与 **model id** 独立 |
| Version | `str` | 只读；编译成功后由后端写入（如 `v1.0`）；未编译时为 `—` |
| BatchSize | `int` | 推理 batch；创建时设定；**>0 且 ≤128** |
| Type | `enum` | `Static` / `Dynamic` |
| Precision | `str` | 只读；当前阶段固定 **`FP16`** |
| Task | `str` | 只读；由编译或元数据解析（如 `det` / `seg` / `cls`）；未知为 `—` |
| Num Class | `int` | 只读；类别数；未知为 `—` |
| Classes | link | 只读；有类别数据时为**链接**（文案 `View`）；无数据为 `—` 且不可点击；点击打开 **Classes 弹窗** |
| Source File | `str` | `.pt` 路径或已上传文件名；未上传为 `—` |
| Status | `str` | 只读；`Built` / `Not Built` / `Building` 三态 |
| Last Build | `datetime` | 只读；最近一次 Build **任务结束**时间；格式同 Streams **Last Test**（如 `06-28 10:15`）；从未 Build 时为 `—` |
| Operations | 图标组 | Upload / Build / Log（见下文） |

---

## Operations 列

图标顺序固定：**Upload → Build → Log**。

| 顺序 | 图标 | 名称 | 状态与交互 |
|:----:|:----:|------|------------|
| 1 | ⬆ | Upload | 上传或替换 `.pt` 源文件；无文件时灰色，有文件时默认色；**Source File** 为空时选文件后直接上传；**已有源文件**时选文件后**二次确认**再覆盖；覆盖后 **Status** 回退为 `Not Built`，Version / Task / Num Class / Classes 清空 |
| 2 | 🔨 | Build | 触发编译（pt → engine）；受理后 **Status** 为 `Building`，图标旋转 / loading；`Building` 时 **Build** 不可重复点击；任务结束：成功 → `Built`；失败 → 恢复为 `Not Built`（从未成功过）或 `Built`（曾成功过）；并更新 **Last Build**、Version、Task、Num Class、Classes |
| 3 | 📓 | Log | 打开该模型编译/运行日志面板（抽屉或弹窗） |

行内 **Upload / Build / Log** 均按当前行 **model id** 操作，无需先勾选 Checkbox。

---

## Classes 弹窗

点击表格 **Classes** 列链接打开。只读展示该模型全部类别；弹窗**不展示** model id。

```
+------------------------------------------------+
| Classes: yolo11n                           [x] |
+------------------------------------------------+
| Class ID | Class Name                        |
+----------+-----------------------------------+
| 0        | person                            |
| 1        | bicycle                           |
| 2        | car                               |
| ...      | ...                               |
+------------------------------------------------+
|                                    [ Close ]   |
+------------------------------------------------+
```

| 元素 | 说明 |
|------|------|
| 标题 | `Classes: {Name}`；**Name** 为当前行模型名 |
| 表格 | 两列：**Class ID**、**Class Name**；按 Class ID 升序 |
| **Close** / **×** | 关闭弹窗 |

**Classes** 列为 `—` 时无链接，弹窗不可用。

---

## Upload 确认

**Source File** 已有值（非 `—`）时，Operations **⬆ Upload** 选中新 `.pt` 后弹出：

```
+--------------------------------+
| Confirm Replace Source     [x] |
+--------------------------------+
| Replace source file for        |
| "yolo11n"?                     |
| Status will reset to           |
| Not Built.                     |
+--------------------------------+
|        [ Replace ]  [ Cancel ] |
+--------------------------------+
```

| 按钮 | 行为 |
|------|------|
| **Replace** | 上传并覆盖原 `.pt`；**Status** → `Not Built`；Version / Task / Num Class / Classes 清空 |
| **Cancel** / **×** | 取消，不上传 |

**Source File** 为 `—` 时无确认框，选文件后直接上传。

---

## Add Model（弹窗）

由工具栏 **Add** 打开。保存后在表格追加一行（后端生成 **model id**，UUID）。弹窗**不展示** id。

```
+------------------------------------------------------------------+
| Add Model                                                    [x] |
+------------------------------------------------------------------+
| File        [ __________________________ ]  [ Import ]           |
| Name        [ __________________________ ]  [ ] Use file name    |
| Type        [ Static              v ]                            |
| BatchSize   [ 1                     ]                            |
| Precision   [ FP16                  ]   (read-only)              |
+------------------------------------------------------------------+
|                              [ Save ]    [ Cancel ]              |
+------------------------------------------------------------------+
```

### 字段

| 字段 | 控件 | 校验与行为 |
|------|------|------------|
| **File** | 文本展示 + **Import** | **Import** 打开文件浏览器，筛选 `*.pt`；选中后展示文件名；Save 时随 `POST /` 以 **`source_file`（UploadFile）** 提交 |
| **Name** | 文本输入 | 非空；**全库唯一**（不可与其他模型 **name** 重名）；Save 时 trim；与 **model id** 无关 |
| **Use file name** | Name 右侧 Checkbox | 勾选为 `true` 时，Name **自动同步**为 File 的 **basename**（不含扩展名）；用户**手动修改 Name** 后自动取消勾选；改 File 时若仍为勾选状态则再次同步 |
| **Type** | 下拉 | `Static` / `Dynamic`；默认 `Static` |
| **BatchSize** | 数字输入 | 整数；**>0** 且 **≤128**；默认 `1` |
| **Precision** | 下拉（禁用） | 写死 **`FP16`**；不可编辑；仅展示 |

### 按钮

| 按钮 | 行为 |
|------|------|
| **Save** | 校验通过后创建模型记录；表格新增一行；Version / Task / Num Class / Classes 初始为 `—`；**Status** 为 `Not Built`；**Last Build** 为 `—`；若已选 File 则写入 Source File |
| **Cancel** | 关闭弹窗，不保存 |
| **×** | 同 Cancel |

Save 失败（重名、BatchSize 非法、`source_file` 非 `.pt` 等）时在弹窗内展示错误，不关闭弹窗。

---

## Remove 确认

工具栏 **Remove**（≥1 行勾选）时弹出：

```
+--------------------------------+
| Confirm Delete Model       [x] |
+--------------------------------+
| Delete 2 selected model(s)?    |
| This cannot be undone.         |
+--------------------------------+
|        [ Delete ]  [ Cancel ]  |
+--------------------------------+
```

确认后：按 **model id** 删除记录及关联上传文件 / engine（若策略允许）。

---

## Toast 提示

轻量 **Toast**（自动消失）；文案为英文；`{name}`、`{n}` 等为运行时替换。无 Toast 的操作见表中「—」。

| 操作 | 文案 | 说明 |
|------|------|------|
| **Add Model** 弹窗 **Save** 成功 | **`Model created`** | 表格新增一行 |
| Operations **⬆ Upload** 成功 | **`File uploaded`** | 含首次上传与覆盖（覆盖前经 **Confirm**） |
| Operations **🔨 Build** 受理 | **`Build started`** | **Status** → `Building`；轮询至结束 |
| Build 任务成功 | **`Build completed`** | **Status** → `Built` |
| Build 任务失败 | **`Build failed`** | **Status** 恢复为 `Not Built` 或 `Built` |
| 工具栏 **Remove** 确认后 | **`{n} model(s) deleted`** | `{n}` 为删除条数 |
| 工具栏 **Refresh** | — | 按 **model id** 恢复勾选 |
| Operations **📓 Log** | — | 只读弹窗 |
| Add 弹窗 / 表单校验失败 | — | 错误展示在弹窗内，不 Toast |
| API 409（重名、`status=building` 等） | **`{message}`** | 沿用后端 `message` |

> **Confirm** 弹窗（**Remove**、Upload 覆盖已有源文件等）**不是** Toast。

---

## 状态与只读列来源

| 列 | 何时有值 |
|----|----------|
| Status | 创建时为 `Not Built`；Build 受理后为 `Building`；成功为 `Built`；失败恢复为 `Not Built` 或 `Built`（见 Operations **Build**）；**Upload** 覆盖已有源文件时回退为 `Not Built` |
| Last Build | 每次 Build **任务结束**（成功或失败）时写入结束时间；进行中不更新 |
| Version | **Build** 成功后由后端写入 |
| Task / Num Class / Classes | **Build** 成功或解析 `.pt` 元数据后填充；**Classes** 有数据后列显示为可点击链接 |
| Source File | **Add** 时指定 File，或行内 **Upload** 后更新 |
| Precision | 创建时固定 `FP16`；表格只读展示 |

未编译或未上传源文件时，除 **Status**（`Not Built`）外，对应只读列显示 `—`。

---

## 与后端约定（概要）

| 操作 | 说明 |
|------|------|
| 列表 | `GET /models`（相对前缀 `GET /`）；**Search**、**Refresh** 与初次加载共用 `search` 参数 |
| 创建 | `POST /models`（`POST /`）；`multipart/form-data`；含 name、type、batch_size、precision（固定 fp16）、可选 **source_file**；响应 `data.id` 即 **model_id** |
| 删除 | `POST /models/batch/delete` 批量（body `{ "ids": [...] }`）；UI **Remove** 仅走此路径。`DELETE /models/{model_id}` 为 API 保留，当前 UI 无单条删除 |
| 上传 | `POST /models/{model_id}/upload`；multipart 字段 **`file`**（`.pt`） |
| 编译 | `POST /models/{model_id}/build`；异步；轮询 `GET /models/{model_id}/build/status`（实现可选 WebSocket 推送）更新 **Status**、**Last Build**、Version 等列 |
| 日志 | `GET /models/{model_id}/logs`；**Log** 图标打开 |

路径参数 **`model_id`** 与响应字段 **`id`** 同值（UUID）。

具体路径与字段名见 [model_api.md](./model_api.md)。

---

## 唯一标识

| 实体 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 模型 | `id` | UUID | 后端生成；不可改；API 与前端行 key |
| 模型 | `name` | `str` | 展示名；表格列 **Name**；**全库唯一** |

---

## 边界与校验（建议）

| 规则 | 说明 |
|------|------|
| id（模型） | UUID；后端生成；不可改 |
| Name（模型） | 非空；**全库唯一**（不可与其他模型 **name** 重名）；Add 弹窗可编辑 |
| 删除模型 | 二次确认；工具栏 **Remove** 批量删除（`POST /batch/delete`） |
| Refresh | 按 **model id** 恢复勾选状态 |
| 弹窗 | Add Model **不展示** id；Save 成功后表格行 key 为响应 **id** |
