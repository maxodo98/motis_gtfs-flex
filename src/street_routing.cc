#include "motis/street_routing.h"

#include <nigiri/loader/gtfs/booking_rule.h>

#include "geo/polyline_format.h"

#include "utl/concat.h"

#include "osr/routing/route.h"
#include "osr/routing/sharing_data.h"

#include "motis/constants.h"
#include "motis/gbfs/mode.h"
#include "motis/gbfs/routing_data.h"
#include "motis/mode_to_profile.h"
#include "motis/place.h"
#include "motis/polyline.h"
#include "motis/update_rtt_td_footpaths.h"

namespace n = nigiri;

namespace motis {

std::optional<osr::path> get_path(osr::ways const& w,
                                  osr::lookup const& l,
                                  osr::location const& from,
                                  osr::location const& to,
                                  osr::search_profile const profile,
                                  nigiri::unixtime_t const start_time,
                                  osr::cost_t const max) {
  auto cache = street_routing_cache_t{};
  auto blocked = osr::bitvec<osr::node_idx_t>{};
  return get_path(w, l, nullptr, nullptr, from, to, 0, profile, start_time, max,
                  cache, blocked);
}

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
                                  osr::bitvec<osr::node_idx_t>& blocked_mem) {
  auto const s = e ? get_states_at(w, l, *e, start_time, from.pos_)
                   : std::optional{std::pair<nodes_t, states_t>{}};
  auto const& [e_nodes, e_states] = *s;
  auto const key =
      street_routing_cache_key_t{from, to, transport_mode, start_time};
  auto const it = cache.find(key);
  auto const path =
      it != end(cache)
          ? it->second
          : osr::route(
                w, l, profile, from, to, max, osr::direction::kForward,
                kMaxMatchingDistance,
                s ? &set_blocked(e_nodes, e_states, blocked_mem) : nullptr,
                sharing);
  if (it == end(cache)) {
    cache.emplace(std::pair{key, path});
  }
  if (!path.has_value()) {
    if (it == end(cache)) {
      std::cout << "no path found: " << from << " -> " << to
                << ", profile=" << to_str(profile) << std::endl;
    }
  }
  return path;
}

std::vector<api::StepInstruction> get_step_instructions(
    osr::ways const& w,
    osr::location const& from,
    osr::location const& to,
    std::span<osr::path::segment const> segments) {
  auto steps = std::vector<api::StepInstruction>{};
  auto pred_lvl = from.lvl_.to_float();
  for (auto const& s : segments) {
    if (s.from_ != osr::node_idx_t::invalid() && s.from_ < w.n_nodes() &&
        w.r_->node_properties_[s.from_].is_elevator()) {
      steps.push_back(api::StepInstruction{
          .relativeDirection_ = api::DirectionEnum::ELEVATOR,
          .fromLevel_ = pred_lvl,
          .toLevel_ = s.from_level_.to_float()});
    }

    auto const way_name = s.way_ == osr::way_idx_t::invalid()
                              ? osr::string_idx_t::invalid()
                              : w.way_names_[s.way_];
    auto const props = s.way_ != osr::way_idx_t::invalid()
                           ? w.r_->way_properties_[s.way_]
                           : osr::way_properties{};
    steps.push_back(api::StepInstruction{
        .relativeDirection_ =
            s.way_ != osr::way_idx_t::invalid()
                ? (props.is_elevator() ? api::DirectionEnum::ELEVATOR
                   : props.is_steps()  ? api::DirectionEnum::STAIRS
                                       : api::DirectionEnum::CONTINUE)
                : api::DirectionEnum::CONTINUE,  // TODO entry/exit/u-turn
        .distance_ = static_cast<double>(s.dist_),
        .fromLevel_ = s.from_level_.to_float(),
        .toLevel_ = s.to_level_.to_float(),
        .osmWay_ = s.way_ == osr::way_idx_t ::invalid()
                       ? std::nullopt
                       : std::optional{static_cast<std::int64_t>(
                             to_idx(w.way_osm_idx_[s.way_]))},
        .polyline_ = to_polyline<7>(s.polyline_),
        .streetName_ = way_name == osr::string_idx_t::invalid()
                           ? ""
                           : std::string{w.strings_[way_name].view()},
        .exit_ = {},  // TODO
        .stayOn_ = false,  // TODO
        .area_ = false  // TODO
    });
  }

  if (!segments.empty()) {
    auto& last = segments.back();
    if (last.to_ != osr::node_idx_t::invalid() && last.to_ < w.n_nodes() &&
        w.r_->node_properties_[last.to_].is_elevator()) {
      steps.push_back(api::StepInstruction{
          .relativeDirection_ = api::DirectionEnum::ELEVATOR,
          .fromLevel_ = pred_lvl,
          .toLevel_ = to.lvl_.to_float()});
    }
  }

  return steps;
}

