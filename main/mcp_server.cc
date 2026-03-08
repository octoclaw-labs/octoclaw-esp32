/*
 * MCP Server Implementation
 * Reference: https://modelcontextprotocol.io/specification/2024-11-05
 */

#include "mcp_server.h"
#include <esp_log.h>
#include <esp_app_desc.h>
#include <algorithm>
#include <cstring>
#include <esp_pthread.h>

#include "application.h"
#include "display.h"
#include "oled_display.h"
#include "board.h"
#include "settings.h"
#include "lvgl_theme.h"
#include "lvgl_display.h"
#include "extensions/octoclaw/runtime/agent_lite.h"
#include "extensions/octoclaw/profile/board_profile.h"
#include "extensions/octoclaw/policy/capability_manifest.h"
#include "extensions/octoclaw/policy/policy_guard.h"
#include "extensions/octoclaw/policy/policy_store.h"
#include "extensions/octoclaw/transport/receipt_queue.h"
#include "extensions/channels/openclaw_extension_catalog.h"

#define TAG "MCP"

namespace {

octo::PolicyStore& GetPolicyStore() {
    static octo::PolicyStore store;
    return store;
}

octo::CapabilityManifest& GetCapabilityManifest() {
    static octo::CapabilityManifest manifest;
    return manifest;
}

octo::PolicyGuard& GetPolicyGuard() {
    static octo::PolicyGuard guard;
    return guard;
}

octo::ReceiptQueue& GetReceiptQueue() {
    static octo::ReceiptQueue queue(8);
    return queue;
}

octo::AgentLiteRuntime& GetAgentLite() {
    static octo::AgentLiteRuntime runtime(static_cast<size_t>(octo::GetTelemetryRingSize()));
    return runtime;
}

uint32_t GetFeatureMask() {
    const auto& snapshot = GetPolicyStore().GetSnapshot();
    return octo::GetEffectiveFeatureMask(snapshot.feature_mask_override);
}

bool IsExtMetaEnabled() {
    return octo::FeatureEnabled(GetFeatureMask(), octo::kFeatureMcpExtFields);
}

cJSON* BuildMetaJson(const octo::ReplyMeta& meta) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "decision", meta.decision.c_str());
    cJSON_AddNumberToObject(json, "risk", meta.risk_level);
    cJSON_AddNumberToObject(json, "policyVersion", meta.policy_version);
    cJSON_AddStringToObject(json, "requestId", meta.request_id.c_str());
    cJSON_AddStringToObject(json, "reasonCode", meta.reason_code.c_str());
    return json;
}

std::string InjectMetaIntoResult(const std::string& result, const octo::ReplyMeta& meta) {
    cJSON* parsed = cJSON_Parse(result.c_str());
    if (!cJSON_IsObject(parsed)) {
        if (parsed != nullptr) {
            cJSON_Delete(parsed);
        }
        return result;
    }
    cJSON_AddItemToObject(parsed, "meta", BuildMetaJson(meta));
    char* out = cJSON_PrintUnformatted(parsed);
    std::string ret = out != nullptr ? out : result;
    if (out != nullptr) {
        cJSON_free(out);
    }
    cJSON_Delete(parsed);
    return ret;
}

}  // namespace

McpServer::McpServer() {
    GetPolicyStore().Load();
}

McpServer::~McpServer() {
    for (auto tool : tools_) {
        delete tool;
    }
    tools_.clear();
}

