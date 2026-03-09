#include "thread_ownership_service.h"

#include "board.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <utility>

#define TAG "thread_ownership"

namespace octo::channels {

namespace {

constexpr const char* kNamespace = "octo_thread";
constexpr const char* kEnabledKey = "enabled";
constexpr const char* kForwarderUrlKey = "forwarder_url";
constexpr const char* kAbChannelsKey = "ab_channels";
constexpr const char* kBotUserIdKey = "bot_user_id";
constexpr const char* kAgentIdKey = "agent_id";
constexpr const char* kAgentNameKey = "agent_name";

constexpr const char* kDefaultForwarderUrl = "http://slack-forwarder:8750";
constexpr int64_t kMentionTtlMs = 5 * 60 * 1000;
constexpr size_t kMentionMaxCount = 64;

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

bool StartsWith(const std::string& text, const char* prefix) {
    const size_t len = strlen(prefix);
    return text.size() >= len && text.compare(0, len, prefix) == 0;
}

std::string NormalizeForwarderUrl(const std::string& raw) {
    std::string url = Trim(raw);
    if (url.empty()) {
        return "";
    }
    if (url.find("://") == std::string::npos) {
        url = "http://" + url;
    }
    if (!StartsWith(url, "http://") && !StartsWith(url, "https://")) {
        return "";
    }
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

std::vector<std::string> ParseCsv(const std::string& csv) {
    std::vector<std::string> values;
    std::string token;
    std::stringstream ss(csv);
    while (std::getline(ss, token, ',')) {
        token = Trim(token);
        if (token.empty()) {
            continue;
        }
        values.push_back(token);
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

std::string JoinCsv(const std::vector<std::string>& values) {
    std::string result;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += values[i];
    }
    return result;
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

std::string UrlEncodePathSegment(const std::string& input) {
    static const char* kHex = "0123456789ABCDEF";
    std::string output;
    output.reserve(input.size() * 3);
    for (unsigned char ch : input) {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            output.push_back(static_cast<char>(ch));
        } else {
            output.push_back('%');
            output.push_back(kHex[(ch >> 4) & 0x0F]);
            output.push_back(kHex[ch & 0x0F]);
        }
    }
    return output;
}

}  // namespace

ThreadOwnershipService& ThreadOwnershipService::GetInstance() {
    static ThreadOwnershipService instance;
    return instance;
}

void ThreadOwnershipService::Initialize(const std::string& default_agent_id,
                                        const std::string& default_agent_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }
    LoadLocked(default_agent_id, default_agent_name);
    initialized_ = true;
}

void ThreadOwnershipService::LoadLocked(const std::string& default_agent_id,
                                        const std::string& default_agent_name) {
    Settings settings(kNamespace, false);
    config_.enabled = settings.GetBool(kEnabledKey, true);

    std::string stored_forwarder = settings.GetString(kForwarderUrlKey, kDefaultForwarderUrl);
    config_.forwarder_url = NormalizeForwarderUrl(stored_forwarder);
    if (config_.forwarder_url.empty()) {
        config_.forwarder_url = kDefaultForwarderUrl;
    }

    config_.ab_test_channels = ParseCsv(settings.GetString(kAbChannelsKey));
    config_.bot_user_id = Trim(settings.GetString(kBotUserIdKey));

    config_.agent_id = Trim(settings.GetString(kAgentIdKey));
    if (config_.agent_id.empty()) {
        config_.agent_id = Trim(default_agent_id);
    }

    config_.agent_name = Trim(settings.GetString(kAgentNameKey));
    if (config_.agent_name.empty()) {
        config_.agent_name = Trim(default_agent_name);
    }
}

void ThreadOwnershipService::SaveLocked() const {
    Settings settings(kNamespace, true);
    settings.SetBool(kEnabledKey, config_.enabled);
    settings.SetString(kForwarderUrlKey, config_.forwarder_url);
    settings.SetString(kAbChannelsKey, JoinCsv(config_.ab_test_channels));
    settings.SetString(kBotUserIdKey, config_.bot_user_id);
    settings.SetString(kAgentIdKey, config_.agent_id);
    settings.SetString(kAgentNameKey, config_.agent_name);
}

bool ThreadOwnershipService::UpdateConfig(const std::string& forwarder_url,
                                          const std::string& ab_test_channels_csv,
                                          const std::string& bot_user_id,
                                          const std::string& agent_id,
                                          const std::string& agent_name,
                                          bool enabled,
                                          std::string* error_message) {
    std::string normalized_url = NormalizeForwarderUrl(forwarder_url);
    if (!Trim(forwarder_url).empty() && normalized_url.empty()) {
        if (error_message != nullptr) {
            *error_message = "forwarder_url 必须是 http/https URL";
        }
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!normalized_url.empty()) {
        config_.forwarder_url = normalized_url;
    }
    config_.ab_test_channels = ParseCsv(ab_test_channels_csv);
    config_.bot_user_id = Trim(bot_user_id);

    std::string trimmed_agent_id = Trim(agent_id);
    if (!trimmed_agent_id.empty()) {
        config_.agent_id = trimmed_agent_id;
    }
    std::string trimmed_agent_name = Trim(agent_name);
    if (!trimmed_agent_name.empty()) {
        config_.agent_name = trimmed_agent_name;
    }

    config_.enabled = enabled;
    SaveLocked();
    return true;
}

void ThreadOwnershipService::CleanExpiredMentionsLocked(int64_t now_ms) {
    mentions_.erase(std::remove_if(mentions_.begin(), mentions_.end(), [now_ms](const MentionEntry& item) {
        return now_ms - item.ts_ms > kMentionTtlMs;
    }), mentions_.end());
}

bool ThreadOwnershipService::MatchMentionLocked(const std::string& text) const {
    if (text.empty()) {
        return false;
    }
    if (!config_.agent_name.empty()) {
        std::string marker = "@" + config_.agent_name;
        if (text.find(marker) != std::string::npos) {
            return true;
        }
    }
    if (!config_.bot_user_id.empty()) {
        std::string marker = "<@" + config_.bot_user_id + ">";
        if (text.find(marker) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool ThreadOwnershipService::IsTrackedMentionLocked(const std::string& channel_id,
                                                    const std::string& thread_ts) const {
    const std::string key = channel_id + ":" + thread_ts;
    return std::any_of(mentions_.begin(), mentions_.end(), [&key](const MentionEntry& item) {
        return item.key == key;
    });
}

void ThreadOwnershipService::UpsertMentionLocked(const std::string& channel_id,
                                                 const std::string& thread_ts,
                                                 int64_t now_ms) {
    const std::string key = channel_id + ":" + thread_ts;
    auto it = std::find_if(mentions_.begin(), mentions_.end(), [&key](const MentionEntry& item) {
        return item.key == key;
    });
    if (it != mentions_.end()) {
        it->ts_ms = now_ms;
        return;
    }

    MentionEntry entry;
    entry.key = key;
    entry.ts_ms = now_ms;
    mentions_.push_back(entry);
    if (mentions_.size() > kMentionMaxCount) {
        auto oldest = std::min_element(mentions_.begin(), mentions_.end(), [](const MentionEntry& lhs,
                                                                              const MentionEntry& rhs) {
            return lhs.ts_ms < rhs.ts_ms;
        });
        if (oldest != mentions_.end()) {
            mentions_.erase(oldest);
        }
    }
}

bool ThreadOwnershipService::TrackMention(const std::string& channel_type,
                                          const std::string& channel_id,
                                          const std::string& thread_ts,
                                          const std::string& text) {
    if (ToLower(Trim(channel_type)) != "slack") {
        return false;
    }
    if (channel_id.empty() || thread_ts.empty() || text.empty()) {
        return false;
    }

    const int64_t now_ms = NowMs();
    std::lock_guard<std::mutex> lock(mutex_);
    CleanExpiredMentionsLocked(now_ms);
    if (!MatchMentionLocked(text)) {
        return false;
    }

    UpsertMentionLocked(channel_id, thread_ts, now_ms);
    stats_.mention_tracked += 1;
    return true;
}

bool ThreadOwnershipService::ParseIncomingEvent(const cJSON* root,
                                                std::string* channel_type,
                                                std::string* channel_id,
                                                std::string* thread_ts,
                                                std::string* text,
                                                std::string* error_message) const {
    if (!cJSON_IsObject(root)) {
        return false;
    }

    const char* type = GetStringByKeys(root, "type");
    bool maybe_event = false;
    const cJSON* payload = root;

    if (type != nullptr && strcmp(type, "channel_event") == 0) {
        maybe_event = true;
        const char* event_name = GetStringByKeys(root, "event", "name");
        if (event_name != nullptr && strcmp(event_name, "message_received") != 0) {
            return false;
        }
        auto payload_json = cJSON_GetObjectItem(root, "payload");
        if (cJSON_IsObject(payload_json)) {
            payload = payload_json;
        }
    } else if (type != nullptr && strcmp(type, "message_received") == 0) {
        maybe_event = true;
    } else {
        return false;
    }

    const cJSON* metadata = cJSON_GetObjectItem(payload, "metadata");
    if (!cJSON_IsObject(metadata)) {
        metadata = nullptr;
    }

    const char* channel_type_raw = GetStringByKeys(payload, "channel", "channelType", "platform");
    if (channel_type_raw == nullptr) {
        channel_type_raw = GetStringByKeys(root, "channel", "channelType", "platform");
    }
    if (channel_type_raw == nullptr && metadata != nullptr) {
        channel_type_raw = GetStringByKeys(metadata, "channel", "channelType", "platform");
    }
    std::string normalized_channel = channel_type_raw != nullptr ? ToLower(Trim(channel_type_raw)) : "slack";
    if (normalized_channel != "slack") {
        return false;
    }

    const char* channel_id_raw = GetStringByKeys(payload, "channel_id", "channelId", "to");
    if (channel_id_raw == nullptr && metadata != nullptr) {
        channel_id_raw = GetStringByKeys(metadata, "channelId", "channel_id", "conversationId");
    }
    if (channel_id_raw == nullptr) {
        channel_id_raw = GetStringByKeys(root, "channel_id", "channelId", "conversationId");
    }

    const char* thread_ts_raw = GetStringByKeys(payload, "thread_ts", "threadTs");
    if (thread_ts_raw == nullptr && metadata != nullptr) {
        thread_ts_raw = GetStringByKeys(metadata, "threadTs", "thread_ts");
    }

    const char* text_raw = GetStringByKeys(payload, "content", "text");

    if (maybe_event && (channel_id_raw == nullptr || thread_ts_raw == nullptr || text_raw == nullptr)) {
        if (error_message != nullptr) {
            *error_message = "channel_event 缺少 channel_id/thread_ts/text";
        }
        return false;
    }

    if (channel_type != nullptr) {
        *channel_type = normalized_channel;
    }
    if (channel_id != nullptr) {
        *channel_id = channel_id_raw != nullptr ? channel_id_raw : "";
    }
    if (thread_ts != nullptr) {
        *thread_ts = thread_ts_raw != nullptr ? thread_ts_raw : "";
    }
    if (text != nullptr) {
        *text = text_raw != nullptr ? text_raw : "";
    }
    return true;
}

bool ThreadOwnershipService::ConsumeIncomingEvent(const cJSON* root,
                                                  bool* tracked,
                                                  std::string* error_message) {
    if (tracked != nullptr) {
        *tracked = false;
    }

    std::string channel_type;
    std::string channel_id;
    std::string thread_ts;
    std::string text;
    if (!ParseIncomingEvent(root, &channel_type, &channel_id, &thread_ts, &text, error_message)) {
        return false;
    }

    bool tracked_local = TrackMention(channel_type, channel_id, thread_ts, text);
    if (tracked != nullptr) {
        *tracked = tracked_local;
    }
    return true;
}

ThreadOwnershipDecision ThreadOwnershipService::ClaimOwnership(const std::string& forwarder_url,
                                                               const std::string& channel_id,
                                                               const std::string& thread_ts,
                                                               const std::string& agent_id) const {
    ThreadOwnershipDecision decision;
    decision.allow = true;
    decision.checked = false;

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        decision.fail_open = true;
        decision.reason = "network_unavailable";
        return decision;
    }

    auto http = network->CreateHttp(3);
    if (!http) {
        decision.fail_open = true;
        decision.reason = "http_client_unavailable";
        return decision;
    }

    cJSON* body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "agent_id", agent_id.c_str());
    char* raw = cJSON_PrintUnformatted(body);
    std::string request_body = raw != nullptr ? raw : "{}";
    if (raw != nullptr) {
        cJSON_free(raw);
    }
    cJSON_Delete(body);

    std::string url = forwarder_url + "/api/v1/ownership/" + UrlEncodePathSegment(channel_id) + "/" + UrlEncodePathSegment(thread_ts);

    http->SetHeader("Content-Type", "application/json");
    http->SetContent(std::move(request_body));

    if (!http->Open("POST", url)) {
        decision.fail_open = true;
        decision.reason = "http_open_failed";
        return decision;
    }

    decision.status_code = http->GetStatusCode();
    std::string response_body = http->ReadAll();
    http->Close();

    decision.checked = true;

    if (decision.status_code >= 200 && decision.status_code < 300) {
        decision.allow = true;
        decision.reason = "claimed_or_owned";
        return decision;
    }

    if (decision.status_code == 409) {
        decision.allow = false;
        decision.cancelled = true;
        decision.reason = "owned_by_other";

        cJSON* parsed = cJSON_Parse(response_body.c_str());
        if (cJSON_IsObject(parsed)) {
            const char* owner = GetStringByKeys(parsed, "owner");
            if (owner != nullptr) {
                decision.owner = owner;
            }
        }
        if (parsed != nullptr) {
            cJSON_Delete(parsed);
        }
        return decision;
    }

    decision.allow = true;
    decision.fail_open = true;
    decision.reason = "unexpected_status";
    return decision;
}

ThreadOwnershipDecision ThreadOwnershipService::CheckBeforeSend(const std::string& channel_type,
                                                                const std::string& channel_id,
                                                                const std::string& thread_ts,
                                                                const std::string& text) {
    ThreadOwnershipDecision decision;
    decision.allow = true;

    ConfigSnapshot config;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.checks_total += 1;
        config = config_;
    }

    if (!config.enabled) {
        decision.reason = "disabled";
    } else if (ToLower(Trim(channel_type)) != "slack") {
        decision.reason = "channel_not_slack";
    } else if (thread_ts.empty()) {
        decision.reason = "top_level_message";
    } else if (channel_id.empty()) {
        decision.reason = "missing_channel_id";
    } else if (!config.ab_test_channels.empty() &&
               std::find(config.ab_test_channels.begin(), config.ab_test_channels.end(), channel_id) == config.ab_test_channels.end()) {
        decision.reason = "not_in_ab_test";
    } else {
        const int64_t now_ms = NowMs();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            CleanExpiredMentionsLocked(now_ms);
            if (IsTrackedMentionLocked(channel_id, thread_ts)) {
                decision.reason = "mentioned_recently";
                stats_.checks_allowed += 1;
                return decision;
            }
        }

        if (config.forwarder_url.empty()) {
            decision.fail_open = true;
            decision.reason = "forwarder_not_configured";
        } else {
            std::string claim_agent_id = config.agent_id.empty() ? std::string("unknown") : config.agent_id;
            decision = ClaimOwnership(config.forwarder_url, channel_id, thread_ts, claim_agent_id);
            if (decision.checked && decision.cancelled) {
                ESP_LOGI(TAG,
                         "Cancel send to %s:%s, owned by %s",
                         channel_id.c_str(),
                         thread_ts.c_str(),
                         decision.owner.c_str());
            } else if (decision.fail_open) {
                ESP_LOGW(TAG,
                         "Ownership check fail-open: channel=%s thread=%s reason=%s",
                         channel_id.c_str(),
                         thread_ts.c_str(),
                         decision.reason.c_str());
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (decision.cancelled) {
            stats_.checks_cancelled += 1;
        } else {
            stats_.checks_allowed += 1;
        }
        if (decision.fail_open) {
            stats_.checks_fail_open += 1;
            if (decision.reason == "http_open_failed" ||
                decision.reason == "network_unavailable" ||
                decision.reason == "http_client_unavailable") {
                stats_.http_errors += 1;
            }
        }
    }

    (void)text;
    return decision;
}

cJSON* ThreadOwnershipService::ExportConfigJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, "enabled", config_.enabled);
    cJSON_AddStringToObject(root, "forwarderUrl", config_.forwarder_url.c_str());
    cJSON_AddStringToObject(root, "botUserId", config_.bot_user_id.c_str());
    cJSON_AddStringToObject(root, "agentId", config_.agent_id.c_str());
    cJSON_AddStringToObject(root, "agentName", config_.agent_name.c_str());
    cJSON_AddNumberToObject(root, "mentionTtlMs", static_cast<double>(kMentionTtlMs));

