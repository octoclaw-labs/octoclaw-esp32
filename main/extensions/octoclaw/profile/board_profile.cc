#include "board_profile.h"

#include "board_tier_matrix.generated.h"

namespace octo {

const BoardProfile& GetCompiledBoardProfile() {
    return GetCompiledBoardProfileGenerated();
}

uint32_t BuildFeatureMaskFromKconfig() {
    uint32_t feature_mask = 0;

#ifdef CONFIG_OCTO_ENABLE_POLICY_GUARD
    feature_mask |= kFeaturePolicyGuard;
#endif
#ifdef CONFIG_OCTO_ENABLE_RECEIPT_COMPENSATION
    feature_mask |= kFeatureReceiptCompensation;
#endif
#ifdef CONFIG_OCTO_ENABLE_LOCAL_AGENT
    feature_mask |= kFeatureLocalAgent;
#endif
#ifdef CONFIG_OCTO_ENABLE_MCP_EXT_FIELDS
    feature_mask |= kFeatureMcpExtFields;
#endif
#ifdef CONFIG_OCTO_ENABLE_ESP_NOW_AGENT
    feature_mask |= kFeatureEspNowAgent;
#endif

    if (feature_mask == 0) {
        feature_mask = GetCompiledBoardProfile().default_feature_mask;
    }

    return feature_mask;
}

uint32_t GetEffectiveFeatureMask(int32_t policy_feature_mask_override) {
    if (policy_feature_mask_override >= 0) {
        return static_cast<uint32_t>(policy_feature_mask_override);
    }
    return BuildFeatureMaskFromKconfig();
}

int GetRuleMax() {
#ifdef CONFIG_OCTO_RULE_MAX
    return CONFIG_OCTO_RULE_MAX;
#else
    return 8;
#endif
}

int GetTelemetryRingSize() {
#ifdef CONFIG_OCTO_TELEMETRY_RING_SIZE
    return CONFIG_OCTO_TELEMETRY_RING_SIZE;
#else
    return 16;
#endif
}

static cJSON* BuildFeatureListJson(uint32_t feature_mask) {
    cJSON* features = cJSON_CreateArray();
    if (FeatureEnabled(feature_mask, kFeaturePolicyGuard)) {
        cJSON_AddItemToArray(features, cJSON_CreateString("policy_guard"));
    }
    if (FeatureEnabled(feature_mask, kFeatureReceiptCompensation)) {
        cJSON_AddItemToArray(features, cJSON_CreateString("receipt_compensation"));
    }
    if (FeatureEnabled(feature_mask, kFeatureLocalAgent)) {
        cJSON_AddItemToArray(features, cJSON_CreateString("local_agent"));
    }
    if (FeatureEnabled(feature_mask, kFeatureMcpExtFields)) {
        cJSON_AddItemToArray(features, cJSON_CreateString("mcp_ext_fields"));
    }
    if (FeatureEnabled(feature_mask, kFeatureEspNowAgent)) {
        cJSON_AddItemToArray(features, cJSON_CreateString("esp_now_agent"));
    }
    return features;
}

cJSON* BuildBoardProfileJson(uint32_t effective_feature_mask, int policy_version) {
    const auto& profile = GetCompiledBoardProfile();
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "boardConfig", profile.config_symbol);
    cJSON_AddStringToObject(json, "chip", profile.chip);
    cJSON_AddStringToObject(json, "tier", BoardTierToString(profile.tier));
    cJSON_AddNumberToObject(json, "policyVersion", policy_version);
    cJSON_AddNumberToObject(json, "featureMask", static_cast<double>(effective_feature_mask));
    cJSON_AddNumberToObject(json, "ruleMax", GetRuleMax());
    cJSON_AddNumberToObject(json, "telemetryRingSize", GetTelemetryRingSize());
    cJSON_AddItemToObject(json, "features", BuildFeatureListJson(effective_feature_mask));
    return json;
}

}  // namespace octo
