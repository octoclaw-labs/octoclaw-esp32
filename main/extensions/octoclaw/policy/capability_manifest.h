#ifndef OCTO_CAPABILITY_MANIFEST_H
#define OCTO_CAPABILITY_MANIFEST_H

#include <string>
#include <unordered_map>

#include <cJSON.h>

namespace octo {

class CapabilityManifest {
public:
    CapabilityManifest();

    int GetRiskLevel(const std::string& tool_name) const;
    cJSON* ExportJson() const;

private:
    std::unordered_map<std::string, int> risk_levels_;
};

}  // namespace octo

#endif  // OCTO_CAPABILITY_MANIFEST_H
