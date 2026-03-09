#include "nostr_channel_service.h"

#include "settings.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cstring>
#include <sstream>

#define TAG "nostr_channel"

namespace octo::channels {

namespace {

constexpr const char* kNamespace = "octo_nostr";
constexpr const char* kEnabledKey = "enabled";
constexpr const char* kAccountIdKey = "account_id";
constexpr const char* kAllowPubkeysKey = "allow_pubkeys";
constexpr const char* kInboxJsonKey = "inbox_json";

constexpr const char* kDefaultAccountId = "default";
constexpr size_t kInboxMaxCount = 24;

int64_t NowMs() {
    return static_cast<int64_t>(esp_timer_get_time() / 1000);
}

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

std::string ToLower(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool IsHex64(const std::string& value) {
    if (value.size() != 64) {
        return false;
    }
    for (char ch : value) {
        if (!std::isxdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

std::string StripPrefix(const std::string& value, const char* prefix) {
    const size_t prefix_len = strlen(prefix);
    if (value.size() >= prefix_len && value.compare(0, prefix_len, prefix) == 0) {
        return value.substr(prefix_len);
    }
    return value;
}

std::vector<std::string> ParseCsv(const std::string& csv) {
    std::vector<std::string> out;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = Trim(token);
        if (!token.empty()) {
            out.push_back(token);
        }
    }
    return out;
}

std::string JoinCsv(const std::vector<std::string>& values) {
    std::string out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out += ",";
        }
        out += values[i];
    }
    return out;
}

const char* GetStringByKeys(const cJSON* obj,
                            const char* key_a,
                            const char* key_b = nullptr,
                            const char* key_c = nullptr) {
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
    if (key_c != nullptr) {
        auto value_c = cJSON_GetObjectItem(obj, key_c);
        if (cJSON_IsString(value_c)) {
            return value_c->valuestring;
        }
    }
    return nullptr;
}

}  // namespace

NostrChannelService& NostrChannelService::GetInstance() {
    static NostrChannelService instance;
    return instance;
}

void NostrChannelService::Initialize(const std::string& device_id, const std::string& board_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }

    device_id_ = device_id;
    board_name_ = board_name;
    LoadLocked();
    initialized_ = true;
}

bool NostrChannelService::NormalizePubkey(const std::string& input, std::string* normalized) const {
    if (normalized == nullptr) {
        return false;
    }
    std::string value = ToLower(Trim(input));
    value = StripPrefix(value, "nostr:");

    if (IsHex64(value)) {
        *normalized = value;
        return true;
    }
    if (value.size() >= 8 && value.rfind("npub1", 0) == 0) {
        for (char ch : value) {
            bool ok = std::isdigit(static_cast<unsigned char>(ch)) ||
                      (ch >= 'a' && ch <= 'z');
            if (!ok) {
                return false;
            }
        }
        *normalized = value;
        return true;
    }
    return false;
}

void NostrChannelService::LoadLocked() {
    Settings settings(kNamespace, false);
    config_.enabled = settings.GetBool(kEnabledKey, true);

    std::string account_id = Trim(settings.GetString(kAccountIdKey, kDefaultAccountId));
    config_.account_id = account_id.empty() ? kDefaultAccountId : account_id;

    config_.allow_pubkeys.clear();
    std::vector<std::string> entries = ParseCsv(settings.GetString(kAllowPubkeysKey));
    for (const auto& entry : entries) {
        std::string normalized;
        if (NormalizePubkey(entry, &normalized)) {
            config_.allow_pubkeys.push_back(normalized);
        }
    }
    std::sort(config_.allow_pubkeys.begin(), config_.allow_pubkeys.end());
    config_.allow_pubkeys.erase(std::unique(config_.allow_pubkeys.begin(), config_.allow_pubkeys.end()),
                                config_.allow_pubkeys.end());

    inbox_.clear();
    std::string inbox_json = settings.GetString(kInboxJsonKey);
    if (inbox_json.empty()) {
        return;
    }

    cJSON* array = cJSON_Parse(inbox_json.c_str());
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

        const char* sender_id = GetStringByKeys(item, "senderId", "sender_id", "from");
        const char* text = GetStringByKeys(item, "text", "content", "message");
        if (sender_id == nullptr || text == nullptr) {
            continue;
        }

        NostrInboxMessage msg;
        const char* local_id = GetStringByKeys(item, "localId", "local_id");
        if (local_id != nullptr) {
            msg.local_id = local_id;
        }
        const char* event_id = GetStringByKeys(item, "eventId", "event_id", "id");
        if (event_id != nullptr) {
            msg.event_id = event_id;
        }
        msg.sender_id = sender_id;

        const char* chat_id = GetStringByKeys(item, "chatId", "chat_id", "conversationId");
        msg.chat_id = chat_id != nullptr ? chat_id : msg.sender_id;

        msg.text = text;
        const char* relay = GetStringByKeys(item, "relay", "relayUrl");
        if (relay != nullptr) {
            msg.relay = relay;
        }

        auto ts = cJSON_GetObjectItem(item, "ts");
        if (cJSON_IsNumber(ts)) {
            msg.ts_ms = static_cast<int64_t>(ts->valuedouble);
        }
        if (msg.ts_ms <= 0) {
            msg.ts_ms = NowMs();
        }
        if (msg.local_id.empty()) {
            msg.local_id = "nostr-" + std::to_string(msg.ts_ms);
        }

        inbox_.push_back(msg);
    }