struct sharing {
  sharing(osr::ways const& w,
          gbfs::gbfs_routing_data& gbfs_rd,
          gbfs::gbfs_products_ref const prod_ref)
      : w_{w},
        gbfs_rd_{gbfs_rd},
        provider_{*gbfs_rd_.data_->providers_.at(prod_ref.provider_)},
        products_{provider_.products_.at(prod_ref.products_)},
        prod_rd_{gbfs_rd_.get_products_routing_data(prod_ref)} {}

  api::Rental get_rental(osr::node_idx_t const n) const {
    auto ret = rental_;
    auto const& an =
        prod_rd_->compressed_.additional_nodes_.at(get_additional_node_idx(n));
    std::visit(utl::overloaded{
                   [&](gbfs::additional_node::station const& s) {
                     auto const& st = provider_.stations_.at(s.id_);
                     ret.stationName_ = st.info_.name_;
                     ret.rentalUriAndroid_ = st.info_.rental_uris_.android_;
                     ret.rentalUriIOS_ = st.info_.rental_uris_.ios_;
                     ret.rentalUriWeb_ = st.info_.rental_uris_.web_;
                   },
                   [&](gbfs::additional_node::vehicle const& v) {
                     auto const& vi = provider_.vehicle_status_.at(v.idx_);
                     ret.rentalUriAndroid_ = vi.rental_uris_.android_;
                     ret.rentalUriIOS_ = vi.rental_uris_.ios_;
                     ret.rentalUriWeb_ = vi.rental_uris_.web_;
                   }},
               an.data_);
    return ret;
  }

  geo::latlng get_node_pos(osr::node_idx_t const n) const {
    return std::visit(
        utl::overloaded{
            [&](gbfs::additional_node::station const& s) {
              return provider_.stations_.at(s.id_).info_.pos_;
            },
            [&](gbfs::additional_node::vehicle const& vehicle) {
              return provider_.vehicle_status_.at(vehicle.idx_).pos_;
            }},
        prod_rd_->compressed_.additional_nodes_.at(get_additional_node_idx(n))
            .data_);
  }

  std::size_t get_additional_node_idx(osr::node_idx_t const n) const {
    return to_idx(n) - sharing_data_.additional_node_offset_;
  }

  osr::ways const& w_;
  gbfs::gbfs_routing_data& gbfs_rd_;
  gbfs::gbfs_provider const& provider_;
  gbfs::provider_products const& products_;
  gbfs::products_routing_data const* prod_rd_;
  osr::sharing_data sharing_data_{
      .start_allowed_ = prod_rd_->start_allowed_,
      .end_allowed_ = prod_rd_->end_allowed_,
      .through_allowed_ = prod_rd_->through_allowed_,
      .additional_node_offset_ = w_.n_nodes(),
      .additional_edges_ = prod_rd_->compressed_.additional_edges_};
  api::Rental rental_{
      .systemId_ = provider_.sys_info_.id_,
      .systemName_ = provider_.sys_info_.name_,
      .url_ = provider_.sys_info_.url_,
      .formFactor_ = gbfs::to_api_form_factor(products_.form_factor_),
      .propulsionType_ =
          gbfs::to_api_propulsion_type(products_.propulsion_type_),
      .returnConstraint_ =
          gbfs::to_api_return_constraint(products_.return_constraint_)};
};

