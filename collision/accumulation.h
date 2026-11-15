#pragma once

#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "base/not_null.hpp"
#include "geometry/frame.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/instant.hpp"
#include "geometry/space_transformations.hpp"
#include "integrators/integrators.hpp"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/identification.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/discrete_trajectory_segment_iterator.hpp"
#include "physics/ephemeris.hpp"
#include "physics/euler_solver.hpp"
#include "physics/massless_body.hpp"
#include "physics/mechanical_system.hpp"
#include "physics/rigid_motion.hpp"
#include "physics/tensors.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "serialization/ksp_plugin.pb.h"

namespace cosmos_logic {
namespace ksp_plugin {

FORWARD_DECLARE(class, Part, FROM(part), INTO(pile_up));

class TestablePileUp;

namespace _pile_up {
namespace internal {

using namespace cosmos_logic::base::_not_null;
using namespace cosmos_logic::geometry::_frame;
using namespace cosmos_logic::geometry::_grassmann;
using namespace cosmos_logic::geometry::_instant;
using namespace cosmos_logic::geometry::_space_transformations;
using namespace cosmos_logic::integrators::_integrators;
using namespace cosmos_logic::ksp_plugin::_frames;
using namespace cosmos_logic::ksp_plugin::_identification;
using namespace cosmos_logic::physics::_degrees_of_freedom;
using namespace cosmos_logic::physics::_discrete_trajectory;
using namespace cosmos_logic::physics::_discrete_trajectory_segment_iterator;
using namespace cosmos_logic::physics::_ephemeris;
using namespace cosmos_logic::physics::_euler_solver;
using namespace cosmos_logic::physics::_massless_body;
using namespace cosmos_logic::physics::_mechanical_system;
using namespace cosmos_logic::physics::_rigid_motion;
using namespace cosmos_logic::physics::_tensors;
using namespace cosmos_logic::quantities::_named_quantities;
using namespace cosmos_logic::quantities::_quantities;





using ApparentPileUp = Frame<struct ApparentPileUpTag,
                             NonRotating,
                             Handedness::Right>;









using NonRotatingPileUp = Frame<serialization::Frame::PluginTag,
                                NonRotating,
                                Handedness::Right,
                                serialization::Frame::NON_ROTATING_PILE_UP>;



using PileUpPrincipalAxes = Frame<serialization::Frame::PluginTag,
                                  Arbitrary,
                                  Handedness::Right,
                                  serialization::Frame::PILE_UP_PRINCIPAL_AXES>;




class PileUp {
 public:
  PileUp(
      std::list<not_null<Part*>> parts,
      Instant const& t,
      Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters,
      Ephemeris<Barycentric>::FixedStepParameters fixed_step_parameters,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void()> deletion_callback);

  
  virtual ~PileUp();

  std::list<not_null<Part*>> const& parts() const;
  Ephemeris<Barycentric>::FixedStepParameters const& fixed_step_parameters()
      const;

  
  
  
  void SetPartApparentRigidMotion(
      not_null<Part*> part,
      RigidMotion<RigidPart, Apparent> const& rigid_motion);

  
  
  
  
  absl::Status DeformAndAdvanceTime(Instant const& t);

  
  void RecomputeFromParts();

  using PileUpForSerializationIndex =
      std::function<not_null<std::shared_ptr<PileUp>>(int)>;
  using SerializationIndexForPileUp =
      std::function<int(not_null<PileUp const*>)>;

  void WriteToMessage(not_null<serialization::PileUp*> message) const;
  static not_null<std::unique_ptr<PileUp>> ReadFromMessage(
      serialization::PileUp const& message,
      std::function<not_null<Part*>(PartId)> const& part_id_to_part,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void()> deletion_callback);

 private:
  
  
  using AppendToPartTrajectory =
      void (Part::*)(Instant const&, DegreesOfFreedom<Barycentric> const&);

  
  
  PileUp(
      std::list<not_null<Part*>>&& parts,
      Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters,
      Ephemeris<Barycentric>::FixedStepParameters fixed_step_parameters,
      DiscreteTrajectory<Barycentric> trajectory,
      std::optional<DiscreteTrajectorySegmentIterator<Barycentric>> history,
      std::optional<DiscreteTrajectorySegmentIterator<Barycentric>>
          psychohistory,
      Bivector<AngularMomentum, NonRotatingPileUp> const& angular_momentum,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void()> deletion_callback);

  
  void MakeEulerSolver(InertiaTensor<NonRotatingPileUp> const& inertia_tensor,
                       Instant const& t);

  
  
  
  
  
  
  void DeformPileUpIfNeeded(Instant const& t);

  
  
  
  
  absl::Status AdvanceTime(Instant const& t);

  
  
  
  
  void NudgeParts() const;

  template<AppendToPartTrajectory append_to_part_trajectory>
  void AppendToPart(DiscreteTrajectory<Barycentric>::iterator it) const;

  
  not_null<std::unique_ptr<absl::Mutex>> lock_;

  std::list<not_null<Part*>> parts_;
  not_null<Ephemeris<Barycentric>*> ephemeris_;
  Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters_;
  Ephemeris<Barycentric>::FixedStepParameters fixed_step_parameters_;

  
  Mass mass_;
  Vector<Force, Barycentric> intrinsic_force_;
  Bivector<Torque, NonRotatingPileUp> intrinsic_torque_;
  
  
  
  
  Bivector<AngularMomentum, NonRotatingPileUp> angular_momentum_change_;

  
  
  DiscreteTrajectory<Barycentric> trajectory_;

  
  
  
  
  DiscreteTrajectorySegmentIterator<Barycentric> history_;

  
  
  
  
  
  
  
  
  DiscreteTrajectorySegmentIterator<Barycentric> psychohistory_;

  
  Bivector<AngularMomentum, NonRotatingPileUp> angular_momentum_;

  
  
  
  
  std::unique_ptr<typename Integrator<
      Ephemeris<Barycentric>::NewtonianMotionEquation>::Instance>
      fixed_instance_;

  PartTo<RigidMotion<RigidPart, NonRotatingPileUp>> actual_part_rigid_motion_;
  PartTo<RigidMotion<RigidPart, Apparent>> apparent_part_rigid_motion_;

  PartTo<RigidTransformation<RigidPart, PileUpPrincipalAxes>> rigid_pile_up_;
  std::optional<EulerSolver<NonRotatingPileUp, PileUpPrincipalAxes>>
      euler_solver_;

  
  std::function<void()> deletion_callback_;

  friend class ksp_plugin::TestablePileUp;
};


struct PileUpFuture {
  PileUpFuture(not_null<PileUp const*> pile_up,
               std::future<absl::Status> future);
  not_null<PileUp const*> pile_up;
  std::future<absl::Status> future;
};

}  

using internal::ApparentPileUp;
using internal::NonRotatingPileUp;
using internal::PileUp;
using internal::PileUpFuture;
using internal::PileUpPrincipalAxes;

}  
}  
}  
