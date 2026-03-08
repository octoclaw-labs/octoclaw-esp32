#include "receipt_queue.h"

#include <esp_log.h>

#include "settings.h"

#define TAG "octo_receipt_queue"

namespace octo {

namespace {
constexpr const char* kRuntimeNamespace = "octo_runtime";
constexpr const char* kQueuedKey = "rq_total_q";
constexpr const char* kFlushedKey = "rq_total_f";
constexpr const char* kDroppedKey = "rq_total_d";
}  // namespace

ReceiptQueue::ReceiptQueue(size_t max_size) : max_size_(max_size) {
    LoadRuntimeStats();
}

void ReceiptQueue::LoadRuntimeStats() {
    Settings settings(kRuntimeNamespace, false);
    total_queued_ = static_cast<uint64_t>(settings.GetInt(kQueuedKey, 0));
    total_flushed_ = static_cast<uint64_t>(settings.GetInt(kFlushedKey, 0));
    total_dropped_ = static_cast<uint64_t>(settings.GetInt(kDroppedKey, 0));
}

void ReceiptQueue::PersistRuntimeStats() const {
    Settings settings(kRuntimeNamespace, true);
    settings.SetInt(kQueuedKey, static_cast<int32_t>(total_queued_));
    settings.SetInt(kFlushedKey, static_cast<int32_t>(total_flushed_));
    settings.SetInt(kDroppedKey, static_cast<int32_t>(total_dropped_));
}

bool ReceiptQueue::Enqueue(const std::string& request_id, const std::string& payload) {
    if (payload.empty()) {
        return false;
    }

    if (!request_id.empty()) {
        for (auto& item : queue_) {
            if (item.request_id == request_id) {
                item.payload = payload;
                return true;
            }
        }
    }

    if (queue_.size() >= max_size_) {
        queue_.pop_front();
        ++total_dropped_;
    }

    queue_.push_back(ReceiptEnvelope{request_id, payload});
    ++total_queued_;
    PersistRuntimeStats();
    ESP_LOGW(TAG, "Reply queued for compensation. pending=%u", static_cast<unsigned>(queue_.size()));
    return true;
}

size_t ReceiptQueue::Flush(const std::function<bool(const std::string&)>& sender) {
    if (!sender) {
        return 0;
    }

    size_t flushed = 0;
    while (!queue_.empty()) {
        const auto& item = queue_.front();
        if (!sender(item.payload)) {
            break;
        }
        queue_.pop_front();
        ++flushed;
        ++total_flushed_;
    }

    if (flushed > 0) {
        PersistRuntimeStats();
        ESP_LOGI(TAG, "Flushed pending replies: %u", static_cast<unsigned>(flushed));
    }
    return flushed;
}

cJSON* ReceiptQueue::ExportStatsJson() const {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "pending", static_cast<double>(queue_.size()));
    cJSON_AddNumberToObject(json, "totalQueued", static_cast<double>(total_queued_));
    cJSON_AddNumberToObject(json, "totalFlushed", static_cast<double>(total_flushed_));
    cJSON_AddNumberToObject(json, "totalDropped", static_cast<double>(total_dropped_));
    return json;
}

}  // namespace octo
