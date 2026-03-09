# synology-chat（ESP32 翻译状态）

- 源扩展：`research/openclaw/extensions/synology-chat`
- 适配类型：`channel`
- 当前状态：`translated-skeleton`

## 能力摘要

- channels: `synology-chat`
- providers: `-`
- commands: `-`
- skills: `-`
- kind: `-`
- 配置键数量: `0`
- WebSocket 直连可行: `no`
- WebSocket 证据: `-`

## 设备侧待实现

- [ ] 补齐设备侧配置映射（NVS/menuconfig）
- [ ] 补齐 MCP 工具与渠道事件的转换层
- [ ] 补齐故障回执与遥测字段映射
- [ ] 当前不纳入 ESP32 实现范围（仅实现可 WebSocket 直连的 channel）
