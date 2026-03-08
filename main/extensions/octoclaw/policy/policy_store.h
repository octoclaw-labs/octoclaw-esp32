#ifndef OCTO_POLICY_STORE_H
#define OCTO_POLICY_STORE_H

#include <stdint.h>
#include <string>
#include <vector>

#include <cJSON.h>

namespace octo {

struct PolicySnapshot {
    int policy_version = 1;
    int risk_threshold = 3;
    bool emergency_stop = false;
    std::string tool_whitelist = "*";
    int32_t feature_mask_override = -1;
};

class PolicyStore {
public:
    PolicyStore();

    bool Load();
    bool Save();

    const PolicySnapshot& GetSnapshot() const { return snapshot_; }
    bool IsToolAllowed(const std::string& tool_name) const;
    bool UpdateFromJson(const cJSON* policy_json, std::string* error_message);
    cJSON* ExportJson() const;

private:
    void ParseWhitelist();
    bool IsToolPatternMatched(const std::string& pattern, const std::string& tool_name) const;

    PolicySnapshot snapshot_;
    std::vector<std::string> whitelist_patterns_;
};

}  // namespace octo

#endif  // OCTO_POLICY_STORE_H
