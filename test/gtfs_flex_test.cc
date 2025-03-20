#include <motis/mode_to_profile.h>
#include <motis/street_routing.h>
#include <nigiri/loader/hrd/util.h>
#include <nigiri/timetable.h>
#include <utl/init_from.h>
#include <chrono>
#include "motis/elevators/parse_fasta.h"

#include "gtest/gtest.h"

#include "boost/json.hpp"

#include "motis/config.h"
#include "motis/endpoints/routing.h"
#include "motis/import.h"

using namespace motis;
using namespace std::string_view_literals;
namespace json = boost::json;

constexpr auto const kGTFS_complex = R"(
# agency.txt
agency_id,agency_name,agency_url,agency_timezone,agency_lang,agency_phone,agency_fare_url,agency_email
a_1,Deutsche Bahn,db.de,Europe/London,de,,,

# booking_rules.txt
booking_rule_id,booking_rule_name,booking_type,prior_notice_duration_min,prior_notice_duration_max,prior_notice_last_day,prior_notice_last_time,prior_notice_start_day,prior_notice_start_time,prior_notice_service_id,message,pickup_message,drop_off_message,phone_number,info_url,booking_url
b_Arbeitswoche,Darmstadt Arbeitswoche,1,60,1440,,,,,,,,,,,
b_eine_stunde,,1,60,,,,,,,,,,,,

# calendar.txt
service_id,service_name,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date
c_1,Arbeitswoche,1,1,1,1,1,0,0,20250101,20251231
c_2,Wochenende,0,0,0,0,0,1,1,20250101,20251231
c_3,Allgemein,1,1,1,1,1,1,1,20250101,20251231

# calendar_dates.txt
service_id,date,exception_type
c_1,20250101,2
c_1,20250418,2
c_1,20250421,2
c_1,20250501,2
c_1,20250529,2
c_1,20250609,2
c_1,20250619,2
c_1,20251003,2
c_1,20251225,2
c_1,20251226,2
c_2,20250101,1
c_2,20250418,1
c_2,20250421,1
c_2,20250501,1
c_2,20250529,1
c_2,20250609,1
c_2,20250619,1
c_2,20251003,1
c_2,20251225,1
c_2,20251226,1
c_3,20250101,2
c_3,20250418,2
c_3,20250421,2
c_3,20250501,2
c_3,20250529,2
c_3,20250609,2
c_3,20250619,2
c_3,20251003,2
c_3,20251225,2
c_3,20251226,2
c_3,20250703,2
c_3,20250704,2
c_3,20250705,2
c_3,20250706,2
c_3,20250707,2

