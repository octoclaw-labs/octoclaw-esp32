/**
 * @file audit_log.h
 * @brief V3 审计：关键操作日志环，供 audit_report 与可追溯使用（落点 extensions/octoclaw/audit）
 *
 * 与 agent_lite、task_step 协同；不改变主循环，仅扩展治理能力。
 */

#ifndef OCTO_AUDIT_LOG_H
#define OCTO_AUDIT_LOG_H

#include <cstdint>
#include <string>
#include <vector>

#include <cJSON.h>

namespace octo {

/** 单条审计记录 */
struct AuditEntry {
    int64_t ts_ms = 0;
    std::string request_id;
    std::string tool_name;
    std::string status;   // "done" | "fault" | "hold" | "escalate"
    std::string reason_code;
};

/**
 * 审计日志：固定容量环，超出覆盖最旧；导出 JSON 供上报或 MCP 工具（如 self.system.audit_report）。
 */
class AuditLog {
public:
    explicit AuditLog(size_t max_entries = 64);

    /** 记录一次 tools/call 结果（由 mcp_server 或 agent_lite 侧在回执前调用） */
    void RecordToolCall(const std::string& request_id,
                        const std::string& tool_name,
                        const std::string& status,
                        const std::string& reason_code = "");

    /** 导出最近记录为 JSON 数组，调用方负责 cJSON_Delete */
    cJSON* ExportJson() const;

private:
    size_t max_entries_;
    std::vector<AuditEntry> entries_;
};

}  // namespace octo

#endif  // OCTO_AUDIT_LOG_H
