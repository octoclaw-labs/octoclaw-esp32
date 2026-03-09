/**
 * @file espnow_agent.cc
 * @brief V2 ESP-NOW Agent 占位实现
 */

#include "espnow_agent.h"

namespace octo {

void EspNowAgent::Init() {
#ifdef CONFIG_OCTO_ENABLE_ESP_NOW_AGENT
    // 占位：后续接入 ESP-NOW 栈、与 receipt_queue / task_step 协同
#else
    (void)0;
#endif
}

void EspNowAgent::Shutdown() {}

bool EspNowAgent::IsReady() {
#ifdef CONFIG_OCTO_ENABLE_ESP_NOW_AGENT
    return false;  // 占位直至实现
#else
    return false;
#endif
}

}  // namespace octo