    cJSON_Delete(array);

    if (inbox_.size() > kInboxMaxCount) {
        inbox_.erase(inbox_.begin(), inbox_.begin() + static_cast<std::ptrdiff_t>(inbox_.size() - kInboxMaxCount));
    }
}

void NostrChannelService::SaveConfigLocked() const {
    Settings settings(kNamespace, true);
    settings.SetBool(kEnabledKey, config_.enabled);
    settings.SetString(kAccountIdKey, config_.account_id);
    settings.SetString(kAllowPubkeysKey, JoinCsv(config_.allow_pubkeys));
}

void NostrChannelService::SaveInboxLocked() const {
    cJSON* array = cJSON_CreateArray();
    for (const auto& msg : inbox_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "localId", msg.local_id.c_str());
        cJSON_AddStringToObject(item, "eventId", msg.event_id.c_str());
        cJSON_AddStringToObject(item, "senderId", msg.sender_id.c_str());
        cJSON_AddStringToObject(item, "chatId", msg.chat_id.c_str());
        cJSON_AddStringToObject(item, "text", msg.text.c_str());
        cJSON_AddStringToObject(item, "relay", msg.relay.c_str());
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(msg.ts_ms));
        cJSON_AddItemToArray(array, item);
    }

    char* raw = cJSON_PrintUnformatted(array);
    std::string payload = raw != nullptr ? raw : "[]";
    if (raw != nullptr) {
        cJSON_free(raw);
    }
    cJSON_Delete(array);

    Settings settings(kNamespace, true);
    settings.SetString(kInboxJsonKey, payload);
}

bool NostrChannelService::UpdateConfig(bool enabled,
                                       const std::string& account_id,
                                       const std::string& allow_pubkeys_csv,
                                       std::string* error_message) {
    std::string clean_account_id = Trim(account_id);
    if (clean_account_id.empty()) {
        clean_account_id = kDefaultAccountId;
    }

    std::vector<std::string> normalized;
    std::vector<std::string> raw_entries = ParseCsv(allow_pubkeys_csv);
    for (const auto& raw : raw_entries) {
        std::string value;
        if (!NormalizePubkey(raw, &value)) {
            if (error_message != nullptr) {
                *error_message = "allow_pubkeys 包含非法 pubkey: " + raw;
            }
            return false;
        }
        normalized.push_back(value);
    }
    std::sort(normalized.begin(), normalized.end());
    normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());

    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabled = enabled;
    config_.account_id = clean_account_id;
    config_.allow_pubkeys = normalized;
    SaveConfigLocked();
    return true;
}

