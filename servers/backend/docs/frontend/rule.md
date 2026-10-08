# 前端页面编写规则

管理端前端（Vue 3）页面实现约定。UI 规格见各页 `*.md`；HTTP 契约见对应 `*_api.md`。本文约束**代码形态**，不重复业务交互细节。

壳层见 [shell.md](./shell.md) / [shell_api.md](./shell_api.md)。

---

## 1. 文档与目录

### 文档驱动

| 规则 | 说明 |
|------|------|
| 先文档后代码 | 有 UI 规格（`docs/<page>.md`）与 API（`docs/<page>_api.md`）才实现页面 |
| 字段对齐 | 请求/响应字段名、枚举值以 `*_api.md` 为准；禁止另造别名 |
| 偏离即改文档 | 实现发现契约不足时**先改 API/UI 文档**，再改代码 |

### 一页一目录

```
src/
  pages/<page>/           # 如 streams、preview、shell
    Page.vue              # 页面入口
    components/           # 仅本页组件
    composables/          # 仅本页组合逻辑
    i18n/                 # 本页文案（en / zh）
  shared/
    http/                 # 鉴权、外壳 unwrap、错误映射
    ui/                   # Design System Adapter（见 §5）
    i18n/                 # 全局文案、locale 切换
  api/                    # OpenAPI 生成物（见 §2）；禁止手改
```

| 规则 | 说明 |
|------|------|
| 跨页复用 | 必须进入 `shared/`，并写明归属；禁止 A 页 import B 页内部文件 |
| 壳与业务 | Shell 在 `pages/shell/`；业务页只渲染于内容区，不复制导航 |

---

## 2. API 模块（OpenAPI 生成）

### 原则

- **传输与类型由 OpenAPI 生成**；业务页**禁止**手写 URL、手写 DTO、页面内直接 `fetch` / `axios`。
- 源真相顺序：`*_api.md` → **OpenAPI 规格**（如 `openapi.yaml` / 后端导出）→ **生成客户端** → 页面调用。
- 推荐工具：**orval** 或 **openapi-typescript + openapi-fetch**（二选一，全仓统一）。

### 生成约定

| 项 | 约定 |
|----|------|
| 输出目录 | `src/api/`（或 `src/api/generated/`）；目录内文件视为生成物 |
| 禁止手改 | 生成文件顶部保留 `@generated` 标记；改契约只改 OpenAPI 后重新生成 |
| 分组 | 按 tag / 路径前缀分文件（如 `streams.ts`、`preview.ts`、`shell.ts`），对应各 `*_api.md` |
| 基址 | 与文档一致：`/ai_stream2/backend/<resource>` |
| 外壳 | 生成层或 `shared/http` 统一处理 `{ success, message, data }`；页面只消费 `data`（及明确的错误） |

### 页面用法

```
# ✅ 调用生成客户端
import { getStreams, createStream } from '@/api/streams'

# ❌ 页面内拼 URL / 裸 fetch
fetch('/ai_stream2/backend/streams/...')
```

| 规则 | 说明 |
|------|------|
| 一页只用本域 API | Streams 页只调 streams 生成模块；跨域数据走对应 API（如 Preview 调 stream 的 tree） |
| 无字典管理器 | 不用 `apis = { list: '...' }` 手维护路径；路径只存在于 OpenAPI |
| 新增接口 | 先写 `*_api.md` → 更新 OpenAPI → 生成 → 页面引用 |

### 与 Python 后端

后端实现须与 `*_api.md` / OpenAPI 一致；可用同一份 OpenAPI 校验或 codegen。前端**不**根据后端 Django 模型猜字段。

---

## 3. i18n（文案，非业务字段字典）

### 原则

- 使用 **vue-i18n**；一键切换 **English / Chinese**（locale：`en` / `zh`）。
- UI 文案（按钮、列头、Toast、空态、弹窗标题）全部走 `t('...')`；**禁止**模板/脚本硬编码中英文句子。
- **业务数据不翻译**（流 Name、模型名、username、后端返回的 name 等）原样展示。