void McpServer::AddCommonTools() {
    // *Important* To speed up the response time, we add the common tools to the beginning of
    // the tools list to utilize the prompt cache.
    // **重要** 为了提升响应速度，我们把常用的工具放在前面，利用 prompt cache 的特性。

    // Backup the original tools list and restore it after adding the common tools.
    auto original_tools = std::move(tools_);
    auto& board = Board::GetInstance();

    // Do not add custom tools here.
    // Custom tools must be added in the board's InitializeTools function.

    AddTool("self.get_device_status",
        "Provides the real-time information of the device, including the current status of the audio speaker, screen, battery, network, etc.\n"
        "Use this tool for: \n"
        "1. Answering questions about current condition (e.g. what is the current volume of the audio speaker?)\n"
        "2. As the first step to control the device (e.g. turn up / down the volume of the audio speaker, etc.)",
        PropertyList(),
        [&board](const PropertyList& properties) -> ReturnValue {
            return board.GetDeviceStatusJson();
        });

    AddTool("self.audio_speaker.set_volume", 
        "Set the volume of the audio speaker. If the current volume is unknown, you must call `self.get_device_status` tool first and then call this tool.",
        PropertyList({
            Property("volume", kPropertyTypeInteger, 0, 100)
        }), 
        [&board](const PropertyList& properties) -> ReturnValue {
            auto codec = board.GetAudioCodec();
            codec->SetOutputVolume(properties["volume"].value<int>());
            return true;
        });
    
    auto backlight = board.GetBacklight();
    if (backlight) {
        AddTool("self.screen.set_brightness",
            "Set the brightness of the screen.",
            PropertyList({
                Property("brightness", kPropertyTypeInteger, 0, 100)
            }),
            [backlight](const PropertyList& properties) -> ReturnValue {
                uint8_t brightness = static_cast<uint8_t>(properties["brightness"].value<int>());
                backlight->SetBrightness(brightness, true);
                return true;
            });
    }

#ifdef HAVE_LVGL
    auto display = board.GetDisplay();
    if (display && display->GetTheme() != nullptr) {
        AddTool("self.screen.set_theme",
            "Set the theme of the screen. The theme can be `light` or `dark`.",
            PropertyList({
                Property("theme", kPropertyTypeString)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto theme_name = properties["theme"].value<std::string>();
                auto& theme_manager = LvglThemeManager::GetInstance();
                auto theme = theme_manager.GetTheme(theme_name);
                if (theme != nullptr) {
                    display->SetTheme(theme);
                    return true;
                }
                return false;
            });
    }

    auto camera = board.GetCamera();
    if (camera) {
        AddTool("self.camera.take_photo",
            "Always remember you have a camera. If the user asks you to see something, use this tool to take a photo and then explain it.\n"
            "Args:\n"
            "  `question`: The question that you want to ask about the photo.\n"
            "Return:\n"
            "  A JSON object that provides the photo information.",
            PropertyList({
                Property("question", kPropertyTypeString)
            }),
            [camera](const PropertyList& properties) -> ReturnValue {
                // Lower the priority to do the camera capture
                TaskPriorityReset priority_reset(1);

                if (!camera->Capture()) {
                    throw std::runtime_error("Failed to capture photo");
                }
                auto question = properties["question"].value<std::string>();
                return camera->Explain(question);
            });
    }
#endif

    // Restore the original tools list to the end of the tools list
    tools_.insert(tools_.end(), original_tools.begin(), original_tools.end());
}

void McpServer::AddUserOnlyTools() {
    // System tools
    AddUserOnlyTool("self.get_system_info",
        "Get the system information",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            auto& board = Board::GetInstance();
            return board.GetSystemInfoJson();
        });

    AddUserOnlyTool("self.reboot", "Reboot the system",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            auto& app = Application::GetInstance();
            app.Schedule([&app]() {
                ESP_LOGW(TAG, "User requested reboot");
                vTaskDelay(pdMS_TO_TICKS(1000));

                app.Reboot();
            });
            return true;
        });

    // Firmware upgrade
    AddUserOnlyTool("self.upgrade_firmware", "Upgrade firmware from a specific URL. This will download and install the firmware, then reboot the device.",
        PropertyList({
            Property("url", kPropertyTypeString, "The URL of the firmware binary file to download and install")
        }),
        [this](const PropertyList& properties) -> ReturnValue {
            auto url = properties["url"].value<std::string>();
            ESP_LOGI(TAG, "User requested firmware upgrade from URL: %s", url.c_str());
            
            auto& app = Application::GetInstance();
            app.Schedule([url, &app]() {
                bool success = app.UpgradeFirmware(url);
                if (!success) {
                    ESP_LOGE(TAG, "Firmware upgrade failed");
                }
            });
            
            return true;
        });

    // Display control
