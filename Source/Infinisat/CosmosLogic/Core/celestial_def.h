#pragma once

#include <memory>

#include "base/not_null.hpp"
#include "geometry/instant.hpp"
#include "geometry/space.hpp"
#include "ksp_plugin/frames.hpp"
#include "physics/body.hpp"
#include "physics/continuous_trajectory.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/rotating_body.hpp"
#include "serialization/ksp_plugin.pb.h"

namespace cosmos_logic {
namespace ksp_plugin {
namespace _celestial {
namespace internal {

using namespace cosmos_logic::base::_not_null;
using namespace cosmos_logic::geometry::_instant;
using namespace cosmos_logic::geometry::_space;
using namespace cosmos_logic::ksp_plugin::_frames;
using namespace cosmos_logic::physics::_body;
using namespace cosmos_logic::physics::_continuous_trajectory;
using namespace cosmos_logic::physics::_degrees_of_freedom;
using namespace cosmos_logic::physics::_rotating_body;


class Celestial {
 public:
  explicit Celestial(not_null<RotatingBody<Barycentric> const*> body);
  Celestial(Celestial const&) = delete;
  Celestial(Celestial&&) = delete;
  virtual ~Celestial() = default;

  
  bool is_initialized() const;
  void set_trajectory(
      not_null<ContinuousTrajectory<Barycentric> const*> trajectory);
  ContinuousTrajectory<Barycentric> const& trajectory() const;
  virtual DegreesOfFreedom<Barycentric> current_degrees_of_freedom(
      Instant const& current_time) const;
  virtual Position<Barycentric> current_position(
      Instant const& current_time) const;
  virtual Velocity<Barycentric> current_velocity(
      Instant const& current_time) const;

  not_null<RotatingBody<Barycentric> const*> body() const;
  bool has_parent() const;
  Celestial const* parent() const;  
  void set_parent(not_null<Celestial const*> parent);

 private:
  not_null<RotatingBody<Barycentric> const*> body_;
  
  
  Celestial const* parent_ = nullptr;
  ContinuousTrajectory<Barycentric> const* trajectory_ = nullptr;
};

}  

using internal::Celestial;

}  
}  
}  
