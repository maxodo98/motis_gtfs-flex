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

constexpr auto const kFastaJson = R"__(
[
  {
    "description": "FFM HBF zu Gleis 101/102 (S-Bahn)",
    "equipmentnumber" : 10561326,
    "geocoordX" : 8.6628995,
    "geocoordY" : 50.1072933,
    "operatorname" : "DB InfraGO",
    "state" : "ACTIVE",
    "stateExplanation" : "available",
    "stationnumber" : 1866,
    "type" : "ELEVATOR",
    "outOfService": [
      ["2019-05-01T01:30:00Z", "2019-05-01T02:30:00Z"]
    ]
  },
  {
    "description": "FFM HBF zu Gleis 103/104 (S-Bahn)",
    "equipmentnumber": 10561327,
    "geocoordX": 8.6627516,
    "geocoordY": 50.1074549,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1866,
    "type": "ELEVATOR"
  },
  {
    "description": "HAUPTWACHE zu Gleis 2/3 (S-Bahn)",
    "equipmentnumber": 10351032,
    "geocoordX": 8.67818,
    "geocoordY": 50.114046,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1864,
    "type": "ELEVATOR"
  },
  {
    "description": "DA HBF zu Gleis 1",
    "equipmentnumber": 10543458,
    "geocoordX": 8.6303864,
    "geocoordY": 49.8725612,
    "state": "ACTIVE",
    "type": "ELEVATOR"
  },
  {
    "description": "DA HBF zu Gleis 3/4",
    "equipmentnumber": 10543453,
    "geocoordX": 8.6300911,
    "geocoordY": 49.8725678,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1126,
    "type": "ELEVATOR"
  },
  {
    "description": "zu Gleis 5/6",
    "equipmentnumber": 10543454,
    "geocoordX": 8.6298163,
    "geocoordY": 49.8725555,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1126,
    "type": "ELEVATOR"
  },
  {
    "description": "zu Gleis 7/8",
    "equipmentnumber": 10543455,
    "geocoordX": 8.6295535,
    "geocoordY": 49.87254,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1126,
    "type": "ELEVATOR"
  },
  {
    "description": "zu Gleis 9/10",
    "equipmentnumber": 10543456,
    "geocoordX": 8.6293117,
    "geocoordY": 49.8725263,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1126,
    "type": "ELEVATOR"
  },
  {
    "description": "zu Gleis 11/12",
    "equipmentnumber": 10543457,
    "geocoordX": 8.6290451,
    "geocoordY": 49.8725147,
    "operatorname": "DB InfraGO",
    "state": "ACTIVE",
    "stateExplanation": "available",
    "stationnumber": 1126,
    "type": "ELEVATOR"
  }
]
)__"sv;

