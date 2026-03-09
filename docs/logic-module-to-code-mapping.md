# OctoClaw-ESP32 逻辑模块与代码目录映射

> 本文档说明架构文档 [OctoClaw-ESP32-Architecture_CN.md](OctoClaw-ESP32-Architecture_CN.md) §8.1 中的**逻辑模块**（0_core～7_espnow + extensions）与当前仓库**实际代码目录**的对应关系。当前工程基于 **xiaozhi-esp32** 基座，未新建 0_core、1_config 等物理目录，逻辑模块通过现有 main/ 与 main/extensions 实现。

## 1. 映射表

| 逻辑模块（架构 §8.1） | 实际代码位置 | 说明 |
|----------------------|--------------|------|
| **0_core** | `main/application.cc`、`main/device_state_machine.cc`、`main/main.cc` | 主循环、状态机、app_main 入口；事件组与 Run 循环在 Application |
| **1_config** | `main/settings.cc`、NVS 读写、`main/extensions/channels/device_pair_service.*`、各 channel 的 NVS 命名空间（octo_feishu、octo_mattermost、octo_nostr）、`extensions/octoclaw/policy/` | 设备身份、配对、策略与配置；配对在 device_pair_service，策略在 octoclaw/policy |
| **2_mcp** | `main/mcp_server.cc`、`main/protocols/protocol.cc`、`main/protocols/websocket_protocol.cc`、`main/protocols/nats_protocol.cc`、`main/protocols/mqtt_protocol.cc` | MCP 协议（initialize、tools/list、tools/call）、OctoWire 语义、传输层；设备为 **MCP Server**，上游调用设备工具 |
| **3_wifi** | 板级与基座（boards 内 WiFi/网络初始化）、Application 中网络状态回调 | 联网、重连由 xiaozhi 基座与 board 承担 |
| **4_hal** | `main/boards/**`、`main/audio/`、`main/display/`、`main/led/` | GPIO、I2C、音频、显示等由 xiaozhi 板型与 HAL 承担 |
| **5_ota** | `main/ota.cc` | OTA 升级、NVS 配置；沿用 xiaozhi OTA 流程 |
| **6_local_agent** | `main/extensions/octoclaw/runtime/agent_lite.cc`、`main/extensions/octoclaw/policy/policy_guard.cc` | 本地轻决策、决策遥测 ring、策略守卫（execute/hold/escalate/fault） |
| **7_espnow** | Kconfig `OCTO_ENABLE_ESP_NOW_AGENT` 占位；实现落点规划在 `main/extensions/octoclaw/` 下（如 espnow/） | 设备协同、任务分发/回收，当前未实现 |
| **extensions/channels** | `main/extensions/channels/` | 渠道插件：feishu、mattermost、nostr、device-pair、thread-ownership、openclaw_extension_catalog |
| **extensions/octoclaw** | `main/extensions/octoclaw/` | 设备治理：core、policy、profile、runtime、transport（回执补偿队列） |

## 2. 说明

- **逻辑划分与物理目录**：架构中的 0_core、1_config 等为**逻辑模块**，用于需求与设计对照；实现时沿用 xiaozhi-esp32 的 `main/` 与 `main/extensions/` 结构，不新增同名目录。
- **扩展增量**：智能体逻辑与渠道整合的增量集中在 `main/extensions/octoclaw/` 与 `main/extensions/channels/`，不改变 xiaozhi 主循环、协议、OTA、板型的主干行为。
- **7_espnow / 任务编排 / 审计**：V2 已落点：`runtime/task_step.*`、`espnow/espnow_agent.*`；存储裁剪由 `ReceiptQueue`、`AgentLiteRuntime` ring 与 NVS 承担。V3 审计已落点：`main/extensions/octoclaw/audit/audit_log.*`，mcp_server 在回执前写入；OTA 增强/纳管/可观测落点为 `ota.cc`、协议层与 MCP/HTTP 扩展。

## 3. 架构进阶 30A 四轴与 policy_guard 收口

- **30A 四轴智能路由**（见 `partme-docs/6、OctoClaw/2、OctoClaw-FullEcosystem-Architect_CN.md` §30A）：ESP32 侧由 **PolicyGuard**（`extensions/octoclaw/policy/policy_guard.*`）承担轴 B 路由判断（execute/hold/escalate/fault），与 POLICY 快照、CapabilityManifest 协同；复杂四轴与 SONA/梦境等上移协作，设备仅回执决策与 reason_code。
- **收口**：策略行为以 `policy_guard.cc` 与 `policy_store` 为准；如需 ADR 可在此目录新增 `docs/adr-30a-esp32-policy.md` 记录设备侧与上游分工。

## 4. Doctor / Self-Check 输出物

- **device-manifest**：能力与配置摘要，可由 `CapabilityManifest`、`BoardProfile`、策略快照导出；落点 MCP 工具（如 `self.system.device_manifest`）或串口 CLI 命令。
- **self-check-report**：自检报告（agent_lite 统计、AuditLog 条数、ReceiptQueue 统计、传输就绪等），落点 MCP 工具（如 `self.system.self_check`）或串口输出；实现时可聚合 `AgentLiteRuntime::ExportJson`、`AuditLog::ExportJson`、`ReceiptQueue::ExportStatsJson` 等。

## 5. 参考

- 架构文档：`docs/OctoClaw-ESP32-Architecture_CN.md` §8.1 模块分区与职责
- 扩展说明：`main/extensions/README.md`
- 版本规划：`partme-docs/6、OctoClaw/OctoClaw-ESP32/` 下 V1/V2/V3 需求与 PRD