api::Itinerary dummy_itinerary(api::Place const& from,
                               api::Place const& to,
                               api::ModeEnum const mode,
                               n::unixtime_t const start_time,
                               n::unixtime_t const end_time) {
  auto itinerary = api::Itinerary{
      .duration_ = std::chrono::duration_cast<std::chrono::seconds>(end_time -
                                                                    start_time)
                       .count(),
      .startTime_ = start_time,
      .endTime_ = end_time};
  auto& leg = itinerary.legs_.emplace_back(api::Leg{
      .mode_ = mode,
      .from_ = from,
      .to_ = to,
      .duration_ = std::chrono::duration_cast<std::chrono::seconds>(end_time -
                                                                    start_time)
                       .count(),
      .startTime_ = start_time,
      .endTime_ = end_time});
  leg.from_.departure_ = leg.startTime_;
  leg.to_.arrival_ = leg.endTime_;
  return itinerary;
}

flex_trip get_flex_trip(nigiri::timetable const& tt,
                        nigiri::unixtime_t const now,
                        nigiri::unixtime_t const t,
                        nigiri::duration_t const travel_time,
                        geo::latlng const& from,
                        geo::latlng const& to,
                        bool const arrive_by) {
  auto const is_available = [&](nigiri::trip_idx_t const t_idx,
                                nigiri::unixtime_t const time) {
    auto const day = date::sys_days(floor<date::days>(time));
    if (day < tt.internal_interval_days().from_) {
      return false;
    }
    auto const bit = static_cast<std::uint32_t>(
        (day - tt.internal_interval_days().from_).count());

    if (tt.bitfields_[tt.trip_service_[t_idx]].size() <= bit) {
      return false;
    }
    return tt.bitfields_[tt.trip_service_[t_idx]][bit];
  };

  auto const process_flex_trip = [](nigiri::timetable const& tt,
                                    nigiri::geometry_trip_idx const& id,
                                    nigiri::unixtime_t const start_time,
                                    nigiri::unixtime_t const now,
                                    bool const is_pickup) {
    auto const gt_it = tt.geometry_trip_idxs_.find(id);
    if (gt_it == end(tt.geometry_trip_idxs_)) {
      log(nigiri::log_lvl::error, "street_routing.get_flex_id",
          "Unknown geometry-trip ({}, {}) required", id.trip_idx_,
          id.geometry_idx_);
      return flex_result{nigiri::duration_t::max(), nigiri::unixtime_t::min(),
                         nigiri::stop_window{}, true};
    }
    auto const gt_idx = gt_it->second;
    auto const type =
        is_pickup ? tt.pickup_types_[gt_idx] : tt.dropoff_types_[gt_idx];
    if (type == nigiri::kUnavailableType) {
      return flex_result{nigiri::duration_t::max(), nigiri::unixtime_t::min(),
                         nigiri::stop_window{}, true};
    }
    auto window = tt.window_times_[gt_idx];

    auto const current_day = floor<date::days>(start_time);
    auto max_time = nigiri::unixtime_t::max();
    auto const b_idx = is_pickup ? tt.pickup_booking_rules_[gt_idx]
                                 : tt.dropoff_booking_rules_[gt_idx];
    nigiri::duration_t booking_time = nigiri::duration_t::zero();
    if (b_idx != nigiri::booking_rule_idx_t::invalid()) {
      auto const booking_rule = tt.booking_rules_[b_idx];
      switch (booking_rule.type_) {
        case nigiri::loader::gtfs::Booking_type::kRealTimeBooking: break;
        case nigiri::loader::gtfs::Booking_type::kSameDayBooking:
          booking_time +=
              nigiri::duration_t{booking_rule.prior_notice_duration_min_};
          if (booking_rule.prior_notice_duration_max_ != 0) {
            max_time = now + nigiri::i32_minutes{
                                 booking_rule.prior_notice_duration_max_};
          }
          break;
        case nigiri::loader::gtfs::Booking_type::kPriorDaysBooking:
          booking_time +=
              nigiri::duration_t{booking_rule.prior_notice_last_day_ * 24 *
                                 60} -
              booking_rule.prior_notice_last_time_;
          if (booking_rule.prior_notice_start_day_ != 0) {
            max_time = floor<date::days>(now) +
                       booking_rule.prior_notice_start_time_ +
                       nigiri::duration_t{booking_rule.prior_notice_start_day_ *
                                          24 * 60};
          }
          break;
        default: {
          return flex_result{nigiri::duration_t::max(),
                             nigiri::unixtime_t::min(), nigiri::stop_window{},
                             true};
        }
      }
      if (now + booking_time > current_day + window.end_) {
        return flex_result{nigiri::duration_t::max(), nigiri::unixtime_t::min(),
                           nigiri::stop_window{}, true};
      }
      if (is_pickup) {
        window.start_ =
            std::max(window.start_,
                     nigiri::duration_t{now + booking_time - current_day});
      }
    }
    return flex_result{booking_time, max_time, window, false};
  };
  nigiri::unixtime_t const start_day = floor<date::days>(t);
  auto best_waiting_time = nigiri::duration_t::max();
  auto best_travel_time = nigiri::duration_t::max();
  auto best_source_flex_stop = nigiri::geometry_idx_t::invalid();
  auto best_target_flex_stop = nigiri::geometry_idx_t::invalid();
  auto best_trip = nigiri::trip_idx_t::invalid();
  for (auto const source_flex_stop : tt.lookup_td_stops(from, 0.0)) {
    for (auto const trip : tt.geometry_idx_to_trip_idxs_[source_flex_stop]) {
      if (!is_available(trip, t)) {
        continue;
      }

      auto [pickup_booking_time, max_dep_time, pickup_window, pickup_skip] =
          process_flex_trip(tt,
                            nigiri::geometry_trip_idx{trip, source_flex_stop},
                            t, now, true);
      if (pickup_skip) {
        continue;
      }

      for (auto const target_flex_stop : tt.trip_idx_to_geometry_idxs_[trip]) {
        if (!tt.geometry_[target_flex_stop].contains(to)) {
          continue;
        }
        auto [dropoff_booking_time, max_arr_time, dropoff_window,
              dropoff_skip] =
            process_flex_trip(tt,
                              nigiri::geometry_trip_idx{trip, target_flex_stop},
                              t, now, false);
        if (dropoff_skip) {
          continue;
        }
        auto travel_time_tmp = travel_time;

        if (std::max(dropoff_window.start_ - pickup_window.end_,
                     nigiri::duration_t::zero()) > travel_time_tmp) {
          travel_time_tmp = dropoff_window.start_ - pickup_window.end_;
        }

        auto booking_time = std::max(pickup_booking_time,
                                     dropoff_booking_time - travel_time_tmp);

        auto earliest_dep_time =
            std::max({arrive_by ? nigiri::unixtime_t::min() : t,
                      start_day + pickup_window.start_, now + booking_time,
                      start_day + dropoff_window.start_ - travel_time_tmp});
        auto latest_dep_time = std::min(
            {arrive_by ? t - travel_time_tmp : nigiri::unixtime_t::max(),
             start_day + pickup_window.end_,
             start_day + dropoff_window.end_ - travel_time_tmp, max_dep_time,
             max_arr_time - travel_time_tmp});
        if (latest_dep_time < earliest_dep_time) {
          continue;
        }

        auto const waiting_time = arrive_by
                                      ? t - (latest_dep_time + travel_time_tmp)
                                      : earliest_dep_time - t;
        if (best_waiting_time == nigiri::duration_t::max() ||
            best_travel_time == nigiri::duration_t::max() ||
            waiting_time + travel_time_tmp <
                best_waiting_time + best_travel_time) {
          best_waiting_time = waiting_time;
          best_travel_time = travel_time;
          best_source_flex_stop = source_flex_stop;
          best_target_flex_stop = target_flex_stop;
          best_trip = trip;
        }
      }
    }
  }

  auto const from_id =
      std::string(tt.geometry_ids_[best_source_flex_stop].begin(),
                  tt.geometry_ids_[best_source_flex_stop].end());
  auto const to_id =
      std::string(tt.geometry_ids_[best_target_flex_stop].begin(),
                  tt.geometry_ids_[best_target_flex_stop].end());
  auto const trip_id =
      std::string(tt.trip_id_strings_[tt.trip_ids_[best_trip][0]].begin(),
                  tt.trip_id_strings_[tt.trip_ids_[best_trip][0]].end());

  return {from_id, to_id, trip_id, best_waiting_time,
          (best_travel_time - travel_time)};
}

