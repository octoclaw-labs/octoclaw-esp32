#ifndef OCTO_POLICY_GUARD_H
#define OCTO_POLICY_GUARD_H

#include <string>

#include "extensions/octoclaw/policy/capability_manifest.h"
#include "extensions/octoclaw/core/octo_agent_types.h"
#include "extensions/octoclaw/policy/policy_store.h"

namespace octo {

struct GuardDecision {
    GuardAction action = GuardAction::kExecute;
    int risk_level = 1;
    std::string reason_code = "ok";
    std::string message = "allowed";
};

class PolicyGuard {
public:
    GuardDecision Evaluate(const std::string& tool_name,
                           bool transport_ready,
                           uint32_t feature_mask,
                           const PolicyStore& policy_store,
                           const CapabilityManifest& capability_manifest) const;
};

}  // namespace octo

#endif  // OCTO_POLICY_GUARD_H
