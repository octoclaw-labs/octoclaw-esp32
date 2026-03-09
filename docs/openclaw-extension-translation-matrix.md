# OpenClaw Extensions → ESP32 翻译矩阵（自动生成）

> 由 `scripts/translate_openclaw_extensions.py` 生成，请勿手工修改。

- 扩展总数：**37**
- 目标目录：`main/extensions/channels/<extension-id>`

## 类型统计

| 类型 | 数量 |
|---|---:|
| `channel` | 21 |
| `memory` | 2 |
| `provider` | 4 |
| `service` | 8 |
| `skill` | 2 |

## 状态统计

| 状态 | 数量 |
|---|---:|
| `implemented` | 5 |
| `translated-skeleton` | 32 |

## Channel WebSocket 适配范围

- channel 总数：**21**
- 可 WebSocket 直连（纳入 ESP32 实现范围）：**3**
- 暂不纳入（非 WebSocket 直连）：**18**

## 全量清单

| 扩展 ID | 适配类型 | channels | providers | 配置键 | WS-Channel | 状态 |
|---|---|---:|---:|---:|---|---|
| `acpx` | `skill` | 0 | 0 | 5 | `-` | `translated-skeleton` |
| `bluebubbles` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `copilot-proxy` | `provider` | 0 | 1 | 0 | `-` | `translated-skeleton` |
| `device-pair` | `service` | 0 | 0 | 1 | `-` | `implemented` |
| `diagnostics-otel` | `service` | 0 | 0 | 0 | `-` | `translated-skeleton` |
| `discord` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `feishu` | `channel` | 1 | 0 | 0 | `yes` | `implemented` |
| `google-gemini-cli-auth` | `provider` | 0 | 1 | 0 | `-` | `translated-skeleton` |
| `googlechat` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `imessage` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `irc` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `line` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `llm-task` | `service` | 0 | 0 | 6 | `-` | `translated-skeleton` |
| `lobster` | `service` | 0 | 0 | 0 | `-` | `translated-skeleton` |
| `matrix` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `mattermost` | `channel` | 1 | 0 | 0 | `yes` | `implemented` |
| `memory-core` | `memory` | 0 | 0 | 0 | `-` | `translated-skeleton` |
| `memory-lancedb` | `memory` | 0 | 0 | 5 | `-` | `translated-skeleton` |
| `minimax-portal-auth` | `provider` | 0 | 1 | 0 | `-` | `translated-skeleton` |
| `msteams` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `nextcloud-talk` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `nostr` | `channel` | 1 | 0 | 0 | `yes` | `implemented` |
| `open-prose` | `skill` | 0 | 0 | 0 | `-` | `translated-skeleton` |
| `phone-control` | `service` | 0 | 0 | 0 | `-` | `translated-skeleton` |
| `qwen-portal-auth` | `provider` | 0 | 1 | 0 | `-` | `translated-skeleton` |
| `signal` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `slack` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `synology-chat` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `talk-voice` | `service` | 0 | 0 | 0 | `-` | `translated-skeleton` |
| `telegram` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `thread-ownership` | `service` | 0 | 0 | 2 | `-` | `implemented` |
| `tlon` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `twitch` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `voice-call` | `service` | 0 | 0 | 28 | `-` | `translated-skeleton` |
| `whatsapp` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `zalo` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
| `zalouser` | `channel` | 1 | 0 | 0 | `no` | `translated-skeleton` |
