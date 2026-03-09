#include "device_pair_service.h"

#include "settings.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <mbedtls/base64.h>

#include <algorithm>
#include <cctype>
#include <cstring>

#define TAG "device_pair"

namespace octo::channels {

namespace {

constexpr const char* kPairNamespace = "octo_pair";
constexpr const char* kPendingKey = "pending_json";
constexpr size_t kPendingMaxCount = 16;

std::string Trim(const std::string& input) {
    size_t left = 0;
    while (left < input.size() && std::isspace(static_cast<unsigned char>(input[left]))) {
        ++left;
    }
    size_t right = input.size();
    while (right > left && std::isspace(static_cast<unsigned char>(input[right - 1]))) {
        --right;
    }
    return input.substr(left, right - left);
}

bool StartsWith(const std::string& text, const char* prefix) {
    size_t len = strlen(prefix);
    return text.size() >= len && text.compare(0, len, prefix) == 0;
}

const char* GetStringByKeys(const cJSON* obj, const char* key_a, const char* key_b = nullptr) {
    if (!cJSON_IsObject(obj)) {
        return nullptr;
    }
    auto value_a = cJSON_GetObjectItem(obj, key_a);
    if (cJSON_IsString(value_a)) {
        return value_a->valuestring;
    }
    if (key_b != nullptr) {
        auto value_b = cJSON_GetObjectItem(obj, key_b);
        if (cJSON_IsString(value_b)) {
            return value_b->valuestring;
        }
    }
    return nullptr;
}

int64_t GetTimestampNowMs() {
    return static_cast<int64_t>(esp_timer_get_time() / 1000);
}

}  // namespace

DevicePairService& DevicePairService::GetInstance() {
    static DevicePairService instance;
    return instance;
}

void DevicePairService::Initialize(const std::string& device_id, const std::string& board_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }
    device_id_ = device_id;
    board_name_ = board_name;
    LoadLocked();
    initialized_ = true;
}

void DevicePairService::LoadLocked() {
    pending_requests_.clear();

    Settings settings(kPairNamespace, false);
    std::string raw = settings.GetString(kPendingKey);
    if (raw.empty()) {
        return;
    }

    cJSON* array = cJSON_Parse(raw.c_str());
    if (!cJSON_IsArray(array)) {
        if (array != nullptr) {
            cJSON_Delete(array);
        }
        return;
    }

    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, array) {
        if (!cJSON_IsObject(item)) {
            continue;
        }
        const char* request_id = GetStringByKeys(item, "request_id", "requestId");
        if (request_id == nullptr || strlen(request_id) == 0) {
            continue;
        }

        PairingRequest req;
        req.request_id = request_id;
        const char* device_id = GetStringByKeys(item, "device_id", "deviceId");
        if (device_id != nullptr) {
            req.device_id = device_id;
        }
        const char* display_name = GetStringByKeys(item, "display_name", "displayName");
        if (display_name != nullptr) {
            req.display_name = display_name;
        }
        const char* platform = GetStringByKeys(item, "platform");
        if (platform != nullptr) {
            req.platform = platform;
        }
        const char* remote_ip = GetStringByKeys(item, "remote_ip", "remoteIp");
        if (remote_ip != nullptr) {
            req.remote_ip = remote_ip;
        }
        auto ts = cJSON_GetObjectItem(item, "ts");
        if (cJSON_IsNumber(ts)) {
            req.ts_ms = static_cast<int64_t>(ts->valuedouble);
        }
        if (req.ts_ms <= 0) {
            req.ts_ms = GetTimestampNowMs();
        }
        pending_requests_.push_back(req);
    }

    cJSON_Delete(array);
}

void DevicePairService::SaveLocked() const {
    cJSON* array = cJSON_CreateArray();
    for (const auto& req : pending_requests_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "request_id", req.request_id.c_str());
        cJSON_AddStringToObject(item, "device_id", req.device_id.c_str());
        cJSON_AddStringToObject(item, "display_name", req.display_name.c_str());
        cJSON_AddStringToObject(item, "platform", req.platform.c_str());
        cJSON_AddStringToObject(item, "remote_ip", req.remote_ip.c_str());
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(req.ts_ms));
        cJSON_AddItemToArray(array, item);
    }
    char* raw = cJSON_PrintUnformatted(array);
    std::string payload = raw != nullptr ? raw : "[]";
    if (raw != nullptr) {
        cJSON_free(raw);
    }
    cJSON_Delete(array);

    Settings settings(kPairNamespace, true);
    settings.SetString(kPendingKey, payload);
}

