#ifndef OCTO_NOSTR_CHANNEL_SERVICE_H
#define OCTO_NOSTR_CHANNEL_SERVICE_H

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <cJSON.h>

namespace octo::channels {

struct NostrInboxMessage {
    std::string local_id;
    std::string event_id;
    std::string sender_id;
    std::string chat_id;
    std::string text;
    std::string relay;
    int64_t ts_ms = 0;
};

class NostrChannelService {
public:
    static NostrChannelService& GetInstance();

    void Initialize(const std::string& device_id, const std::string& board_name);

    bool UpdateConfig(bool enabled,
                      const std::string& account_id,
                      const std::string& allow_pubkeys_csv,
                      std::string* error_message = nullptr);

    bool ConsumeIncomingEvent(const cJSON* root,
                              bool* cached,
                              std::string* error_message = nullptr);

    bool ClearInbox();

    cJSON* BuildSendDmPayloadJson(const std::string& to,
                                  const std::string& text,
                                  const std::string& request_id,
                                  std::string* error_message = nullptr) const;

    cJSON* ExportConfigJson() const;
    cJSON* ExportStatsJson() const;
    cJSON* ExportInboxJson() const;

private:
    NostrChannelService() = default;

    struct ConfigSnapshot {
        bool enabled = true;
        std::string account_id;
        std::vector<std::string> allow_pubkeys;
    };

    struct RuntimeStats {
        uint32_t inbound_total = 0;
        uint32_t cached_total = 0;
        uint32_t dropped_invalid = 0;
        uint32_t dropped_not_allowed = 0;
        uint32_t duplicate_merged = 0;
        uint32_t dropped_disabled = 0;
    };

    enum class ParseResult {
        kNotNostrEvent,
        kInvalid,
        kValid,
    };

    void LoadLocked();
    void SaveConfigLocked() const;
    void SaveInboxLocked() const;

    ParseResult ParseIncomingEventLocked(const cJSON* root,
                                         NostrInboxMessage* message,
                                         std::string* error_message) const;

    bool IsSenderAllowedLocked(const std::string& sender_id) const;
    bool NormalizePubkey(const std::string& input, std::string* normalized) const;

    mutable std::mutex mutex_;
    bool initialized_ = false;

    std::string device_id_;
    std::string board_name_;
    uint32_t local_id_seed_ = 0;

    ConfigSnapshot config_;
    RuntimeStats stats_;
    std::vector<NostrInboxMessage> inbox_;
};

}  // namespace octo::channels

#endif  // OCTO_NOSTR_CHANNEL_SERVICE_H
