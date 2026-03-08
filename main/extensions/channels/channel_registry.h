#ifndef OCTO_CHANNEL_REGISTRY_H
#define OCTO_CHANNEL_REGISTRY_H

#include "channel_extension.h"

#include <memory>
#include <string>
#include <vector>

namespace octo::channels {

class ChannelRegistry {
public:
    static ChannelRegistry& Instance();

    void Register(std::unique_ptr<ChannelExtension> extension);
    ChannelExtension* Find(const std::string& id) const;
    size_t Size() const;
    std::vector<std::string> ListIds() const;

private:
    ChannelRegistry() = default;

    std::vector<std::unique_ptr<ChannelExtension>> extensions_;
};

}  // namespace octo::channels

#endif  // OCTO_CHANNEL_REGISTRY_H
