#pragma once

#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "base/concepts.hpp"
#include "base/not_null.hpp"
#include "base/reanimator.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/instant.hpp"
#include "geometry/space.hpp"
#include "google/protobuf/repeated_field.h"
#include "integrators/integrators.hpp"
#include "integrators/ordinary_differential_equations.hpp"
#include "physics/apsides.hpp"
#include "physics/checkpointer.hpp"
#include "physics/continuous_trajectory.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/geopotential.hpp"
#include "physics/integration_parameters.hpp"
#include "physics/massive_body.hpp"
#include "physics/tensors.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "serialization/ksp_plugin.pb.h"
#include "serialization/numerics.pb.h"
#include "serialization/physics.pb.h"

namespace cosmos_logic {
namespace physics {
namespace _ephemeris {
namespace internal {

using namespace cosmos_logic::base::_concepts;
using namespace cosmos_logic::base::_not_null;
using namespace cosmos_logic::base::_reanimator;
using namespace cosmos_logic::geometry::_grassmann;
using namespace cosmos_logic::geometry::_instant;
using namespace cosmos_logic::geometry::_space;
using namespace cosmos_logic::integrators::_integrators;
using namespace cosmos_logic::integrators::_ordinary_differential_equations;
using namespace cosmos_logic::physics::_apsides;
using namespace cosmos_logic::physics::_checkpointer;
using namespace cosmos_logic::physics::_continuous_trajectory;
using namespace cosmos_logic::physics::_degrees_of_freedom;
using namespace cosmos_logic::physics::_discrete_trajectory;
using namespace cosmos_logic::physics::_geopotential;
using namespace cosmos_logic::physics::_integration_parameters;
using namespace cosmos_logic::physics::_massive_body;
using namespace cosmos_logic::physics::_tensors;
using namespace cosmos_logic::quantities::_named_quantities;
using namespace cosmos_logic::quantities::_quantities;




template<typename Frame>
class Ephemeris {
  static_assert(Frame::is_inertial, "Frame must be inertial");

 public:
  using IntrinsicAcceleration =
      std::function<Vector<Acceleration, Frame>(Instant const& time)>;
  static std::nullptr_t constexpr NoIntrinsicAcceleration = nullptr;
  using GeneralizedIntrinsicAcceleration =
      std::function<Vector<Acceleration, Frame>(
          Instant const& time,
          DegreesOfFreedom<Frame> const& degrees_of_freedom)>;
  using IntrinsicAccelerations = std::vector<IntrinsicAcceleration>;
  static IntrinsicAccelerations const NoIntrinsicAccelerations;
  static std::int64_t constexpr unlimited_max_ephemeris_steps =
      std::numeric_limits<std::int64_t>::max();

  using BodiesToPositions =
      absl::flat_hash_map<not_null<MassiveBody const*>, Position<Frame>>;
  using BodiesToVelocities =
      absl::flat_hash_map<not_null<MassiveBody const*>, Velocity<Frame>>;
  using BodiesToDegreesOfFreedom =
      absl::flat_hash_map<not_null<MassiveBody const*>,
                          DegreesOfFreedom<Frame>>;

  
  using NewtonianMotionEquation =
      SpecialSecondOrderDifferentialEquation<Position<Frame>>;
  using GeneralizedNewtonianMotionEquation =
      ExplicitSecondOrderOrdinaryDifferentialEquation<Position<Frame>>;

  using AdaptiveStepParameters =
      _integration_parameters::AdaptiveStepParameters<NewtonianMotionEquation>;
  using FixedStepParameters =
      _integration_parameters::FixedStepParameters<NewtonianMotionEquation>;
  using GeneralizedAdaptiveStepParameters =
      _integration_parameters::AdaptiveStepParameters<
          GeneralizedNewtonianMotionEquation>;

  class AccuracyParameters final {
   public:
    AccuracyParameters(Length const& fitting_tolerance,
                       double geopotential_tolerance);

    void WriteToMessage(
        not_null<serialization::Ephemeris::AccuracyParameters*> message) const;
    static AccuracyParameters ReadFromMessage(
        serialization::Ephemeris::AccuracyParameters const& message);

   private:
    Length fitting_tolerance_;
    double geopotential_tolerance_ = 0;
    friend class Ephemeris<Frame>;
  };

  
  
  Ephemeris(std::vector<not_null<std::unique_ptr<MassiveBody const>>> bodies,
            std::vector<DegreesOfFreedom<Frame>> const& initial_state,
            Instant const& initial_time,
            AccuracyParameters const& accuracy_parameters,
            FixedStepParameters fixed_step_parameters);

