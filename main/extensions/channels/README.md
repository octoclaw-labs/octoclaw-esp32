# channels 扩展目录

该目录用于承接 `research/openclaw/extensions` 的渠道能力，目标是将服务器侧扩展翻译为 ESP32 可运行的轻量实现。

## 当前基础设施

- `channel_extension.h`：渠道扩展统一接口
- `channel_registry.h/.cc`：渠道扩展注册中心
- `openclaw_extension_catalog.h/.cc`：翻译扩展目录导出（供诊断工具返回）
- `openclaw_extension_catalog.generated.h`：自动生成的 37 个扩展清单
- `openclaw_extensions_manifest.json`：翻译清单 JSON
- `translation_status_overrides.json`：状态覆盖（支持 `status/notes/websocket_capable/websocket_evidence`）
- 已落地可运行扩展：
  - `device-pair`（setup code、待审批缓存、审批回传）
  - `thread-ownership`（Slack ownership claim、@提及旁路、NVS 配置）
  - `nostr`（channel_event 收件缓存、allowlist 过滤、send_dm 上行桥接）

## Channel 落地范围（ESP32）

- 仅实现 **可通过 WebSocket 连接上游服务** 的 channel。
- 自动检测结果写入每个扩展的 `README.md` 与 `extension_manifest.json`：
  - `esp32.websocketCapable=true`：可纳入设备侧实现。
  - `esp32.websocketCapable=false`：默认不纳入（资源/协议不匹配）。
- 如需人工纠偏，可在 `translation_status_overrides.json` 为扩展补充：
  - `websocket_capable: true/false`
  - `websocket_evidence: ["..."]`

## 建议迁移清单

可按下列优先级逐步迁移：

1. `mattermost`（企业 IM，线程/消息模型清晰）
2. `feishu`（生态复杂度较高，放第二阶段）

每个渠道建议建立独立子目录（例如 `main/extensions/channels/douyin`），并提供以下最小实现：

- 配置加载（NVS 或 OTA snapshot）
- 上下行消息转换（渠道协议 <-> MCP/设备事件）
- 故障与回执（保持统一错误码和追踪字段）

## 自动生成

执行以下命令可重建全量翻译骨架：

```bash
python3 scripts/translate_openclaw_extensions.py
```
