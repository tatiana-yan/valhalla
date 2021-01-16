#include "gurka.h"
#include <gtest/gtest.h>

using namespace valhalla;

TEST(Standalone, DestinationOnly) {
  const std::string ascii_map = R"(
      A----B----C----D----E
           |         |    |
           |         |    |
           I----H----G----F
  )";

  const gurka::ways ways = {
      {"AB", {{"highway", "residential"}}},
      {"BC", {{"highway", "residential"}}},
      {"CD", {{"highway", "residential"}}},
      {"DE", {{"highway", "residential"}}},
      {"EF", {{"highway", "residential"}}},
      {"FG", {{"highway", "residential"}}},
      {"GH", {{"highway", "residential"}}},
      {"HI", {{"highway", "residential"}}},
      {"DG", {{"highway", "residential"}, {"motor_vehicle", "destination"}}},
      {"BI", {{"highway", "residential"}, {"access", "private"}}},

  };
  const auto layout = gurka::detail::map_to_coordinates(ascii_map, 100);
  auto map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_destination");
  auto result = gurka::route(map, "A", "I", "auto");

  ASSERT_EQ(result.trip().routes(0).legs_size(), 1);
  auto leg = result.trip().routes(0).legs(0);
  gurka::assert::raw::expect_path(result, {"AB", "BC", "CD", "DE", "EF", "FG", "GH", "HI"});
}

TEST(Standalone, DestinationOnlyTargetArea) {
  const std::string ascii_map = R"(
      A----B----C----D
             ^^   ^^
  )";

  const gurka::ways ways = {
      {"AB", {{"highway", "residential"}}},
      {"BC", {{"highway", "residential"}, {"access", "private"}}},
      {"CD", {{"highway", "residential"}, {"access", "private"}}},

  };
  const auto layout = gurka::detail::map_to_coordinates(ascii_map, 100);
  auto map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_destination_target_area");

  // "bidirectional_a*" is ok
  {
    auto result = gurka::route(map, "A", "D", "auto");
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "bidirectional_a*");
  }

  // "time_dependent_forward_a*" is ok
  {
    auto result = gurka::route(map, "A", "D", "auto",
                               {{"/date_time/type", "1"}, {"/date_time/value", "2020-10-30T09:00"}});
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "time_dependent_forward_a*");
  }

  // "time_dependent_reverse_a*" is ok
  {
    auto result = gurka::route(map, "A", "D", "auto",
                               {{"/date_time/type", "2"}, {"/date_time/value", "2020-10-30T09:00"}});
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "time_dependent_reverse_a*");
  }
}

TEST(Standalone, DestinationOnlyMidway) {
  const std::string ascii_map = R"(
      A----B----C----D----E
             ^^   ^^
  )";

  const gurka::ways ways = {
      {"AB", {{"highway", "residential"}}},
      {"BC", {{"highway", "residential"}, {"access", "private"}}},
      {"CD", {{"highway", "residential"}, {"access", "private"}}},
      {"DE", {{"highway", "residential"}}},
  };
  const auto layout = gurka::detail::map_to_coordinates(ascii_map, 100);
  auto map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_destination_midway");

  {
    auto result = gurka::route(map, "A", "E", "auto");
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "bidirectional_a*");
  }

  {
    auto result = gurka::route(map, "A", "E", "auto",
                               {{"/date_time/type", "1"}, {"/date_time/value", "2020-10-30T09:00"}});
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "time_dependent_forward_a*");
  }

  {
    auto result = gurka::route(map, "A", "E", "auto",
                               {{"/date_time/type", "2"}, {"/date_time/value", "2020-10-30T09:00"}});
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "time_dependent_reverse_a*");
  }
}

TEST(Standalone, DestinationOnlySourceTarget) {
  const std::string ascii_map = R"(
      A----B----C----D----E
        ^^             ^^
  )";

  const gurka::ways ways = {
      {"AB", {{"highway", "residential"}, {"access", "private"}}},
      {"BC", {{"highway", "residential"}}},
      {"CD", {{"highway", "residential"}}},
      {"DE", {{"highway", "residential"}, {"access", "private"}}},
  };
  const auto layout = gurka::detail::map_to_coordinates(ascii_map, 100);
  auto map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_destination_src_tgt");

  {
    auto result = gurka::route(map, "A", "E", "auto");
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "bidirectional_a*");
  }

  {
    auto result = gurka::route(map, "A", "E", "auto",
                               {{"/date_time/type", "1"}, {"/date_time/value", "2020-10-30T09:00"}});
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "time_dependent_forward_a*");
  }

  {
    auto result = gurka::route(map, "A", "E", "auto",
                               {{"/date_time/type", "2"}, {"/date_time/value", "2020-10-30T09:00"}});
    EXPECT_EQ(result.trip().routes(0).legs(0).algorithms(0), "time_dependent_reverse_a*");
  }
}
