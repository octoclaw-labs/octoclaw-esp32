#include "capability_manifest.h"

#include <algorithm>

namespace octo {

CapabilityManifest::CapabilityManifest() {
    risk_levels_.emplace("self.reboot", 3);
    risk_levels_.emplace("self.upgrade_firmware", 3);
    risk_levels_.emplace("self.assets.set_download_url", 3);

    risk_levels_.emplace("self.screen.snapshot", 2);
    risk_levels_.emplace("self.screen.preview_image", 2);
    risk_levels_.emplace("self.camera.take_photo", 2);
    risk_levels_.emplace("self.system.update_octoclaw_policy", 2);

    risk_levels_.emplace("self.audio_speaker.set_volume", 1);
    risk_levels_.emplace("self.screen.set_brightness", 1);
    risk_levels_.emplace("self.screen.set_theme", 1);
    risk_levels_.emplace("self.get_device_status", 1);
    risk_levels_.emplace("self.get_system_info", 1);
    risk_levels_.emplace("self.system.get_octoclaw_profile", 1);
}

int CapabilityManifest::GetRiskLevel(const std::string& tool_name) const {
    auto it = risk_levels_.find(tool_name);
    if (it != risk_levels_.end()) {
        return it->second;
    }

    if (tool_name.find("reboot") != std::string::npos ||
        tool_name.find("upgrade") != std::string::npos ||
        tool_name.find("firmware") != std::string::npos) {
        return 3;
    }
    if (tool_name.find("camera") != std::string::npos ||
        tool_name.find("snapshot") != std::string::npos ||
        tool_name.find("motor") != std::string::npos ||
        tool_name.find("chassis") != std::string::npos) {
        return 2;
    }
    return 1;
}

cJSON* CapabilityManifest::ExportJson() const {
    cJSON* json = cJSON_CreateObject();
    for (const auto& item : risk_levels_) {
        cJSON_AddNumberToObject(json, item.first.c_str(), item.second);
    }
    return json;
}

}  // namespace octo
