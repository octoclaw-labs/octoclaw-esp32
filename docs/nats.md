# NATS over WebSocket 通信协议文档

本文档说明 ESP32 设备端 `NatsProtocol` 的接入方式。当前实现采用 **NATS 文本协议 + WebSocket** 传输，在 NATS Subject 上承载控制 JSON 与音频帧。

## 1. 协议定位

- 传输层：WebSocket（建议 `wss://`）
- 消息层：NATS 协议行（`CONNECT` / `SUB` / `PUB` / `PING` / `PONG`）
- 业务层：
  - 控制消息：JSON（与 websocket/mqtt 语义一致）
  - 音频消息：二进制帧（兼容 BinaryProtocol 1/2/3）

## 2. OTA 下发配置

在 OTA 响应中新增 `nats` 节点，设备会写入 NVS `nats` 命名空间：

```json
{
  "nats": {
    "url": "wss://nats.example.com/ws",
    "token": "your-token",
    "publish_subject": "octoclaw.device.up",
    "subscribe_subject": "octoclaw.device.down",
    "audio_publish_subject": "octoclaw.device.up.audio",
    "audio_subscribe_subject": "octoclaw.device.down.audio",
    "queue_group": "octoclaw-workers",
    "version": 3
  }
}
```

字段说明：

- `url`：NATS WebSocket 地址（必填）
- `token`：可选，映射到 NATS `CONNECT.auth_token`
- `publish_subject`：设备上行控制主题
- `subscribe_subject`：设备下行控制主题
- `audio_publish_subject`：设备上行音频主题（可选，默认 `${publish_subject}.audio`）
- `audio_subscribe_subject`：设备下行音频主题（可选，默认 `${subscribe_subject}.audio`）
- `queue_group`：可选，订阅队列组
- `version`：二进制音频协议版本（1/2/3，默认 3）

## 3. 握手流程

1. 设备连接 NATS WebSocket。
2. 发送 `CONNECT` + `PING`。
3. 发送 `SUB`（控制主题 + 音频主题）。
4. 通过 `PUB <publish_subject>` 发送 `type=hello` 的 JSON。
5. 收到服务端 `type=hello` 后建立会话，进入音频与 MCP 正常流程。

## 4. 与 MCP 的关系

MCP 仍通过 `type: "mcp"` JSON 透传，`payload` 内保持 JSON-RPC 2.0，不改变现有 `initialize / tools/list / tools/call` 语义。
