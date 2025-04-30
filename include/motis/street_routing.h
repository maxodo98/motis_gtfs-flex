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
                     gbfs::gbfs_products_ref,
                     street_routing_cache_t&,
                     osr::bitvec<osr::node_idx_t>& blocked_mem,
                     std::chrono::seconds max = std::chrono::seconds{3600});

api::Itinerary route(
    osr::ways const&,
    osr::lookup const&,
    gbfs::gbfs_routing_data&,
    elevators const*,
    api::Place const& from,
    api::Place const& to,
    std::optional<std::string> from_geometry,
    std::optional<std::string> to_geometry,
    std::optional<std::string> trip_id,
    api::ModeEnum,
    bool wheelchair,
    nigiri::unixtime_t start_time,
    std::optional<nigiri::unixtime_t> end_time,
    gbfs::gbfs_products_ref,
    street_routing_cache_t&,
    osr::bitvec<osr::node_idx_t>& blocked_mem,
    nigiri::timetable const& tt,
    nigiri::unixtime_t const now =
        std::chrono::time_point_cast<nigiri::i32_minutes>(*openapi::now()),
    bool arrive_by = false,
    std::chrono::seconds max = std::chrono::seconds{3600});

api::Itinerary route(
    osr::ways const&,
    osr::lookup const&,
    gbfs::gbfs_routing_data&,
    elevators const*,
    api::Place const& from,
    api::Place const& to,
    api::ModeEnum,
    bool wheelchair,
    nigiri::unixtime_t start_time,
    std::optional<nigiri::unixtime_t> end_time,
    gbfs::gbfs_products_ref,
    street_routing_cache_t&,
    osr::bitvec<osr::node_idx_t>& blocked_mem,
    nigiri::timetable const& tt,
    nigiri::unixtime_t const now =
        std::chrono::time_point_cast<nigiri::i32_minutes>(*openapi::now()),
    bool arrive_by = false,
    std::chrono::seconds max = std::chrono::seconds{3600});

struct flex_trip {
  std::string from_geometry_;
  std::string to_geometry_;
  std::string trip_id_;
  nigiri::duration_t waiting_time_;
  nigiri::duration_t travel_difference_;
};

flex_trip get_flex_trip(nigiri::timetable const& tt,
                        nigiri::unixtime_t const now,
                        nigiri::unixtime_t const t,
                        nigiri::duration_t const travel_time,
                        geo::latlng const& from,
                        geo::latlng const& to,
                        bool const arrive_by);

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
