/**
 * @file task_step.h
 * @brief V2 任务编排：单步状态与步骤状态机（落点 extensions/octoclaw/runtime）
 *
 * 用于 plan/step 消息的步骤追踪与状态回执，与 agent_lite、receipt_queue 协同；
 * 不改变 xiaozhi 主循环，仅扩展 runtime 能力。
 */

#ifndef OCTO_TASK_STEP_H
#define OCTO_TASK_STEP_H

#include <cstdint>
#include <string>
#include <vector>

#include <cJSON.h>

#include "extensions/octoclaw/core/octo_agent_types.h"

namespace octo {

/** 单步执行状态，与 MCP 回执、策略决策一致 */
enum class StepStatus {
    kPending = 0,
    kRunning = 1,
    kDone = 2,
    kFault = 3,
    kHold = 4,
    kEscalate = 5,
};

/** 单步记录：用于步骤可追踪、状态回执 */
struct StepRecord {
    std::string step_id;
    std::string request_id;
    std::string tool_name;
    StepStatus status = StepStatus::kPending;
    int64_t ts_ms = 0;
    std::string reason_code;
};

/**
 * 步骤状态机：维护当前 plan 下各 step 的状态，供编排与回执使用。
 * 轻量实现，仅保留最近 N 步，超出可裁剪。
 */
class TaskStepStateMachine {
public:
    explicit TaskStepStateMachine(size_t max_steps = 32);

    /** 开始一步：标记为 Running，返回是否接受 */
    bool StartStep(const std::string& step_id, const std::string& request_id, const std::string& tool_name);
    /** 结束一步：标记为 Done/Fault/Hold/Escalate */
    void FinishStep(const std::string& step_id, StepStatus status, const std::string& reason_code);
    /** 根据策略动作更新步骤状态 */
    void FinishStepByAction(const std::string& step_id, GuardAction action, const std::string& reason_code);

    StepStatus GetStepStatus(const std::string& step_id) const;
    /** 导出最近步骤（用于诊断/回执），调用方负责 cJSON_Delete */
    struct cJSON* ExportStepsJson() const;

private:
    size_t max_steps_;
    std::vector<StepRecord> steps_;

    static StepStatus GuardActionToStepStatus(GuardAction action);
};

}  // namespace octo

#endif  // OCTO_TASK_STEP_H