#ifdef HAVE_LVGL
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    if (display) {
        AddUserOnlyTool("self.screen.get_info", "Information about the screen, including width, height, etc.",
            PropertyList(),
            [display](const PropertyList& properties) -> ReturnValue {
                cJSON *json = cJSON_CreateObject();
                cJSON_AddNumberToObject(json, "width", display->width());
                cJSON_AddNumberToObject(json, "height", display->height());
                if (dynamic_cast<OledDisplay*>(display)) {
                    cJSON_AddBoolToObject(json, "monochrome", true);
                } else {
                    cJSON_AddBoolToObject(json, "monochrome", false);
                }
                return json;
            });

#if CONFIG_LV_USE_SNAPSHOT
        AddUserOnlyTool("self.screen.snapshot", "Snapshot the screen and upload it to a specific URL",
            PropertyList({
                Property("url", kPropertyTypeString),
                Property("quality", kPropertyTypeInteger, 80, 1, 100)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                auto quality = properties["quality"].value<int>();

                std::string jpeg_data;
                if (!display->SnapshotToJpeg(jpeg_data, quality)) {
                    throw std::runtime_error("Failed to snapshot screen");
                }

                ESP_LOGI(TAG, "Upload snapshot %u bytes to %s", jpeg_data.size(), url.c_str());
                
                // 构造multipart/form-data请求体
                std::string boundary = "----ESP32_SCREEN_SNAPSHOT_BOUNDARY";
                
                auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);
                http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
                if (!http->Open("POST", url)) {
                    throw std::runtime_error("Failed to open URL: " + url);
                }
                {
                    // 文件字段头部
                    std::string file_header;
                    file_header += "--" + boundary + "\r\n";
                    file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.jpg\"\r\n";
                    file_header += "Content-Type: image/jpeg\r\n";
                    file_header += "\r\n";
                    http->Write(file_header.c_str(), file_header.size());
                }

                // JPEG数据
                http->Write((const char*)jpeg_data.data(), jpeg_data.size());

                {
                    // multipart尾部
                    std::string multipart_footer;
                    multipart_footer += "\r\n--" + boundary + "--\r\n";
                    http->Write(multipart_footer.c_str(), multipart_footer.size());
                }
                http->Write("", 0);

                if (http->GetStatusCode() != 200) {
                    throw std::runtime_error("Unexpected status code: " + std::to_string(http->GetStatusCode()));
                }
                std::string result = http->ReadAll();
                http->Close();
                ESP_LOGI(TAG, "Snapshot screen result: %s", result.c_str());
                return true;
            });
        
        AddUserOnlyTool("self.screen.preview_image", "Preview an image on the screen",
            PropertyList({
                Property("url", kPropertyTypeString)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);

                if (!http->Open("GET", url)) {
                    throw std::runtime_error("Failed to open URL: " + url);
                }
                int status_code = http->GetStatusCode();
                if (status_code != 200) {
                    throw std::runtime_error("Unexpected status code: " + std::to_string(status_code));
                }

                size_t content_length = http->GetBodyLength();
                char* data = (char*)heap_caps_malloc(content_length, MALLOC_CAP_8BIT);
                if (data == nullptr) {
                    throw std::runtime_error("Failed to allocate memory for image: " + url);
                }
                size_t total_read = 0;
                while (total_read < content_length) {
                    int ret = http->Read(data + total_read, content_length - total_read);
                    if (ret < 0) {
                        heap_caps_free(data);
                        throw std::runtime_error("Failed to download image: " + url);
                    }
                    if (ret == 0) {
                        break;
                    }
                    total_read += ret;
                }
                http->Close();

                auto image = std::make_unique<LvglAllocatedImage>(data, content_length);
                display->SetPreviewImage(std::move(image));
                return true;
            });
#endif // CONFIG_LV_USE_SNAPSHOT
    }
