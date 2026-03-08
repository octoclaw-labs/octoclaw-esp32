#include "agent_lite.h"

#include <esp_timer.h>

namespace octo {

AgentLiteRuntime::AgentLiteRuntime(size_t ring_size) : ring_size_(ring_size) {
    if (ring_size_ == 0) {
        ring_size_ = 16;
    }
}

void AgentLiteRuntime::RecordDecision(const std::string& tool_name,
                                      GuardAction action,
                                      int risk_level,
                                      const std::string& reason_code) {
    AgentEvent event;
    event.ts_ms = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    event.tool_name = tool_name;
    event.action = action;
    event.risk_level = risk_level;
    event.reason_code = reason_code;

    if (events_.size() >= ring_size_) {
        events_.pop_front();
    }
    events_.push_back(event);

    switch (action) {
        case GuardAction::kExecute:
            ++execute_count_;
            break;
        case GuardAction::kHold:
            ++hold_count_;
            break;
        case GuardAction::kEscalate:
            ++escalate_count_;
            break;
        case GuardAction::kFault:
            ++fault_count_;
            break;
        default:
            ++fault_count_;
            break;
    }
}

cJSON* AgentLiteRuntime::ExportJson() const {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "ringSize", static_cast<double>(ring_size_));
    cJSON_AddNumberToObject(json, "executeCount", static_cast<double>(execute_count_));
    cJSON_AddNumberToObject(json, "holdCount", static_cast<double>(hold_count_));
    cJSON_AddNumberToObject(json, "escalateCount", static_cast<double>(escalate_count_));
    cJSON_AddNumberToObject(json, "faultCount", static_cast<double>(fault_count_));

    cJSON* events = cJSON_CreateArray();
    for (const auto& event : events_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(event.ts_ms));
        cJSON_AddStringToObject(item, "tool", event.tool_name.c_str());
        cJSON_AddStringToObject(item, "action", GuardActionToString(event.action));
        cJSON_AddNumberToObject(item, "risk", event.risk_level);
        cJSON_AddStringToObject(item, "reason", event.reason_code.c_str());
        cJSON_AddItemToArray(events, item);
    }
    cJSON_AddItemToObject(json, "recentEvents", events);
    return json;
}

}  // namespace octo
