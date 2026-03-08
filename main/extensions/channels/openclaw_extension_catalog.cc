#include "openclaw_extension_catalog.h"

#include "openclaw_extension_catalog.generated.h"

namespace octo::channels {

const ExtensionCatalogEntry* GetOpenClawExtensionCatalog(size_t* count) {
    if (count != nullptr) {
        *count = kOpenClawTranslatedExtensionCount;
    }
    return kOpenClawTranslatedExtensions;
}

cJSON* BuildOpenClawExtensionCatalogJson() {
    size_t count = 0;
    const auto* catalog = GetOpenClawExtensionCatalog(&count);

    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "count", static_cast<double>(count));

    cJSON* items = cJSON_CreateArray();
    for (size_t i = 0; i < count; ++i) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", catalog[i].id);
        cJSON_AddStringToObject(item, "adapterType", catalog[i].adapter_type);
        cJSON_AddStringToObject(item, "sourceDir", catalog[i].source_dir);
        cJSON_AddBoolToObject(item, "hasChannels", catalog[i].has_channels);
        cJSON_AddBoolToObject(item, "hasProviders", catalog[i].has_providers);
        cJSON_AddBoolToObject(item, "hasSkills", catalog[i].has_skills);
        cJSON_AddBoolToObject(item, "hasConfigSchema", catalog[i].has_config_schema);
        cJSON_AddNumberToObject(item, "configKeyCount", catalog[i].config_key_count);
        cJSON_AddItemToArray(items, item);
    }
    cJSON_AddItemToObject(root, "items", items);
    return root;
}

}  // namespace octo::channels
