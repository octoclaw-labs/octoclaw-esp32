# 三通道闭环验收样例

本目录提供 **Feishu / Mattermost / Nostr** 三通道闭环验收用的可复制样例 JSON，配合 `../acceptance_three_channel.sh` 使用。

## 文件说明

| 文件 | 用途 |
|------|------|
| `channel_event_inject.json` | 下行 `channel_event` 注入样例（上游 → 设备），用于验证收件缓存与 allowlist 过滤 |
| `mcp_tools_call_examples.json` | MCP `tools/call` 样例（设备 → 上游 `channel_command`），用于验证 send_message / send_dm 上行桥接 |

## 使用方式

1. **仅翻译 + 构建验收**（需本机安装 ESP-IDF 且 `idf.py` 在 PATH 中）：
   ```bash
   cd octoclaw-esp32
   ./scripts/acceptance_three_channel.sh
   ```

2. **闭环验收**（需设备与上游桥接已连接）：
   - 向下行主题发送 `channel_event_inject.json` 中对应 channel 的一条消息（type/channel/event/payload 保持一致）。
   - 在设备或后台通过 MCP 发送 `mcp_tools_call_examples.json` 中的任一 `tools/call` 请求，确认上游收到 `channel_command` 并完成发送。

协议细节见仓库根目录 `docs/nats.md`、`docs/octoclaw-agent-ops-guide.md`。