NostrChannelService::ParseResult NostrChannelService::ParseIncomingEventLocked(
    const cJSON* root,
    NostrInboxMessage* message,
    std::string* error_message) const {
    if (!cJSON_IsObject(root)) {
        return ParseResult::kNotNostrEvent;
    }

    const char* type = GetStringByKeys(root, "type");
    if (type == nullptr || strcmp(type, "channel_event") != 0) {
        return ParseResult::kNotNostrEvent;
    }

    const cJSON* payload = cJSON_GetObjectItem(root, "payload");
    if (!cJSON_IsObject(payload)) {
        payload = root;
    }

    const char* channel = GetStringByKeys(payload, "channel", "channelId", "platform");
    if (channel == nullptr) {
        channel = GetStringByKeys(root, "channel", "channelId", "platform");
    }
    if (channel == nullptr || ToLower(channel) != "nostr") {
        return ParseResult::kNotNostrEvent;
    }

    const char* event = GetStringByKeys(root, "event", "name");
    if (event != nullptr) {
        std::string event_name = ToLower(event);
        if (event_name != "message_received" && event_name != "dm_received") {
            return ParseResult::kNotNostrEvent;
        }
    }

    if (message == nullptr) {
        if (error_message != nullptr) {
            *error_message = "内部错误: message 指针为空";
        }
        return ParseResult::kInvalid;
    }

    const char* sender = GetStringByKeys(payload, "sender_id", "senderId", "from");
    const char* text = GetStringByKeys(payload, "content", "text", "message");
    if (sender == nullptr || text == nullptr) {
        if (error_message != nullptr) {
            *error_message = "nostr channel_event 缺少 sender_id 或 text";
        }
        return ParseResult::kInvalid;
    }

    message->sender_id = sender;
    message->text = text;

    const char* event_id = GetStringByKeys(payload, "event_id", "eventId", "id");
    if (event_id != nullptr) {
        message->event_id = event_id;
    }

    const char* chat_id = GetStringByKeys(payload, "chat_id", "chatId", "conversationId");
    message->chat_id = chat_id != nullptr ? chat_id : message->sender_id;

    const char* relay = GetStringByKeys(payload, "relay", "relayUrl");
    if (relay != nullptr) {
        message->relay = relay;
    }

    auto ts = cJSON_GetObjectItem(payload, "ts");
    if (cJSON_IsNumber(ts)) {
        message->ts_ms = static_cast<int64_t>(ts->valuedouble);
    }
    if (message->ts_ms <= 0) {
        message->ts_ms = NowMs();
    }

    return ParseResult::kValid;
}

bool NostrChannelService::IsSenderAllowedLocked(const std::string& sender_id) const {
    if (config_.allow_pubkeys.empty()) {
        return true;
    }
    std::string normalized_sender;
    if (!NormalizePubkey(sender_id, &normalized_sender)) {
        return false;
    }
    return std::find(config_.allow_pubkeys.begin(),
                     config_.allow_pubkeys.end(),
                     normalized_sender) != config_.allow_pubkeys.end();
}

