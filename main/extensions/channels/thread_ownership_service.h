#ifndef OCTO_THREAD_OWNERSHIP_SERVICE_H
#define OCTO_THREAD_OWNERSHIP_SERVICE_H

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <cJSON.h>

namespace octo::channels {

struct ThreadOwnershipDecision {
    bool allow = true;
    bool checked = false;
    bool cancelled = false;
    bool fail_open = false;
    int status_code = 0;
    std::string reason;
    std::string owner;
};

class ThreadOwnershipService {
public:
    static ThreadOwnershipService& GetInstance();

    void Initialize(const std::string& default_agent_id, const std::string& default_agent_name);

    bool UpdateConfig(const std::string& forwarder_url,
                      const std::string& ab_test_channels_csv,
                      const std::string& bot_user_id,
                      const std::string& agent_id,
                      const std::string& agent_name,
                      bool enabled,
                      std::string* error_message);

    bool TrackMention(const std::string& channel_type,
                      const std::string& channel_id,
                      const std::string& thread_ts,
                      const std::string& text);

    bool ConsumeIncomingEvent(const cJSON* root, bool* tracked, std::string* error_message);

    ThreadOwnershipDecision CheckBeforeSend(const std::string& channel_type,
                                            const std::string& channel_id,
                                            const std::string& thread_ts,
                                            const std::string& text);

    cJSON* ExportConfigJson() const;
    cJSON* ExportRuntimeJson() const;

private:
    ThreadOwnershipService() = default;

    struct MentionEntry {
        std::string key;
        int64_t ts_ms = 0;
    };

    struct RuntimeStats {
        uint32_t mention_tracked = 0;
        uint32_t checks_total = 0;
        uint32_t checks_allowed = 0;
        uint32_t checks_cancelled = 0;
        uint32_t checks_fail_open = 0;
        uint32_t http_errors = 0;
    };

    struct ConfigSnapshot {
        bool enabled = true;
        std::string forwarder_url;
        std::string bot_user_id;
        std::string agent_id;
        std::string agent_name;
        std::vector<std::string> ab_test_channels;
    };

    void LoadLocked(const std::string& default_agent_id, const std::string& default_agent_name);
    void SaveLocked() const;

    void CleanExpiredMentionsLocked(int64_t now_ms);
    bool MatchMentionLocked(const std::string& text) const;
    bool IsTrackedMentionLocked(const std::string& channel_id, const std::string& thread_ts) const;
    void UpsertMentionLocked(const std::string& channel_id, const std::string& thread_ts, int64_t now_ms);

    bool ParseIncomingEvent(const cJSON* root,
                            std::string* channel_type,
                            std::string* channel_id,
                            std::string* thread_ts,
                            std::string* text,
                            std::string* error_message) const;

    ThreadOwnershipDecision ClaimOwnership(const std::string& forwarder_url,
                                           const std::string& channel_id,
                                           const std::string& thread_ts,
                                           const std::string& agent_id) const;

    mutable std::mutex mutex_;
    bool initialized_ = false;
    ConfigSnapshot config_;
    RuntimeStats stats_;
    std::vector<MentionEntry> mentions_;
};

}  // namespace octo::channels

#endif  // OCTO_THREAD_OWNERSHIP_SERVICE_H