api::Itinerary route(osr::ways const& w,
                     osr::lookup const& l,
                     gbfs::gbfs_routing_data& gbfs_rd,
                     elevators const* e,
                     api::Place const& from,
                     api::Place const& to,
                     std::optional<std::string> from_geometry,
                     std::optional<std::string> to_geometry,
                     std::optional<std::string> trip_id,
                     api::ModeEnum mode,
                     bool wheelchair,
                     nigiri::unixtime_t start_time,
                     std::optional<nigiri::unixtime_t> end_time,
                     gbfs::gbfs_products_ref prod_ref,
                     street_routing_cache_t& cache,
                     osr::bitvec<osr::node_idx_t>& blocked_mem,
                     nigiri::timetable const& tt,
                     nigiri::unixtime_t const now,
                     bool arrive_by,
                     std::chrono::seconds max) {
  auto const profile = to_profile(mode, wheelchair);
  utl::verify(
      profile != osr::search_profile::kBikeSharing || gbfs_rd.has_data(),
      "sharing mobility not configured");

  auto const is_additional_node = [&](osr::node_idx_t const n) {
    return n >= w.n_nodes();
  };

  auto const sharing_data = profile == osr::search_profile::kBikeSharing
                                ? std::optional{sharing(w, gbfs_rd, prod_ref)}
                                : std::nullopt;

  auto const get_node_pos = [&](osr::node_idx_t const n) -> geo::latlng {
    if (n == osr::node_idx_t::invalid()) {
      return {};
    } else if (!is_additional_node(n)) {
      return w.get_node_pos(n).as_latlng();
    } else {
      return sharing_data.value().get_node_pos(n);
    }
  };

  auto const path = [&]() {
    auto p = get_path(
        w, l, e, sharing_data ? &sharing_data->sharing_data_ : nullptr,
        get_location(from), get_location(to),
        static_cast<transport_mode_t>(gbfs_rd.get_transport_mode(prod_ref)),
        to_profile(mode, wheelchair), start_time,
        static_cast<osr::cost_t>(max.count()), cache, blocked_mem);

    if (p.has_value() && profile == osr::search_profile::kBikeSharing) {
      // Coordinates of additional nodes are not known to osr.
      // Therefore, segments to/from additional have empty polylines.
      for (auto& s : p->segments_) {
        if (s.polyline_.empty()) {
          s.polyline_ =
              geo::polyline{get_node_pos(s.from_), get_node_pos(s.to_)};
        }
      }
    }

    return p;
  }();

  if (!path.has_value()) {
    if (!end_time.has_value()) {
      return {};
    }
    std::cout << "ROUTING\n  FROM:  " << from << "     \n    TO:  " << to
              << "\n  -> CREATING DUMMY LEG (mode=" << mode
              << ", profile=" << osr::to_str(profile)
              << ", provider=" << prod_ref.provider_
              << ", products=" << prod_ref.products_ << ")\n";
    return dummy_itinerary(from, to, mode, start_time, *end_time);
  }

  auto waiting_time = nigiri::duration_t{0};
  auto travel_difference = nigiri::duration_t{0};
  auto f_geometry = std::string("");
  auto t_geometry = std::string("");
  auto trip = std::string("");
  if (from_geometry.has_value() && to_geometry.has_value() &&
      trip_id.has_value() && profile == osr::search_profile::kFlex) {
    f_geometry = from_geometry.value();
    t_geometry = to_geometry.value();
    trip = trip_id.value();
  } else if (profile == osr::search_profile::kFlex) {
    auto const travel_time = nigiri::duration_t{
        static_cast<std::uint16_t>(std::ceil(path->cost_ / 60.0))};
    auto [f_g, t_g, t_id, wt, td] = get_flex_trip(
        tt, now, start_time, travel_time, geo::latlng{from.lat_, from.lon_},
        geo::latlng{to.lat_, to.lon_}, arrive_by);
    waiting_time = wt;
    travel_difference = td;
    f_geometry = f_g;
    t_geometry = t_g;
    trip = t_id;
  }

  auto itinerary = api::Itinerary{
      .duration_ = end_time ? std::chrono::duration_cast<std::chrono::seconds>(
                                  *end_time - start_time)
                                  .count()
                            : path->cost_ + static_cast<osr::cost_t>(
                                                (waiting_time.count() +
                                                 travel_difference.count()) *
                                                60),
      .startTime_ =
          arrive_by
              ? start_time - (std::chrono::seconds{path->cost_} +
                              std::chrono::duration_cast<std::chrono::seconds>(
                                  waiting_time + travel_difference))
              : start_time,
      .endTime_ = arrive_by
                      ? start_time
                      : start_time + std::chrono::seconds{path->cost_} +
                            std::chrono::duration_cast<std::chrono::seconds>(
                                waiting_time + travel_difference),
      .transfers_ = 0};

  auto t = std::chrono::time_point_cast<std::chrono::seconds>(
      arrive_by ? start_time - waiting_time : start_time + waiting_time);
  auto pred_place = from;
  auto pred_end_time = t;
  utl::equal_ranges_linear(
      path->segments_, [](auto&& a, auto&& b) { return a.mode_ == b.mode_; },
      [&](auto&& lb, auto&& ub) {
        auto const range = std::span{lb, ub};
        auto const is_last_leg = ub == end(path->segments_);
        auto const is_bike_leg = lb->mode_ == osr::mode::kBike;
        auto const from_additional_node =
            is_additional_node(range.front().from_);
        auto const to_additional_node = is_additional_node(range.back().to_);
        auto const is_rental =
            (profile == osr::search_profile::kBikeSharing && is_bike_leg &&
             (from_additional_node || to_additional_node));

        auto const to_node = range.back().to_;
        auto const to_pos = get_node_pos(to_node);
        auto const next_place =
            is_last_leg ? to
            // All modes except sharing mobility have only one leg.
            // -> This is not the last leg = it has to be sharing mobility.
            : profile == osr::search_profile::kBikeSharing
                ? api::Place{.name_ =
                                 sharing_data.value().provider_.sys_info_.name_,
                             .lat_ = to_pos.lat_,
                             .lon_ = to_pos.lng_,
                             .vertexType_ = api::VertexTypeEnum::BIKESHARE}
                : api::Place{.lat_ = to_pos.lat_,
                             .lon_ = to_pos.lng_,
                             .vertexType_ = api::VertexTypeEnum::NORMAL};

        auto concat = geo::polyline{};
        auto dist = 0.0;
        t = arrive_by ? t - std::chrono::duration_cast<std::chrono::seconds>(
                                travel_difference)
                      : t + std::chrono::duration_cast<std::chrono::seconds>(
                                travel_difference);
        for (auto const& p : range) {
          utl::concat(concat, p.polyline_);
          if (p.cost_ != osr::kInfeasible) {
            t = arrive_by ? t - std::chrono::seconds{p.cost_}
                          : t + std::chrono::seconds{p.cost_};
            dist += p.dist_;
          }
        }

        auto& leg = itinerary.legs_.emplace_back(api::Leg{
            .mode_ = is_rental ? api::ModeEnum::RENTAL : to_mode(lb->mode_),
            .from_ = pred_place,
            .to_ = next_place,
            .duration_ = std::chrono::duration_cast<std::chrono::seconds>(
                             arrive_by ? start_time - t : t - pred_end_time)
                             .count(),
            .startTime_ = arrive_by ? t : pred_end_time,
            .endTime_ = arrive_by ? pred_end_time
                                  : (is_last_leg && end_time ? *end_time : t),
            .distance_ = dist,
            .tripId_ = trip.empty() ? std::nullopt : std::optional(trip),
            .legGeometry_ = to_polyline<7>(concat),
            .steps_ = get_step_instructions(w, get_location(from),
                                            get_location(to), range),
            .rental_ = is_rental
                           ? std::optional{sharing_data->get_rental(
                                 from_additional_node ? range.front().from_
                                                      : range.back().to_)}
                           : std::nullopt,
            .from_geometry_ =
                f_geometry.empty() ? std::nullopt : std::optional(f_geometry),
            .to_geometry_ =
                t_geometry.empty() ? std::nullopt : std::optional(t_geometry)});

        leg.from_.departure_ = leg.from_.scheduledDeparture_ =
            leg.scheduledStartTime_ = leg.startTime_;
        leg.to_.arrival_ = leg.to_.scheduledArrival_ = leg.scheduledEndTime_ =
            leg.endTime_;

        pred_place = next_place;
        pred_end_time = t;
      });

  return itinerary;
}

