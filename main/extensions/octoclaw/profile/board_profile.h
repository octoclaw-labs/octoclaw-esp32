#ifndef OCTO_BOARD_PROFILE_H
#define OCTO_BOARD_PROFILE_H

#include <stddef.h>
#include <stdint.h>
#include <cJSON.h>

#include "extensions/octoclaw/core/octo_agent_types.h"

namespace octo {

const BoardProfile& GetCompiledBoardProfile();
uint32_t BuildFeatureMaskFromKconfig();
uint32_t GetEffectiveFeatureMask(int32_t policy_feature_mask_override);
int GetRuleMax();
int GetTelemetryRingSize();
cJSON* BuildBoardProfileJson(uint32_t effective_feature_mask, int policy_version);

}  // namespace octo

#endif  // OCTO_BOARD_PROFILE_H
