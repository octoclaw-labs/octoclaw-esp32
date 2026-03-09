#include "mattermost_channel_service.h"

#include "settings.h"

#include <esp_timer.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <sstream>

namespace octo::channels {

namespace {

constexpr const char* kNamespace = "octo_mattermost";
constexpr const char* kEnabledKey = "enabled";
constexpr const char* kAccountIdKey = "account_id";
constexpr const char* kAllowSendersKey = "allow_senders";
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
                            const char* key_c = nullptr,
                            const char* key_d = nullptr) {
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

    if (key_d != nullptr) {
        auto value_d = cJSON_GetObjectItem(obj, key_d);
        if (cJSON_IsString(value_d)) {
            return value_d->valuestring;
        }
    }

    return nullptr;
}

}  // namespace

MattermostChannelService& MattermostChannelService::GetInstance() {
    static MattermostChannelService instance;
    return instance;
}

void MattermostChannelService::Initialize(const std::string& device_id, const std::string& board_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }

    device_id_ = device_id;
    board_name_ = board_name;
    LoadLocked();
    initialized_ = true;
}

std::string MattermostChannelService::NormalizeSender(const std::string& raw) const {
    std::string value = ToLower(Trim(raw));
    if (value.rfind("mattermost:", 0) == 0) {
        value.erase(0, strlen("mattermost:"));
    }
    if (value.rfind("user:", 0) == 0) {
        value.erase(0, strlen("user:"));
    }
    if (!value.empty() && value.front() == '@') {
        value.erase(0, 1);
    }
    return Trim(value);
}

void MattermostChannelService::LoadLocked() {
    Settings settings(kNamespace, false);
    config_.enabled = settings.GetBool(kEnabledKey, true);

    std::string account_id = Trim(settings.GetString(kAccountIdKey, kDefaultAccountId));
    config_.account_id = account_id.empty() ? kDefaultAccountId : account_id;

    config_.allow_senders.clear();
    std::vector<std::string> raw = ParseCsv(settings.GetString(kAllowSendersKey));
    for (const auto& item : raw) {
        std::string normalized = NormalizeSender(item);
        if (!normalized.empty()) {
            config_.allow_senders.push_back(normalized);
        }
    }
    std::sort(config_.allow_senders.begin(), config_.allow_senders.end());
    config_.allow_senders.erase(std::unique(config_.allow_senders.begin(), config_.allow_senders.end()),
                                config_.allow_senders.end());

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

        const char* sender = GetStringByKeys(item, "senderId", "sender_id", "from");
        const char* text = GetStringByKeys(item, "text", "content", "message");
        if (sender == nullptr || text == nullptr) {
            continue;
        }

        MattermostInboxMessage msg;
        const char* local_id = GetStringByKeys(item, "localId", "local_id");
        if (local_id != nullptr) {
            msg.local_id = local_id;
        }
        const char* message_id = GetStringByKeys(item, "messageId", "message_id", "postId", "post_id");
        if (message_id != nullptr) {
            msg.message_id = message_id;
        }
        msg.sender_id = sender;

        const char* channel_id = GetStringByKeys(item, "channelId", "channel_id", "chatId", "conversationId");
        if (channel_id != nullptr) {
            msg.channel_id = channel_id;
        }
        const char* thread_id = GetStringByKeys(item, "threadId", "thread_id", "rootId", "root_id");
        if (thread_id != nullptr) {
            msg.thread_id = thread_id;
        }
        msg.text = text;

        auto ts = cJSON_GetObjectItem(item, "ts");
        if (cJSON_IsNumber(ts)) {
            msg.ts_ms = static_cast<int64_t>(ts->valuedouble);
        }
        if (msg.ts_ms <= 0) {
            msg.ts_ms = NowMs();
        }

        if (msg.local_id.empty()) {
            msg.local_id = "mattermost-" + std::to_string(msg.ts_ms);
        }

        inbox_.push_back(msg);
    }
    cJSON_Delete(array);

    if (inbox_.size() > kInboxMaxCount) {
        inbox_.erase(inbox_.begin(), inbox_.begin() + static_cast<std::ptrdiff_t>(inbox_.size() - kInboxMaxCount));
    }
    local_seq_ = static_cast<uint32_t>(inbox_.size());
}

