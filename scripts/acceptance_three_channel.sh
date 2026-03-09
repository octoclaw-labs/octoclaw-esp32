#!/usr/bin/env bash
# 三通道闭环最小验收脚本（Feishu / Mattermost / Nostr）
# 在有 ESP-IDF 环境的机器上可一次性完成：翻译检查 + C3/S3 构建 + 样例说明
# 用法：从 octoclaw-esp32 仓库根目录执行 ./scripts/acceptance_three_channel.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SAMPLES_DIR="${SCRIPT_DIR}/acceptance_three_channel_samples"

cd "${PROJECT_ROOT}"

echo "=== 1/3 翻译一致性检查 ==="
"${SCRIPT_DIR}/check_openclaw_extension_translation.sh"
echo ""

echo "=== 2/3 ESP-IDF 构建（C3 / S3）==="
if command -v idf.py &>/dev/null; then
  idf.py -B build-c3 -DIDF_TARGET=esp32c3 build
  echo "[OK] esp32c3 build done"
  idf.py -B build-s3 -DIDF_TARGET=esp32s3 build
  echo "[OK] esp32s3 build done"
else
  echo "[SKIP] idf.py not in PATH; skip C3/S3 build."
  echo "       使用 ESP-IDF 5.5.2：先 source \$IDF_PATH/export.sh（若未安装：git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git）。"
fi
echo ""

echo "=== 3/3 三通道闭环验收样例 ==="
echo "以下 JSON 可用于："
echo "  - channel_event 注入：模拟上游下行消息，验证设备收件缓存与 allowlist 过滤"
echo "  - MCP tools/call：验证设备上行 send_message / send_dm 桥接"
echo ""
echo "样例文件："
echo "  - channel_event 注入: ${SAMPLES_DIR}/channel_event_inject.json"
echo "  - MCP tools/call:     ${SAMPLES_DIR}/mcp_tools_call_examples.json"
echo ""
echo "闭环验证步骤（需设备与上游桥接在线）："
echo "  1) 向下行主题发送 channel_event_inject.json 中对应 channel 的一条事件"
echo "  2) 设备侧确认 inbox 可见（如 self.<channel>.get_state）"
echo "  3) 通过 MCP 下发 tools/call，调用 self.<channel>.send_message 或 send_dm"
echo "  4) 上游桥接收到 channel_command 并完成发送"
echo ""
