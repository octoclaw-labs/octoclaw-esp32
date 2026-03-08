#ifndef OCTO_RECEIPT_QUEUE_H
#define OCTO_RECEIPT_QUEUE_H

#include <deque>
#include <functional>
#include <string>

#include <cJSON.h>

namespace octo {

struct ReceiptEnvelope {
    std::string request_id;
    std::string payload;
};

class ReceiptQueue {
public:
    explicit ReceiptQueue(size_t max_size = 8);

    bool Enqueue(const std::string& request_id, const std::string& payload);
    size_t Flush(const std::function<bool(const std::string&)>& sender);
    cJSON* ExportStatsJson() const;

private:
    void LoadRuntimeStats();
    void PersistRuntimeStats() const;

    size_t max_size_ = 8;
    std::deque<ReceiptEnvelope> queue_;

    uint64_t total_queued_ = 0;
    uint64_t total_flushed_ = 0;
    uint64_t total_dropped_ = 0;
};

}  // namespace octo

#endif  // OCTO_RECEIPT_QUEUE_H