bool DevicePairService::NormalizeGatewayUrl(const std::string& raw, std::string* normalized) const {
    if (normalized == nullptr) {
        return false;
    }
    std::string candidate = Trim(raw);
    if (candidate.empty()) {
        return false;
    }

    if (StartsWith(candidate, "http://")) {
        candidate.replace(0, strlen("http://"), "ws://");
    } else if (StartsWith(candidate, "https://")) {
        candidate.replace(0, strlen("https://"), "wss://");
    } else if (!StartsWith(candidate, "ws://") && !StartsWith(candidate, "wss://")) {
        if (candidate.find("://") != std::string::npos) {
            return false;
        }
        candidate = "wss://" + candidate;
    }

    size_t schema_end = candidate.find("://");
    if (schema_end == std::string::npos) {
        return false;
    }
    size_t host_start = schema_end + 3;
    size_t slash = candidate.find('/', host_start);
    if (slash != std::string::npos) {
        candidate = candidate.substr(0, slash);
    }
    if (candidate.size() <= host_start) {
        return false;
    }

    *normalized = candidate;
    return true;
}

std::string DevicePairService::ResolveDefaultGatewayUrl(std::string* source,
                                                        std::string* error_message) const {
    std::string normalized;
    {
        Settings nats("nats", false);
        std::string raw = nats.GetString("url");
        if (!raw.empty() && NormalizeGatewayUrl(raw, &normalized)) {
            if (source != nullptr) {
                *source = "settings.nats.url";
            }
            return normalized;
        }
    }
    {
        Settings ws("websocket", false);
        std::string raw = ws.GetString("url");
        if (!raw.empty() && NormalizeGatewayUrl(raw, &normalized)) {
            if (source != nullptr) {
                *source = "settings.websocket.url";
            }
            return normalized;
        }
    }

    if (error_message != nullptr) {
        *error_message = "未找到可用的网关 URL（nats.url / websocket.url）";
    }
    return "";
}

std::string DevicePairService::EncodeSetupCode(const std::string& payload_json) const {
    size_t out_len = 0;
    mbedtls_base64_encode(nullptr, 0, &out_len,
                          reinterpret_cast<const unsigned char*>(payload_json.data()),
                          payload_json.size());
    std::string base64(out_len, '\0');
    size_t written = 0;
    mbedtls_base64_encode(reinterpret_cast<unsigned char*>(base64.data()),
                          base64.size(),
                          &written,
                          reinterpret_cast<const unsigned char*>(payload_json.data()),
                          payload_json.size());
    base64.resize(written);

    for (auto& ch : base64) {
        if (ch == '+') {
            ch = '-';
        } else if (ch == '/') {
            ch = '_';
        }
    }
    while (!base64.empty() && base64.back() == '=') {
        base64.pop_back();
    }
    return base64;
}

