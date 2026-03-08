#ifndef NATS_PROTOCOL_H
#define NATS_PROTOCOL_H

#include "protocol.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <web_socket.h>

#include <cstddef>
#include <memory>
#include <string>

#define NATS_PROTOCOL_SERVER_HELLO_EVENT (1 << 0)

class NatsProtocol : public Protocol {
public:
    NatsProtocol();
    ~NatsProtocol();

    bool Start() override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel(bool send_goodbye = true) override;
    bool IsAudioChannelOpened() const override;

private:
    struct PendingMessage {
        std::string subject;
        size_t payload_size = 0;
        bool waiting_payload = false;
    };

    EventGroupHandle_t event_group_handle_ = nullptr;
    std::unique_ptr<WebSocket> websocket_;

    std::string url_;
    std::string token_;
    std::string publish_subject_;
    std::string subscribe_subject_;
    std::string audio_publish_subject_;
    std::string audio_subscribe_subject_;
    std::string queue_group_;
    std::string connect_payload_;

    int binary_version_ = 3;
    int control_sid_ = 1;
    int audio_sid_ = 2;

    std::string rx_buffer_;
    PendingMessage pending_message_;

    bool SendText(const std::string& text) override;
    bool LoadSettings();
    bool SendCommand(const std::string& command);
    bool SendNatsPayload(const std::string& subject, const uint8_t* payload, size_t payload_size);
    bool SendNatsText(const std::string& subject, const std::string& text);
    bool SubscribeSubject(const std::string& subject, int sid);
    std::string BuildConnectCommand() const;
    std::string GetHelloMessage();
    void ParseServerHello(const cJSON* root);
    void HandleIncomingData(const char* data, size_t len);
    void ParseFrameBuffer();
    bool HandleControlLine(const std::string& line);
    bool ParseMsgHeader(const std::string& line, std::string& subject, size_t& payload_size) const;
    void HandleNatsMessage(const std::string& subject, const uint8_t* payload, size_t payload_size);
};

#endif  // NATS_PROTOCOL_H