    cJSON* channels = cJSON_CreateArray();
    for (const auto& channel : config_.ab_test_channels) {
        cJSON_AddItemToArray(channels, cJSON_CreateString(channel.c_str()));
    }
    cJSON_AddItemToObject(root, "abTestChannels", channels);

    cJSON_AddNumberToObject(root, "trackedMentionCount", static_cast<double>(mentions_.size()));
    return root;
}

cJSON* ThreadOwnershipService::ExportRuntimeJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "mentionTracked", static_cast<double>(stats_.mention_tracked));
    cJSON_AddNumberToObject(root, "checksTotal", static_cast<double>(stats_.checks_total));
    cJSON_AddNumberToObject(root, "checksAllowed", static_cast<double>(stats_.checks_allowed));
    cJSON_AddNumberToObject(root, "checksCancelled", static_cast<double>(stats_.checks_cancelled));
    cJSON_AddNumberToObject(root, "checksFailOpen", static_cast<double>(stats_.checks_fail_open));
    cJSON_AddNumberToObject(root, "httpErrors", static_cast<double>(stats_.http_errors));

    cJSON* mentions = cJSON_CreateArray();
    for (const auto& item : mentions_) {
        cJSON* node = cJSON_CreateObject();
        cJSON_AddStringToObject(node, "key", item.key.c_str());
        cJSON_AddNumberToObject(node, "ts", static_cast<double>(item.ts_ms));
        cJSON_AddItemToArray(mentions, node);
    }
    cJSON_AddItemToObject(root, "mentions", mentions);
    return root;
}

}  // namespace octo::channels