bool NostrChannelService::ConsumeIncomingEvent(const cJSON* root,
                                               bool* cached,
                                               std::string* error_message) {
    if (cached != nullptr) {
        *cached = false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    NostrInboxMessage message;
    ParseResult parse_result = ParseIncomingEventLocked(root, &message, error_message);
    if (parse_result == ParseResult::kNotNostrEvent) {
        return false;
    }

    stats_.inbound_total += 1;

    if (parse_result == ParseResult::kInvalid) {
        stats_.dropped_invalid += 1;
        return true;
    }

    if (!config_.enabled) {
        stats_.dropped_disabled += 1;
        return true;
    }

    if (!IsSenderAllowedLocked(message.sender_id)) {
        stats_.dropped_not_allowed += 1;
        return true;
    }

    if (!message.event_id.empty()) {
        auto dup = std::find_if(inbox_.begin(), inbox_.end(), [&message](const NostrInboxMessage& item) {
            return !item.event_id.empty() && item.event_id == message.event_id;
        });
        if (dup != inbox_.end()) {
            *dup = message;
            stats_.duplicate_merged += 1;
            SaveInboxLocked();
            if (cached != nullptr) {
                *cached = true;
            }
            return true;
        }
    }

    local_id_seed_ += 1;
    message.local_id = "nostr-" + std::to_string(local_id_seed_);
    inbox_.push_back(message);
    while (inbox_.size() > kInboxMaxCount) {
        inbox_.erase(inbox_.begin());
    }

    stats_.cached_total += 1;
    SaveInboxLocked();

    if (cached != nullptr) {
        *cached = true;
    }
    return true;
}

bool NostrChannelService::ClearInbox() {
    std::lock_guard<std::mutex> lock(mutex_);
    inbox_.clear();
    SaveInboxLocked();
    return true;
}

cJSON* NostrChannelService::BuildSendDmPayloadJson(const std::string& to,
                                                   const std::string& text,
                                                   const std::string& request_id,
                                                   std::string* error_message) const {
    std::string normalized_to;
    if (!NormalizePubkey(to, &normalized_to)) {
        if (error_message != nullptr) {
            *error_message = "to 必须是 64 位十六进制 pubkey 或 npub1...";
        }
        return nullptr;
    }

    std::string clean_text = Trim(text);
    if (clean_text.empty()) {
        if (error_message != nullptr) {
            *error_message = "text 不能为空";
        }
        return nullptr;
    }

    if (clean_text.size() > 4000) {
        if (error_message != nullptr) {
            *error_message = "text 长度不能超过 4000";
        }
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "to", normalized_to.c_str());
    cJSON_AddStringToObject(payload, "text", clean_text.c_str());
    cJSON_AddStringToObject(payload, "accountId", config_.account_id.c_str());
    cJSON_AddStringToObject(payload, "deviceId", device_id_.c_str());
    cJSON_AddStringToObject(payload, "board", board_name_.c_str());

    std::string clean_request_id = Trim(request_id);
    if (!clean_request_id.empty()) {
        cJSON_AddStringToObject(payload, "requestId", clean_request_id.c_str());
    }
    return payload;
}

cJSON* NostrChannelService::ExportConfigJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "enabled", config_.enabled);
    cJSON_AddStringToObject(root, "accountId", config_.account_id.c_str());

    cJSON* allow = cJSON_CreateArray();
    for (const auto& entry : config_.allow_pubkeys) {
        cJSON_AddItemToArray(allow, cJSON_CreateString(entry.c_str()));
    }
    cJSON_AddItemToObject(root, "allowPubkeys", allow);
    return root;
}

cJSON* NostrChannelService::ExportStatsJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "inboundTotal", static_cast<double>(stats_.inbound_total));
    cJSON_AddNumberToObject(root, "cachedTotal", static_cast<double>(stats_.cached_total));
    cJSON_AddNumberToObject(root, "droppedInvalid", static_cast<double>(stats_.dropped_invalid));
    cJSON_AddNumberToObject(root, "droppedNotAllowed", static_cast<double>(stats_.dropped_not_allowed));
    cJSON_AddNumberToObject(root, "droppedDisabled", static_cast<double>(stats_.dropped_disabled));
    cJSON_AddNumberToObject(root, "duplicateMerged", static_cast<double>(stats_.duplicate_merged));
    return root;
}

cJSON* NostrChannelService::ExportInboxJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "count", static_cast<double>(inbox_.size()));

    cJSON* messages = cJSON_CreateArray();
    for (const auto& msg : inbox_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "localId", msg.local_id.c_str());
        cJSON_AddStringToObject(item, "eventId", msg.event_id.c_str());
        cJSON_AddStringToObject(item, "senderId", msg.sender_id.c_str());
        cJSON_AddStringToObject(item, "chatId", msg.chat_id.c_str());
        cJSON_AddStringToObject(item, "text", msg.text.c_str());
        cJSON_AddStringToObject(item, "relay", msg.relay.c_str());
        cJSON_AddNumberToObject(item, "ts", static_cast<double>(msg.ts_ms));
        cJSON_AddItemToArray(messages, item);
    }
    cJSON_AddItemToObject(root, "messages", messages);
    return root;
}

}  // namespace octo::channels