# location.geojson
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "id": "komponistenviertel",
      "geometry": {
        "coordinates": [
          [
            [
              8.67548014424321,
              49.888227182727036
            ],
            [
              8.671032845869718,
              49.88198104911751
            ],
            [
              8.67036658394116,
              49.87907235029769
            ],
            [
              8.685990426166427,
              49.88419197231954
            ],
            [
              8.677612182414691,
              49.88853839505035
            ],
            [
              8.67548014424321,
              49.888227182727036
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "lichtwiese",
      "geometry": {
        "coordinates": [
          [
            [
              8.669193394697402,
              49.867873620786014
            ],
            [
              8.671655325310837,
              49.86360451237067
            ],
            [
              8.673673142951202,
              49.8641148338136
            ],
            [
              8.677660505083196,
              49.86681571363076
            ],
            [
              8.675169610579616,
              49.86748157554257
            ],
            [
              8.671462232714475,
              49.86804163920226
            ],
            [
              8.669193394697402,
              49.867873620786014
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "paulusviertel",
      "geometry": {
        "coordinates": [
          [
            [
              8.656417114465597,
              49.866376184902805
            ],
            [
              8.65931750219903,
              49.85867212908792
            ],
            [
              8.667956955024323,
              49.85896387130859
            ],
            [
              8.667010729238598,
              49.862875699092996
            ],
            [
              8.663801789618105,
              49.86739711215088
            ],
            [
              8.656417114465597,
              49.866376184902805
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "verlegerviertel",
      "geometry": {
        "coordinates": [
          [
            [
              8.636928487206859,
              49.85585968444397
            ],
            [
              8.637396064714267,
              49.855451834314266
            ],
            [
              8.64619752369265,
              49.85358986608145
            ],
            [
              8.647160183269705,
              49.86414007083789
            ],
            [
              8.646692605760876,
              49.865239287378785
            ],
            [
              8.632445244039644,
              49.86364364226634
            ],
            [
              8.636928487206859,
              49.85585968444397
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "ludwigshoehe",
      "geometry": {
        "coordinates": [
          [
            [
              8.647027375732932,
              49.8570169765743
            ],
            [
              8.64688036228884,
              49.85346262701262
            ],
            [
              8.65595109182857,
              49.853528977265654
            ],
            [
              8.656700860396114,
              49.85377542026464
            ],
            [
              8.65511311519333,
              49.85528248664798
            ],
            [
              8.651276064285582,
              49.85709279985031
            ],
            [
              8.647027375732932,
              49.8570169765743
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "johannesviertel",
      "geometry": {
        "coordinates": [
          [
            [
              8.643448727685154,
              49.875184140155
            ],
            [
              8.65001100522133,
              49.8762036425762
            ],
            [
              8.651228913691085,
              49.883882026266065
            ],
            [
              8.646175374070822,
              49.88193771040051
            ],
            [
              8.644329733513928,
              49.87999331622834
            ],
            [
              8.643448727685154,
              49.875184140155
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "kapellenplatzviertel",
      "geometry": {
        "coordinates": [
          [
            [
              8.656970216410315,
              49.87303267014235
            ],
            [
              8.65566209870505,
              49.86651472506222
            ],
            [
              8.658228021896093,
              49.86062836219213
            ],
            [
              8.667611250818027,
              49.86226623765333
            ],
            [
              8.66356111715345,
              49.8726759894318
            ],
            [
              8.656970216410315,
              49.87303267014235
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "hochschulen",
      "geometry": {
        "coordinates": [
          [
            [
              8.632402673884826,
              49.87495498743286
            ],
            [
              8.63469809686248,
              49.86396744340618
            ],
            [
              8.665671004245127,
              49.86777490054183
            ],
            [
              8.662702257193871,
              49.87813045461715
            ],
            [
              8.632402673884826,
              49.87495498743286
            ]
          ],
          [
            [
              8.65198632147937,
              49.87514454147674
            ],
            [
              8.661412801572027,
              49.876391004588896
            ],
            [
              8.662227184301457,
              49.87086696025591
            ],
            [
              8.65198632147937,
              49.869462886285845
            ],
            [
              8.65198632147937,
              49.87514454147674
            ]
          ]
        ],
        "type": "Polygon"
      }
    }
  ]
}

# routes.txt
route_id,agency_id,route_short_name,route_long_name,route_type
r_hochschulen_n_kapellenplatzviertel,a_1,,,715
r_johannesviertel,a_1,,,715
r_lichtwiese_komponistenviertel,a_1,,,715
r_west,a_1,,Westroute,100
r_sued,a_1,,Südroute,100
r_ost,a_1,,Ostroute,100
r_verbindung,a_1,,Umstieg,100

# stop_times.txt
trip_id,arrival_time,departure_time,stop_id,location_group_id,location_id,stop_sequence,start_pickup_drop_off_window,end_pickup_drop_off_window,pickup_booking_rule_id,drop_off_booking_rule_id,stop_headsign,pickup_type,drop_off_type
hochschulen_n_kapellenplatzviertel,,,,,hochschulen,,08:00:00,20:00:00,,,,2,1
hochschulen_n_kapellenplatzviertel,,,,,hochschulen,,08:00:00,20:00:00,,,,2,1
t_johannesviertel,,,,,johannesviertel,,08:00:00,20:00:00,b_eine_stunde,b_eine_stunde,,2,2
lichtwiese_komponistenviertel,,,,,lichtwiese,,10:00:00,18:00:00,,,,2,2
lichtwiese_komponistenviertel,,,,,komponistenviertel,,08:00:00,16:30:00,b_eine_stunde,b_eine_stunde,,1,2
westtrip,01:00:00,01:01:00,liebigstrasse,,,0,,,,,,0,0
westtrip,01:02:00,01:03:00,johanneskirche,,,1,,,,,,0,0
westtrip,01:04:00,01:05:00,alicenstrasse,,,2,,,,,,0,0
westtrip,01:06:00,01:07:00,kunsthalle,,,3,,,,,,0,0
westtrip,01:08:00,01:09:00,grossgerauerweg,,,4,,,,,,0,0
westtrip,01:30:00,02:01:00,liebigstrasse,,,5,,,,,,1,0
suedtrip,01:00:00,01:15:00,heinrichhoffmannschule,,,0,,,,,,0,0
suedtrip,01:17:00,01:18:00,kiesstrasse,,,1,,,,,,0,0
suedtrip,01:20:00,01:21:00,ohlystrasse,,,2,,,,,,0,0
suedtrip,01:23:00,01:24:00,jahnstrasse,,,3,,,,,,0,0
osttrip,01:00:00,01:30:00,weinbergstrasse,,,0,,,,,,0,0
osttrip,01:31:00,01:35:00,herdestrasse,,,1,,,,,,0,0
osttrip,01:45:00,01:46:00,heidenreichstrasse,,,2,,,,,,0,0
osttrip,01:47:00,01:48:00,glasbergweg,,,3,,,,,,0,0
osttrip,01:48:00,01:49:00,dachsbergweg,,,4,,,,,,0,0
osttrip,02:00:00,02:01:00,mozartweg,,,5,,,,,,0,0
osttrip,02:02:00,02:03:00,haydnweg,,,6,,,,,,0,0
verbindungstrip,01:10:00,01:15:00,grossgerauerweg,,,0,,,,,,0,0
verbindungstrip,01:20:00,01:25:00,weinbergstrasse,,,1,,,,,,0,0

# stops.txt
stop_id,stop_code,stop_name,tts_stop_name,stop_desc,stop_lat,stop_lon,zone_id,stop_url,location_type,parent_station,stop_timezone,wheelchair_boarding,level_id,platform_code
mozartweg,,Mozartweg,,,49.883981682059385,8.679799127750982,,,,,Europe/London,0,,
haydnweg,,Haydnweg,,,49.88497208804969,8.676307405835786,,,,,Europe/London,0,,
dachsbergweg,,Dachsbergweg,,,49.86659673897526,8.673930543644161,,,,,Europe/London,0,,
heidenreichstrasse,,Heidenreichstrasse,,,49.86522280125203,8.67247353165655,,,,,Europe/London,0,,
glasbergweg,,Glasbergweg,,,49.86728643661195,8.670889572364302,,,,,Europe/London,0,,
alicenstrasse,,Alicenstrasse,,,49.87874397281573,8.649201900512736,,,,,Europe/London,0,,
liebigstrasse,,Liebigstrasse,,,49.881812737965106,8.647974511806382,,,,,Europe/London,0,,
johanneskriche,,Johanneskriche,,,49.878348839432164,8.647210547749836,,,,,Europe/London,0,,
heinrichhoffmannschule,,Heinrich-Hoffmann-Schule,,,49.871801086283796,8.660440930537305,,,,,Europe/London,0,,
kiesstrasse,,Kiesstrasse,,,49.86836555981881,8.66008883253076,,,,,Europe/London,0,,
ohlystrasse,,Ohlystrasse,,,49.86299873261112,8.662451204040508,,,,,Europe/London,0,,
jahnstrasse,,Jahnstrasse,,,49.86057048370631,8.663950720932746,,,,,Europe/London,0,,
herderstrasse,,Herderstrasse,,,49.855079470013436,8.652489850958887,,,,,Europe/London,0,,
weinbergstrasse,,Weinbergstrasse,,,49.85583945866941,8.649836130296478,,,,,Europe/London,0,,
grossgerauerweg,,Gross-Gerauer-Weg,,,49.86216393997364,8.64115008310469,,,,,Europe/London,0,,
kunsthalle,,Kunsthalle,,,49.8721583279831,8.642161844284885,,,,,Europe/London,0,,

# trips.txt
route_id,service_id,trip_id,trip_headsign,trip_short_name,direction_id,block_id
r_hochschulen_n_kapellenplatzviertel,c_1,hochschulen_n_kapellenplatzviertel,,,,
r_johannesviertel,c_1,t_johannesviertel,,,,
r_lichtwiese_komponistenviertel,c_1,lichtwiese_komponistenviertel,,,,
r_west,c_1,westtrip,,,,
r_sued,c_1,suedtrip,,,,
r_ost,c_1,osttrip,,,,
r_verbindung,c_1,verbindungstrip,,,,

# frequencies.txt
trip_id,start_time,end_time,headway_secs
westtrip,01:01:00,24:30:00,3600
suedtrip,01:15:00,22:24:00,3600
osttrip,01:30:00,22:03:00,7200
verbindungstrip,01:10:00,23:25:00,3600
)";

constexpr auto const kGTFS_simple = R"(
# agency.txt
agency_id,agency_name,agency_url,agency_timezone,agency_lang,agency_phone,agency_fare_url,agency_email
a_1,Deutsche Bahn,db.de,Europe/Berlin,de,,,

# booking_rules.txt
booking_rule_id,booking_rule_name,booking_type,prior_notice_duration_min,prior_notice_duration_max,prior_notice_last_day,prior_notice_last_time,prior_notice_start_day,prior_notice_start_time,prior_notice_service_id,message,pickup_message,drop_off_message,phone_number,info_url,booking_url
b_1,Buchungsregel,1,10,,,,,,,,,,,,
b_2,Buchungsregel,1,30,,,,,,,,,,,,
b_3,Buchungsregel,1,60,,,,,,,,,,,,

# calendar.txt
service_id,service_name,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date
c_1,Allgemein,1,1,1,1,1,1,1,20250101,20251231

# calendar_dates.txt
service_id,date,exception_type
c_1,20250101,2

# location.geojson
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "id": "Startbereich",
      "geometry": {
        "coordinates": [
          [
            [
              8.649368123737446,
              49.86672466559648
            ],
            [
              8.649938225550244,
              49.864599687984736
            ],
            [
              8.656184558460978,
              49.86535063060248
            ],
            [
              8.655317012224145,
              49.86758741178036
            ],
            [
              8.649368123737446,
              49.86672466559648
            ]
          ]
        ],
        "type": "Polygon"
      }
    },
    {
      "type": "Feature",
      "id": "Zielbereich",
      "geometry": {
        "coordinates": [
          [
            [
              8.660447928040043,
              49.86837026071527
            ],
            [
              8.658743254562353,
              49.86816256733292
            ],
            [
              8.659759523012582,
              49.866069607267235
            ],
            [
              8.66627851331208,
              49.86714006382127
            ],
            [
              8.665510115216563,
              49.86904126399435
            ],
            [
              8.660447928040043,
              49.86837026071527
            ]
          ]
        ],
        "type": "Polygon"
      }
    }
  ]
}

# routes.txt
route_id,agency_id,route_short_name,route_long_name,route_type
r_flex_1,a_1,,Flex-Fahrt-Start,715
r_flex_2,a_1,,Flex-Fahrt-Ziel,715
r_flex_3,a_1,,Flex-Fahrt-Direkt,715
r_bahn,a_1,,Bahnfahrt,100

# stop_times.txt
trip_id,arrival_time,departure_time,stop_id,location_group_id,location_id,stop_sequence,start_pickup_drop_off_window,end_pickup_drop_off_window,pickup_booking_rule_id,drop_off_booking_rule_id,stop_headsign,pickup_type,drop_off_type
flex_1,,,,,Startbereich,,08:00:00,20:00:00,b_1,b_1,,2,2
flex_2,,,,,Zielbereich,,08:00:00,20:00:00,b_2,b_2,,2,2
flex_3,,,,,Startbereich,,08:00:00,20:00:00,b_3,b_3,,2,1
flex_3,,,,,Zielbereich,,08:00:00,8:30:00,b_3,b_3,,1,2
bahn,12:52:00,12:58:00,bahn_start,,,0,,,,,0,0
bahn,12:59:00,13:15:00,bahn_ziel,,,1,,,,,0,0

# stops.txt
stop_id,stop_code,stop_name,tts_stop_name,stop_desc,stop_lat,stop_lon,zone_id,stop_url,location_type,parent_station,stop_timezone,wheelchair_boarding,level_id,platform_code
bahn_start,,Bahn-Startpunkt,,,49.86612067335565,8.65312725611551,,,0,,Europe/Berlin,0,,
bahn_ziel,,Bahn-Zielpunkt,,,49.86734352295011,8.661965445062833,,,0,,Europe/Berlin,0,,

# trips.txt
route_id,service_id,trip_id,trip_headsign,trip_short_name,direction_id,block_id
r_flex_1,c_1,flex_1,,,,
r_flex_2,c_1,flex_2,,,,
r_flex_3,c_1,flex_3,,,,
r_bahn,c_1,bahn,,,,
)"sv;

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

TEST(motis, direct_depature) {
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_simple}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);

  auto const routing = utl::init_from<ep::routing>(d).value();
  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 8h + 30min;
  auto plan_response = routing(
      "?fromPlace=49.86576808937855,8.650554050873524"
      "&toPlace=49.86768861746879,8.665222857978648"
      "&time=2025-01-02T09:00Z"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=09:30, end=09:32, duration=00:32, transfers=0, legs=[
    (from=- geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 09:30, mode="FLEX", trip="flex_3", end=2025-01-02 09:32)
])",
      ss.str());
}

TEST(motis, direct_arrival) {
  std::cout << "Current working directory: " << std::filesystem::current_path()
            << std::endl;
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_simple}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);

  auto const routing = utl::init_from<ep::routing>(d).value();
  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 6h + 0min;
  auto plan_response = routing(
      "?fromPlace=49.86576808937855,8.650554050873524"
      "&toPlace=49.86768861746879,8.665222857978648"
      "&time=2025-01-02T09:00Z"
      "&arriveBy=true"
      "&timetableView=false"
      "&useRoutedTransfers=false"
      "&directModes=FLEX"
      "&preTransitModes="
      "&postTransitModes=");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.direct_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=08:28, end=08:30, duration=00:32, transfers=0, legs=[
    (from=- geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 08:28, mode="FLEX", trip="flex_3", end=2025-01-02 08:30)
])",
      ss.str());
}

TEST(motis, simple_offsets_departure) {
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_simple}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);
  auto const max = osr::cost_t{900};

  // Einfachste Fahrt mit erster und letzter Meile
  auto const routing = utl::init_from<ep::routing>(d).value();
  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 11h + 40min;
  auto plan_response = routing(
      "?fromPlace=49.86576808937855,8.650554050873524"
      "&toPlace=49.86768861746879,8.665222857978648"
      "&time=2025-01-02T11:50Z"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=11:57, end=12:11, duration=00:21, transfers=0, legs=[
    (from=- geometry=Startbereich [track=-, scheduled_track=-, level=0], to=test_bahn_start geometry=Startbereich [track=-, scheduled_track=-, level=0], start=2025-01-02 11:57, mode="FLEX", trip="flex_1", end=2025-01-02 11:58),
    (from=test_bahn_start geometry=- [track=-, scheduled_track=-, level=0], to=test_bahn_ziel geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 11:58, mode="REGIONAL_RAIL", trip="20250102_12:58_test_bahn", end=2025-01-02 11:59),
    (from=test_bahn_ziel geometry=Zielbereich [track=-, scheduled_track=-, level=0], to=- geometry=Zielbereich [track=-, scheduled_track=-, level=0], start=2025-01-02 11:59, mode="FLEX", trip="flex_2", end=2025-01-02 12:11)
])",
      ss.str());
}

TEST(motis, simple_offsets_arrival) {
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_simple}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);
  auto const max = osr::cost_t{900};

  // Einfachste Fahrt mit erster und letzter Meile
  auto const routing = utl::init_from<ep::routing>(d).value();
  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 7h + 0min;
  auto plan_response = routing(
      "?fromPlace=49.86576808937855,8.650554050873524"
      "&toPlace=49.86768861746879,8.665222857978648"
      "&time=2025-01-02T12:15Z"
      "&arriveBy=true"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=11:57, end=12:00, duration=00:18, transfers=0, legs=[
    (from=- geometry=Startbereich [track=-, scheduled_track=-, level=0], to=test_bahn_start geometry=Startbereich [track=-, scheduled_track=-, level=0], start=2025-01-02 11:57, mode="FLEX", trip="flex_1", end=2025-01-02 11:58),
    (from=test_bahn_start geometry=- [track=-, scheduled_track=-, level=0], to=test_bahn_ziel geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 11:58, mode="REGIONAL_RAIL", trip="20250102_12:58_test_bahn", end=2025-01-02 11:59),
    (from=test_bahn_ziel geometry=Zielbereich [track=-, scheduled_track=-, level=0], to=- geometry=Zielbereich [track=-, scheduled_track=-, level=0], start=2025-01-02 11:59, mode="FLEX", trip="flex_2", end=2025-01-02 12:00)
])",
      ss.str());
}

TEST(motis, complex_mixed_offsets) {
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_complex}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);
  auto const max = osr::cost_t{900};

  auto const routing = utl::init_from<ep::routing>(d).value();

  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 7h + 0min;
  auto plan_response = routing(
      "?fromPlace=49.87713252734085,8.66146342943432"
      "&toPlace=49.85862485997319,8.664434771107324"
      "&time=2025-01-02T09:00Z"
      "&arriveBy=false"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=FLEX,WALK"
      "&postTransitModes=FLEX,WALK");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=09:18, end=09:26, duration=00:26, transfers=0, legs=[
    (from=- geometry=hochschulen [track=-, scheduled_track=-, level=0], to=test_ohlystrasse geometry=kapellenplatzviertel [track=-, scheduled_track=-, level=0], start=2025-01-02 09:18, mode="FLEX", trip="hochschulen_n_kapellenplatzviertel", end=2025-01-02 09:21),
    (from=test_ohlystrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_jahnstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 09:21, mode="REGIONAL_RAIL", trip="20250102_09:15_test_suedtrip", end=2025-01-02 09:23),
    (from=test_jahnstrasse geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 09:23, mode="WALK", trip="-", end=2025-01-02 09:26)
])",
      ss.str());

  plan_response = routing(
      "?fromPlace=49.87713252734085,8.66146342943432"
      "&toPlace=49.85862485997319,8.664434771107324"
      "&time=2025-01-02T09:00Z"
      "&arriveBy=true"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=FLEX,WALK"
      "&postTransitModes=FLEX,WALK");

  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=08:16, end=08:26, duration=00:44, transfers=0, legs=[
    (from=- geometry=hochschulen [track=-, scheduled_track=-, level=0], to=test_kiesstrasse geometry=kapellenplatzviertel [track=-, scheduled_track=-, level=0], start=2025-01-02 08:16, mode="FLEX", trip="hochschulen_n_kapellenplatzviertel", end=2025-01-02 08:18),
    (from=test_kiesstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_jahnstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 08:18, mode="REGIONAL_RAIL", trip="20250102_08:15_test_suedtrip", end=2025-01-02 08:23),
    (from=test_jahnstrasse geometry=- [track=-, scheduled_track=-, level=0], to=- geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 08:23, mode="WALK", trip="-", end=2025-01-02 08:26)
])",
      ss.str());
}

TEST(motis, complex_booking_times) {
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_complex}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);
  auto const max = osr::cost_t{900};

  auto const routing = utl::init_from<ep::routing>(d).value();

  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 7h + 0min;
  auto plan_response = routing(
      "?fromPlace=49.8800781368721,8.648973223060807"
      "&toPlace=49.86610220813756,8.673087788598849"
      "&time=2025-01-02T13:00Z"
      "&arriveBy=false"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=13:03, end=13:46, duration=00:46, transfers=2, legs=[
    (from=- geometry=johannesviertel [track=-, scheduled_track=-, level=0], to=test_alicenstrasse geometry=johannesviertel [track=-, scheduled_track=-, level=0], start=2025-01-02 13:03, mode="FLEX", trip="t_johannesviertel", end=2025-01-02 13:05),
    (from=test_alicenstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 13:05, mode="REGIONAL_RAIL", trip="20250102_13:01_test_westtrip", end=2025-01-02 13:08),
    (from=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], to=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 13:08, mode="WALK", trip="-", end=2025-01-02 13:10),
    (from=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], to=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 13:10, mode="REGIONAL_RAIL", trip="20250102_13:10_test_verbindungstrip", end=2025-01-02 13:15),
    (from=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 13:15, mode="WALK", trip="-", end=2025-01-02 13:17),
    (from=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_heidenreichstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 13:30, mode="REGIONAL_RAIL", trip="20250102_13:30_test_osttrip", end=2025-01-02 13:45),
    (from=test_heidenreichstrasse geometry=lichtwiese [track=-, scheduled_track=-, level=0], to=- geometry=lichtwiese [track=-, scheduled_track=-, level=0], start=2025-01-02 13:45, mode="FLEX", trip="lichtwiese_komponistenviertel", end=2025-01-02 13:46)
])",
      ss.str());

  plan_response = routing(
      "?fromPlace=49.8800781368721,8.648973223060807"
      "&toPlace=49.86610220813756,8.673087788598849"
      "&time=2025-01-02T19:00Z"
      "&arriveBy=true"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=FLEX"
      "&postTransitModes=FLEX");

  ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ(
      R"(date=2025-01-02, start=17:03, end=17:46, duration=01:57, transfers=2, legs=[
    (from=- geometry=johannesviertel [track=-, scheduled_track=-, level=0], to=test_alicenstrasse geometry=johannesviertel [track=-, scheduled_track=-, level=0], start=2025-01-02 17:03, mode="FLEX", trip="t_johannesviertel", end=2025-01-02 17:05),
    (from=test_alicenstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 17:05, mode="REGIONAL_RAIL", trip="20250102_17:01_test_westtrip", end=2025-01-02 17:08),
    (from=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], to=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 17:08, mode="WALK", trip="-", end=2025-01-02 17:10),
    (from=test_grossgerauerweg geometry=- [track=-, scheduled_track=-, level=0], to=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 17:10, mode="REGIONAL_RAIL", trip="20250102_17:10_test_verbindungstrip", end=2025-01-02 17:15),
    (from=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 17:15, mode="WALK", trip="-", end=2025-01-02 17:17),
    (from=test_weinbergstrasse geometry=- [track=-, scheduled_track=-, level=0], to=test_heidenreichstrasse geometry=- [track=-, scheduled_track=-, level=0], start=2025-01-02 17:30, mode="REGIONAL_RAIL", trip="20250102_17:30_test_osttrip", end=2025-01-02 17:45),
    (from=test_heidenreichstrasse geometry=lichtwiese [track=-, scheduled_track=-, level=0], to=- geometry=lichtwiese [track=-, scheduled_track=-, level=0], start=2025-01-02 17:45, mode="FLEX", trip="lichtwiese_komponistenviertel", end=2025-01-02 17:46)
])",
      ss.str());
}

TEST(motis, real_data_swiss) {
  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  auto const c = config{
      .server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
      .osm_ = {"test/resources/gtfs-flex/switzerland.osm.pbf"},
      .timetable_ =
          config::timetable{
              .first_day_ = "2025-01-01",
              .num_days_ = 365,
              .datasets_ = {{"test", {.path_ = std::string{kGTFS_complex}}}}},
      .street_routing_ = true,
      .osr_footpath_ = false,
      .geocoding_ = false,
      .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);
  auto const max = osr::cost_t{900};

  auto const routing = utl::init_from<ep::routing>(d).value();

  openapi::now_test = date::sys_days{date::January / 02 / 2025} + 7h + 0min;
}

TEST(motis, real_data_australia) {}