#endif // HAVE_LVGL

    // Assets download url
    auto& assets = Assets::GetInstance();
    if (assets.partition_valid()) {
        AddUserOnlyTool("self.assets.set_download_url", "Set the download url for the assets",
            PropertyList({
                Property("url", kPropertyTypeString)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                Settings settings("assets", true);
                settings.SetString("download_url", url);
                return true;
            });
    }

    AddUserOnlyTool("self.system.get_octoclaw_profile",
        "Get OctoClaw board profile, policy snapshot and runtime diagnostics",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            (void)properties;
            const auto& snapshot = GetPolicyStore().GetSnapshot();
            uint32_t feature_mask = GetFeatureMask();

            cJSON* json = octo::BuildBoardProfileJson(feature_mask, snapshot.policy_version);
            cJSON_AddItemToObject(json, "policy", GetPolicyStore().ExportJson());
            cJSON_AddItemToObject(json, "capabilityManifest", GetCapabilityManifest().ExportJson());
            cJSON_AddItemToObject(json, "receiptQueue", GetReceiptQueue().ExportStatsJson());
            cJSON_AddItemToObject(json, "agentLite", GetAgentLite().ExportJson());
            return json;
        });

    AddUserOnlyTool("self.system.update_octoclaw_policy",
        "Update OctoClaw policy snapshot. "
        "Args: policy(JSON string), fields: policy_version/risk_threshold/emergency_stop/tool_whitelist/feature_mask",
        PropertyList({
            Property("policy", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto policy_str = properties["policy"].value<std::string>();
            cJSON* policy_json = cJSON_Parse(policy_str.c_str());
            if (!cJSON_IsObject(policy_json)) {
                if (policy_json != nullptr) {
                    cJSON_Delete(policy_json);
                }
                throw std::runtime_error("Invalid policy JSON");
            }

            std::string error_message;
            bool ok = GetPolicyStore().UpdateFromJson(policy_json, &error_message);
            cJSON_Delete(policy_json);
            if (!ok) {
                throw std::runtime_error(error_message.empty() ? "Policy update failed" : error_message);
            }

            cJSON* result = cJSON_CreateObject();
            cJSON_AddBoolToObject(result, "updated", true);
            cJSON_AddItemToObject(result, "policy", GetPolicyStore().ExportJson());
            cJSON_AddNumberToObject(result, "featureMask", static_cast<double>(GetFeatureMask()));
            return result;
        });

    AddUserOnlyTool("self.system.list_translated_extensions",
        "List all translated OpenClaw extensions on ESP32 side",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            (void)properties;
            return octo::channels::BuildOpenClawExtensionCatalogJson();
        });
}

void McpServer::AddTool(McpTool* tool) {
    // Prevent adding duplicate tools
    if (std::find_if(tools_.begin(), tools_.end(), [tool](const McpTool* t) { return t->name() == tool->name(); }) != tools_.end()) {
        ESP_LOGW(TAG, "Tool %s already added", tool->name().c_str());
        return;
    }

    ESP_LOGI(TAG, "Add tool: %s%s", tool->name().c_str(), tool->user_only() ? " [user]" : "");
    tools_.push_back(tool);
}

void McpServer::AddTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    AddTool(new McpTool(name, description, properties, callback));
}

void McpServer::AddUserOnlyTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    auto tool = new McpTool(name, description, properties, callback);
    tool->set_user_only(true);
    AddTool(tool);
}

void McpServer::ParseMessage(const std::string& message) {
    cJSON* json = cJSON_Parse(message.c_str());
    if (json == nullptr) {
        ESP_LOGE(TAG, "Failed to parse MCP message: %s", message.c_str());
        return;
    }
    ParseMessage(json);
    cJSON_Delete(json);
}

