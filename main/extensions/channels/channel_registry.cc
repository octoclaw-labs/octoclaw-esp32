#include "channel_registry.h"

namespace octo::channels {

ChannelRegistry& ChannelRegistry::Instance() {
    static ChannelRegistry instance;
    return instance;
}

void ChannelRegistry::Register(std::unique_ptr<ChannelExtension> extension) {
    if (extension == nullptr) {
        return;
    }
    for (const auto& item : extensions_) {
        if (std::string(item->Id()) == extension->Id()) {
            return;
        }
    }
    extensions_.push_back(std::move(extension));
}

ChannelExtension* ChannelRegistry::Find(const std::string& id) const {
    for (const auto& item : extensions_) {
        if (id == item->Id()) {
            return item.get();
        }
    }
    return nullptr;
}

size_t ChannelRegistry::Size() const {
    return extensions_.size();
}

std::vector<std::string> ChannelRegistry::ListIds() const {
    std::vector<std::string> ids;
    ids.reserve(extensions_.size());
    for (const auto& item : extensions_) {
        ids.emplace_back(item->Id());
    }
    return ids;
}

}  // namespace octo::channels
