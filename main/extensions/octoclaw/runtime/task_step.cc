/**
 * @file task_step.cc
 * @brief V2 任务编排步骤状态机实现
 */

#include "task_step.h"

#include <esp_timer.h>
#include <algorithm>

namespace octo {

namespace {

StepStatus GuardActionToStepStatusStatic(GuardAction action) {
    switch (action) {
        case GuardAction::kExecute:
            return StepStatus::kDone;
        case GuardAction::kHold:
            return StepStatus::kHold;
        case GuardAction::kEscalate:
            return StepStatus::kEscalate;
        case GuardAction::kFault:
        default:
            return StepStatus::kFault;
    }
}

const char* StepStatusToString(StepStatus s) {
    switch (s) {
        case StepStatus::kPending: return "pending";
        case StepStatus::kRunning: return "running";
        case StepStatus::kDone: return "done";
        case StepStatus::kFault: return "fault";
        case StepStatus::kHold: return "hold";
        case StepStatus::kEscalate: return "escalate";
        default: return "pending";
    }
}

}  // namespace

TaskStepStateMachine::TaskStepStateMachine(size_t max_steps) : max_steps_(max_steps) {
    if (max_steps_ == 0) {
        max_steps_ = 32;
    }
}

bool TaskStepStateMachine::StartStep(const std::string& step_id,
                                     const std::string& request_id,
                                     const std::string& tool_name) {
    if (step_id.empty()) {
        return false;
    }
    auto it = std::find_if(steps_.begin(), steps_.end(),
                           [&step_id](const StepRecord& r) { return r.step_id == step_id; });
    if (it != steps_.end()) {
        it->status = StepStatus::kRunning;
        it->request_id = request_id;
        it->tool_name = tool_name;
        it->ts_ms = static_cast<int64_t>(esp_timer_get_time() / 1000);
        return true;
    }
    while (steps_.size() >= max_steps_) {
        steps_.erase(steps_.begin());
    }
    StepRecord rec;
    rec.step_id = step_id;
    rec.request_id = request_id;
    rec.tool_name = tool_name;
    rec.status = StepStatus::kRunning;
    rec.ts_ms = static_cast<int64_t>(esp_timer_get_time() / 1000);
    steps_.push_back(rec);
    return true;
}

void TaskStepStateMachine::FinishStep(const std::string& step_id,
                                       StepStatus status,
                                       const std::string& reason_code) {
    auto it = std::find_if(steps_.begin(), steps_.end(),
                           [&step_id](const StepRecord& r) { return r.step_id == step_id; });
    if (it != steps_.end()) {
        it->status = status;
        it->reason_code = reason_code;
    }
}

void TaskStepStateMachine::FinishStepByAction(const std::string& step_id,
                                               GuardAction action,
                                               const std::string& reason_code) {
    FinishStep(step_id, GuardActionToStepStatus(action), reason_code);
}

StepStatus TaskStepStateMachine::GuardActionToStepStatus(GuardAction action) {
    return GuardActionToStepStatusStatic(action);
}

StepStatus TaskStepStateMachine::GetStepStatus(const std::string& step_id) const {
    auto it = std::find_if(steps_.begin(), steps_.end(),
                           [&step_id](const StepRecord& r) { return r.step_id == step_id; });
    if (it != steps_.end()) {
        return it->status;
    }
    return StepStatus::kPending;
}

cJSON* TaskStepStateMachine::ExportStepsJson() const {
    cJSON* arr = cJSON_CreateArray();
    if (arr == nullptr) {
        return nullptr;
    }
    for (const auto& r : steps_) {
        cJSON* item = cJSON_CreateObject();
        if (item == nullptr) {
            continue;
        }
        cJSON_AddStringToObject(item, "stepId", r.step_id.c_str());
        cJSON_AddStringToObject(item, "requestId", r.request_id.c_str());
        cJSON_AddStringToObject(item, "tool", r.tool_name.c_str());
        cJSON_AddStringToObject(item, "status", StepStatusToString(r.status));
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(r.ts_ms));
        cJSON_AddStringToObject(item, "reason", r.reason_code.c_str());
        cJSON_AddItemToArray(arr, item);
    }
    return arr;
}

}  // namespace octo
