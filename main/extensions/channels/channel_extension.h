#ifndef OCTO_CHANNEL_EXTENSION_H
#define OCTO_CHANNEL_EXTENSION_H

#include <string>

namespace octo::channels {

struct ChannelRuntimeContext {
    std::string device_id;
    std::string board_name;
};

class ChannelExtension {
public:
    virtual ~ChannelExtension() = default;

    virtual const char* Id() const = 0;
    virtual bool Initialize(const ChannelRuntimeContext& context) = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
};

}  // namespace octo::channels

#endif  // OCTO_CHANNEL_EXTENSION_H
