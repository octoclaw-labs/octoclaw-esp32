# OctoClaw-ESP32 运维与开关手册

本文档说明基于 `xiaozhi-esp32` 基座改造后的 OctoClaw 设备侧能力开关、策略快照格式和错误码。

## 0. 扩展目录分层

为保持与 `xiaozhi-esp32` 主体目录兼容，OctoClaw 增量能力统一放在：

- `main/extensions/channels`：OpenClaw 外部扩展翻译层（全量扩展骨架）
- `main/extensions/octoclaw/core`：公共类型定义
- `main/extensions/octoclaw/profile`：板型档位与特性矩阵
- `main/extensions/octoclaw/policy`：策略存储、白名单、风险判定
- `main/extensions/octoclaw/runtime`：本地 agent-lite 运行时
- `main/extensions/octoclaw/transport`：回执补偿队列与传输态适配

OpenClaw 扩展翻译矩阵见：`docs/openclaw-extension-translation-matrix.md`（自动生成，当前 37 个扩展，含 5 个已实现）。

当前已落地可运行扩展：

- `device-pair`：配对码生成、待审批缓存、审批回传。
- `thread-ownership`：Slack 线程归属校验、@提及旁路、forwarder claim、NVS 配置持久化。
- `nostr`：channel_event 收件缓存、allowlist 过滤、send_dm 上行桥接、NVS 配置持久化。
- `mattermost`：channel_event 收件缓存、allowlist 过滤、send_message 上行桥接、NVS 配置持久化。
- `feishu`：channel_event 收件缓存、allowlist 过滤、send_message 上行桥接、NVS 配置持久化。

Channel 实施边界（ESP32）：

- 仅实现可通过 WebSocket 对接上游服务的 channel。
- 当前纳入范围：`feishu`、`mattermost`、`nostr`（以翻译矩阵和 manifest 为准）。
  - 当前已实现：`nostr`、`mattermost`、`feishu`（三通道独立插件，见 `main/extensions/channels/<channel>/`）。

## 1. 板型档位与默认能力

- 全板型矩阵见：`docs/octoclaw-board-tier-matrix.md`（自动生成，覆盖 131 个 `BOARD_TYPE_*`）。
- 档位规则：
  - Lite：ESP32 / ESP32-C3 / ESP32-C5 / ESP32-C6
  - Standard：ESP32-S3
  - Pro：ESP32-P4
- 默认特性：
  - Lite：`policy_guard`、`receipt_compensation`、`mcp_ext_fields`
  - Standard：Lite + `local_agent`
  - Pro：Standard + `esp_now_agent`

## 2. Kconfig 开关

在 `menuconfig -> Xiaozhi Assistant -> OctoClaw Extension`：

- `OCTO_ENABLE_POLICY_GUARD`
- `OCTO_ENABLE_RECEIPT_COMPENSATION`
- `OCTO_ENABLE_LOCAL_AGENT`
- `OCTO_ENABLE_MCP_EXT_FIELDS`
- `OCTO_ENABLE_ESP_NOW_AGENT`
- `OCTO_RULE_MAX`
- `OCTO_TELEMETRY_RING_SIZE`

## 3. 策略快照（Policy Snapshot）

设备通过 NVS 命名空间 `octo_policy` 持久化策略：

- `policy_version`：策略版本（int，>0）
- `risk_threshold`：风险阈值（1-3）
- `emergency_stop`：紧急停机（bool）
- `tool_whitelist`：工具白名单（string，逗号分隔，支持 `*` 和前缀 `xxx*`）
- `feature_mask`：特性覆盖（int，`-1` 表示不覆盖，使用板型/Kconfig 默认）

更新入口（用户工具）：

- `self.system.update_octoclaw_policy`
  - 参数：`policy`（JSON 字符串）
  - 示例：

```json
{
  "policy_version": 5,
  "risk_threshold": 2,
  "emergency_stop": false,
  "tool_whitelist": "self.get_device_status,self.audio_speaker.set_volume,self.screen.*",
  "feature_mask": -1
}
```

## 4. 诊断入口

- `self.system.get_octoclaw_profile`
  - 返回：板型档位、feature mask、policy、capability manifest、回执队列统计、agent-lite 遥测。
- `self.system.list_translated_extensions`
  - 返回：已翻译的 OpenClaw 扩展目录清单（id、类型、来源、配置键统计）。
- `self.pair.generate_setup_code`
  - 根据 `nats.url` / `websocket.url` 生成移动端可用 setup code（支持传入 `public_url` 覆盖）。
- `self.pair.list_pending_requests`
  - 返回当前缓存的待审批配对请求。
- `self.pair.approve_request`
  - 按 `request_id`（默认 `latest`）审批配对，并回传上游 `pairing approve` 事件。
- `self.thread_ownership.get_state`
  - 查看线程归属配置（forwarder、A/B channels、agent identity）和运行态统计。
- `self.thread_ownership.set_config`
  - 更新线程归属配置并持久化到 NVS（命名空间 `octo_thread`）。
- `self.thread_ownership.record_message_received`
  - 记录 `message_received` 事件用于 @ 提及短期旁路（默认 5 分钟）。
- `self.thread_ownership.check_before_send`
  - 发送前执行 ownership claim；若返回 `cancel=true` 则表示线程已被其他 agent 占有。
- `self.nostr.get_state`
  - 返回 nostr 配置、运行统计和设备侧缓存 inbox。
- `self.nostr.set_config`
  - 设置并持久化 nostr 配置（`enabled/account_id/allow_pubkeys`）。
- `self.nostr.clear_inbox`
  - 清空设备侧 nostr inbox 缓存。
- `self.nostr.send_dm`
  - 发送 `channel_command` 到上游 bridge，请求发送 nostr DM。
- `self.mattermost.get_state` / `self.mattermost.set_config` / `self.mattermost.clear_inbox` / `self.mattermost.send_message`
  - Mattermost 通道：配置与 inbox 持久化在 NVS `octo_mattermost`，上行命令 `channel_command(channel=mattermost, command=send_message)`。
- `self.feishu.get_state` / `self.feishu.set_config` / `self.feishu.clear_inbox` / `self.feishu.send_message`
  - 飞书通道：配置与 inbox 持久化在 NVS `octo_feishu`，上行命令 `channel_command(channel=feishu, command=send_message)`。

## 5. 回执错误与决策码

`policy_guard` 会返回以下决策（可选扩展字段 `meta.decision`）：

- `execute`：允许执行
- `hold`：挂起（例如 `transport_not_ready`、`emergency_stop`）
- `escalate`：需上移/审批（例如 `risk_threshold_exceeded`）
- `fault`：本地拒绝（例如 `tool_not_whitelisted`）

常见 `reasonCode`：

- `policy_guard_disabled`
- `transport_not_ready`
- `emergency_stop`
- `tool_not_whitelisted`
- `risk_threshold_exceeded`
- `tool_execution_exception`

## 6. 补偿队列说明

- 当链路不可用且开启 `OCTO_ENABLE_RECEIPT_COMPENSATION` 时，回执进入本地内存队列。
- 链路恢复后自动补发（`OnTransportReady` / 后续请求触发刷新）。
- 统计落在 NVS 命名空间 `octo_runtime`：
  - `rq_total_q`
  - `rq_total_f`
  - `rq_total_d`
