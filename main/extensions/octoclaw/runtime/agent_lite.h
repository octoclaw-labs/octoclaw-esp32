#ifndef OCTO_AGENT_LITE_H
#define OCTO_AGENT_LITE_H

#include <deque>
#include <string>

#include <cJSON.h>

#include "extensions/octoclaw/core/octo_agent_types.h"

namespace octo {

struct AgentEvent {
    uint64_t ts_ms = 0;
    std::string tool_name;
    GuardAction action = GuardAction::kExecute;
    int risk_level = 0;
    std::string reason_code;
};

class AgentLiteRuntime {
public:
    explicit AgentLiteRuntime(size_t ring_size);

    void RecordDecision(const std::string& tool_name,
                        GuardAction action,
                        int risk_level,
                        const std::string& reason_code);
    cJSON* ExportJson() const;

private:
    size_t ring_size_ = 16;
    std::deque<AgentEvent> events_;

    uint64_t execute_count_ = 0;
    uint64_t hold_count_ = 0;
    uint64_t escalate_count_ = 0;
    uint64_t fault_count_ = 0;
};

}  // namespace octo

#endif  // OCTO_AGENT_LITE_H