### 键与枚举

| 规则 | 说明 |
|------|------|
| Key 语言 | 稳定英文 key（如 `streams.toolbar.add`）；禁止中文当 key |
| 按页拆分 | `pages/<page>/i18n/en.ts` + `zh.ts`；全局键在 `shared/i18n` |
| API 枚举 | 传输值保持英文（`online`、`sidebar`）；展示用 i18n：`status.online` → Online / 在线 |
| 插值 | 用 i18n 插值，禁止字符串拼接多语言句子 |

locale 偏好可进 Shell Page Settings（与 Mode 并列）；持久化方式与 [shell_api.md](./shell_api.md) 扩展字段对齐（未扩展前可先本地会话）。

---

## 4. 数据契约（禁止业务解析）

### 目标

API 返回形状使前端**可直接绑定**；前端不做业务语义解析或派生。

### 允许 vs 禁止

| 允许（薄适配） | 禁止（业务解析） |
|----------------|------------------|
| 解外壳取 `data` | 正则/拆分字符串得到业务字段 |
| 生成类型校验 / Zod 校验形状 | 前端拼装本应由后端提供的派生字段 |
| ISO 时间 → 展示格式化 | 前端推断 status、槽位数、权限 |
| 控件双向绑定 | 维护与 API 不一致的第二套「规范模型」 |

| 规则 | 说明 |
|------|------|
| 缺字段 | 改后端 / OpenAPI，**不**在前端凑数或静默默认掩盖 |
| 默认值 | 以 `*_api.md` 常量表为准（如 `DEFAULT_MODE`）；页面不发明默认 |
| 校验失败 | 视为 API 违约：日志 + Toast；禁止吞掉后继续「猜」 |

「不做解析」= **不做业务语义变换**；unwrap、类型校验、展示格式化除外。

---

## 5. 骨架与 UI 适配层

### 两层分工

| 层 | 职责 | 目录 |
|----|------|------|
| **骨架** | 布局与信息架构：侧栏/顶栏、导航、内容区、路由、本页结构 | `pages/<page>/`（含 `pages/shell/`） |
| **Adapter** | 可换的控件实现：Button / Select / Dialog / Icon / Toast 等 | `shared/ui/` |

换 EP ↔ Ant：**只改 Adapter + 入口接线**（`main.ts` 注册与 CSS、`vite` 按需解析、`package.json` 依赖）；**不改骨架**组件结构与业务逻辑。

### 骨架规则（完全独立于具体 UI 库）

| 规则 | 说明 |
|------|------|
| 禁止直连 UI 库 | 骨架与业务页**禁止** `import` `element-plus` / `ant-design-vue` / `@element-plus/icons-vue`，禁止模板写 `el-*` / `a-*` |
| 允许 | 布局 DOM、本页 CSS、`vue-router` / `vue-i18n` / Pinia、以及 `@/shared/ui/*` |
| 图标 | 页面只写 `<UiIcon name="setting" />` 等；具体图标组件只存在于 Adapter |
| Toast | 只调 `toast.*`；禁止 `ElMessage` / Ant `message` |
| 主题色 | 壳/页样式优先用 **CSS 变量**（Design Token）；禁止大面积写死某一库主色（如到处 `#409eff`）以致换库还要改骨架 CSS |

「骨架独立」= **无具体 UI 库依赖**；不是「没有任何界面」。界面结构在骨架，控件皮肤在 Adapter。

### Adapter 约定

```
shared/ui/
  Button.vue
  Table.vue
  Dialog.vue
  Select.vue
  Icon.vue        # name → 当前库图标（默认 EP Icons）
  toast.ts        # success / error / info / warning
  ...
pages/streams/Page.vue   # 只使用 @/shared/ui/*
```

