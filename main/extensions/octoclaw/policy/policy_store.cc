#include "policy_store.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

#include <esp_log.h>

#include "settings.h"

#define TAG "octo_policy_store"

namespace octo {

namespace {
constexpr const char* kPolicyNamespace = "octo_policy";
constexpr const char* kPolicyVersionKey = "policy_version";
constexpr const char* kRiskThresholdKey = "risk_threshold";
constexpr const char* kEmergencyStopKey = "emergency_stop";
constexpr const char* kWhitelistKey = "tool_whitelist";
constexpr const char* kFeatureMaskKey = "feature_mask";
}  // namespace

PolicyStore::PolicyStore() {
    ParseWhitelist();
}

bool PolicyStore::Load() {
    Settings settings(kPolicyNamespace, false);
    snapshot_.policy_version = settings.GetInt(kPolicyVersionKey, 1);
    snapshot_.risk_threshold = settings.GetInt(kRiskThresholdKey, 3);
    snapshot_.emergency_stop = settings.GetBool(kEmergencyStopKey, false);
    snapshot_.tool_whitelist = settings.GetString(kWhitelistKey, "*");
    snapshot_.feature_mask_override = settings.GetInt(kFeatureMaskKey, -1);

    if (snapshot_.risk_threshold < 1) {
        snapshot_.risk_threshold = 1;
    }
    if (snapshot_.risk_threshold > 3) {
        snapshot_.risk_threshold = 3;
    }

    ParseWhitelist();
    return true;
}

bool PolicyStore::Save() {
    Settings settings(kPolicyNamespace, true);
    settings.SetInt(kPolicyVersionKey, snapshot_.policy_version);
    settings.SetInt(kRiskThresholdKey, snapshot_.risk_threshold);
    settings.SetBool(kEmergencyStopKey, snapshot_.emergency_stop);
    settings.SetString(kWhitelistKey, snapshot_.tool_whitelist);
    settings.SetInt(kFeatureMaskKey, snapshot_.feature_mask_override);
    ParseWhitelist();
    return true;
}

bool PolicyStore::IsToolPatternMatched(const std::string& pattern, const std::string& tool_name) const {
    if (pattern.empty()) {
        return false;
    }
    if (pattern == "*") {
        return true;
    }
    if (pattern.back() == '*') {
        std::string prefix = pattern.substr(0, pattern.size() - 1);
        return tool_name.rfind(prefix, 0) == 0;
    }
    return pattern == tool_name;
}

bool PolicyStore::IsToolAllowed(const std::string& tool_name) const {
    if (whitelist_patterns_.empty()) {
        return false;
    }
    for (const auto& pattern : whitelist_patterns_) {
        if (IsToolPatternMatched(pattern, tool_name)) {
            return true;
        }
    }
    return false;
}

void PolicyStore::ParseWhitelist() {
    whitelist_patterns_.clear();
    std::stringstream stream(snapshot_.tool_whitelist);
    std::string item;
    while (std::getline(stream, item, ',')) {
        item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), item.end());
        if (!item.empty()) {
            whitelist_patterns_.push_back(item);
        }
    }
    if (whitelist_patterns_.empty()) {
        whitelist_patterns_.push_back("*");
    }
}

bool PolicyStore::UpdateFromJson(const cJSON* policy_json, std::string* error_message) {
    if (!cJSON_IsObject(policy_json)) {
        if (error_message != nullptr) {
            *error_message = "policy 必须是 JSON 对象";
        }
        return false;
    }

    auto next = snapshot_;

    auto version = cJSON_GetObjectItem(policy_json, "policy_version");
    if (cJSON_IsNumber(version)) {
        if (version->valueint <= 0) {
            if (error_message != nullptr) {
                *error_message = "policy_version 必须大于 0";
            }
            return false;
        }
        next.policy_version = version->valueint;
    }

    auto threshold = cJSON_GetObjectItem(policy_json, "risk_threshold");
    if (cJSON_IsNumber(threshold)) {
        if (threshold->valueint < 1 || threshold->valueint > 3) {
            if (error_message != nullptr) {
                *error_message = "risk_threshold 范围必须是 1-3";
            }
            return false;
        }
        next.risk_threshold = threshold->valueint;
    }

    auto emergency_stop = cJSON_GetObjectItem(policy_json, "emergency_stop");
    if (cJSON_IsBool(emergency_stop)) {
        next.emergency_stop = emergency_stop->valueint == 1;
    }

    auto whitelist = cJSON_GetObjectItem(policy_json, "tool_whitelist");
    if (cJSON_IsString(whitelist)) {
        if (strlen(whitelist->valuestring) == 0) {
            if (error_message != nullptr) {
                *error_message = "tool_whitelist 不能为空";
            }
            return false;
        }
        next.tool_whitelist = whitelist->valuestring;
    }

    auto feature_mask = cJSON_GetObjectItem(policy_json, "feature_mask");
    if (cJSON_IsNumber(feature_mask)) {
        if (feature_mask->valueint < -1) {
            if (error_message != nullptr) {
                *error_message = "feature_mask 不能小于 -1";
            }
            return false;
        }
        next.feature_mask_override = feature_mask->valueint;
    }

    if (!cJSON_IsNumber(version)) {
        next.policy_version = snapshot_.policy_version + 1;
    }

    snapshot_ = next;
    Save();
    ESP_LOGI(TAG, "Policy updated. version=%d, emergency_stop=%d",
             snapshot_.policy_version, snapshot_.emergency_stop ? 1 : 0);
    return true;
}

cJSON* PolicyStore::ExportJson() const {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "policy_version", snapshot_.policy_version);
    cJSON_AddNumberToObject(json, "risk_threshold", snapshot_.risk_threshold);
    cJSON_AddBoolToObject(json, "emergency_stop", snapshot_.emergency_stop);
    cJSON_AddStringToObject(json, "tool_whitelist", snapshot_.tool_whitelist.c_str());
    cJSON_AddNumberToObject(json, "feature_mask", snapshot_.feature_mask_override);
    return json;
}

}  // namespace octo
