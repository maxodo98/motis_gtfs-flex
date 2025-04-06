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

TEST(motis, stops_10) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Stops 10.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-stops_10",
                  true);

  ASSERT_EQ(d.tt_->geometry_.size(), 2);
  ASSERT_EQ(d.tt_->geometry_trip_idxs_.size(), 4);
  ASSERT_EQ(d.tt_->geometry_idx_to_trip_idxs_.size(), 2);
  ASSERT_EQ(d.tt_->geometry_idx_to_trip_idxs_[nigiri::geometry_idx_t{0}].size(),
            2);
  ASSERT_EQ(d.tt_->geometry_idx_to_trip_idxs_[nigiri::geometry_idx_t{1}].size(),
            2);
  ASSERT_EQ(d.tt_->trip_idx_to_geometry_idxs_.size(), 13);
  ASSERT_EQ(d.tt_->trip_idx_to_geometry_idxs_[nigiri::trip_idx_t{0}].size(), 2);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, stops_100) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Stops 100.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-stops_100",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, stops_500) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Stops 500.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-stops_500",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, stops_1000) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Stops 1000.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-stops_1000",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, trips_5) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Trips 5.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-trips_5",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, trips_10) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Trips 10.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-trips_10",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, trips_15) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Trips 15.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-trips_15",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, trips_20) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Trips 20.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-trips_20",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, areas_5) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Areas 5.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-areas_5",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, areas_10) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Areas 10.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-areas_10",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, areas_15) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Areas 15.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-areas_15",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, areas_20) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Areas 20.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-areas_20",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}

TEST(motis, maximum) {
  auto const c = config{
      .osm_ = {"C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
               "resources/synthetic/Darmstadt.osm.pbf"},
      .timetable_ = {config::timetable{
          .first_day_ = "2024-12-15",
          .num_days_ = 363U,
          .with_shapes_ = false,
          .datasets_ = {{"flex",
                         {.path_ = "C:/Users/maxod/CLionProjects/motis_gtfs-"
                                   "flex/benchmark/resources/synthetic/"
                                   "Maximum.zip"}}}}},
      .street_routing_ = true,
      .osr_footpath_ = true,
      .geocoding_ = true};

  auto d = import(c,
                  "C:/users/maxod/clionprojects/motis_gtfs-flex/benchmark/"
                  "data-maximum",
                  true);

  auto const routing = utl::init_from<ep::routing>(d).value();

  // clang-format off
  /*  Test Case: Direct Travel by Car as reference
   */
  // clang-format on
  std::cout << "----------direct Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer);
  auto plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=CAR"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer) << std::endl;

  // clang-format off
  /*  Test Case: Direct Travel by Flex
   */
  // clang-format on
  std::cout << "----------direct Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer2);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");
  UTL_STOP_TIMING(timer2);
  ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer2) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Car----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer3);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=CAR"
      "&postTransitModes=CAR");
  UTL_STOP_TIMING(timer3);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer3) << std::endl;

  // clang-format off
  /*  Test Case: Transit with car as first and last mile as reference
   */
  // clang-format on
  std::cout << "----------transit Flex----------" << std::endl;
  openapi::now_test = date::sys_days{date::January / 04 / 2025} + 6h;
  UTL_START_TIMING(timer4);
  plan_response = routing(
      "?fromPlace=49.87077744685371,8.64654356554017"
      "&toPlace=49.86775126093775,8.673477706525915"
      "&time=2025-01-04T07:58Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer4);
  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());

  std::cout << "Duration: " << UTL_TIMING_MS(timer4) << std::endl;
}