void McpServer::OnTransportReady() {
    FlushPendingReplies();
}

void McpServer::ParseCapabilities(const cJSON* capabilities) {
    auto vision = cJSON_GetObjectItem(capabilities, "vision");
    if (cJSON_IsObject(vision)) {
        auto url = cJSON_GetObjectItem(vision, "url");
        auto token = cJSON_GetObjectItem(vision, "token");
        if (cJSON_IsString(url)) {
            auto camera = Board::GetInstance().GetCamera();
            if (camera) {
                std::string url_str = std::string(url->valuestring);
                std::string token_str;
                if (cJSON_IsString(token)) {
                    token_str = std::string(token->valuestring);
                }
                camera->SetExplainUrl(url_str, token_str);
            }
        }
    }
}

void McpServer::ParseMessage(const cJSON* json) {
    FlushPendingReplies();

    // Check JSONRPC version
    auto version = cJSON_GetObjectItem(json, "jsonrpc");
    if (version == nullptr || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
        ESP_LOGE(TAG, "Invalid JSONRPC version: %s", version ? version->valuestring : "null");
        return;
    }
    
    // Check method
    auto method = cJSON_GetObjectItem(json, "method");
    if (method == nullptr || !cJSON_IsString(method)) {
        ESP_LOGE(TAG, "Missing method");
        return;
    }
    
    auto method_str = std::string(method->valuestring);
    if (method_str.find("notifications") == 0) {
        return;
    }
    
    // Check params
    auto params = cJSON_GetObjectItem(json, "params");
    if (params != nullptr && !cJSON_IsObject(params)) {
        ESP_LOGE(TAG, "Invalid params for method: %s", method_str.c_str());
        return;
    }

    auto id = cJSON_GetObjectItem(json, "id");
    if (id == nullptr || !cJSON_IsNumber(id)) {
        ESP_LOGE(TAG, "Invalid id for method: %s", method_str.c_str());
        return;
    }
    auto id_int = id->valueint;
    
    if (method_str == "initialize") {
        if (cJSON_IsObject(params)) {
            auto capabilities = cJSON_GetObjectItem(params, "capabilities");
            if (cJSON_IsObject(capabilities)) {
                ParseCapabilities(capabilities);
            }
        }
        auto app_desc = esp_app_get_description();
        std::string message = "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\"";
        message += app_desc->version;
        message += "\"}}";
        ReplyResult(id_int, message);
    } else if (method_str == "tools/list") {
        std::string cursor_str = "";
        bool list_user_only_tools = false;
        if (params != nullptr) {
            auto cursor = cJSON_GetObjectItem(params, "cursor");
            if (cJSON_IsString(cursor)) {
                cursor_str = std::string(cursor->valuestring);
            }
            auto with_user_tools = cJSON_GetObjectItem(params, "withUserTools");
            if (cJSON_IsBool(with_user_tools)) {
                list_user_only_tools = with_user_tools->valueint == 1;
            }
        }
        GetToolsList(id_int, cursor_str, list_user_only_tools);
    } else if (method_str == "tools/call") {
        if (!cJSON_IsObject(params)) {
            ESP_LOGE(TAG, "tools/call: Missing params");
            ReplyError(id_int, "Missing params");
            return;
        }
        auto tool_name = cJSON_GetObjectItem(params, "name");
        if (!cJSON_IsString(tool_name)) {
            ESP_LOGE(TAG, "tools/call: Missing name");
            ReplyError(id_int, "Missing name");
            return;
        }
        auto tool_arguments = cJSON_GetObjectItem(params, "arguments");
        if (tool_arguments != nullptr && !cJSON_IsObject(tool_arguments)) {
            ESP_LOGE(TAG, "tools/call: Invalid arguments");
            ReplyError(id_int, "Invalid arguments");
            return;
        }
        DoToolCall(id_int, std::string(tool_name->valuestring), tool_arguments);
    } else {
        ESP_LOGE(TAG, "Method not implemented: %s", method_str.c_str());
        ReplyError(id_int, "Method not implemented: " + method_str);
    }
}

