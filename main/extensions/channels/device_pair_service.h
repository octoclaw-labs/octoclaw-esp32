#ifndef OCTO_DEVICE_PAIR_SERVICE_H
#define OCTO_DEVICE_PAIR_SERVICE_H

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <cJSON.h>

namespace octo::channels {

struct PairingRequest {
    std::string request_id;
    std::string device_id;
    std::string display_name;
    std::string platform;
    std::string remote_ip;
    int64_t ts_ms = 0;
};

class DevicePairService {
public:
    static DevicePairService& GetInstance();

    void Initialize(const std::string& device_id, const std::string& board_name);

    bool AddPendingRequestFromJson(const cJSON* root, std::string* error_message = nullptr);
    bool ApproveRequest(const std::string& request_id, PairingRequest* approved_request);

    cJSON* ExportPendingJson() const;
    cJSON* GenerateSetupCodeJson(const std::string& public_url,
                                 const std::string& auth_token,
                                 const std::string& auth_password,
                                 std::string* error_message) const;

private:
    DevicePairService() = default;

    void LoadLocked();
    void SaveLocked() const;
    bool NormalizeGatewayUrl(const std::string& raw, std::string* normalized) const;
    std::string ResolveDefaultGatewayUrl(std::string* source, std::string* error_message) const;
    std::string EncodeSetupCode(const std::string& payload_json) const;

    std::string device_id_;
    std::string board_name_;
    bool initialized_ = false;
    mutable std::mutex mutex_;
    std::vector<PairingRequest> pending_requests_;
};

}  // namespace octo::channels

#endif  // OCTO_DEVICE_PAIR_SERVICE_H
