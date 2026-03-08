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

OpenClaw 扩展翻译矩阵见：`docs/openclaw-extension-translation-matrix.md`（自动生成，当前 37 个扩展骨架）。

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
