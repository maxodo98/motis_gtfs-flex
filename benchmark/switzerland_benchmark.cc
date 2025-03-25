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

TEST(motis, switzerland) {
  std::cout << "Current working directory: " << std::filesystem::current_path()
            << std::endl;
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/switzerland.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ =
              {{"regular",
                {.path_ = "C:/users/maxod/clionprojects/motis_gtfs-flex/"
                          "benchmark/resources/switzerland_gtfs.zip"}},
               {"flex",
                {.path_ = "C:/users/maxod/clionprojects/motis_gtfs-flex/"
                          "benchmark/resources/switzerland_gtfs_flex.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(
      c,
      "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/data-switzerland",
      true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   *  From-Geometry:  odv_10
   *  To-Geometry:    odv_12
   *  From-Trips:     odv_j25_3_1_10_10_14-_24, odv_j25_3_1_10_10_56-_25, odv_j25_3_1_10_10_77+_26, odv_j25_3_3_10_13_77+_29, odv_j25_3_4_10_12_77+_28
   *  To-Trips:       odv_j25_3_2_12_12_77+_27, odv_j25_3_4_10_12_77+_28, odv_j25_3_5_12_13_77+_29
   *  Possible Trips: odv_j25_3_4_10_12_77+_28
   *  Distance:       ca. 5km
   */
  // clang-format on
  std::cout << "----------direct----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 05 / 2025} + 8h + 30min;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=47.41064282478368,9.539225124003337"
      "&toPlace=47.4485994339791,9.571887527087142"
      "&time=2025-01-05T20:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());
  std::cout << "Duration (ms): " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case:      Trip with many stops
   *  From-Geometry:  odv_27
   *  To-Geometry:    odv_32
   *  From-Stops:     1359
   *  To-Stops:       212
   *  Distance:       ca. 10km
   */
  // clang-format on
  std::cout << "----------010km----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 03 / 2025} + 8h + 30min;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=46.77859375171761,6.647990839623077"
      "&toPlace=46.722300906957145,6.531654831253093"
      "&time=2025-01-03T22:30Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());
  std::cout << "Direct Duration (ms): " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case:      Trip with few stops
   *  From-Geometry:  odv_31
   *  To-Geometry:    odv_29
   *  From-Stops:     139
   *  To-Stops:       111
   *  Distance:       ca. 100km
   */
  // clang-format on
  std::cout << "----------100km----------" << std::endl;
  openapi::now_test = date::sys_days{date::February / 03 / 2025} + 8h + 30min;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=46.633283638420636,8.596274873067557"
      "&toPlace=47.332279891568135,9.413500965679333"
      "&time=2025-02-04T08:40Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());
  std::cout << "Direct Duration (ms): " << UTL_TIMING_MS(timer3) << std::endl;
}

TEST(motis, australia) {
  std::cout << "Current working directory: " << std::filesystem::current_path()
            << std::endl;
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/australia.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ =
              {{"regular",
                {.path_ = "C:/users/maxod/clionprojects/motis_gtfs-flex/"
                          "benchmark/resources/australia_gtfs.zip"}},
               {"flex",
                {.path_ = "C:/users/maxod/clionprojects/motis_gtfs-flex/"
                          "benchmark/resources/australia_gtfs_flex.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(
      c,
      "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/data-australia",
      true);

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
  std::cout << "----------direct----------" << std::endl;
  openapi::now_test = date::sys_days{date::February / 20 / 2025} + 8h + 30min;
  UTL_START_TIMING(timer);
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
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());
  std::cout << "Duration (ms): " << UTL_TIMING_MS(timer) << std::endl;

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
  std::cout << "----------direct----------" << std::endl;
  openapi::now_test = date::sys_days{date::February / 19 / 2025} + 8h + 30min;
  UTL_START_TIMING(timer2);
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
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());
  std::cout << "Duration (ms): " << UTL_TIMING_MS(timer2) << std::endl;

  // // clang-format off
  // /*  Test Case:      Trip with many stops
  //  *  From-Geometry:  area_400a, area_400b, area_400c
  //  *  To-Geometry:    -
  //  *  From-Stops:     647, 607, 458
  //  *  To-Stops:       -
  //  *  Distance:       ca. 4km
  //  */
  // // clang-format on
  // std::cout << "----------004km----------" << std::endl;
  // openapi::now_test = date::sys_days{date::March / 02 / 2025} + 8h + 30min;
  // UTL_START_TIMING(timer4);
  // plan_response = routing(
  //     "?fromPlace=-33.69792666305133,150.92295041547175"
  //     "&toPlace=-33.77558246783908,150.91374511466796"
  //     "&time=2025-03-03T9:15Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&directModes="
  //     "&preTransitModes=FLEX"
  //     "&postTransitModes=WALK");
  // UTL_STOP_TIMING(timer4);
  // ss = std::stringstream{};
  // for (auto const& j : plan_response.itineraries_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ(R"()", ss.str());
  // std::cout << "Direct Duration (ms): " << UTL_TIMING_MS(timer4) <<
  // std::endl;

  // // clang-format off
  // /*  Test Case:      Trip with many stops
  //  *  From-Geometry:  area_400a, area_400b, area_400c
  //  *  To-Geometry:    -
  //  *  From-Stops:     647, 607, 458
  //  *  To-Stops:       -
  //  *  Distance:       ca. 4km
  //  */
  // // clang-format on
  // std::cout << "----------004km----------" << std::endl;
  // openapi::now_test = date::sys_days{date::March / 02 / 2025} + 8h + 30min;
  // UTL_START_TIMING(timer3);
  // plan_response = routing(
  //     "?fromPlace=-33.86079160281717,151.10353392308656"
  //     "&toPlace=-33.889436966588086,151.12571457725858"
  //     "&time=2025-03-19T21:00Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&directModes="
  //     "&preTransitModes=FLEX"
  //     "&postTransitModes=WALK");
  // UTL_STOP_TIMING(timer3);
  // ss = std::stringstream{};
  // for (auto const& j : plan_response.itineraries_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ(R"()", ss.str());
  // std::cout << "Direct Duration (ms): " << UTL_TIMING_MS(timer3) <<
  // std::endl;
}