void MattermostChannelService::SaveConfigLocked() const {
    Settings settings(kNamespace, true);
    settings.SetBool(kEnabledKey, config_.enabled);
    settings.SetString(kAccountIdKey, config_.account_id);
    settings.SetString(kAllowSendersKey, JoinCsv(config_.allow_senders));
}

void MattermostChannelService::SaveInboxLocked() const {
    cJSON* array = cJSON_CreateArray();
    for (const auto& msg : inbox_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "localId", msg.local_id.c_str());
        cJSON_AddStringToObject(item, "messageId", msg.message_id.c_str());
        cJSON_AddStringToObject(item, "senderId", msg.sender_id.c_str());
        cJSON_AddStringToObject(item, "channelId", msg.channel_id.c_str());
        cJSON_AddStringToObject(item, "threadId", msg.thread_id.c_str());
        cJSON_AddStringToObject(item, "text", msg.text.c_str());
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

bool MattermostChannelService::UpdateConfig(bool enabled,
                                            const std::string& account_id,
                                            const std::string& allow_senders_csv,
                                            std::string* error_message) {
    std::vector<std::string> normalized;
    for (const auto& raw : ParseCsv(allow_senders_csv)) {
        std::string sender = NormalizeSender(raw);
        if (sender.empty()) {
            if (error_message != nullptr) {
                *error_message = "allow_senders 包含空白项";
            }
            return false;
        }
        normalized.push_back(sender);
    }
    std::sort(normalized.begin(), normalized.end());
    normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());

    std::string clean_account_id = Trim(account_id);
    if (clean_account_id.empty()) {
        clean_account_id = kDefaultAccountId;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabled = enabled;
    config_.account_id = clean_account_id;
    config_.allow_senders = normalized;
    SaveConfigLocked();
    return true;
}

MattermostChannelService::ParseResult MattermostChannelService::ParseIncomingEventLocked(
    const cJSON* root,
    MattermostInboxMessage* message,
    std::string* error_message) const {
    if (!cJSON_IsObject(root)) {
        return ParseResult::kNotMattermostEvent;
    }

    const char* type = GetStringByKeys(root, "type");
    if (type == nullptr || strcmp(type, "channel_event") != 0) {
        return ParseResult::kNotMattermostEvent;
    }

    const cJSON* payload = cJSON_GetObjectItem(root, "payload");
    if (!cJSON_IsObject(payload)) {
        payload = root;
    }

    const char* channel = GetStringByKeys(payload, "channel", "channelType", "platform");
    if (channel == nullptr) {
        channel = GetStringByKeys(root, "channel", "channelType", "platform");
    }
    if (channel == nullptr || ToLower(channel) != "mattermost") {
        return ParseResult::kNotMattermostEvent;
    }

    const char* event = GetStringByKeys(root, "event", "name");
    if (event != nullptr) {
        std::string event_name = ToLower(event);
        if (event_name != "message_received") {
            return ParseResult::kNotMattermostEvent;
        }
    }

    if (message == nullptr) {
        if (error_message != nullptr) {
            *error_message = "内部错误: message 指针为空";
        }
        return ParseResult::kInvalid;
    }

    const char* sender = GetStringByKeys(payload, "sender_id", "senderId", "from", "user_id");
    const char* text = GetStringByKeys(payload, "content", "text", "message");
    if (sender == nullptr || text == nullptr) {
        if (error_message != nullptr) {
            *error_message = "mattermost channel_event 缺少 sender_id 或 text";
        }
        return ParseResult::kInvalid;
    }

    message->sender_id = sender;
    message->text = text;

    const char* message_id = GetStringByKeys(payload, "message_id", "messageId", "post_id", "postId");
    if (message_id != nullptr) {
        message->message_id = message_id;
    }
    const char* channel_id = GetStringByKeys(payload, "channel_id", "channelId", "chat_id", "chatId");
    if (channel_id != nullptr) {
        message->channel_id = channel_id;
    }
    const char* thread_id = GetStringByKeys(payload, "thread_id", "threadId", "root_id", "rootId");
    if (thread_id != nullptr) {
        message->thread_id = thread_id;
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

bool MattermostChannelService::IsSenderAllowedLocked(const std::string& sender_id) const {
    if (config_.allow_senders.empty()) {
        return true;
    }
    std::string sender = NormalizeSender(sender_id);
    if (sender.empty()) {
        return false;
    }
    return std::find(config_.allow_senders.begin(), config_.allow_senders.end(), sender) !=
           config_.allow_senders.end();
}

bool MattermostChannelService::ConsumeIncomingEvent(const cJSON* root,
                                                    bool* cached,
                                                    std::string* error_message) {
    if (cached != nullptr) {
        *cached = false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    MattermostInboxMessage message;
    ParseResult parsed = ParseIncomingEventLocked(root, &message, error_message);
    if (parsed == ParseResult::kNotMattermostEvent) {
        return false;
    }

    stats_.inbound_total += 1;

    if (parsed == ParseResult::kInvalid) {
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

    if (!message.message_id.empty()) {
        auto it = std::find_if(inbox_.begin(), inbox_.end(), [&message](const MattermostInboxMessage& item) {
            return !item.message_id.empty() && item.message_id == message.message_id;
        });
        if (it != inbox_.end()) {
            *it = message;
            stats_.duplicate_merged += 1;
            SaveInboxLocked();
            if (cached != nullptr) {
                *cached = true;
            }
            return true;
        }
    }

    local_seq_ += 1;
    message.local_id = "mattermost-" + std::to_string(local_seq_);
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

bool MattermostChannelService::ClearInbox() {
    std::lock_guard<std::mutex> lock(mutex_);
    inbox_.clear();
    SaveInboxLocked();
    return true;
}

cJSON* MattermostChannelService::BuildSendMessagePayloadJson(const std::string& to,
                                                             const std::string& text,
                                                             const std::string& request_id,
                                                             std::string* error_message) const {
    std::string clean_to = Trim(to);
    if (clean_to.empty()) {
        if (error_message != nullptr) {
            *error_message = "to 不能为空";
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
    cJSON_AddStringToObject(payload, "to", clean_to.c_str());
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

cJSON* MattermostChannelService::ExportConfigJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "enabled", config_.enabled);
    cJSON_AddStringToObject(root, "accountId", config_.account_id.c_str());

    cJSON* allow = cJSON_CreateArray();
    for (const auto& item : config_.allow_senders) {
        cJSON_AddItemToArray(allow, cJSON_CreateString(item.c_str()));
    }
    cJSON_AddItemToObject(root, "allowSenders", allow);
    return root;
}

cJSON* MattermostChannelService::ExportStatsJson() const {
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

cJSON* MattermostChannelService::ExportInboxJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "count", static_cast<double>(inbox_.size()));

    cJSON* messages = cJSON_CreateArray();
    for (const auto& item : inbox_) {
        cJSON* node = cJSON_CreateObject();
        cJSON_AddStringToObject(node, "localId", item.local_id.c_str());
        cJSON_AddStringToObject(node, "messageId", item.message_id.c_str());
        cJSON_AddStringToObject(node, "senderId", item.sender_id.c_str());
        cJSON_AddStringToObject(node, "channelId", item.channel_id.c_str());
        cJSON_AddStringToObject(node, "threadId", item.thread_id.c_str());
        cJSON_AddStringToObject(node, "text", item.text.c_str());
        cJSON_AddNumberToObject(node, "ts", static_cast<double>(item.ts_ms));
        cJSON_AddItemToArray(messages, node);
    }
    cJSON_AddItemToObject(root, "messages", messages);
    return root;
}

}  // namespace octo::channels
