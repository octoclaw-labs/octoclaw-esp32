#ifndef OCTO_OPENCLAW_EXTENSION_CATALOG_H
#define OCTO_OPENCLAW_EXTENSION_CATALOG_H

#include <cstddef>

#include <cJSON.h>

namespace octo::channels {

struct ExtensionCatalogEntry {
    const char* id;
    const char* adapter_type;
    const char* source_dir;
    bool has_channels;
    bool has_providers;
    bool has_skills;
    bool has_config_schema;
    int config_key_count;
};

const ExtensionCatalogEntry* GetOpenClawExtensionCatalog(size_t* count);
cJSON* BuildOpenClawExtensionCatalogJson();

}  // namespace octo::channels

#endif  // OCTO_OPENCLAW_EXTENSION_CATALOG_H
