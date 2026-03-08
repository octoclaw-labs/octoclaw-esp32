#include "policy_guard.h"

namespace octo {

GuardDecision PolicyGuard::Evaluate(const std::string& tool_name,
                                    bool transport_ready,
                                    uint32_t feature_mask,
                                    const PolicyStore& policy_store,
                                    const CapabilityManifest& capability_manifest) const {
    GuardDecision decision;
    decision.risk_level = capability_manifest.GetRiskLevel(tool_name);

    if (!FeatureEnabled(feature_mask, kFeaturePolicyGuard)) {
        decision.reason_code = "policy_guard_disabled";
        decision.message = "policy guard disabled";
        return decision;
    }

    const auto& snapshot = policy_store.GetSnapshot();
    if (snapshot.emergency_stop) {
        decision.action = GuardAction::kHold;
        decision.reason_code = "emergency_stop";
        decision.message = "device is in emergency stop mode";
        return decision;
    }

    if (!transport_ready) {
        decision.action = GuardAction::kHold;
        decision.reason_code = "transport_not_ready";
        decision.message = "mcp transport is not ready";
        return decision;
    }

    if (!policy_store.IsToolAllowed(tool_name)) {
        decision.action = GuardAction::kFault;
        decision.reason_code = "tool_not_whitelisted";
        decision.message = "tool is not whitelisted by policy";
        return decision;
    }

    if (decision.risk_level >= snapshot.risk_threshold) {
        decision.action = GuardAction::kEscalate;
        decision.reason_code = "risk_threshold_exceeded";
        decision.message = "tool risk level exceeds current threshold";
        return decision;
    }

    return decision;
}

}  // namespace octo
