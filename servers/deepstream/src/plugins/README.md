# plugins

## boxsnapshot

类型: 插件
插件名称: boxsnapshot
说明: 对 nvinfer 产出的 box 与 seg 做快照，写入 frame.user_meta，供下游 *drawerwithtracker 读取

## fadedrawer

类型: 插件
插件名称: detfadedrawer, detfadedrawerwithtracker, segfadedrawer, segfadedrawerwithtracker, posefadedrawer, posefadedrawerwithtracker, stgcnppfadedrawerwithtracker
说明: 按检测框、实例分割、姿态或 ST-GCN++ 动作标签做淡出绘制；带 tracker 的元件读取 boxsnapshot 快照

## presenceeventcoder

类型: 插件
插件名称: presenceeventcoder
说明: 按类别在滑窗里的出现比例编码事件码，写入 `NvDsFrameMeta.frame_user_meta_list`。槽位 0 到 7，`slotCount` 为已用槽的最大下标加 1，中间空槽码为 `0`。仅 `bInferDone` 时重算，否则沿用该路上一帧结果。

入参:

- `event-slot-map`：`槽位:类别id:事件名`，分号分隔。例 `0:0:Fire;1:1:Smoke`。事件名写入该槽 `name`；名为空时，用该类别第一次出现的 `obj_label`。
- `length`：统计窗口帧数，默认 `10`。窗口未满不按比例判定。
- `threshold`：窗口内出现比例阈值，默认 `0.5`，范围 `0` 到 `1`。
- `mode`：`fold`（默认）窗口满后整窗清空再累计；`slide` 每满一帧丢掉最旧的一帧。

编码（每位独立，看本帧是否检出该类别）:

- 本帧未检出：`0`。
- 本帧检出，且窗口已满、出现比例 ≥ `threshold`：`1`。
- 其余本帧检出（窗口未满，或已满但比例不够）：`2`。

`0` 缺席，`1` 告警，`2` 过渡。

## capturer

类型: 插件
插件名称: rawcapturer, viscapturer
说明: 事件码命中时落盘到 `/root/output`。rawcapturer 存 `images/raw_<pad>_<id>.png`；viscapturer 存 `vis/vis_<pad>_<id>.jpg`，并落盘 YOLO 标签 `labels/raw_<pad>_<id>.txt` 与 labelme 标签 `labelme/raw_<pad>_<id>.json`

## dscollector

类型: 插件
插件名称: detcollector, presencecollector, stgcnppcollector
说明: 把当前帧收成一行 JSON，写入 `frame_user_meta_list`，供 dslogger 落盘、nvmsgconv 发送。无入参。测不到延迟时 `latency` 为 null。`object_id` 未跟踪时为 `-1`。

公共字段: `{"ts","pad","source","frame","latency","num":{"all","<class_id>"},"object":[...]}`

detcollector: `object` 每项为 `[left,top,right,bottom,conf,class_id,label,object_id]`

presencecollector: 在 `num` 与 `object` 之间增加 `event`。没有 presence 元数据时为 null。有则为：

- `event_code`：各槽码拼成的字符串，含 `0`、`1`、`2`。
- `event_label`：码为 `1` 的槽名，逗号分隔。
- `debouncer`：`max_length`、`ratio_threshold`、`current_length`（每槽已累计帧数）、`counts`（每槽 `[0 的次数, 1 的次数, 2 的次数]`）、`ratio`（窗口未满的槽为 null）。

`object` 格式同 detcollector。

stgcnppcollector: `object` 每项为 `{"det":[left,top,right,bottom,conf,class_id,label,object_id],"action":[conf,class_id,label,ready,length]}`。`action` 来自 stgcnppactionlabel；没有动作元数据时 `conf` 为 `0`、`class_id` 为 `-1`、`label` 为空、`ready` 为 false。`length` 来自预处理的 ready meta，没有则为 `0`。

## dslogger

