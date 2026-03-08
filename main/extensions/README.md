# main/extensions 模块说明

`main/extensions` 专门用于承载 **ESP32 可运行的扩展实现**，以便对齐 `research/openclaw/extensions` 的生态能力。

## 目录约定

- `main/extensions/channels`：外部渠道接入扩展（如 douyin/meituan/rednode/kuaishou/amap 的 ESP32 适配）
- `main/extensions/octoclaw`：设备侧治理扩展（板型、策略、运行时、传输补偿）

其中 `main/extensions/channels` 已支持从 `research/openclaw/extensions` 自动翻译全量骨架（当前 37 项）。

## 渠道扩展落地原则

1. 先复用 `main/protocols`（websocket/mqtt/nats）作为统一传输层；
2. 渠道差异收敛到 `ChannelExtension` 实现，不侵入 `Application` 主状态机；
3. 所有扩展能力必须经过设备策略链（白名单、风险阈值、紧急停机）；
4. 资源受限板型默认只启用必要扩展，高配板型按 feature mask 打开更多能力。

## 从 openclaw 扩展迁移到 ESP32 的建议步骤

1. 抽取渠道扩展的最小消息模型（事件、指令、回执）；
2. 适配为 `ChannelExtension` 的初始化、启动、停止接口；
3. 将重依赖能力上移（复杂解析、重多媒体处理）；
4. 在设备侧保留轻量执行与故障回执闭环。