// constexpr auto const kGTFS = R"(
// # agency.txt
// agency_id,agency_name,agency_url,agency_timezone,agency_lang,agency_phone,agency_fare_url,agency_email
// a_1,Deutsche Bahn,db.de,Europe/Berlin,de,,,
// a_2,Rhein-Main-Verkehrsbund,rmv.de,Europe/Berlin,de,,,
//
// # booking_rules.txt
// booking_rule_id,booking_rule_name,booking_type,prior_notice_duration_min,prior_notice_duration_max,prior_notice_last_day,prior_notice_last_time,prior_notice_start_day,prior_notice_start_time,prior_notice_service_id,message,pickup_message,drop_off_message,phone_number,info_url,booking_url
// b_Arbeitswoche,Darmstadt Arbeitswoche,1,60,1440,,,,,,,,,,,
// b_Wochenende,Darmstadt Wochenende,1,30,120,,,,,,,,,,,
// b_Nord_Ein,Nord Einstieg,1,60,,,,,,,,,,,,
// b_Nord_Aus,Nord Ausstieg,1,90,,,,,,,,,,,,
// b_Ost,Ost,2,,,1,18:00:00,,,,,,,,,
// b_Süd,Süd,2,,,2,12:00:00,7,00:00:00,,,,,,,
// b_West_Vormittag,West Vormittags,1,60,,,,,,,,,,,,
// b_West_Nachmittag,West Nachmittags,1,20,,,,,,,,,,,,
//
// # calendar.txt
// service_id,service_name,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date
// c_1,Arbeitswoche,1,1,1,1,1,0,0,20250101,20251231
// c_2,Wochenende,0,0,0,0,0,1,1,20250101,20251231
// c_3,Allgemein,1,1,1,1,1,1,1,20250101,20251231
//
// # calendar_dates.txt
// service_id,date,exception_type
// c_1,20250101,2
// c_1,20250418,2
// c_1,20250421,2
// c_1,20250501,2
// c_1,20250529,2
// c_1,20250609,2
// c_1,20250619,2
// c_1,20251003,2
// c_1,20251225,2
// c_1,20251226,2
// c_2,20250101,1
// c_2,20250418,1
// c_2,20250421,1
// c_2,20250501,1
// c_2,20250529,1
// c_2,20250609,1
// c_2,20250619,1
// c_2,20251003,1
// c_2,20251225,1
// c_2,20251226,1
// c_3,20250101,2
// c_3,20250418,2
// c_3,20250421,2
// c_3,20250501,2
// c_3,20250529,2
// c_3,20250609,2
// c_3,20250619,2
// c_3,20251003,2
// c_3,20251225,2
// c_3,20251226,2
// c_3,20250703,2
// c_3,20250704,2
// c_3,20250705,2
// c_3,20250706,2
// c_3,20250707,2
//
// # location.geojson
// {
//   "type": "FeatureCollection",
//   "features": [
//     {
//       "type": "Feature",
//       "id": "Nord",
//       "geometry": {
//         "type": "Polygon",
//         "coordinates": [
//           [
//             [
//               8.652484802610935,
//               49.88950826461542
//             ],
//             [
//               8.632531855280726,
//               49.874863308155824
//             ],
//             [
//               8.633456826348993,
//               49.87012261291535
//             ],
//             [
//               8.667680755875645,
//               49.871740746835144
//             ],
//             [
//               8.664157056567763,
//               49.88255530236867
//             ],
//             [
//               8.652484802610935,
//               49.88950826461542
//             ]
//           ]
//         ]
//       }
//     },
//     {
//       "type": "Feature",
//       "id": "Ost",
//       "geometry": {
//         "type": "Polygon",
//         "coordinates": [
//           [
//             [
//               8.669980537584316,
//               49.879342215997035
//             ],
//             [
//               8.690065623638578,
//               49.86838499193017
//             ],
//             [
//               8.688450594787996,
//               49.858816677986475
//             ],
//             [
//               8.676558109624278,
//               49.856857353264274
//             ],
//             [
//               8.66717626021736,
//               49.861854893978304
//             ],
//             [
//               8.656252792363125,
//               49.86920828943974
//             ],
//             [
//               8.65457903519201,
//               49.874715504945975
//             ],
//             [
//               8.660393139049853,
//               49.8821520879108
//             ],
//             [
//               8.669980537584316,
//               49.879342215997035
//             ]
//           ]
//         ]
//       }
//     },
//     {
//       "type": "Feature",
//       "id": "Süd",
//       "geometry": {
//         "type": "Polygon",
//         "coordinates": [
//           [
//             [
//               8.638377009844826,
//               49.85267109261012
//             ],
//             [
//               8.655907413900893,
//               49.85312547521351
//             ],
//             [
//               8.666786835513683,
//               49.86783380401884
//             ],
//             [
//               8.653183072305495,
//               49.87444825258534
//             ],
//             [
//               8.638176545376032,
//               49.872147294725785
//             ],
//             [
//               8.638377009844826,
//               49.85267109261012
//             ]
//           ]
//         ]
//       }
//     },
//     {
//       "type": "Feature",
//       "id": "West",
//       "geometry": {
//         "type": "Polygon",
//         "coordinates": [
//           [
//             [
//               8.615742026526112,
//               49.87558530573456
//             ],
//             [
//               8.61992641945426,
//               49.86178794375207
//             ],
//             [
//               8.657894279495252,
//               49.8626113537035
//             ],
//             [
//               8.670047037904022,
//               49.87572465292158
//             ],
//             [
//               8.65137143157277,
//               49.88381384194538
//             ],
//             [
//               8.615605883599105,
//               49.87575303841029
//             ],
//             [
//               8.615742026526112,
//               49.87558530573456
//             ]
//           ]
//         ]
//       }
//     },
// {
//   "type": "Feature",
//   "id": "Darmstadt",
//   "geometry": {
//     "type": "Polygon",
//     "coordinates": [
//       [
//         [
//           8.654200005936705,
//           49.89605856027628
//         ],
//         [
//           8.615821336359803,
//           49.87570244657914
//         ],
//         [
//           8.614919725647121,
//           49.846600119930265
//         ],
//         [
//           8.677517269409314,
//           49.84307687627821
//         ],
//         [
//           8.690268620916164,
//           49.86653618300896
//         ],
//         [
//           8.68576056735159,
//           49.88438253735107
//         ],
//         [
//           8.671141593654223,
//           49.88948621165312
//         ],
//         [
//           8.654200005936705,
//           49.89605856027628
//         ]
//       ]
//     ]
//   }
// }
//   ]
// }
//
// # routes.txt
// route_id,agency_id,route_short_name,route_long_name,route_type
// Darmstadt_nach_West,a_1,,Darmstadt nach West,715
// Nord_nach_Nord_und_Ost,a_1,,Nord nach Nord und Ost,715
// Ost_nach_Ost_und_Süd,a_1,,Ost nach Ost und Süd,715
// Süd_nach_Süd,a_1,,Süd nach Süd,715
// West_nach_Ost_und_Süd,a_1,,West nach Ost und Süd,715
// Nord_Ost_Lichtwiese,a_1,,Nord nach Ost nach Lichtwiese,100
//
// # stop_times.txt
// trip_id,arrival_time,departure_time,stop_id,location_group_id,location_id,stop_sequence,start_pickup_drop_off_window,end_pickup_drop_off_window,pickup_booking_rule_id,drop_off_booking_rule_id,stop_headsign,pickup_type,drop_off_type
// dnw_Arbeitswoche_1,,,,,Darmstadt,,08:00:00,12:00:00,b_Arbeitswoche,b_Arbeitswoche,,2,1
// dnw_Arbeitswoche_1,,,,,West,,08:00:00,12:00:00,b_Arbeitswoche,b_Arbeitswoche,,1,2
// dnw_Arbeitswoche_2,,,,,Darmstadt,,13:00:00,20:00:00,b_Arbeitswoche,b_Arbeitswoche,,2,1
// dnw_Arbeitswoche_2,,,,,West,,13:00:00,20:00:00,b_Arbeitswoche,b_Arbeitswoche,,1,2
// dnw_Wochenende,,,,,Darmstadt,,10:00:00,18:00:00,b_Wochenende,b_Wochenende,,2,1
// dnw_Wochenende,,,,,West,,10:00:00,18:00:00,b_Wochenende,b_Wochenende,,1,2
// nnno_1,,,,,Nord,,12:00:00,16:00:00,b_Nord_Ein,b_Nord_Aus,,2,2
// nnno_1,,,,,Ost,,14:00:00,16:00:00,b_Nord_Ein,b_Nord_Aus,,1,2
// nnno_2,,,,,Nord,,09:00:00,18:00:00,b_Nord_Ein,b_Nord_Aus,,2,1
// nnno_2,,,,,Nord,,09:00:00,18:00:00,b_Nord_Ein,b_Nord_Aus,,1,2
// nnno_2,,,,,Ost,,09:00:00,18:00:00,,b_Nord_Aus,,1,2
// onos_1,,,,,Ost,,08:00:00,20:00:00,b_Ost,b_Ost,,2,1
// onos_1,,,,,Ost,,08:00:00,20:00:00,b_Ost,b_Ost,,1,2
// onos_1,,,,,Süd,,15:00:00,20:00:00,b_Ost,b_Ost,,1,2
// onos_2,,,,,Ost,,06:00:00,12:00:00,b_Ost,b_Ost,,2,1
// onos_2,,,,,Ost,,06:00:00,12:00:00,b_Ost,b_Ost,,1,2
// onos_2,,,,,Süd,,10:00:00,13:00:00,b_Ost,b_Ost,,1,2
// sns_1,,,,,Süd,,06:00:00,14:00:00,b_Süd,b_Süd,,2,1
// sns_1,,,,,Süd,,06:00:00,14:00:00,b_Süd,b_Süd,,1,2
// sns_2,,,,,Süd,,10:00:00,18:00:00,b_Ost,b_Ost,,2,1
// sns_2,,,,,Süd,,10:00:00,18:00:00,b_Ost,b_Ost,,1,2
// sns_3,,,,,Süd,,07:00:00,16:00:00,b_Süd,b_Süd,,2,1
// sns_3,,,,,Süd,,07:00:00,16:00:00,b_Süd,b_Süd,,1,2
// wnos_1,,,,,West,,08:00:00,12:00:00,b_West_Vormittag,b_West_Vormittag,,2,1
// wnos_1,,,,,Ost,,08:00:00,12:00:00,b_West_Vormittag,b_West_Vormittag,,1,2
// wnos_1,,,,,Süd,,08:00:00,12:00:00,b_West_Vormittag,b_West_Vormittag,,1,2
// wnos_2,,,,,West,,13:00:00,18:00:00,b_West_Nachmittag,b_West_Nachmittag,,2,1
// wnos_2,,,,,Ost,,13:00:00,18:00:00,b_West_Nachmittag,b_West_Nachmittag,,1,2
// wnos_2,,,,,Süd,,13:00:00,18:00:00,b_West_Nachmittag,b_West_Nachmittag,,1,2
// r_nol,13:00:00,13:05:00,s_4,,,0,,,,,,0,0
// r_nol,13:10:00,13:13:00,s_1,,,1,,,,,,0,0
// r_nol,13:15:00,13:20:00,s_8,,,2,,,,,,0,0
//
// # stops.txt
// stop_id,stop_code,stop_name,tts_stop_name,stop_desc,stop_lat,stop_lon,zone_id,stop_url,location_type,parent_station,stop_timezone,wheelchair_boarding,level_id,platform_code
// s_1,,Darmstadt-Ostbahnhof,,,49.87441240201039,8.673522730953579,,,,,Europe/Berlin,0,,
// s_2,,Darmstadt-HBF,,,49.872831594042964,8.628161540647596,,,,,Europe/Berlin,0,,
// s_3,,Darmstadt-Südbahnhof,,,49.8529412214732,8.64680776859845,,,,,Europe/Berlin,0,,
// s_4,,Darmstadt-Nordbahnhof,,,49.891010734582096,8.653007984390484,,,,,Europe/Berlin,0,,
// s_5,,Darmstadt-Schloss,,,49.872384112892746,8.657118620363718,,,,,Europe/Berlin,0,,
// s_6,,Darmstadt-Luisenplatz,,,49.872658818126524,8.650172036499612,,,,,Europe/Berlin,0,,
// s_7,,Darmstadt-Jugendstilbad,,,49.872922544554115,8.663944746090607,,,,,Europe/Berlin,0,,
// s_8,,Darmstadt-Lichtwiese,,,49.86749783288525,8.678919024050288,,,,,Europe/Berlin,0,,
// s_9,,TU-Darmstadt-Stadtmitte,,,49.87467964052996,8.656740285924798,,,,,Europe/Berlin,0,,
//
// # trips.txt
// route_id,service_id,trip_id,trip_headsign,trip_short_name,direction_id,block_id
// Darmstadt_nach_West,c_1,dnw_Arbeitswoche_1,,,,
// Darmstadt_nach_West,c_1,dnw_Arbeitswoche_2,,,,
// Darmstadt_nach_West,c_2,dnw_Wochenende,,,,
// Nord_nach_Nord_und_Ost,c_3,nnno_1,,,,
// Nord_nach_Nord_und_Ost,c_3,nnno_2,,,,
// Ost_nach_Ost_und_Süd,c_3,onos_1,,,,
// Ost_nach_Ost_und_Süd,c_3,onos_2,,,,
// Süd_nach_Süd,c_3,sns_1,,,,
// Süd_nach_Süd,c_3,sns_2,,,,
// Süd_nach_Süd,c_3,sns_3,,,,
// West_nach_Ost_und_Süd,c_3,wnos_1,,,,
// West_nach_Ost_und_Süd,c_3,wnos_2,,,,
// Nord_Ost_Lichtwiese,c_3,r_nol,,,,
// )"sv;

