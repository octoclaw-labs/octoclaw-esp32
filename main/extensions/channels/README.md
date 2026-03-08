# channels 扩展目录

该目录用于承接 `research/openclaw/extensions` 的渠道能力，目标是将服务器侧扩展翻译为 ESP32 可运行的轻量实现。

## 当前基础设施

- `channel_extension.h`：渠道扩展统一接口
- `channel_registry.h/.cc`：渠道扩展注册中心
- `openclaw_extension_catalog.h/.cc`：翻译扩展目录导出（供诊断工具返回）
- `openclaw_extension_catalog.generated.h`：自动生成的 37 个扩展清单
- `openclaw_extensions_manifest.json`：翻译清单 JSON

## 建议迁移清单

可按下列优先级逐步迁移：

1. `douyin`
2. `meituan`
3. `rednode`
4. `kuaishou`
5. `amap`

每个渠道建议建立独立子目录（例如 `main/extensions/channels/douyin`），并提供以下最小实现：

- 配置加载（NVS 或 OTA snapshot）
- 上下行消息转换（渠道协议 <-> MCP/设备事件）
- 故障与回执（保持统一错误码和追踪字段）

## 自动生成

执行以下命令可重建全量翻译骨架：

```bash
python3 scripts/translate_openclaw_extensions.py
```