api::Itinerary route(osr::ways const& w,
                     osr::lookup const& l,
                     gbfs::gbfs_routing_data& gbfs_rd,
                     elevators const* e,
                     api::Place const& from,
                     api::Place const& to,
                     api::ModeEnum mode,
                     bool wheelchair,
                     nigiri::unixtime_t start_time,
                     std::optional<nigiri::unixtime_t> end_time,
                     gbfs::gbfs_products_ref prod_ref,
                     street_routing_cache_t& cache,
                     osr::bitvec<osr::node_idx_t>& blocked_mem,
                     nigiri::timetable const& tt,
                     nigiri::unixtime_t const now,
                     bool arrive_by,
                     std::chrono::seconds max) {
  return route(w, l, gbfs_rd, e, from, to, std::nullopt, std::nullopt,
               std::nullopt, mode, wheelchair, start_time, end_time, prod_ref,
               cache, blocked_mem, tt, now, arrive_by, max);
}

api::Itinerary route(osr::ways const& w,
                     osr::lookup const& l,
                     gbfs::gbfs_routing_data& gbfs_rd,
                     elevators const* e,
                     api::Place const& from,
                     api::Place const& to,
                     api::ModeEnum const mode,
                     bool const wheelchair,
                     n::unixtime_t const start_time,
                     std::optional<n::unixtime_t> const end_time,
                     gbfs::gbfs_products_ref const prod_ref,
                     street_routing_cache_t& cache,
                     osr::bitvec<osr::node_idx_t>& blocked_mem,
                     std::chrono::seconds const max) {
  return route(w, l, gbfs_rd, e, from, to, std::nullopt, std::nullopt,
               std::nullopt, mode, wheelchair, start_time, end_time, prod_ref,
               cache, blocked_mem, nigiri::timetable{}, nigiri::unixtime_t{},
               false, max);
}

}  // namespace motis