constexpr auto const kGTFS = R"(
# agency.txt
agency_id,agency_name,agency_url,agency_timezone,agency_lang,agency_phone,agency_fare_url,agency_email
a_1,Deutsche Bahn,db.de,Europe/Berlin,de,,,

# booking_rules.txt
booking_rule_id,booking_rule_name,booking_type,prior_notice_duration_min,prior_notice_duration_max,prior_notice_last_day,prior_notice_last_time,prior_notice_start_day,prior_notice_start_time,prior_notice_service_id,message,pickup_message,drop_off_message,phone_number,info_url,booking_url
b_1,Buchungsregel,1,60,,,,,,,,,,,,

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
r_bahn,a_1,,Bahnfahrt,100

# stop_times.txt
trip_id,arrival_time,departure_time,stop_id,location_group_id,location_id,stop_sequence,start_pickup_drop_off_window,end_pickup_drop_off_window,pickup_booking_rule_id,drop_off_booking_rule_id,stop_headsign,pickup_type,drop_off_type
flex_1,,,,,Startbereich,,08:00:00,20:00:00,b_1,b_1,,2,2
flex_2,,,,,Zielbereich,,08:00:00,20:00:00,b_1,b_1,,2,2
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
r_bahn,c_1,bahn,,,,
)"sv;

