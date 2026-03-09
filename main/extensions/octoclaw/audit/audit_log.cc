/**
 * @file audit_log.cc
 * @brief V3 审计日志实现
 */

#include "audit_log.h"

#include <esp_timer.h>
#include <algorithm>

namespace octo {

AuditLog::AuditLog(size_t max_entries) : max_entries_(max_entries) {
    if (max_entries_ == 0) {
        max_entries_ = 64;
    }
}

void AuditLog::RecordToolCall(const std::string& request_id,
                               const std::string& tool_name,
                               const std::string& status,
                               const std::string& reason_code) {
    AuditEntry e;
    e.ts_ms = static_cast<int64_t>(esp_timer_get_time() / 1000);
    e.request_id = request_id;
    e.tool_name = tool_name;
    e.status = status.empty() ? "done" : status;
    e.reason_code = reason_code;

    if (entries_.size() >= max_entries_) {
        entries_.erase(entries_.begin());
    }
    entries_.push_back(e);
}

cJSON* AuditLog::ExportJson() const {
    cJSON* arr = cJSON_CreateArray();
    if (arr == nullptr) {
        return nullptr;
    }
    for (const auto& e : entries_) {
        cJSON* item = cJSON_CreateObject();
        if (item == nullptr) {
            continue;
        }
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(e.ts_ms));
        cJSON_AddStringToObject(item, "requestId", e.request_id.c_str());
        cJSON_AddStringToObject(item, "tool", e.tool_name.c_str());
        cJSON_AddStringToObject(item, "status", e.status.c_str());
        cJSON_AddStringToObject(item, "reason", e.reason_code.c_str());
        cJSON_AddItemToArray(arr, item);
    }
    return arr;
}

}  // namespace octo
