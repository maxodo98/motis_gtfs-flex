#pragma once

#include <optional>

#include "osr/location.h"
#include "osr/routing/route.h"

#include "motis-api/motis-api.h"
#include "motis/fwd.h"
#include "motis/types.h"

namespace motis {

using transport_mode_t = std::uint32_t;

using street_routing_cache_key_t = std::
    tuple<osr::location, osr::location, transport_mode_t, nigiri::unixtime_t>;

using street_routing_cache_t =
    hash_map<street_routing_cache_key_t, std::optional<osr::path>>;

api::Itinerary dummy_itinerary(api::Place const& from,
                               api::Place const& to,
                               api::ModeEnum,
                               nigiri::unixtime_t const start_time,
                               nigiri::unixtime_t const end_time);

api::Itinerary route(osr::ways const&,
                     osr::lookup const&,
                     gbfs::gbfs_routing_data&,
                     elevators const*,
                     api::Place const& from,
                     api::Place const& to,
                     api::ModeEnum,
                     bool wheelchair,
                     nigiri::unixtime_t start_time,
                     std::optional<nigiri::unixtime_t> end_time,
                     double max_matching_distance,
                     gbfs::gbfs_products_ref,
                     street_routing_cache_t&,
                     osr::bitvec<osr::node_idx_t>& blocked_mem,
                     std::chrono::seconds max = std::chrono::seconds{3600},
                     bool dummy = false);

api::Itinerary route(osr::ways const&,
                     osr::lookup const&,
                     gbfs::gbfs_routing_data&,
                     elevators const*,
                     api::Place const& from,
                     api::Place const& to,
                     std::string const& from_geo,
                     std::string const& to_geo,
                     std::string const& trip,
                     api::ModeEnum,
                     bool wheelchair,
                     nigiri::unixtime_t start_time,
                     std::optional<nigiri::unixtime_t> end_time,
                     gbfs::gbfs_products_ref,
                     street_routing_cache_t&,
                     osr::bitvec<osr::node_idx_t>& blocked_mem,
                     bool is_flex = false,
                     std::chrono::seconds max = std::chrono::seconds{3600});

std::optional<osr::path> get_path(osr::ways const& w,
                                  osr::lookup const& l,
                                  osr::location const& from,
                                  osr::location const& to,
                                  osr::search_profile const profile,
                                  nigiri::unixtime_t const start_time,
                                  osr::cost_t const max);

std::optional<osr::path> get_path(osr::ways const& w,
                                  osr::lookup const& l,
                                  elevators const* e,
                                  osr::sharing_data const* sharing,
                                  osr::location const& from,
                                  osr::location const& to,
                                  transport_mode_t const transport_mode,
                                  osr::search_profile const profile,
                                  nigiri::unixtime_t const start_time,
                                  osr::cost_t const max,
                                  street_routing_cache_t& cache,
                                  osr::bitvec<osr::node_idx_t>& blocked_mem);

}  // namespace motis
