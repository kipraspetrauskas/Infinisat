#pragma once

#include <memory>
#include <queue>
#include <string>
#include <variant>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "base/not_null.hpp"
#include "base/reanimator.hpp"
#include "base/recurring_thread.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/instant.hpp"
#include "geometry/interval.hpp"
#include "ksp_plugin/celestial.hpp"
#include "ksp_plugin/flight_plan.hpp"
#include "ksp_plugin/flight_plan_optimization_driver.hpp"
#include "ksp_plugin/flight_plan_optimizer.hpp"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/identification.hpp"
#include "ksp_plugin/manœuvre.hpp"
#include "ksp_plugin/orbit_analyser.hpp"
#include "ksp_plugin/part.hpp"
#include "ksp_plugin/pile_up.hpp"
#include "physics/checkpointer.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/discrete_trajectory_segment.hpp"
#include "physics/discrete_trajectory_segment_iterator.hpp"
#include "physics/ephemeris.hpp"
#include "physics/massless_body.hpp"
#include "physics/rotating_body.hpp"
#include "quantities/quantities.hpp"
#include "serialization/ksp_plugin.pb.h"

namespace principia {
namespace ksp_plugin {

class VesselTest;

namespace _vessel {
namespace internal {

using namespace principia::base::_not_null;
using namespace principia::base::_reanimator;
using namespace principia::base::_recurring_thread;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_instant;
using namespace principia::geometry::_interval;
using namespace principia::ksp_plugin::_celestial;
using namespace principia::ksp_plugin::_flight_plan;
using namespace principia::ksp_plugin::_flight_plan_optimization_driver;
using namespace principia::ksp_plugin::_flight_plan_optimizer;
using namespace principia::ksp_plugin::_frames;
using namespace principia::ksp_plugin::_identification;
using namespace principia::ksp_plugin::_manœuvre;
using namespace principia::ksp_plugin::_orbit_analyser;
using namespace principia::ksp_plugin::_part;
using namespace principia::ksp_plugin::_pile_up;
using namespace principia::physics::_checkpointer;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_discrete_trajectory;
using namespace principia::physics::_discrete_trajectory_segment;
using namespace principia::physics::_discrete_trajectory_segment_iterator;
using namespace principia::physics::_ephemeris;
using namespace principia::physics::_massless_body;
using namespace principia::physics::_rotating_body;
using namespace principia::quantities::_quantities;


class Vessel {
 public:
  using Manœuvres = std::vector<
      not_null<std::unique_ptr<Manœuvre<Barycentric, Navigation> const>>>;

  
  Vessel(GUID guid,
         std::string name,
         not_null<Celestial const*> parent,
         not_null<Ephemeris<Barycentric>*> ephemeris,
         Ephemeris<Barycentric>::AdaptiveStepParameters
             prediction_adaptive_step_parameters,
         DiscreteTrajectorySegment<Barycentric>::DownsamplingParameters const&
             downsampling_parameters);

  Vessel(Vessel const&) = delete;
  Vessel(Vessel&&) = delete;
  Vessel& operator=(Vessel const&) = delete;
  Vessel& operator=(Vessel&&) = delete;

  virtual ~Vessel();

  
  virtual GUID const& guid() const;

  
  virtual std::string const& name() const;
  
  virtual void set_name(std::string const& new_name);

  
  virtual not_null<MasslessBody const*> body() const;

  virtual not_null<Celestial const*> parent() const;
  virtual void set_parent(not_null<Celestial const*> parent);

  
  
  
  virtual void AddPart(not_null<std::unique_ptr<Part>> part);
  
  
  
  virtual not_null<std::unique_ptr<Part>> ExtractPart(PartId id);
  
  
  virtual void KeepPart(PartId id);
  
  
  virtual bool WillKeepPart(PartId id) const;
  
  
  
  
  virtual void FreeParts();

  
  virtual void ClearAllIntrinsicForcesAndTorques();

  
  
  
  virtual void DetectCollapsibilityChange();

  
  
  
  
  virtual void CreateTrajectoryIfNeeded(Instant const& t);

  
  
  
  virtual void DisableDownsampling();

  
  
  virtual not_null<Part*> part(PartId id) const;

  
  virtual void ForSomePart(std::function<void(Part&)> const& action) const;
  
  virtual void ForAllParts(std::function<void(Part&)> const& action) const;

  virtual DiscreteTrajectory<Barycentric> const& trajectory() const;
  virtual DiscreteTrajectorySegmentIterator<Barycentric> psychohistory() const;
  virtual DiscreteTrajectorySegmentIterator<Barycentric> prediction() const;