using namespace std::chrono_literals;

TEST(motis, create_offsets) {
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
          << " [track=" << leg.from_.track_.value_or("-")
          << ", scheduled_track=" << leg.from_.scheduledTrack_.value_or("-")
          << ", level=" << leg.from_.level_ << "]"
          << ", to=" << leg.to_.stopId_.value_or("-")
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

  auto ec = std::error_code{};
  std::filesystem::remove_all("test/data", ec);

  // openapi::now_testwise = date::sys_days{date::January / 2 / 2025} + 8h +
  // 30min;
  auto const c =
      config{.server_ = {{.web_folder_ = "ui/build", .n_threads_ = 1U}},
             .osm_ = {"test/resources/gtfs-flex/Darmstadt.osm.pbf"},
             .timetable_ =
                 config::timetable{
                     .first_day_ = "2025-01-01",
                     .num_days_ = 365,
                     .datasets_ = {{"test", {.path_ = std::string{kGTFS}}}}},
             .street_routing_ = true,
             .osr_footpath_ = false,
             .geocoding_ = false,
             .reverse_geocoding_ = false};
  auto d = import(c, "test/data", true);
  d.rt_->e_ = std::make_unique<elevators>(*d.w_, *d.elevator_nodes_,
                                          parse_fasta(kFastaJson));
  d.init_rtt(date::sys_days{date::May / 1 / 2019});

  std::cout << "Stops -------------------------" << std::endl;
  for (auto i = 0; i < d.tt_->locations_.names_.size(); ++i) {
    std::cout
        << "idx: " << i << ", id: "
        << std::string(
               d.tt_->locations_.ids_[nigiri::location_idx_t{i}].begin(),
               d.tt_->locations_.ids_[nigiri::location_idx_t{i}].end())
        << ", name: "
        << std::string(
               d.tt_->locations_.names_[nigiri::location_idx_t{i}].begin(),
               d.tt_->locations_.names_[nigiri::location_idx_t{i}].end())
        << std::endl;
  }
  std::cout << "Ende --------------------------" << std::endl;

  // Einfachste Fahrt mit erster und letzter Meile
  auto const routing = utl::init_from<ep::routing>(d).value();
  openapi::now_testwise = date::sys_days{date::January / 01 / 2025} + 7h + 0min;
  // FROM: 49.86612067335565,8.65312725611551
  // TO: 49.86734352295011,8.661965445062833
  auto plan_response = routing(
      "?fromPlace=49.86576808937855,8.650554050873524"
      "&toPlace=49.86768861746879,8.665222857978648"
      "&time=2025-01-02T11:50Z"
      "&timetableView=false"
      "&directModes="
      "&useRoutedTransfers=false"
      "&transferModes=RAIL"
      "&preTransitModes=WALK"
      "&postTransitModes=WALK");

  auto ss = std::stringstream{};
  for (auto const& j : plan_response.itineraries_) {
    print_short(ss, j);
  }

  EXPECT_EQ("", ss.str());

  // DIREKTE FAHRTEN

  // auto plan_response = routing(
  //     "?fromPlace=49.87928210850674,8.670346686172053"
  //     "&toPlace=49.87467964052996,8.656740285924798"
  //     "&time=2025-01-02T09:00Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&directModes=FLEX"
  //     "&preTransitModes="
  //     "&postTransitModes=");
  //
  // auto ss = std::stringstream{};
  // for (auto const& j : plan_response.direct_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ("", ss.str());
  // openapi::now_testwise = date::sys_days{date::January / 2 / 2025} + 7h +
  // 0min; plan_response = routing(
  //     "?fromPlace=49.88620379937703,8.651951333124543"
  //     "&toPlace=49.876956522500876,8.650293148416495"
  //     "&time=2025-01-02T13:30Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&directModes=FLEX"
  //     "&preTransitModes="
  //     "&postTransitModes=");
  //
  // ss = std::stringstream{};
  // for (auto const& j : plan_response.direct_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ("", ss.str());
  //
  // openapi::now_testwise =
  //     date::sys_days{date::December / 31 / 2024} + 7h + 0min;
  // plan_response = routing(
  //     "?fromPlace=49.8529412214732,8.64680776859845"
  //     "&toPlace=49.86368377097219,8.662545815759273"
  //     "&time=2025-01-02T10:00Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&directModes=FLEX"
  //     "&preTransitModes="
  //     "&postTransitModes=");
  //
  // ss = std::stringstream{};
  // for (auto const& j : plan_response.direct_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ("", ss.str());

  // FAHRTEN MIT ERSTER UND LETZTER MEILE
  //    Von Waldspirale nach Ostbahnhof zu Lichtwiese zu Kronenapotheke
  // openapi::now_testwise =
  //     date::sys_days{date::December / 31 / 2024} + 7h + 0min;
  // auto plan_response = routing(
  //     "?fromPlace=49.88506789924182,8.656324202355307"
  //     "&toPlace=49.855772322709726,8.653508944895748"
  //     "&time=2025-01-02T12:50Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&transitModes=REGIONAL_RAIL"
  //     "&preTransitModes=WALK"
  //     "&postTransitModes=WALK"
  //     "&maxPreTransitTime=3600");
  //
  // auto ss = std::stringstream{};
  // for (auto const& j : plan_response.itineraries_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ("", ss.str());

  // FROM: 49.88506789924182,8.656324202355307
  // TO: 49.86918982161268,8.66997253239569

  // openapi::now_testwise =
  //     date::sys_days{date::December / 31 / 2024} + 7h + 0min;
  // auto plan_response = routing(
  // "?fromPlace=49.88506789924182,8.656324202355307"
  // "&toPlace=49.86918982161268,8.66997253239569"
  // "&time=2025-01-02T11:50Z"
  //     "&timetableView=false"
  //     "&directModes="
  //     "&useRoutedTransfers=true"
  //     "&transferModes=RAIL"
  //     "&preTransitModes=WALK"
  //     "&postTransitModes=WALK");
  //
  // auto ss = std::stringstream{};
  // for (auto const& j : plan_response.itineraries_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ("", ss.str());

  // plan_response = routing(
  //     "?fromPlace=49.87441240201039,8.673522730953579"
  //     "&toPlace=49.87531011337251,8.627555930582304"
  //     "&time=2025-01-02T10:25Z"
  //     "&timetableView=false"
  //     "&useRoutedTransfers=false"
  //     "&preTransitModes=FLEX");
  //
  // ss = std::stringstream{};
  // for (auto const& j : plan_response.direct_) {
  //   print_short(ss, j);
  // }
  //
  // EXPECT_EQ("", ss.str());

  // auto const plan_response = routing(
  // "?fromPlace=49.87531011337251,8.627555930582304"
  // "&toPlace=49.87462546699038,8.656425288735107"
  //       "&time=2019-05-01T01:25Z"
  //       "&timetableView=false"
  //       "&useRoutedTransfers=false"
  //       "&preTransitModes=FLEX");
}