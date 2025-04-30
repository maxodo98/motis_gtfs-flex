#include <motis/mode_to_profile.h>
#include <motis/street_routing.h>
#include <nigiri/loader/hrd/util.h>
#include <nigiri/timetable.h>
#include <utl/init_from.h>
#include <utl/timing.h>

#include <chrono>
#include "motis/elevators/parse_fasta.h"

#include "gtest/gtest.h"

#include "boost/json.hpp"

#include "motis/config.h"
#include "motis/endpoints/routing.h"
#include "motis/import.h"

// #define BENCHMARK ;

using namespace motis;

namespace json = boost::json;

auto const print_short = [](std::ostream& out, api::Itinerary const& j) {
  auto const format_time = [&](auto&& t, char const* fmt = "%F %H:%M") {
    out << date::format(fmt, *t);
  };
  auto const format_duration = [&](auto&& t, char const* fmt = "%H:%M") {
    out << date::format(fmt, std::chrono::milliseconds{t});
  };

  out << "date=";
  format_time(j.startTime_, "%F");
  out << ", start=";
  format_time(j.startTime_, "%H:%M");
  out << ", end=";
  format_time(j.endTime_, "%H:%M");

  out << ", duration=";
  format_duration(j.duration_ * 1000U);
  out << ", transfers=" << j.transfers_;

  out << ", legs=[\n";
  auto first = true;
  for (auto const& leg : j.legs_) {
    if (!first) {
      out << ",\n    ";
    } else {
      out << "    ";
    }
    first = false;
    out << "(";
    out << "from=" << leg.from_.stopId_.value_or("-")
        << " geometry=" << leg.from_geometry_.value_or("-")
        << " [track=" << leg.from_.track_.value_or("-")
        << ", scheduled_track=" << leg.from_.scheduledTrack_.value_or("-")
        << ", level=" << leg.from_.level_ << "]"
        << ", to=" << leg.to_.stopId_.value_or("-")
        << " geometry=" << leg.to_geometry_.value_or("-")
        << " [track=" << leg.to_.track_.value_or("-")
        << ", scheduled_track=" << leg.to_.scheduledTrack_.value_or("-")
        << ", level=" << leg.to_.level_ << "], ";
    out << "start=";
    format_time(leg.startTime_);
    out << ", mode=";
    out << json::serialize(json::value_from(leg.mode_));
    out << ", trip=\"" << leg.tripId_.value_or("-") << "\"";
    out << ", end=";
    format_time(leg.endTime_);
    out << ")";
  }
  out << "\n]";
};

using namespace std::chrono_literals;

auto const exec_benchmark = [](openapi::date_time_t now,
                               geo::latlng from,
                               geo::latlng to,
                               openapi::date_time_t time,
                               bool arriveBy,
                               std::optional<osr::mode> direct,
                               std::initializer_list<osr::mode> pretransit,
                               std::initializer_list<osr::mode> posttransit) {
  auto const c = config{
      .osm_ = {"benchmark/"
               "resources/australia/australia.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ =
              {{"regular", {.path_ = "benchmark/resources/australia/gtfs.zip"}},
               {"flex",
                {.path_ = "benchmark/resources/australia/gtfs_flex.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c, "benchmark/data-australia", true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  openapi::now_test = now;

  auto request_str = std::stringstream{};
  request_str << "?fromPlace=" << from.lat_ << "," << from.lng_;
  request_str << "&toPlace=" << to.lat_ << "," << to.lng_;
  request_str << "&time=" << time;
  request_str << "&arriveBy=" << (arriveBy ? "true" : "false");
  request_str << "&timetableView=false" << "&useRoutedTransfers=false";
  request_str << "&directModes=";
  if (direct.has_value()) {
    request_str << to_mode(direct.value());
  }
  request_str << "&preTransitModes=";
  for (auto i = 0U; i < pretransit.size(); i++) {
    request_str << to_mode(*(pretransit.begin() + i));
    if (i < pretransit.size() - 1) {
      request_str << ",";
    }
  }
  request_str << "&postTransitModes=";
  for (auto i = 0U; i < posttransit.size(); i++) {
    request_str << to_mode(*(posttransit.begin() + i));
    if (i < posttransit.size() - 1) {
      request_str << ",";
    }
  }
  request_str << "&maxDirectTime=10800";

  UTL_START_TIMING(timer);
  auto plan_response = routing(request_str.str());
  UTL_STOP_TIMING(timer);
  return UTL_TIMING_MS(timer);
};

TEST(motis, motis_switzerland_request_1_car) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::January / 05 / 2025} + 8h + 30min,
                   geo::latlng{47.41064282478368, 9.539225124003337},
                   geo::latlng{47.4485994339791, 9.571887527087142},
                   date::sys_days{date::January / 05 / 2025} + 20h + 0min,
                   false, osr::mode::kCar, {}, {});
}

TEST(motis, motis_switzerland_request_1_flex) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::January / 05 / 2025} + 8h + 30min,
                   geo::latlng{47.41064282478368, 9.539225124003337},
                   geo::latlng{47.4485994339791, 9.571887527087142},
                   date::sys_days{date::January / 05 / 2025} + 20h + 0min,
                   false, osr::mode::kFlex, {}, {});
}