  virtual void set_prediction_adaptive_step_parameters(
      Ephemeris<Barycentric>::AdaptiveStepParameters const&
          prediction_adaptive_step_parameters);
  virtual Ephemeris<Barycentric>::AdaptiveStepParameters const&
  prediction_adaptive_step_parameters() const;

  
  
  virtual bool has_flight_plan() const;

  
  virtual int flight_plan_count() const;

  
  
  virtual int selected_flight_plan_index() const;

  
  
  virtual void SelectFlightPlan(int index);

  
  
  virtual FlightPlan& flight_plan() const;

  
  
  
  
  virtual void MakeFlightPlanOptimizationDriver(
      FlightPlanOptimizer::MetricFactory metric_factory);

  
  virtual void StartFlightPlanOptimizationDriver(
      FlightPlanOptimizationDriver::Parameters const& parameters);

  
  
  virtual std::optional<FlightPlanOptimizationDriver::Parameters>
  FlightPlanOptimizationDriverInProgress() const;

  virtual bool UpdateFlightPlanFromOptimization();

  
  
  
  
  void ReadFlightPlanFromMessage();

  
  
  
  virtual void AdvanceTime();

  
  
  
  
  void RequestReanimation(Instant const& desired_t_min,
                          bool quiet = false) ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  
  
  void AwaitReanimation(Instant const& desired_t_min,
                        bool quiet = false) ABSL_LOCKS_EXCLUDED(lock_);

  
  
  virtual void CreateFlightPlan(
      Instant const& final_time,
      Mass const& initial_mass,
      Ephemeris<Barycentric>::AdaptiveStepParameters const&
          flight_plan_adaptive_step_parameters,
      Ephemeris<Barycentric>::GeneralizedAdaptiveStepParameters const&
          flight_plan_generalized_adaptive_step_parameters);

  
  
  
  
  
  
  
  virtual void DuplicateFlightPlan();

  
  
  virtual void DeleteFlightPlan();

  
  
  
  
  
  
  
  
  
  absl::Status RebaseFlightPlan(Mass const& initial_mass);

  
  
  
  virtual void RefreshPrediction();

  
  
  virtual void RefreshPrediction(Instant const& time);

  
  void StopPrognosticator();

  
  
  void RequestOrbitAnalysis(Time const& mission_duration);

  
  void ClearOrbitAnalyser();

  
  
  double progress_of_orbit_analysis() const;

  
  
  void RefreshOrbitAnalysis();

  
  OrbitAnalyser::Analysis* orbit_analysis();

  
  std::string ShortDebugString() const;

  
  struct PlottingFramePayload {
    Interval<Instant> plottable_time_interval{.min = InfinitePast,
                                              .max = InfiniteFuture};
    friend bool operator==(PlottingFramePayload const&,
                           PlottingFramePayload const&) = default;
  };

  void ClearPayload(not_null<PlottingFrame const*> frame);
  void SetPayload(not_null<std::unique_ptr<PlottingFrame const>> frame,
                  PlottingFramePayload payload);
  
  
  PlottingFramePayload GetPayload(not_null<PlottingFrame const*> frame) const;

  
  virtual void WriteToMessage(not_null<serialization::Vessel*> message,
                              PileUp::SerializationIndexForPileUp const&
                                  serialization_index_for_pile_up) const;
  static not_null<std::unique_ptr<Vessel>> ReadFromMessage(
      serialization::Vessel const& message,
      not_null<Celestial const*> parent,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void(PartId)> const& deletion_callback,
      std::function<void(bool will_be_slow)> expected_performance_callback =
          nullptr);
  void FillContainingPileUpsFromMessage(
      serialization::Vessel const& message,
      PileUp::PileUpForSerializationIndex const&
          pile_up_for_serialization_index);

  static void MakeAsynchronous();
  static void MakeSynchronous();

  static bool disallow_leibniz_conversion_for_testing_;

 protected:
  
  Vessel();

 private:
  struct PrognosticatorParameters {
    Instant first_time;
    DegreesOfFreedom<Barycentric> first_degrees_of_freedom;
    Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters;
  };
  friend bool operator!=(PrognosticatorParameters const& left,
                         PrognosticatorParameters const& right);

  struct ReanimatorParameters {
    bool quiet;
  };