类型: 插件
插件名称: dslogger
说明: 读 dscollector 写入的那一行 JSON，按路追加到 `/root/logs/deepstream/<pipeline>/<pad>/<page>.jsonl`。`<pad>` 为三位，如 `000`。单文件 10MB，保留 5 个。没有采集行时不写。

入参:

- `interval`：每路每隔多少帧写一行，默认 `1`。
- `page`：文件名主干，默认 `det`。常见值为 `det`、`presence`、`stgcnpp`。

## nvmsgconv

类型: nvmsgconv payload lib
msg2p-lib:

- `/opt/ai_stream2/servers/deepstream/libs/libnvds_msgconv_det.so`
- `/opt/ai_stream2/servers/deepstream/libs/libnvds_msgconv_presence.so`
- `/opt/ai_stream2/servers/deepstream/libs/libnvds_msgconv_stgcnpp.so`

说明: 读 dscollector 写入的那一行 JSON，原样作为消息载荷。采集行为空则不发。`det` 与 `stgcnpp` 有采集行就发。`presence` 还要求任一槽码不是 `0`，即出现 `1` 或 `2` 才发；全是 `0` 不发。

## stgcnppactionlabel

类型: 插件
插件名称: stgcnppactionlabel
说明: 读取 ST-GCN++ 分类结果，格式化后写入每个目标的 `obj_user_meta`（`label`、`conf`、`classId`、`ready`）。`ready` 来自预处理挂上的 ready meta。

入参: `classifier-unique-id`，要读取的 ST-GCN++ SGIE `unique-id`，默认 `4`。

label:

- 未 ready：记 `NA`，`conf` 为 `0`，`classId` 为 `-1`。
- ready 但该 SGIE 没有有效分类名：记 `Unknown`，`conf` 为 `0`，`classId` 为 `-1`。
- ready 且有分类名：沿用分类器的 `label`、`conf`、`classId`。

## rtmposepostprocess

类型: 插件
插件名称: rtmposepostprocess
说明: 读取 RTMPose 的 `keypoints` 张量，按与预处理相同的扩展裁剪和 letterbox 映回检测框，写入对象 `mask_params`（宽 3、高为关节数，每点 x、y、score，相对框归一化）

入参:

- `sgie-unique-id`：姿态 SGIE 的 `unique-id`，默认 `2`。
- `infer-width`、`infer-height`、`padding`：与 rtmpose 预处理一致，默认 `192`、`256`、`1.25`。

## sahipostprocesspose

类型: 插件
插件名称: nvsahipostprocesspose
说明: 对 SAHI 姿态实例先按框重叠抑制，再按 OKS 抑制。整帧框在 `prefer-full-frame` 开启时只当锚点，不参与后续抑制

## nvdspreprocess/rtmposepreprocess

类型: nvdspreprocess custom lib
custom-lib-path: /opt/ai_stream2/servers/deepstream/libs/libnvds_rtmpose_preprocess.so
custom-input-transformation-function: CustomTransformation
custom-tensor-preparation-function: CustomTensorPreparation
说明: 按 padding 扩展目标裁剪并 letterbox 到 192x256；张量按 user-configs 的 pixel-normalization-factor 与 offsets 做成 NCHW RGB

## nvdspreprocess/stgcnpppreprocess

类型: nvdspreprocess custom lib
custom-lib-path: /opt/ai_stream2/servers/deepstream/libs/libnvds_stgcnpp_preprocess.so
custom-tensor-preparation-function: CustomTensorPreparation
说明: 把 RTMPose 关键点序列收成 ST-GCN++ 输入张量 (N, 2, 100, 17, 3)，放在 RTMPose SGIE 之后

## gie/yolopose

类型: nvinfer custom parser
custom-lib-path: /opt/ai_stream2/servers/deepstream/libs/libnvinfer_yolo_pose.so
parse-bbox-instance-mask-func-name: NvInferYoloPose
说明: 解析 YOLO-Pose 输出为检测框与关键点