void McpServer::ReplyResult(int id, const std::string& result, const octo::ReplyMeta* meta) {
    std::string final_result = result;
    if (meta != nullptr && IsExtMetaEnabled()) {
        final_result = InjectMetaIntoResult(result, *meta);
    }

    cJSON* payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(payload, "id", id);

    cJSON* result_json = cJSON_Parse(final_result.c_str());
    if (result_json == nullptr) {
        result_json = cJSON_CreateObject();
        cJSON_AddStringToObject(result_json, "text", final_result.c_str());
    }
    cJSON_AddItemToObject(payload, "result", result_json);

    char* payload_str = cJSON_PrintUnformatted(payload);
    std::string message = payload_str != nullptr ? payload_str : "";
    if (payload_str != nullptr) {
        cJSON_free(payload_str);
    }
    cJSON_Delete(payload);

    std::string request_id = std::to_string(id);
    if (meta != nullptr && !meta->request_id.empty()) {
        request_id = meta->request_id;
    }
    SendReplyPayload(message, request_id);
}

void McpServer::ReplyError(int id, const std::string& message, const octo::ReplyMeta* meta) {
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(payload, "id", id);

    cJSON* error = cJSON_CreateObject();
    cJSON_AddStringToObject(error, "message", message.c_str());
    if (meta != nullptr && IsExtMetaEnabled()) {
        cJSON* data = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "meta", BuildMetaJson(*meta));
        cJSON_AddItemToObject(error, "data", data);
    }
    cJSON_AddItemToObject(payload, "error", error);

    char* payload_str = cJSON_PrintUnformatted(payload);
    std::string body = payload_str != nullptr ? payload_str : "";
    if (payload_str != nullptr) {
        cJSON_free(payload_str);
    }
    cJSON_Delete(payload);

    std::string request_id = std::to_string(id);
    if (meta != nullptr && !meta->request_id.empty()) {
        request_id = meta->request_id;
    }
    SendReplyPayload(body, request_id);
}

bool McpServer::SendReplyPayload(const std::string& payload, const std::string& request_id) {
    auto& app = Application::GetInstance();
    if (app.IsMcpTransportReady()) {
        app.SendMcpMessage(payload);
        return true;
    }

    if (octo::FeatureEnabled(GetFeatureMask(), octo::kFeatureReceiptCompensation)) {
        GetReceiptQueue().Enqueue(request_id, payload);
        GetAgentLite().RecordDecision("reply", octo::GuardAction::kHold, 0, "transport_not_ready");
    }
    return false;
}

void McpServer::FlushPendingReplies() {
    auto& app = Application::GetInstance();
    if (!app.IsMcpTransportReady()) {
        return;
    }
    if (!octo::FeatureEnabled(GetFeatureMask(), octo::kFeatureReceiptCompensation)) {
        return;
    }

    GetReceiptQueue().Flush([&app](const std::string& message) {
        app.SendMcpMessage(message);
        return true;
    });
}

