#pragma once

#include "osr/routing/profile.h"

#include "nigiri/types.h"

namespace motis {

constexpr auto const kOdmTransportModeId =
    static_cast<nigiri::transport_mode_id_t>(osr::kNumProfiles);
constexpr auto const kGbfsTransportModeIdOffset =
    static_cast<nigiri::transport_mode_id_t>(osr::kNumProfiles + 1U);

}  // namespace motis