| 规则 | 说明 |
|------|------|
| 默认实现 | 当前默认 Adapter 后端为 Element Plus |
| 换库范围 | `shared/ui/*` + `main.ts` + `vite.config` 解析器 + 依赖；**不含** `pages/**` |
| 目标 | 交互与信息架构一致；不要求两套库像素级一致 |
| 配置项 | 组件库偏好与 Shell **Mode**（侧栏/顶栏）分离；均可进 Page Settings |

### 例外白名单

下列可直连专用库，**不**纳入换肤 Adapter：

| 例外 | 说明 |
|------|------|
| vue-konva / konva | Analyzer 画布（见 [pipeline.md](./pipeline.md)） |
| 视频播放器 | Preview / Recordings 播放相关 |

例外组件仍通过本页 `components/` 封装，避免业务页散落底层 API。

---

## 6. 状态、错误、确认与三态

### 状态分层

| 层 | 内容 | 持久化 |
|----|------|--------|
| 服务端状态 | 列表、详情、配置 | 经生成 API；可用请求缓存（如 TanStack Query） |
| UI 会话态 | 勾选、弹窗开关、未 Save 编辑、Focus 下标等 | 默认**不**写后端（与各页文档一致） |

禁止把会话态塞进 API 响应模型。

### 错误与写操作

| 规则 | 说明 |
|------|------|
| 错误上浮 | `success: false` / HTTP 4xx/5xx 在 `shared/http` 转为错误；页面展示 `message` |
| 写操作反馈 | 增删改启停等须 Toast（文案走 i18n）+ 明确刷新策略（用响应更新或再 GET，与该页文档一致） |
| 破坏性操作 | 二次确认（Remove、Delete、Restart 等） |

### 列表页三态

每个列表/主数据区必备：**Loading** / **Empty** / **Error**；禁止空白无说明。

### 路由与导航

当前路由为 Shell 导航高亮的**唯一数据源**；禁止另维护「当前页」全局变量与路由双写。

---

## 7. 工程与质量

| 规则 | 说明 |
|------|------|
| **PROJECT_NAME** | 仓库根 [`project.env`](../project.env) 为唯一真相源；HTTP 前缀、Docker 名前缀、前端 `VITE_PROJECT_NAME` / storage key 均派生自此，禁止业务代码写死旧名 |
| 类型 | 页面使用生成 DTO；禁止 `any` 绕过契约（确需时局部 + 注释原因） |
| 改 API 流程 | 文档 → OpenAPI → 生成 → 改页；禁止只改前端类型 |
| 共享逻辑 | 进 `shared/`；禁止页面内堆积「万能 utils」 |
| 用户偏好 | 语言、布局 Mode、组件库等进 Shell 配置区；分期实现时在文档标明 |
| 可访问性 | 弹窗焦点可关可逃；图标按钮须有可读名称（aria / tooltip，文案走 i18n） |
| 性能 | 大表按需虚拟滚动；播放器与重型页懒加载路由 |

---

## 8. 检查清单（开 PR 前）

- [ ] 对应 `*.md` / `*_api.md` 已更新且与实现一致
- [ ] 新接口已进 OpenAPI 并完成生成；无手写 URL
- [ ] 业务页与骨架无直接 EP/Ant import（含图标）；控件经 `shared/ui`；文案无硬编码中英文
- [ ] 无业务向「解析/拼装」；缺字段已回推后端
- [ ] 列表具备 Loading / Empty / Error；破坏性操作有确认
- [ ] 例外（konva/播放器）仅出现在白名单封装内

---

## 9. 相关文档

| 文档 | 用途 |
|------|------|
| [shell.md](./shell.md) | 壳布局 Mode、导航、页面配置 |
| [shell_api.md](./shell_api.md) | 当前用户、页面 settings |
| 各页 `*.md` / `*_api.md` | 业务 UI 与 HTTP 契约 |