void McpServer::GetToolsList(int id, const std::string& cursor, bool list_user_only_tools) {
    const int max_payload_size = 8000;
    std::string json = "{\"tools\":[";
    
    bool found_cursor = cursor.empty();
    auto it = tools_.begin();
    std::string next_cursor = "";
    
    while (it != tools_.end()) {
        // 如果我们还没有找到起始位置，继续搜索
        if (!found_cursor) {
            if ((*it)->name() == cursor) {
                found_cursor = true;
            } else {
                ++it;
                continue;
            }
        }

        if (!list_user_only_tools && (*it)->user_only()) {
            ++it;
            continue;
        }
        
        // 添加tool前检查大小
        std::string tool_json = (*it)->to_json() + ",";
        if (json.length() + tool_json.length() + 30 > max_payload_size) {
            // 如果添加这个tool会超出大小限制，设置next_cursor并退出循环
            next_cursor = (*it)->name();
            break;
        }
        
        json += tool_json;
        ++it;
    }
    
    if (json.back() == ',') {
        json.pop_back();
    }
    
    if (json.back() == '[' && !tools_.empty()) {
        // 如果没有添加任何tool，返回错误
        ESP_LOGE(TAG, "tools/list: Failed to add tool %s because of payload size limit", next_cursor.c_str());
        ReplyError(id, "Failed to add tool " + next_cursor + " because of payload size limit");
        return;
    }

    if (next_cursor.empty()) {
        json += "]}";
    } else {
        json += "],\"nextCursor\":\"" + next_cursor + "\"}";
    }
    
    ReplyResult(id, json);
}

void McpServer::DoToolCall(int id, const std::string& tool_name, const cJSON* tool_arguments) {
    auto tool_iter = std::find_if(tools_.begin(), tools_.end(), 
                                 [&tool_name](const McpTool* tool) { 
                                     return tool->name() == tool_name; 
                                 });
    
    if (tool_iter == tools_.end()) {
        ESP_LOGE(TAG, "tools/call: Unknown tool: %s", tool_name.c_str());
        ReplyError(id, "Unknown tool: " + tool_name);
        return;
    }

    PropertyList arguments = (*tool_iter)->properties();
    try {
        for (auto& argument : arguments) {
            bool found = false;
            if (cJSON_IsObject(tool_arguments)) {
                auto value = cJSON_GetObjectItem(tool_arguments, argument.name().c_str());
                if (argument.type() == kPropertyTypeBoolean && cJSON_IsBool(value)) {
                    argument.set_value<bool>(value->valueint == 1);
                    found = true;
                } else if (argument.type() == kPropertyTypeInteger && cJSON_IsNumber(value)) {
                    argument.set_value<int>(value->valueint);
                    found = true;
                } else if (argument.type() == kPropertyTypeString && cJSON_IsString(value)) {
                    argument.set_value<std::string>(value->valuestring);
                    found = true;
                }
            }

            if (!argument.has_default_value() && !found) {
                ESP_LOGE(TAG, "tools/call: Missing valid argument: %s", argument.name().c_str());
                ReplyError(id, "Missing valid argument: " + argument.name());
                return;
            }
        }
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "tools/call: %s", e.what());
        ReplyError(id, e.what());
        return;
    }

    auto feature_mask = GetFeatureMask();
    auto guard_decision = GetPolicyGuard().Evaluate(
        tool_name,
        Application::GetInstance().IsMcpTransportReady(),
        feature_mask,
        GetPolicyStore(),
        GetCapabilityManifest()
    );

    const auto& snapshot = GetPolicyStore().GetSnapshot();
    octo::ReplyMeta meta;
    meta.decision = octo::GuardActionToString(guard_decision.action);
    meta.risk_level = guard_decision.risk_level;
    meta.policy_version = snapshot.policy_version;
    meta.request_id = std::to_string(id);
    meta.reason_code = guard_decision.reason_code;

    GetAgentLite().RecordDecision(tool_name, guard_decision.action, guard_decision.risk_level, guard_decision.reason_code);

    if (guard_decision.action != octo::GuardAction::kExecute) {
        ReplyError(id, guard_decision.message, &meta);
        return;
    }

    // Use main thread to call the tool
    auto& app = Application::GetInstance();
    app.Schedule([this, id, tool_iter, arguments = std::move(arguments), meta]() {
        try {
            ReplyResult(id, (*tool_iter)->Call(arguments), &meta);
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "tools/call: %s", e.what());
            auto fault_meta = meta;
            fault_meta.decision = octo::GuardActionToString(octo::GuardAction::kFault);
            fault_meta.reason_code = "tool_execution_exception";
            ReplyError(id, e.what(), &fault_meta);
        }
    });
}
