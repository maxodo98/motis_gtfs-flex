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

  // openapi::now_test = date::sys_days{date::January / 05 / 2025} + 8h + 30min;
  // UTL_START_TIMING(timer);
  // auto plan_response = routing(
  //     "?fromPlace=47.41064282478368,9.539225124003337"
  //     "&toPlace=47.4485994339791,9.571887527087142"
  //     "&time=2025-01-05T20:00Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&directModes=FLEX"
  //     "&preTransitModes="
  //     "&postTransitModes=");
  // UTL_STOP_TIMING(timer);
  // auto ss = std::stringstream{};
  // for (auto const& j : plan_response.direct_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ(R"()", ss.str());
  // std::cout << "Duration (ms): " << UTL_TIMING_MS(timer) << std::endl;

  openapi::now_test = date::sys_days{date::January / 03 / 2025} + 8h + 30min;
  UTL_START_TIMING(timer2);
  auto plan_response = routing(
      "?fromPlace=46.77859375171761,6.647990839623077"
      "&toPlace=46.722300906957145,6.531654831253093"
      "&time=2025-01-03T22:30Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes="
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");
  UTL_STOP_TIMING(timer2);
  auto ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(R"()", ss.str());
  std::cout << "Direct Duration (ms): " << UTL_TIMING_MS(timer2) << std::endl;
}

TEST(motis, australia) {
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
  std::cout << "Direct Duration (ms): " << UTL_TIMING_MS(timer) << std::endl;
}