  virtual ~Ephemeris();

  
  virtual std::vector<not_null<MassiveBody const*>> const& bodies() const;

  
  virtual not_null<ContinuousTrajectory<Frame> const*> trajectory(
      not_null<MassiveBody const*> body) const;

  
  virtual bool empty() const;

  
  virtual Instant t_min() const ABSL_LOCKS_EXCLUDED(lock_);
  
  virtual Instant t_max() const ABSL_LOCKS_EXCLUDED(lock_);

  virtual FixedStepSizeIntegrator<NewtonianMotionEquation> const&
  planetary_integrator() const;

  virtual absl::Status last_severe_integration_status() const;

  
  
  BodiesToPositions EvaluateAllPositions(Instant const& t) const
      ABSL_LOCKS_EXCLUDED(lock_);
  BodiesToVelocities EvaluateAllVelocities(Instant const& t) const
      ABSL_LOCKS_EXCLUDED(lock_);
  BodiesToDegreesOfFreedom EvaluateAllDegreesOfFreedom(Instant const& t) const
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  virtual absl::Status Prolong(
      Instant const& t,
      std::int64_t max_ephemeris_steps = unlimited_max_ephemeris_steps)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  
  void RequestReanimation(Instant const& desired_t_min);

  
  
  
  
  
  void AwaitReanimation(Instant const& desired_t_min);

  
  
  
  virtual not_null<
      std::unique_ptr<typename Integrator<NewtonianMotionEquation>::Instance>>
  NewInstance(
      std::vector<not_null<DiscreteTrajectory<Frame>*>> const& trajectories,
      IntrinsicAccelerations const& intrinsic_accelerations,
      FixedStepParameters const& parameters);

  
  virtual absl::StatusOr<not_null<
      std::unique_ptr<typename Integrator<NewtonianMotionEquation>::Instance>>>
  StoppableNewInstance(
      std::vector<not_null<DiscreteTrajectory<Frame>*>> const& trajectories,
      IntrinsicAccelerations const& intrinsic_accelerations,
      FixedStepParameters const& parameters);

  
  
  
  
  
  virtual absl::Status FlowWithAdaptiveStep(
      not_null<DiscreteTrajectory<Frame>*> trajectory,
      IntrinsicAcceleration intrinsic_acceleration,
      Instant const& t,
      AdaptiveStepParameters const& parameters,
      std::int64_t max_ephemeris_steps = unlimited_max_ephemeris_steps)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  virtual absl::Status FlowWithAdaptiveStep(
      not_null<DiscreteTrajectory<Frame>*> trajectory,
      GeneralizedIntrinsicAcceleration intrinsic_acceleration,
      Instant const& t,
      GeneralizedAdaptiveStepParameters const& parameters,
      std::int64_t max_ephemeris_steps = unlimited_max_ephemeris_steps)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  
  virtual absl::Status FlowWithFixedStep(
      Instant const& t,
      typename Integrator<NewtonianMotionEquation>::Instance& instance)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  JacobianOfAcceleration<Frame> ComputeJacobianOnMassiveBody(
      not_null<MassiveBody const*> body,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  Vector<Jerk, Frame> ComputeGravitationalJerkOnMasslessBody(
      DegreesOfFreedom<Frame> const& degrees_of_freedom,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  Vector<Jerk, Frame> ComputeGravitationalJerkOnMassiveBody(
      not_null<MassiveBody const*> body,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  std::vector<Vector<Jerk, Frame>> ComputeGravitationalJerkOnMassiveBodies(
      std::vector<not_null<MassiveBody const*>> const& bodies,
      BodiesToDegreesOfFreedom const& bodies_to_degrees_of_freedom) const;

  
  
  virtual Vector<Acceleration, Frame>
  ComputeGravitationalAccelerationOnMasslessBody(
      Position<Frame> const& position,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  virtual Vector<Acceleration, Frame>
  ComputeGravitationalAccelerationOnMasslessBody(
      not_null<DiscreteTrajectory<Frame>*> trajectory,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  virtual Vector<Acceleration, Frame>
  ComputeGravitationalAccelerationOnMassiveBody(
      not_null<MassiveBody const*> body,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  virtual std::vector<Vector<Acceleration, Frame>>
  ComputeGravitationalAccelerationOnMassiveBodies(
      std::vector<not_null<MassiveBody const*>> const& bodies,
      BodiesToPositions const& bodies_to_positions,
      Instant const& t) const;

  
  SpecificEnergy ComputeGravitationalPotential(
      Position<Frame> const& position,
      Instant const& t) const ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  
  virtual void ComputeApsides(not_null<MassiveBody const*> body1,
                              not_null<MassiveBody const*> body2,
                              DistinguishedPoints<Frame>& apoapsides1,
                              DistinguishedPoints<Frame>& periapsides1,
                              DistinguishedPoints<Frame>& apoapsides2,
                              DistinguishedPoints<Frame>& periapsides2);

  
  
  
  virtual int serialization_index_for_body(
      not_null<MassiveBody const*> body) const;

  virtual not_null<MassiveBody const*> body_for_serialization_index(
      int serialization_index) const;

  virtual void WriteToMessage(not_null<serialization::Ephemeris*> message) const
      ABSL_LOCKS_EXCLUDED(lock_);
  
  
  
  static not_null<std::unique_ptr<Ephemeris>> ReadFromMessage(
      Instant const& desired_t_min,
      serialization::Ephemeris const& message)
    requires serializable<Frame>;

 protected:
  
  
  explicit Ephemeris(FixedStepSizeIntegrator<typename Ephemeris<
                         Frame>::NewtonianMotionEquation> const& integrator);

 private:
  
  void WriteToCheckpointIfNeeded(Instant const& time) const
      ABSL_SHARED_LOCKS_REQUIRED(lock_);
  Checkpointer<serialization::Ephemeris>::Writer MakeCheckpointerWriter();
  Checkpointer<serialization::Ephemeris>::Reader MakeCheckpointerReader();

  
  
  
  
  absl::Status Reanimate(Instant const& desired_t_min)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  absl::Status ReanimateOneCheckpoint(
      serialization::Ephemeris::Checkpoint const& message,
      Instant const& t_initial,
      Instant const& t_final) ABSL_LOCKS_EXCLUDED(lock_);

  bool DesiredTMinReachedOrFullyReanimated(Instant const& desired_t_min)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  void AppendMassiveBodiesState(
      typename NewtonianMotionEquation::State const& state)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(lock_);
  template<typename ContinuousTrajectoryPtr>
  static std::vector<absl::Status> AppendMassiveBodiesStateToTrajectories(
      typename NewtonianMotionEquation::State const& state,
      std::vector<not_null<ContinuousTrajectoryPtr>> const& trajectories);
  static void AppendMasslessBodiesStateToTrajectories(
      typename NewtonianMotionEquation::State const& state,
      std::vector<not_null<DiscreteTrajectory<Frame>*>> const& trajectories);

  
  
  NewtonianMotionEquation MakeMassiveBodiesNewtonianMotionEquation();

  Instant instance_time_locked() const ABSL_SHARED_LOCKS_REQUIRED(lock_);

  virtual Instant t_min_locked() const ABSL_SHARED_LOCKS_REQUIRED(lock_);
  virtual Instant t_max_locked() const ABSL_SHARED_LOCKS_REQUIRED(lock_);

  
  
  
  
  
  template<typename MassiveBodyConstPtr>
  static void ComputeJacobianByMassiveBodyOnMassiveBodies(
      MassiveBody const& body1,
      std::size_t b1,
      std::vector<not_null<MassiveBodyConstPtr>> const& bodies2,
      std::size_t b2_begin,
      std::size_t b2_end,
      std::vector<Position<Frame>> const& positions,
      std::vector<JacobianOfAcceleration<Frame>>& jacobians);

  
  
  
  
  
  template<typename MassiveBodyConstPtr>
  static void ComputeGravitationalJerkByMassiveBodyOnMassiveBodies(
      MassiveBody const& body1,
      std::size_t b1,
      std::vector<not_null<MassiveBodyConstPtr>> const& bodies2,
      std::size_t b2_begin,
      std::size_t b2_end,
      std::vector<DegreesOfFreedom<Frame>> const& degrees_of_freedom,
      std::vector<Vector<Jerk, Frame>>& jerks);

  
  
  
  Vector<Acceleration, Frame>
  ComputeGravitationalAccelerationOnMassiveBody(
      not_null<MassiveBody const*> body,
      std::vector<Position<Frame>> const& positions,
      Instant const& t) const;

  
  
  
  
  
  Vector<Jerk, Frame>
  ComputeGravitationalJerkOnMassiveBody(
      not_null<MassiveBody const*> body,
      std::vector<DegreesOfFreedom<Frame>> const& degrees_of_freedom) const;

  
  
  
  
  
  
  
  template<bool body1_is_oblate,
           bool body2_is_oblate,
           typename MassiveBodyConstPtr>
  static void ComputeGravitationalAccelerationByMassiveBodyOnMassiveBodies(
      Instant const& t,
      MassiveBody const& body1,
      std::size_t b1,
      std::vector<not_null<MassiveBodyConstPtr>> const& bodies2,
      std::size_t b2_begin,
      std::size_t b2_end,
      std::vector<Position<Frame>> const& positions,
      std::vector<Vector<Acceleration, Frame>>& accelerations,
      std::vector<Geopotential<Frame>> const& geopotentials);

  
  
  
  
  
  template<bool body1_is_oblate>
  std::underlying_type_t<absl::StatusCode>
  ComputeGravitationalAccelerationByMassiveBodyOnMasslessBodies(
      Instant const& t,
      MassiveBody const& body1,
      std::size_t b1,
      std::vector<Position<Frame>> const& positions,
      std::vector<Vector<Acceleration, Frame>>& accelerations) const
      ABSL_SHARED_LOCKS_REQUIRED(lock_);

  
  
  
  
  template<bool body1_is_oblate>
  void ComputeGravitationalPotentialsOfMassiveBody(
      Instant const& t,
      MassiveBody const& body1,
      std::size_t b1,
      std::vector<Position<Frame>> const& positions,
      std::vector<SpecificEnergy>& potentials) const
      ABSL_SHARED_LOCKS_REQUIRED(lock_);

  
  absl::Status ComputeGravitationalAccelerationBetweenAllMassiveBodies(
      Instant const& t,
      std::vector<Position<Frame>> const& positions,
      std::vector<Vector<Acceleration, Frame>>& accelerations) const;

  
  
  
  
  absl::StatusCode
  ComputeGravitationalAccelerationByAllMassiveBodiesOnMasslessBodies(
      Instant const& t,
      std::vector<Position<Frame>> const& positions,
      std::vector<Vector<Acceleration, Frame>>& accelerations) const
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  void ComputeGravitationalPotentialsOfAllMassiveBodies(
      Instant const& t,
      std::vector<Position<Frame>> const& positions,
      std::vector<SpecificEnergy>& potentials) const
      ABSL_LOCKS_EXCLUDED(lock_);

  
  template<typename ODE>
  absl::Status FlowODEWithAdaptiveStep(
      typename ODE::RightHandSideComputation compute_acceleration,
      not_null<DiscreteTrajectory<Frame>*> trajectory,
      Instant const& t,
      _integration_parameters::AdaptiveStepParameters<ODE> const& parameters,
      std::int64_t max_ephemeris_steps) ABSL_LOCKS_EXCLUDED(lock_);

  
  static double ToleranceToErrorRatio(
      Length const& length_integration_tolerance,
      Speed const& speed_integration_tolerance,
      Time const& current_step_size,
      typename NewtonianMotionEquation::State const& ,
      typename NewtonianMotionEquation::State::Error const& error);

  
  std::vector<not_null<MassiveBody const*>> unowned_entities_;

  
  absl::flat_hash_map<not_null<MassiveBody const*>,
                      int> unowned_entities_indices_;

  
  
  std::vector<not_null<std::unique_ptr<MassiveBody const>>> bodies_;

  
  absl::flat_hash_map<not_null<MassiveBody const*>, int> bodies_indices_;

  
  std::vector<Geopotential<Frame>> geopotentials_;

  
  std::vector<not_null<ContinuousTrajectory<Frame>*>> trajectories_;

  absl::flat_hash_map<not_null<MassiveBody const*>,
                      not_null<std::unique_ptr<ContinuousTrajectory<Frame>>>>
      bodies_to_trajectories_;

  AccuracyParameters const accuracy_parameters_;
  FixedStepParameters const fixed_step_parameters_;

  int number_of_oblate_entities_ = 0;
  int number_of_spherical_entities_ = 0;

  not_null<
      std::unique_ptr<Checkpointer<serialization::Ephemeris>>> checkpointer_;

  
  
  Instant oldest_reanimated_checkpoint_ ABSL_GUARDED_BY(lock_) = InfinitePast;

  
  Reanimator<Instant> reanimator_;

  
  
  
  
  mutable absl::Mutex lock_;

  std::unique_ptr<typename Integrator<NewtonianMotionEquation>::Instance>
      instance_ ABSL_GUARDED_BY(lock_);

  absl::Status last_severe_integration_status_ ABSL_GUARDED_BY(lock_);
};

}  

using internal::Ephemeris;

}  
}  
}  

#include "physics/ephemeris_body.hpp"