bool DevicePairService::AddPendingRequestFromJson(const cJSON* root, std::string* error_message) {
    if (!cJSON_IsObject(root)) {
        if (error_message != nullptr) {
            *error_message = "pairing request 不是 JSON 对象";
        }
        return false;
    }

    const cJSON* payload = root;
    auto nested = cJSON_GetObjectItem(root, "payload");
    if (cJSON_IsObject(nested)) {
        payload = nested;
    }

    const char* request_id = GetStringByKeys(payload, "request_id", "requestId");
    if (request_id == nullptr || strlen(request_id) == 0) {
        if (error_message != nullptr) {
            *error_message = "缺少 request_id";
        }
        return false;
    }

    PairingRequest req;
    req.request_id = request_id;
    const char* device_id = GetStringByKeys(payload, "device_id", "deviceId");
    if (device_id != nullptr) {
        req.device_id = device_id;
    }
    const char* display_name = GetStringByKeys(payload, "display_name", "displayName");
    if (display_name != nullptr) {
        req.display_name = display_name;
    }
    const char* platform = GetStringByKeys(payload, "platform");
    if (platform != nullptr) {
        req.platform = platform;
    }
    const char* remote_ip = GetStringByKeys(payload, "remote_ip", "remoteIp");
    if (remote_ip != nullptr) {
        req.remote_ip = remote_ip;
    }
    auto ts = cJSON_GetObjectItem(payload, "ts");
    if (cJSON_IsNumber(ts)) {
        req.ts_ms = static_cast<int64_t>(ts->valuedouble);
    }
    if (req.ts_ms <= 0) {
        req.ts_ms = GetTimestampNowMs();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(pending_requests_.begin(), pending_requests_.end(),
                           [&req](const PairingRequest& existing) {
                               return existing.request_id == req.request_id;
                           });
    if (it != pending_requests_.end()) {
        *it = req;
    } else {
        pending_requests_.push_back(req);
    }

    std::sort(pending_requests_.begin(), pending_requests_.end(),
              [](const PairingRequest& lhs, const PairingRequest& rhs) {
                  return lhs.ts_ms < rhs.ts_ms;
              });
    while (pending_requests_.size() > kPendingMaxCount) {
        pending_requests_.erase(pending_requests_.begin());
    }
    SaveLocked();
    return true;
}

bool DevicePairService::ApproveRequest(const std::string& request_id, PairingRequest* approved_request) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_requests_.empty()) {
        return false;
    }

    size_t index = pending_requests_.size();
    if (request_id.empty() || request_id == "latest") {
        index = pending_requests_.size() - 1;
    } else {
        auto it = std::find_if(pending_requests_.begin(), pending_requests_.end(),
                               [&request_id](const PairingRequest& req) {
                                   return req.request_id == request_id;
                               });
        if (it != pending_requests_.end()) {
            index = static_cast<size_t>(std::distance(pending_requests_.begin(), it));
        }
    }

    if (index >= pending_requests_.size()) {
        return false;
    }

    if (approved_request != nullptr) {
        *approved_request = pending_requests_[index];
    }
    pending_requests_.erase(
        pending_requests_.begin() +
        static_cast<std::vector<PairingRequest>::difference_type>(index));
    SaveLocked();
    return true;
}

cJSON* DevicePairService::ExportPendingJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "count", static_cast<double>(pending_requests_.size()));

    cJSON* pending = cJSON_CreateArray();
    for (const auto& req : pending_requests_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "requestId", req.request_id.c_str());
        cJSON_AddStringToObject(item, "deviceId", req.device_id.c_str());
        cJSON_AddStringToObject(item, "displayName", req.display_name.c_str());
        cJSON_AddStringToObject(item, "platform", req.platform.c_str());
        cJSON_AddStringToObject(item, "remoteIp", req.remote_ip.c_str());
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(req.ts_ms));
        cJSON_AddItemToArray(pending, item);
    }
    cJSON_AddItemToObject(root, "pending", pending);
    return root;
}

cJSON* DevicePairService::GenerateSetupCodeJson(const std::string& public_url,
                                                const std::string& auth_token,
                                                const std::string& auth_password,
                                                std::string* error_message) const {
    std::string url;
    std::string source = "arg.public_url";
    if (!Trim(public_url).empty()) {
        if (!NormalizeGatewayUrl(public_url, &url)) {
            if (error_message != nullptr) {
                *error_message = "public_url 格式无效";
            }
            return nullptr;
        }
    } else {
        url = ResolveDefaultGatewayUrl(&source, error_message);
        if (url.empty()) {
            return nullptr;
        }
    }

    cJSON* payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "url", url.c_str());
    if (!Trim(auth_token).empty()) {
        cJSON_AddStringToObject(payload, "token", Trim(auth_token).c_str());
    }
    if (!Trim(auth_password).empty()) {
        cJSON_AddStringToObject(payload, "password", Trim(auth_password).c_str());
    }
    char* payload_raw = cJSON_PrintUnformatted(payload);
    std::string payload_json = payload_raw != nullptr ? payload_raw : "{}";
    if (payload_raw != nullptr) {
        cJSON_free(payload_raw);
    }
    cJSON_Delete(payload);

    std::string setup_code = EncodeSetupCode(payload_json);

    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "setupCode", setup_code.c_str());
    cJSON_AddStringToObject(result, "gatewayUrl", url.c_str());
    cJSON_AddStringToObject(result, "source", source.c_str());
    cJSON_AddStringToObject(result, "deviceId", device_id_.c_str());
    cJSON_AddStringToObject(result, "board", board_name_.c_str());

    std::string auth_mode = "none";
    if (!Trim(auth_token).empty()) {
        auth_mode = "token";
    } else if (!Trim(auth_password).empty()) {
        auth_mode = "password";
    }
    cJSON_AddStringToObject(result, "authMode", auth_mode.c_str());
    return result;
}

}  // namespace octo::channels
