# probes

`SourceLivenessProbe` 挂在 `queue_demuxN` 的 `src` 上，记录这一路最后一帧的时间。`nvurisrcbin` 的 `src` 要等管道跑起来才存在；`nvstreammux` 的 `sink_%u` 是输入 pad，`addProbe` 不接受。`PipelineRunner::status()` 用它判断该路是 `running`、`reconnecting` 还是 `dead`。

## 写入

`handleBuffer` 每次只做一次 relaxed 原子写：把当前时间（纳秒）写入 `SourceLivenessSlot::lastFrameNs`，然后返回 `Probe_Ok`。不读 buffer，不写 `exceeded`。

表在 `fork` 之前用 `mmap(MAP_SHARED|MAP_ANONYMOUS)` 建好，父子进程看同一块内存。probe 在子进程里写，父进程 `status()` 读。probe 不加锁。

| 字段 | 谁写 | 含义 |
| --- | --- | --- |
| `lastFrameNs` | probe | 最近一帧的时间，0 表示还没有帧 |
| `exceeded` | 尚未接入 | 非 0 表示该路重连次数已用完 |
| `count` | `attachSourceProbes` | 实际挂上的路数 |
| `reconnectIntervalSec` | `attachSourceProbes` | 判定「没帧」的秒数 |

## 挂载

`PipelineRunner::attachSourceProbes()` 在 `start()` 之前调用。只挂 `uri` 以 `rtsp://` 开头的源，元件名是该源序号对应的 `queue_demuxN`，pad 是 `src`。`file://` 以及 `http://`、`rtmp://`、`udp://`、`rtsps://` 不挂：前者播完就停帧，后者没有 `nvurisrcbin` 的重连。

`source_id` 是挂载顺序，不是元件的 `source-id` 属性。一路一个 slot，最多 `SLOT_COUNT`（256）。

没帧的判定秒数取第一条源的 `rtsp-reconnect-interval`；该属性为 0 或未设置时用 `STALE_SEC`（10）。

## status

| 条件 | 该路 |
| --- | --- |
| `exceeded != 0` | `dead` |
| 还没有帧，或距上一帧超过间隔 | `reconnecting` |
| 间隔内有帧 | `running` |

任一已挂载的路不是 `running`，管道为 `degraded`。子进程已退出则为 `stopped`，`streams` 为 null。

全部 `rtsp://` 源重连次数用完也不会让 `pipeline.start().wait()` 返回，子进程继续跑，`status` 仍是 `degraded`（各路为 `dead`），不会变成 `stopped`。`nvurisrcbin` 只为每一路发 `gst_nvmessage_reconnect_attempt_exceeded`，不会发出 Service Maker 用来退出等待的那条错误。

`dead` 目前不会出现：还没有总线回调把 `gst_nvmessage_reconnect_attempt_exceeded` 写进 `exceeded`。次数为 `-1` 时元件也不会发出这条消息。生成的 RTSP 源次数是 18，间隔 10 秒，约 3 分钟后用完。
