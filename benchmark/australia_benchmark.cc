#include <motis/hashes.h>
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

#include "absl/strings/internal/str_format/extension.h"
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

  std::cout << request_str.str() << std::endl;

  UTL_START_TIMING(timer);
  auto plan_response = routing(request_str.str());
  UTL_STOP_TIMING(timer);
  return UTL_TIMING_MS(timer);
};

TEST(motis, motis_australia_request_1_flex) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::February / 20 / 2025} + 8h + 30min,
                   geo::latlng{-30.325162623550483, 149.78461109169166},
                   geo::latlng{-29.544369343617667, 148.58177271302816},
                   date::sys_days{date::February / 21 / 2025} + 13h + 00min,
                   false, osr::mode::kFlex, {}, {});
}

TEST(motis, motis_australia_request_1_car) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::February / 20 / 2025} + 8h + 30min,
                   geo::latlng{-30.325162623550483, 149.78461109169166},
                   geo::latlng{-29.544369343617667, 148.58177271302816},
                   date::sys_days{date::February / 21 / 2025} + 13h + 00min,
                   false, osr::mode::kCar, {}, {});
}

TEST(motis, motis_australia_request_2_car) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::February / 19 / 2025} + 8h + 30min,
                   geo::latlng{-32.894127631807336, 144.29584012289865},
                   geo::latlng{-34.51342576243014, 144.84241915033908},
                   date::sys_days{date::February / 20 / 2025} + 11h + 0min,
                   true, osr::mode::kCar, {}, {});
}

TEST(motis, motis_australia_request_2_flex) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::February / 19 / 2025} + 8h + 30min,
                   geo::latlng{-32.894127631807336, 144.29584012289865},
                   geo::latlng{-34.51342576243014, 144.84241915033908},
                   date::sys_days{date::February / 20 / 2025} + 11h + 0min,
                   true, osr::mode::kFlex, {}, {});
}

TEST(motis, motis_australia_request_3_car) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::March / 02 / 2025} + 8h + 30min,
                   geo::latlng{-33.86079160281717, 151.10353392308656},
                   geo::latlng{-33.889436966588086, 151.12571457725858},
                   date::sys_days{date::March / 19 / 2025} + 21h + 0min, false,
                   std::nullopt, {osr::mode::kCar}, {osr::mode::kFoot});
}

TEST(motis, motis_australia_request_3_flex) {
  std::cout << "Duration (ms): "
            << exec_benchmark(
                   date::sys_days{date::March / 02 / 2025} + 8h + 30min,
                   geo::latlng{-33.86079160281717, 151.10353392308656},
                   geo::latlng{-33.889436966588086, 151.12571457725858},
                   date::sys_days{date::March / 19 / 2025} + 21h + 0min, false,
                   std::nullopt, {osr::mode::kFlex}, {osr::mode::kFoot});
}

