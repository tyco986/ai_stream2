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
说明: 按类别在滑窗里的出现比例编码事件码，写入 `NvDsFrameMeta.frame_user_meta_list`。`eventCodes` 最多 8 位，每位对应 `class-event-map` 里的一个类别，顺序与映射一致；未用到的位为 `0`。仅 `bInferDone` 时重算，否则沿用该路上一帧结果。

入参:

- `class-event-map`：`类别id:事件名`，分号分隔，最多 8 项。例 `0:smoke;1:fire`。顺序即码位下标，事件名只在该位为 `1` 时写入 `eventNames`。
- `length`：统计窗口帧数，默认 `10`。窗口未满不按比例判定。
- `threshold`：窗口内出现比例阈值，默认 `0.5`，范围 `0` 到 `1`。
- `mode`：`fold`（默认）窗口满后整窗清空再累计；`slide` 每满一帧丢掉最旧的一帧。

编码（按类别、按当前帧是否检出）:

- 窗口未满：本帧检出为 `2`，未检出为 `0`。
- 窗口已满且比例 ≥ `threshold`：本帧检出为 `1`，未检出为 `2`。
- 窗口已满且比例 < `threshold`：本帧检出为 `2`，未检出为 `0`。

`0` 缺席，`1` 告警，`2` 过渡。

## capturer

类型: 插件
插件名称: rawcapturer, viscapturer
说明: 事件码命中时落盘到 `/root/output`。rawcapturer 存 `images/raw_<pad>_<id>.png`；viscapturer 存 `vis/vis_<pad>_<id>.jpg`，并落盘 YOLO 标签 `labels/raw_<pad>_<id>.txt` 与 labelme 标签 `labelme/raw_<pad>_<id>.json`

## dslogger

类型: 插件
插件名称: detlogger, presencelogger, stgcnpplogger
说明: 按路写日志，文件为 `/root/logs/deepstream/<pad>/<page>.jsonl`，一行一条 JSON

公共字段: `{"ts","pad","source","frame","latency","num":{"all","<class_id>"},"object":[...]}`

detlogger: `object` 每项为 `[left,top,right,bottom,conf,class_id,label,object_id]`

presencelogger: 在公共字段上增加 `"eventcode"`（无事件时为 null），`object` 格式同 detlogger

stgcnpplogger: `object` 每项为 `{"det":[left,top,right,bottom,conf,class_id,label,object_id],"action":[conf,class_id,label,ready,length]}`

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