TEST(motis, motis_switzerland_request_2_car) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::January / 03 / 2025} + 8h + 30min,
                   geo::latlng{46.77859375171761, 6.647990839623077},
                   geo::latlng{46.722300906957145, 6.531654831253093},
                   date::sys_days{date::January / 03 / 2025} + 22h + 30min,
                   false, std::nullopt, {osr::mode::kCar}, {osr::mode::kCar});
}

TEST(motis, motis_switzerland_request_2_flex) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::January / 03 / 2025} + 8h + 30min,
                   geo::latlng{46.77859375171761, 6.647990839623077},
                   geo::latlng{46.722300906957145, 6.531654831253093},
                   date::sys_days{date::January / 03 / 2025} + 22h + 30min,
                   false, std::nullopt, {osr::mode::kFlex}, {osr::mode::kFlex});
}

TEST(motis, motis_switzerland_request_3_car) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::February / 03 / 2025} + 8h + 30min,
                   geo::latlng{46.633283638420636, 8.596274873067557},
                   geo::latlng{47.332279891568135, 9.413500965679333},
                   date::sys_days{date::February / 04 / 2025} + 8h + 40min,
                   false, std::nullopt, {osr::mode::kCar}, {osr::mode::kCar});
}

TEST(motis, motis_switzerland_request_3_flex) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::February / 03 / 2025} + 8h + 30min,
                   geo::latlng{46.633283638420636, 8.596274873067557},
                   geo::latlng{47.332279891568135, 9.413500965679333},
                   date::sys_days{date::February / 04 / 2025} + 8h + 40min,
                   false, std::nullopt, {osr::mode::kFlex}, {osr::mode::kFlex});
}

