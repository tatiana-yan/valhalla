#include "baldr/datetime.h"
#include "baldr/rapidjson_utils.h"
#include "loki/search.h"
#include "loki/worker.h"
#include "midgard/logging.h"

using namespace valhalla;
using namespace valhalla::baldr;

namespace {
midgard::PointLL to_ll(const valhalla::Location& l) {
  return midgard::PointLL{l.ll().lng(), l.ll().lat()};
}

void check_distance(const google::protobuf::RepeatedPtrField<valhalla::Location>& locations,
                    float matrix_max_distance,
                    float& max_location_distance) {
  // see if any locations pairs are unreachable or too far apart
  for (auto source = locations.begin(); source != locations.end() - 1; ++source) {
    for (auto target = source + 1; target != locations.end(); ++target) {
      // check if distance between latlngs exceed max distance limit
      auto path_distance = to_ll(*source).Distance(to_ll(*target));

      if (path_distance >= max_location_distance) {
        max_location_distance = path_distance;
      }

      if (path_distance > matrix_max_distance) {
        throw valhalla_exception_t{154};
      };
    }
  }
}

const std::tuple<float, float>
get_max_metrics(const google::protobuf::RepeatedPtrField<valhalla::Contour>& contours) {
  float max_dist, max_time = 0.0f;
  for (auto& contour : contours) {
    max_time = std::max(max_time, contour.has_time() ? contour.time() : max_time);
    max_dist = std::max(max_dist, contour.has_distance() ? contour.distance() : max_dist);
  }
  return std::make_tuple(max_time, max_dist);
}
} // namespace

namespace valhalla {
namespace loki {

void loki_worker_t::init_isochrones(Api& request) {
  auto& options = *request.mutable_options();

  // strip off unused information
  parse_locations(options.mutable_locations());
  if (options.locations_size() < 1) {
    throw valhalla_exception_t{120};
  };
  for (auto& l : *options.mutable_locations()) {
    l.clear_heading();
  }

  // check that the number of contours is ok
  if (options.contours_size() < 1) {
    throw valhalla_exception_t{113};
  } else if (options.contours_size() > max_contours) {
    throw valhalla_exception_t{152, std::to_string(max_contours)};
  }

  // check the contour metrics
  float max_time_c, max_dist_c;
  std::tie(max_time_c, max_dist_c) = get_max_metrics(options.contours());
  if (max_time_c > max_contour_min) {
    throw valhalla_exception_t{151, std::to_string(max_contour_min)};
  }
  if (max_dist_c > max_contour_km) {
    throw valhalla_exception_t{166, std::to_string(max_contour_km)};
  }

  parse_costing(request);
}
void loki_worker_t::isochrones(Api& request) {
  // time this whole method and save that statistic
  auto _ = measure_scope_time(request, "loki_worker_t::isochrones");

  init_isochrones(request);
  auto& options = *request.mutable_options();
  // check that location size does not exceed max
  if (options.locations_size() > max_locations.find("isochrone")->second) {
    throw valhalla_exception_t{150, std::to_string(max_locations.find("isochrone")->second)};
  };

  // check the distances between the locations
  auto max_location_distance = std::numeric_limits<float>::min();
  check_distance(options.locations(), max_distance.find("isochrone")->second, max_location_distance);

  try {
    // correlate the various locations to the underlying graph
    auto locations = PathLocation::fromPBF(options.locations());
    const auto projections = loki::Search(locations, *reader, costing);
    for (size_t i = 0; i < locations.size(); ++i) {
      const auto& projection = projections.at(locations[i]);
      PathLocation::toPBF(projection, options.mutable_locations(i), *reader);
    }
  } catch (const std::exception&) { throw valhalla_exception_t{171}; }
}

} // namespace loki
} // namespace valhalla