  using TrajectoryIterator =
      DiscreteTrajectory<Barycentric>::iterator (Part::*)();

  
  
  
  struct OptimizableFlightPlan {
    not_null<std::shared_ptr<FlightPlan>> flight_plan;
    std::unique_ptr<FlightPlanOptimizationDriver> optimization_driver;
  };

  using LazilyDeserializedFlightPlan =
      std::variant<OptimizableFlightPlan, serialization::FlightPlan>;

  
  
  Checkpointer<serialization::Vessel>::Writer MakeCheckpointerWriter(
      std::function<Ephemeris<Barycentric>::FixedStepParameters()> const&
          fixed_step_parameters);

  Checkpointer<serialization::Vessel>::Writer
  MakeCheckpointerWriterFromCheckpoint(
      serialization::Vessel::Checkpoint const& checkpoint);

  Checkpointer<serialization::Vessel>::Writer
  MakeCheckpointerWriterFromPileUp();

  Checkpointer<serialization::Vessel>::Reader
  static MakeCheckpointerReader();

  absl::Status Reanimate(Instant const& desired_t_min,
                         ReanimatorParameters const& reanimator_parameters)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  
  absl::StatusOr<Instant> ReanimateOneCheckpoint(
      serialization::Vessel::Checkpoint const& message,
      Instant const& t_initial,
      Instant const& t_final,
      bool quiet) ABSL_LOCKS_EXCLUDED(lock_);

  
  
  
  void MergeReanimatedTrajectories() ABSL_LOCKS_EXCLUDED(lock_);

  bool DesiredTMinReachedOrFullyReanimated(Instant const& desired_t_min)
      ABSL_LOCKS_EXCLUDED(lock_);

  
  
  absl::StatusOr<DiscreteTrajectory<Barycentric>>
  FlowPrognostication(PrognosticatorParameters prognosticator_parameters);

  
  
  
  void AppendToVesselTrajectory(
      TrajectoryIterator part_trajectory_begin,
      TrajectoryIterator part_trajectory_end,
      DiscreteTrajectorySegment<Barycentric> const& segment);

  
  
  void AttachPrediction(DiscreteTrajectory<Barycentric>&& trajectory);

  
  
  bool IsCollapsible() const;

  
  
  void EnactCollapsibilityChange(bool will_be_collapsible);

  
  bool has_deserialized_flight_plan() const;

  LazilyDeserializedFlightPlan& selected_flight_plan();
  LazilyDeserializedFlightPlan const& selected_flight_plan() const;

  GUID const guid_;
  std::string name_;

  MasslessBody const body_;
  Ephemeris<Barycentric>::AdaptiveStepParameters
      prediction_adaptive_step_parameters_;
  
  not_null<Celestial const*> parent_;
  not_null<Ephemeris<Barycentric>*> const ephemeris_;
  std::optional<DiscreteTrajectorySegment<Barycentric>::DownsamplingParameters>
      downsampling_parameters_;

  mutable absl::Mutex lock_;

  
  
  bool is_collapsible_ = false;

  
  
  absl::btree_map<PartId, not_null<std::unique_ptr<Part>>> parts_;
  absl::flat_hash_set<PartId> kept_parts_;

  
  
  
  
  DiscreteTrajectory<Barycentric> trajectory_;

  not_null<std::unique_ptr<Checkpointer<serialization::Vessel>>> checkpointer_;

  
  
  Instant oldest_reanimated_checkpoint_ ABSL_GUARDED_BY(lock_) = InfinitePast;

  
  
  Reanimator<Instant, ReanimatorParameters> reanimator_;

  
  
  
  
  
  std::queue<DiscreteTrajectory<Barycentric>> reanimated_trajectories_
      ABSL_GUARDED_BY(lock_);

  
  
  
  DiscreteTrajectorySegmentIterator<Barycentric> backstory_;

  
  
  DiscreteTrajectorySegmentIterator<Barycentric> psychohistory_;
  DiscreteTrajectorySegmentIterator<Barycentric> prediction_;

  RecurringThread<PrognosticatorParameters,
                  DiscreteTrajectory<Barycentric>> prognosticator_;

  std::vector<LazilyDeserializedFlightPlan> flight_plans_;
  int selected_flight_plan_index_ = -1;

  std::optional<OrbitAnalyser> orbit_analyser_;

  static std::atomic_bool synchronous_;

  absl::flat_hash_map<PlottingFrameKey, PlottingFramePayload> payloads_;

  friend class ksp_plugin::VesselTest;
};

}  

using internal::Vessel;

}  
}  
}  
