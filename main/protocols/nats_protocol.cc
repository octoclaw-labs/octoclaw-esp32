#include "nats_protocol.h"

#include "assets/lang_config.h"
#include "board.h"
#include "settings.h"
#include "system_info.h"

#include <arpa/inet.h>
#include <cJSON.h>
#include <esp_log.h>

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <string_view>
#include <vector>

#define TAG "NATS"

namespace {

bool StartsWith(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

}  // namespace

NatsProtocol::NatsProtocol() {
    event_group_handle_ = xEventGroupCreate();
}

NatsProtocol::~NatsProtocol() {
    websocket_.reset();
    if (event_group_handle_ != nullptr) {
        vEventGroupDelete(event_group_handle_);
        event_group_handle_ = nullptr;
    }
}

bool NatsProtocol::Start() {
    return true;
}

bool NatsProtocol::LoadSettings() {
    Settings settings("nats", false);
    url_ = settings.GetString("url");
    token_ = settings.GetString("token");
    publish_subject_ = settings.GetString("publish_subject", "octoclaw.device.up");
    subscribe_subject_ = settings.GetString("subscribe_subject", "octoclaw.device.down");
    audio_publish_subject_ = settings.GetString("audio_publish_subject");
    audio_subscribe_subject_ = settings.GetString("audio_subscribe_subject");
    queue_group_ = settings.GetString("queue_group");
    connect_payload_ = settings.GetString("connect_payload");
    binary_version_ = settings.GetInt("version", 3);

    if (audio_publish_subject_.empty()) {
        audio_publish_subject_ = publish_subject_ + ".audio";
    }
    if (audio_subscribe_subject_.empty()) {
        audio_subscribe_subject_ = subscribe_subject_ + ".audio";
    }
    if (binary_version_ < 1 || binary_version_ > 3) {
        binary_version_ = 3;
    }

    if (url_.empty()) {
        ESP_LOGW(TAG, "NATS URL is not configured");
        return false;
    }
    if (publish_subject_.empty() || subscribe_subject_.empty()) {
        ESP_LOGE(TAG, "NATS subjects are not configured correctly");
        return false;
    }
    return true;
}

bool NatsProtocol::SendCommand(const std::string& command) {
    if (websocket_ == nullptr || !websocket_->IsConnected()) {
        return false;
    }
    if (!websocket_->Send(command)) {
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    return true;
}

bool NatsProtocol::SendNatsPayload(const std::string& subject, const uint8_t* payload, size_t payload_size) {
    if (subject.empty() || payload == nullptr) {
        return false;
    }
    if (websocket_ == nullptr || !websocket_->IsConnected()) {
        return false;
    }

    std::string header = "PUB " + subject + " " + std::to_string(payload_size) + "\r\n";
    std::string frame;
    frame.reserve(header.size() + payload_size + 2);
    frame.append(header);
    frame.append(reinterpret_cast<const char*>(payload), payload_size);
    frame.append("\r\n");

    if (!websocket_->Send(frame.data(), frame.size(), true)) {
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    return true;
}

bool NatsProtocol::SendNatsText(const std::string& subject, const std::string& text) {
    return SendNatsPayload(subject,
                           reinterpret_cast<const uint8_t*>(text.data()),
                           text.size());
}

bool NatsProtocol::SubscribeSubject(const std::string& subject, int sid) {
    if (subject.empty()) {
        return false;
    }
    std::string command;
    if (queue_group_.empty()) {
        command = "SUB " + subject + " " + std::to_string(sid) + "\r\n";
    } else {
        command = "SUB " + subject + " " + queue_group_ + " " + std::to_string(sid) + "\r\n";
    }
    return SendCommand(command);
}

std::string NatsProtocol::BuildConnectCommand() const {
    std::string payload = connect_payload_;
    if (payload.empty()) {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "verbose", false);
        cJSON_AddBoolToObject(root, "pedantic", false);
        cJSON_AddBoolToObject(root, "echo", false);
        cJSON_AddNumberToObject(root, "protocol", 1);
        cJSON_AddStringToObject(root, "lang", "cpp");
        cJSON_AddStringToObject(root, "version", "esp32");
        cJSON_AddStringToObject(root, "name", SystemInfo::GetMacAddress().c_str());
        if (!token_.empty()) {
            cJSON_AddStringToObject(root, "auth_token", token_.c_str());
        }
        char* raw = cJSON_PrintUnformatted(root);
        payload = raw != nullptr ? raw : "{}";
        if (raw != nullptr) {
            cJSON_free(raw);
        }
        cJSON_Delete(root);
    }

    std::string command = "CONNECT ";
    command += payload;
    command += "\r\n";
    command += "PING\r\n";
    return command;
}

std::string NatsProtocol::GetHelloMessage() {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", binary_version_);
    cJSON_AddStringToObject(root, "transport", "nats");

    cJSON* features = cJSON_CreateObject();
#if CONFIG_USE_SERVER_AEC
    cJSON_AddBoolToObject(features, "aec", true);
#endif
    cJSON_AddBoolToObject(features, "mcp", true);
    cJSON_AddItemToObject(root, "features", features);

    cJSON* audio_params = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_params, "format", "opus");
    cJSON_AddNumberToObject(audio_params, "sample_rate", 16000);
    cJSON_AddNumberToObject(audio_params, "channels", 1);
    cJSON_AddNumberToObject(audio_params, "frame_duration", OPUS_FRAME_DURATION_MS);
    cJSON_AddItemToObject(root, "audio_params", audio_params);

    char* raw = cJSON_PrintUnformatted(root);
    std::string message = raw != nullptr ? raw : "{}";
    if (raw != nullptr) {
        cJSON_free(raw);
    }
    cJSON_Delete(root);
    return message;
}

void NatsProtocol::ParseServerHello(const cJSON* root) {
    auto transport = cJSON_GetObjectItem(root, "transport");
    if (cJSON_IsString(transport) && strcmp(transport->valuestring, "nats") != 0) {
        ESP_LOGW(TAG, "Unexpected transport in hello: %s", transport->valuestring);
    }

    auto session_id = cJSON_GetObjectItem(root, "session_id");
    if (cJSON_IsString(session_id)) {
        session_id_ = session_id->valuestring;
    }

    auto audio_params = cJSON_GetObjectItem(root, "audio_params");
    if (cJSON_IsObject(audio_params)) {
        auto sample_rate = cJSON_GetObjectItem(audio_params, "sample_rate");
        if (cJSON_IsNumber(sample_rate)) {
            server_sample_rate_ = sample_rate->valueint;
        }
        auto frame_duration = cJSON_GetObjectItem(audio_params, "frame_duration");
        if (cJSON_IsNumber(frame_duration)) {
            server_frame_duration_ = frame_duration->valueint;
        }
    }

    xEventGroupSetBits(event_group_handle_, NATS_PROTOCOL_SERVER_HELLO_EVENT);
}

void NatsProtocol::HandleIncomingData(const char* data, size_t len) {
    if (data == nullptr || len == 0) {
        return;
    }
    rx_buffer_.append(data, len);
    ParseFrameBuffer();
}

bool NatsProtocol::ParseMsgHeader(const std::string& line, std::string& subject, size_t& payload_size) const {
    std::istringstream iss(line);
    std::vector<std::string> parts;
    std::string item;
    while (iss >> item) {
        parts.push_back(item);
    }
    if (parts.size() != 4 && parts.size() != 5) {
        return false;
    }
    if (parts[0] != "MSG") {
        return false;
    }

    subject = parts[1];
    std::string size_token = parts.back();
    if (size_token.empty()) {
        return false;
    }
    char* endptr = nullptr;
    unsigned long v = std::strtoul(size_token.c_str(), &endptr, 10);
    if (endptr == nullptr || *endptr != '\0') {
        return false;
    }
    payload_size = static_cast<size_t>(v);
    return true;
}

void NatsProtocol::HandleNatsMessage(const std::string& subject, const uint8_t* payload, size_t payload_size) {
    if (payload == nullptr) {
        return;
    }

    if (subject == audio_subscribe_subject_) {
        if (on_incoming_audio_ == nullptr) {
            return;
        }

        auto packet = std::make_unique<AudioStreamPacket>();
        packet->sample_rate = server_sample_rate_;
        packet->frame_duration = server_frame_duration_;
        packet->timestamp = 0;

        if (binary_version_ == 2 && payload_size >= sizeof(BinaryProtocol2)) {
            auto bp2 = reinterpret_cast<const BinaryProtocol2*>(payload);
            uint16_t type = ntohs(bp2->type);
            uint32_t timestamp = ntohl(bp2->timestamp);
            uint32_t bytes = ntohl(bp2->payload_size);
            size_t available = payload_size - sizeof(BinaryProtocol2);
            if (type == 0 && bytes <= available) {
                packet->timestamp = timestamp;
                packet->payload.assign(bp2->payload, bp2->payload + bytes);
            } else {
                packet->payload.assign(payload, payload + payload_size);
            }
        } else if (binary_version_ == 3 && payload_size >= sizeof(BinaryProtocol3)) {
            auto bp3 = reinterpret_cast<const BinaryProtocol3*>(payload);
            uint16_t bytes = ntohs(bp3->payload_size);
            size_t available = payload_size - sizeof(BinaryProtocol3);
            if (bytes <= available) {
                packet->payload.assign(bp3->payload, bp3->payload + bytes);
            } else {
                packet->payload.assign(payload, payload + payload_size);
            }
        } else {
            packet->payload.assign(payload, payload + payload_size);
        }

        on_incoming_audio_(std::move(packet));
        return;
    }

    std::string text(reinterpret_cast<const char*>(payload), payload_size);
    cJSON* root = cJSON_Parse(text.c_str());
    if (root == nullptr) {
        ESP_LOGW(TAG, "Skip non-JSON control payload");
        return;
    }

    auto type = cJSON_GetObjectItem(root, "type");
    if (cJSON_IsString(type) && strcmp(type->valuestring, "hello") == 0) {
        ParseServerHello(root);
    } else if (on_incoming_json_ != nullptr) {
        on_incoming_json_(root);
    }

    cJSON_Delete(root);
}

bool NatsProtocol::HandleControlLine(const std::string& line) {
    if (line.empty()) {
        return true;
    }

    if (StartsWith(line, "PING")) {
        return SendCommand("PONG\r\n");
    }
    if (StartsWith(line, "PONG")) {
        return true;
    }
    if (StartsWith(line, "+OK")) {
        return true;
    }
    if (StartsWith(line, "INFO ")) {
        return true;
    }
    if (StartsWith(line, "-ERR")) {
        ESP_LOGE(TAG, "NATS server error: %s", line.c_str());
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    if (StartsWith(line, "MSG ")) {
        std::string subject;
        size_t payload_size = 0;
        if (!ParseMsgHeader(line, subject, payload_size)) {
            ESP_LOGE(TAG, "Invalid MSG header: %s", line.c_str());
            return false;
        }
        pending_message_.subject = subject;
        pending_message_.payload_size = payload_size;
        pending_message_.waiting_payload = true;
        return true;
    }

    ESP_LOGW(TAG, "Unknown NATS frame: %s", line.c_str());
    return true;
}

void NatsProtocol::ParseFrameBuffer() {
    while (true) {
        if (pending_message_.waiting_payload) {
            if (rx_buffer_.size() < pending_message_.payload_size + 2) {
                return;
            }
            const uint8_t* payload = reinterpret_cast<const uint8_t*>(rx_buffer_.data());
            HandleNatsMessage(pending_message_.subject, payload, pending_message_.payload_size);

            size_t trailer_pos = pending_message_.payload_size;
            if (rx_buffer_[trailer_pos] == '\r' && rx_buffer_[trailer_pos + 1] == '\n') {
                rx_buffer_.erase(0, pending_message_.payload_size + 2);
            } else {
                rx_buffer_.erase(0, pending_message_.payload_size);
            }

            pending_message_ = PendingMessage{};
            last_incoming_time_ = std::chrono::steady_clock::now();
            continue;
        }

        size_t line_end = rx_buffer_.find("\r\n");
        if (line_end == std::string::npos) {
            return;
        }

        std::string line = rx_buffer_.substr(0, line_end);
        rx_buffer_.erase(0, line_end + 2);

        if (!HandleControlLine(line)) {
            return;
        }
        last_incoming_time_ = std::chrono::steady_clock::now();
    }
}

bool NatsProtocol::SendText(const std::string& text) {
    if (!SendNatsText(publish_subject_, text)) {
        ESP_LOGE(TAG, "Failed to publish text payload");
        return false;
    }
    return true;
}

bool NatsProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (packet == nullptr) {
        return false;
    }
    if (websocket_ == nullptr || !websocket_->IsConnected()) {
        return false;
    }

    if (binary_version_ == 2) {
        std::string serialized;
        serialized.resize(sizeof(BinaryProtocol2) + packet->payload.size());
        auto bp2 = reinterpret_cast<BinaryProtocol2*>(serialized.data());
        bp2->version = htons(binary_version_);
        bp2->type = htons(0);
        bp2->reserved = 0;
        bp2->timestamp = htonl(packet->timestamp);
        bp2->payload_size = htonl(packet->payload.size());
        memcpy(bp2->payload, packet->payload.data(), packet->payload.size());
        return SendNatsPayload(audio_publish_subject_,
                               reinterpret_cast<const uint8_t*>(serialized.data()),
                               serialized.size());
    }

    if (binary_version_ == 3) {
        std::string serialized;
        serialized.resize(sizeof(BinaryProtocol3) + packet->payload.size());
        auto bp3 = reinterpret_cast<BinaryProtocol3*>(serialized.data());
        bp3->type = 0;
        bp3->reserved = 0;
        bp3->payload_size = htons(packet->payload.size());
        memcpy(bp3->payload, packet->payload.data(), packet->payload.size());
        return SendNatsPayload(audio_publish_subject_,
                               reinterpret_cast<const uint8_t*>(serialized.data()),
                               serialized.size());
    }

    return SendNatsPayload(audio_publish_subject_, packet->payload.data(), packet->payload.size());
}

bool NatsProtocol::OpenAudioChannel() {
    if (!LoadSettings()) {
        SetError(Lang::Strings::SERVER_NOT_FOUND);
        return false;
    }

    error_occurred_ = false;
    session_id_.clear();
    rx_buffer_.clear();
    pending_message_ = PendingMessage{};
    xEventGroupClearBits(event_group_handle_, NATS_PROTOCOL_SERVER_HELLO_EVENT);

    auto network = Board::GetInstance().GetNetwork();
    websocket_ = network->CreateWebSocket(1);
    if (websocket_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create websocket for NATS");
        return false;
    }

    websocket_->SetHeader("Sec-WebSocket-Protocol", "nats");
    websocket_->OnData([this](const char* data, size_t len, bool binary) {
        (void)binary;
        HandleIncomingData(data, len);
    });
    websocket_->OnDisconnected([this]() {
        ESP_LOGI(TAG, "NATS disconnected");
        if (on_audio_channel_closed_ != nullptr) {
            on_audio_channel_closed_();
        }
    });

    ESP_LOGI(TAG, "Connecting to NATS server: %s", url_.c_str());
    if (!websocket_->Connect(url_.c_str())) {
        ESP_LOGE(TAG, "Failed to connect NATS server, code=%d", websocket_->GetLastError());
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }

    if (!SendCommand(BuildConnectCommand())) {
        return false;
    }

    if (!SubscribeSubject(subscribe_subject_, control_sid_)) {
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    if (!audio_subscribe_subject_.empty() && audio_subscribe_subject_ != subscribe_subject_) {
        if (!SubscribeSubject(audio_subscribe_subject_, audio_sid_)) {
            SetError(Lang::Strings::SERVER_ERROR);
            return false;
        }
    }

    if (!SendText(GetHelloMessage())) {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(event_group_handle_,
                                           NATS_PROTOCOL_SERVER_HELLO_EVENT,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000));
    if ((bits & NATS_PROTOCOL_SERVER_HELLO_EVENT) == 0) {
        ESP_LOGE(TAG, "Timeout waiting server hello on NATS");
        SetError(Lang::Strings::SERVER_TIMEOUT);
        return false;
    }

    if (on_audio_channel_opened_ != nullptr) {
        on_audio_channel_opened_();
    }
    return true;
}

void NatsProtocol::CloseAudioChannel(bool send_goodbye) {
    if (send_goodbye && websocket_ != nullptr && websocket_->IsConnected()) {
        std::string message = "{\"session_id\":\"" + session_id_ + "\",\"type\":\"goodbye\"}";
        SendText(message);
    }

    websocket_.reset();
    rx_buffer_.clear();
    pending_message_ = PendingMessage{};

    if (on_audio_channel_closed_ != nullptr) {
        on_audio_channel_closed_();
    }
}

bool NatsProtocol::IsAudioChannelOpened() const {
    return websocket_ != nullptr && websocket_->IsConnected() && !error_occurred_ && !IsTimeout();
}
