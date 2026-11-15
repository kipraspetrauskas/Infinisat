#pragma once

#include <optional>
#include <ostream>
#include <string>

#include "base/not_null.hpp"
#include "geometry/instant.hpp"
#include "physics/body.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/massive_body.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace physics {
namespace _kepler_orbit {
namespace internal {

using namespace principia::base::_not_null;
using namespace principia::geometry::_instant;
using namespace principia::physics::_body;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_massive_body;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;

template<typename Frame>
struct KeplerianElements final {
  
  
  
  std::optional<double> eccentricity;
  
  std::optional<Angle> asymptotic_true_anomaly;
  std::optional<Angle> turning_angle;
  
  std::optional<Length> semimajor_axis;
  std::optional<SpecificEnergy> specific_energy;
  std::optional<SpecificEnergy> characteristic_energy;
  
  std::optional<AngularFrequency> mean_motion;
  std::optional<Time> period;
  
  std::optional<AngularFrequency> hyperbolic_mean_motion;
  std::optional<Speed> hyperbolic_excess_velocity;
  
  
  std::optional<Length> semiminor_axis;
  std::optional<Length> impact_parameter;
  
  std::optional<Length> semilatus_rectum;
  std::optional<SpecificAngularMomentum>
      specific_angular_momentum;
  
  std::optional<Length> periapsis_distance;
  
  std::optional<Length> apoapsis_distance;

  
  
  Angle inclination;
  Angle longitude_of_ascending_node;
  std::optional<Angle> argument_of_periapsis;
  std::optional<Angle> longitude_of_periapsis;

  
  std::optional<Angle> true_anomaly;
#if NOT_YET_IMPLEMENTED
  std::optional<Angle> true_longitude;
  std::optional<Time> time_since_periapsis;
#endif
  
  std::optional<Angle> mean_anomaly;
#if NOT_YET_IMPLEMENTED
  std::optional<Angle> mean_longitude;
#endif
  
  std::optional<Angle> hyperbolic_mean_anomaly;

  void WriteToMessage(
      not_null<serialization::KeplerianElements*> message) const;
};

template<typename Frame>
std::string DebugString(KeplerianElements<Frame> const& elements);

template<typename Frame>
std::ostream& operator<<(std::ostream& out,
                         KeplerianElements<Frame> const& elements);

template<typename Frame>
class KeplerOrbit final {
  static_assert(!Frame::may_rotate);

 public:
  
  
  KeplerOrbit(MassiveBody const& primary,
              Body const& secondary,
              KeplerianElements<Frame> const& elements_at_epoch,
              Instant const& epoch);
  KeplerOrbit(MassiveBody const& primary,
              Body const& secondary,
              RelativeDegreesOfFreedom<Frame> const& state_vectors,
              Instant const& epoch);

  
  RelativeDegreesOfFreedom<Frame> StateVectors(Instant const& t) const;

  
  KeplerianElements<Frame> const& elements_at_epoch() const;

 private:
  
  
  static void CompleteElements(KeplerianElements<Frame>& elements,
                               GravitationalParameter const& μ);
  
  
  
  static void CompleteConicParametersByCategory(
      KeplerianElements<Frame>& elements,
      GravitationalParameter const& μ);
  
  static void CompleteConicParameters(KeplerianElements<Frame>& elements,
                                      GravitationalParameter const& μ);
  
  static void CompleteOrientationParameters(KeplerianElements<Frame>& elements);
  
  
  static void CompleteAnomalies(KeplerianElements<Frame>& elements);

  GravitationalParameter const gravitational_parameter_;
  KeplerianElements<Frame> elements_at_epoch_;
  Instant const epoch_;
};

}  

using internal::KeplerianElements;
using internal::KeplerOrbit;

}  
}  
}  

#include "physics/kepler_orbit_body.hpp"