TEST(motis, switzerland) {
  auto const c = config{
      .osm_ = {"benchmark/"
               "resources/switzerland/switzerland.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"regular",
                         {.path_ = "benchmark/resources/switzerland/gtfs.zip"}},
                        {"flex",
                         {.path_ = "benchmark/resources/switzerland/"
                                   "gtfs_flex.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c, "benchmark/data-switzerland", true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   *  Distance:       ca. 5km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::January / 05 / 2025} + 8h + 30min;
  auto plan_response = routing(
      "?fromPlace=47.41064282478368,9.539225124003337"
      "&toPlace=47.4485994339791,9.571887527087142"
      "&time=2025-01-05T20:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-05, start=20:00, end=20:20, duration=00:20, transfers=0, legs=[
    (from=- geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-05 20:00, mode="CAR", trip="-", end=2025-01-05 20:20)
])",
      ss.str());

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   *  From-Geometry:  odv_10
   *  To-Geometry:    odv_12
   *  From-Trips:     odv_j25_3_1_10_10_14-_24, odv_j25_3_1_10_10_56-_25,
   odv_j25_3_1_10_10_77+_26, odv_j25_3_3_10_13_77+_29,
   odv_j25_3_4_10_12_77+_28
   *  To-Trips:       odv_j25_3_2_12_12_77+_27, odv_j25_3_4_10_12_77+_28,
   odv_j25_3_5_12_13_77+_29
   *  Possible Trips: odv_j25_3_4_10_12_77+_28
   *  Distance:       ca. 5km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::January / 05 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=47.41064282478368,9.539225124003337"
      "&toPlace=47.4485994339791,9.571887527087142"
      "&time=2025-01-05T20:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-05, start=20:00, end=21:20, duration=01:20, transfers=0, legs=[
    (from=- geometry=odv_10 [track=-, scheduled_track=-, level=0], to=- geometry=odv_12 [track=-, scheduled_track=-, level=0], start=2025-01-05 21:00, mode="FLEX", trip="odv_j25_3_4_10_12_77+_28", end=2025-01-05 21:20)
])",
      ss.str());

  // clang-format off
  /*  Test Case:      Trip with many stops (by Car as reference)
   *  Distance:       ca. 10km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::January / 03 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=46.77859375171761,6.647990839623077"
      "&toPlace=46.722300906957145,6.531654831253093"
      "&time=2025-01-03T22:30Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  // clang-format off
  /*  Test Case:      Trip with many stops
   *  From-Geometry:  odv_27
   *  To-Geometry:    odv_32
   *  From-Stops:     1359
   *  To-Stops:       212
   *  Distance:       ca. 10km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::January / 03 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=46.77859375171761,6.647990839623077"
      "&toPlace=46.722300906957145,6.531654831253093"
      "&time=2025-01-03T22:30Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-03, start=22:51, end=23:08, duration=00:38, transfers=0, legs=[
    (from=- geometry=odv_27 [track=-, scheduled_track=-, level=0], to=regular_8504774:0:C geometry=odv_27 [track=-, scheduled_track=-, level=0], start=2025-01-03 22:51, mode="FLEX", trip="odv_j25_2_15_27_27_56_24", end=2025-01-03 22:54),
    (from=regular_8504774:0:C geometry=- [track=-, scheduled_track=-, level=0], to=regular_8579294 geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-03 22:54, mode="BUS", trip="20250103_23:54_regular_225.TA.92-603-j25-1.7.R", end=2025-01-03 22:55),
    (from=regular_8579294 geometry=odv_27 [track=-, scheduled_track=-, level=0], to=- geometry=odv_27 [track=-, scheduled_track=-, level=0], start=2025-01-03 22:55, mode="FLEX", trip="odv_j25_2_15_27_27_56_24", end=2025-01-03 23:08)
])",
      ss.str());

  // clang-format off
  /*  Test Case:      Trip with few stops (by Car as reference)
   *  From-Stops:     139
   *  To-Stops:       111
   *  Distance:       ca. 100km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::February / 03 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=46.633283638420636,8.596274873067557"
      "&toPlace=47.332279891568135,9.413500965679333"
      "&time=2025-02-04T08:40Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-02-04, start=08:41, end=12:03, duration=03:23, transfers=3, legs=[
    (from=- geometry=- [track=-, scheduled_track=-, level=0], to=regular_8573106:0:A geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 08:41, mode="CAR", trip="-", end=2025-02-04 08:46),
    (from=regular_8573106:0:A geometry=- [track=-, scheduled_track=-, level=0], to=regular_8577386 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 08:46, mode="BUS", trip="20250204_09:46_regular_48.TA.92-401-j25-1.8.H", end=2025-02-04 09:25),
    (from=regular_8577386 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8505114:0:2 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 09:25, mode="WALK", trip="-", end=2025-02-04 09:29),
    (from=regular_8505114:0:2 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8505004:0:4 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 09:34, mode="REGIONAL_RAIL", trip="20250204_08:33_regular_14.TA.91-46-j25-1.8.R", end=2025-02-04 10:07),
    (from=regular_8505004:0:4 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8505004:0:6 geometry=- [track=-, scheduled_track=-, level=1], start=2025-02-04 10:07, mode="WALK", trip="-", end=2025-02-04 10:11),
    (from=regular_8505004:0:6 geometry=- [track=-, scheduled_track=-, level=1], to=regular_8506290:0:1 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 10:16, mode="REGIONAL_RAIL", trip="20250204_10:39_regular_71.TA.91-VAE-j25-1.29.R", end=2025-02-04 11:46),
    (from=regular_8506290:0:1 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8578507:0:B geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 11:46, mode="WALK", trip="-", end=2025-02-04 11:48),
    (from=regular_8578507:0:B geometry=- [track=-, scheduled_track=-, level=0], to=regular_8588239 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 11:48, mode="BUS", trip="20250204_12:48_regular_41.TA.96-220-1-j25-1.6.R", end=2025-02-04 11:49),
    (from=regular_8588239 geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 11:49, mode="CAR", trip="-", end=2025-02-04 12:03)
])",
      ss.str());

  // clang-format off
  /*  Test Case:      Trip with few stops
   *  From-Geometry:  odv_31
   *  To-Geometry:    odv_29
   *  From-Stops:     139
   *  To-Stops:       111
   *  Distance:       ca. 100km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::February / 03 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=46.633283638420636,8.596274873067557"
      "&toPlace=47.332279891568135,9.413500965679333"
      "&time=2025-02-04T08:40Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-02-04, start=08:47, end=12:14, duration=03:34, transfers=3, legs=[
    (from=- geometry=odv_31 [track=-, scheduled_track=-, level=0], to=regular_8587429 geometry=odv_31 [track=-, scheduled_track=-, level=0], start=2025-02-04 08:47, mode="FLEX", trip="odv_j25_8_2_31_31_14-_1", end=2025-02-04 08:58),
    (from=regular_8587429 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8577386 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 08:58, mode="BUS", trip="20250204_09:46_regular_48.TA.92-401-j25-1.8.H", end=2025-02-04 09:25),
    (from=regular_8577386 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8505114:0:2 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 09:25, mode="WALK", trip="-", end=2025-02-04 09:29),
    (from=regular_8505114:0:2 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8505004:0:4 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 09:34, mode="REGIONAL_RAIL", trip="20250204_08:33_regular_14.TA.91-46-j25-1.8.R", end=2025-02-04 10:07),
    (from=regular_8505004:0:4 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8505004:0:6 geometry=- [track=-, scheduled_track=-, level=1], start=2025-02-04 10:07, mode="WALK", trip="-", end=2025-02-04 10:11),
    (from=regular_8505004:0:6 geometry=- [track=-, scheduled_track=-, level=1], to=regular_8506290:0:1 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 10:16, mode="REGIONAL_RAIL", trip="20250204_10:39_regular_71.TA.91-VAE-j25-1.29.R", end=2025-02-04 11:46),
    (from=regular_8506290:0:1 geometry=- [track=-, scheduled_track=-, level=0], to=regular_8578507:0:B geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 11:46, mode="WALK", trip="-", end=2025-02-04 11:48),
    (from=regular_8578507:0:B geometry=- [track=-, scheduled_track=-, level=0], to=regular_8506775 geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-04 11:48, mode="BUS", trip="20250204_12:48_regular_41.TA.96-220-1-j25-1.6.R", end=2025-02-04 12:04),
    (from=regular_8506775 geometry=odv_29 [track=-, scheduled_track=-, level=0], to=- geometry=odv_29 [track=-, scheduled_track=-, level=0], start=2025-02-04 12:04, mode="FLEX", trip="odv_j25_1_1_29_29_14-_1", end=2025-02-04 12:14)
])",
      ss.str());
}