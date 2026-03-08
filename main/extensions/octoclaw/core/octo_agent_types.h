#ifndef OCTO_AGENT_TYPES_H
#define OCTO_AGENT_TYPES_H

#include <stdint.h>
#include <string>

namespace octo {

enum class BoardTier {
    kLite = 0,
    kStandard = 1,
    kPro = 2,
};

enum class GuardAction {
    kExecute = 0,
    kHold = 1,
    kEscalate = 2,
    kFault = 3,
};

constexpr uint32_t kFeaturePolicyGuard = 1u << 0;
constexpr uint32_t kFeatureReceiptCompensation = 1u << 1;
constexpr uint32_t kFeatureLocalAgent = 1u << 2;
constexpr uint32_t kFeatureMcpExtFields = 1u << 3;
constexpr uint32_t kFeatureEspNowAgent = 1u << 4;

struct BoardProfile {
    const char* config_symbol;
    const char* chip;
    BoardTier tier;
    uint32_t default_feature_mask;
};

struct ReplyMeta {
    std::string decision;
    int risk_level = 0;
    int policy_version = 0;
    std::string request_id;
    std::string reason_code;
};

inline const char* GuardActionToString(GuardAction action) {
    switch (action) {
        case GuardAction::kExecute:
            return "execute";
        case GuardAction::kHold:
            return "hold";
        case GuardAction::kEscalate:
            return "escalate";
        case GuardAction::kFault:
            return "fault";
        default:
            return "fault";
    }
}

inline const char* BoardTierToString(BoardTier tier) {
    switch (tier) {
        case BoardTier::kLite:
            return "lite";
        case BoardTier::kStandard:
            return "standard";
        case BoardTier::kPro:
            return "pro";
        default:
            return "lite";
    }
}

inline bool FeatureEnabled(uint32_t feature_mask, uint32_t feature_bit) {
    return (feature_mask & feature_bit) != 0;
}

}  // namespace octo

#endif  // OCTO_AGENT_TYPES_H