TEST(motis, australia) {
  std::cout << "Current working directory: " << std::filesystem::current_path()
            << std::endl;
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

  // clang-format off
    /*  Test Case: Direct Travel by Flex
     *  From-Geometry:  area_21a
     *  To-Geometry:    area_21b
     *  From-Trips:     11.D21.10.1, 11.D21.10.2
     *  To-Trips:       11.D21.10.1, 11.D21.10.2
     *  Possible Trips: 11.D21.10.2
     *  Distance:       ca. 145km
     */
  // clang-format on
  openapi::now_test = date::sys_days{date::February / 20 / 2025} + 8h + 30min;
  auto plan_response = routing(
      "?fromPlace=-30.325162623550483,149.78461109169166"
      "&toPlace=-29.544369343617667,148.58177271302816"
      "&time=2025-02-21T13:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes="
      "&maxDirectTime=10800");
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-02-21, start=13:00, end=14:58, duration=01:58, transfers=0, legs=[
    (from=- geometry=area_21a [track=-, scheduled_track=-, level=0], to=- geometry=area_21b [track=-, scheduled_track=-, level=0], start=2025-02-21 13:15, mode="FLEX", trip="11.D21.10.2", end=2025-02-21 14:58)
])",
      ss.str());

  // clang-format off
    /*  Test Case: Direct Travel by Flex
     *  From-Geometry:  area_21a
     *  To-Geometry:    area_21b
     *  From-Trips:     11.D21.10.1, 11.D21.10.2
     *  To-Trips:       11.D21.10.1, 11.D21.10.2
     *  Possible Trips: 11.D21.10.2
     *  Distance:       ca. 145km
     */
  // clang-format on
  openapi::now_test = date::sys_days{date::February / 20 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=-30.325162623550483,149.78461109169166"
      "&toPlace=-29.544369343617667,148.58177271302816"
      "&time=2025-02-21T13:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes="
      "&maxDirectTime=10800");
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-02-21, start=13:00, end=14:43, duration=01:43, transfers=0, legs=[
    (from=- geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-21 13:00, mode="CAR", trip="-", end=2025-02-21 14:43)
])",
      ss.str());

  // clang-format off
    /*  Test Case: Direct Travel by Flex
     *  From-Geometry:  area_20b
     *  To-Geometry:    area_20a
     *  From-Trips:     11.D20.9.1, 11.D20.9.2
     *  To-Trips:       11.D20.9.1, 11.D20.9.2
     *  Possible Trips: 11.D20.9.1
     *  Distance:       ca. 188km
     */
  // clang-format on
  openapi::now_test = date::sys_days{date::February / 19 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=-32.894127631807336,144.29584012289865"
      "&toPlace=-34.51342576243014,144.84241915033908"
      "&time=2025-02-20T11:00Z"
      "&arriveBy=true"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes="
      "&maxDirectTime=10800");
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-02-20, start=08:47, end=11:00, duration=02:12, transfers=0, legs=[
    (from=- geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-02-20 08:47, mode="CAR", trip="-", end=2025-02-20 11:00)
])",
      ss.str());

  // clang-format off
    /*  Test Case: Direct Travel by Flex
     *  From-Geometry:  area_20b
     *  To-Geometry:    area_20a
     *  From-Trips:     11.D20.9.1, 11.D20.9.2
     *  To-Trips:       11.D20.9.1, 11.D20.9.2
     *  Possible Trips: 11.D20.9.1
     *  Distance:       ca. 188km
     */
  // clang-format on
  openapi::now_test = date::sys_days{date::February / 19 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=-32.894127631807336,144.29584012289865"
      "&toPlace=-34.51342576243014,144.84241915033908"
      "&time=2025-02-20T11:00Z"
      "&arriveBy=true"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes="
      "&maxDirectTime=10800");
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-02-20, start=08:07, end=11:00, duration=02:52, transfers=0, legs=[
    (from=- geometry=area_20b [track=-, scheduled_track=-, level=0], to=- geometry=area_20a [track=-, scheduled_track=-, level=0], start=2025-02-20 08:07, mode="FLEX", trip="11.D20.9.1", end=2025-02-20 10:20)
])",
      ss.str());

  // clang-format off
  /*  Test Case:      Trip with many stops
   *  From-Geometry:  area_400a, area_400b, area_400c
   *  To-Geometry:    -
   *  From-Stops:     647, 607, 458
   *  To-Stops:       -
   *  Distance:       ca. 4km
   */
  // clang-format on
  openapi::now_test = date::sys_days{date::March / 02 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=-33.86079160281717,151.10353392308656"
      "&toPlace=-33.889436966588086,151.12571457725858"
      "&time=2025-03-19T21:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=CAR"
      "&postTransitModes=WALK"
      "&maxDirectTime=0");
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"()",
      ss.str());  // Keine Ergebnisse, da Direktverbindung schneller ist. Die
                  // Zeiversätze werden dennoch erstellt für die Messung.

  //   // clang-format off
  //   /*  Test Case:      Trip with many stops
  //    *  From-Geometry:  area_400a, area_400b, area_400c
  //    *  To-Geometry:    -
  //    *  From-Stops:     647, 607, 458
  //    *  To-Stops:       -
  //    *  Distance:       ca. 4km
  //    */
  // clang-format on
  openapi::now_test = date::sys_days{date::March / 02 / 2025} + 8h + 30min;
  plan_response = routing(
      "?fromPlace=-33.86079160281717,151.10353392308656"
      "&toPlace=-33.889436966588086,151.12571457725858"
      "&time=2025-03-19T21:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=WALK");
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-03-19, start=21:02, end=21:19, duration=00:19, transfers=0, legs=[
    (from=- geometry=area_400b [track=-, scheduled_track=-, level=0], to=regular_213444 geometry=area_400b [track=-, scheduled_track=-, level=0], start=2025-03-19 21:02, mode="FLEX", trip="1.D400.1.2", end=2025-03-19 21:06),
    (from=regular_213444 geometry=- [track=-, scheduled_track=-, level=0], to=regular_213121 geometry=- [track=-, scheduled_track=-, level=0], start=2025-03-19 21:06, mode="BUS", trip="20250320_08:03_regular_2349567", end=2025-03-19 21:17),
    (from=regular_213121 geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-03-19 21:17, mode="WALK", trip="-", end=2025-03-19 21:19)
])",
      ss.str());
}