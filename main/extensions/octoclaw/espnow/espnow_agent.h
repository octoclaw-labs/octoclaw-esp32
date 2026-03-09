/**
 * @file espnow_agent.h
 * @brief V2 ESP-NOW 设备协同占位（落点 extensions/octoclaw/espnow）
 *
 * 当 CONFIG_OCTO_ENABLE_ESP_NOW_AGENT 时编译；与 WebSocket 主链并存，
 * 用于设备间任务分发/回收与状态同步，具体实现后续按 V2 PRD 扩展。
 */

#ifndef OCTO_ESP_NOW_AGENT_H
#define OCTO_ESP_NOW_AGENT_H

namespace octo {

/** ESP-NOW Agent 占位：初始化/去初始化，与 receipt 语义不冲突 */
class EspNowAgent {
public:
    /** 初始化 ESP-NOW 栈（若已使能）；无-op 占位 */
    static void Init();
    /** 去初始化 */
    static void Shutdown();
    /** 是否已就绪（占位恒为 false，直至实现） */
    static bool IsReady();
};

}  // namespace octo

#endif  // OCTO_ESP_NOW_AGENT_H
