#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>

#include "base/algebra.hpp"
#include "base/disjoint_sets.hpp"
#include "base/not_null.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/instant.hpp"
#include "geometry/space.hpp"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/identification.hpp"
#include "ksp_plugin/part_subsets.hpp"  
#include "ksp_plugin/pile_up.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/discrete_trajectory_segment_iterator.hpp"
#include "physics/rigid_motion.hpp"
#include "physics/tensors.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "serialization/ksp_plugin.pb.h"

namespace cosmos_logic {
namespace ksp_plugin {
namespace _part {
namespace internal {

using namespace cosmos_logic::base::_algebra;
using namespace cosmos_logic::base::_disjoint_sets;
using namespace cosmos_logic::base::_not_null;
using namespace cosmos_logic::geometry::_grassmann;
using namespace cosmos_logic::geometry::_instant;
using namespace cosmos_logic::geometry::_space;
using namespace cosmos_logic::ksp_plugin::_frames;
using namespace cosmos_logic::ksp_plugin::_identification;
using namespace cosmos_logic::ksp_plugin::_pile_up;
using namespace cosmos_logic::physics::_degrees_of_freedom;
using namespace cosmos_logic::physics::_discrete_trajectory;
using namespace cosmos_logic::physics::_discrete_trajectory_segment_iterator;
using namespace cosmos_logic::physics::_rigid_motion;
using namespace cosmos_logic::physics::_tensors;
using namespace cosmos_logic::quantities::_named_quantities;
using namespace cosmos_logic::quantities::_quantities;


class Part final {
 public:
  
  Part(PartId part_id,
       std::string const& name,
       Mass const& mass,
       Position<EccentricPart> const& centre_of_mass,
       InertiaTensor<RigidPart> const& inertia_tensor,
       RigidMotion<EccentricPart, Barycentric> const& rigid_motion,
       std::function<void()> deletion_callback);

  
  Part(PartId part_id,
       std::string const& name,
       DegreesOfFreedom<Barycentric> const& degrees_of_freedom,
       std::function<void()> deletion_callback);

  
  
  ~Part();

  PartId part_id() const;

  
  
  bool truthful() const;
  void make_truthful();

  
  
  
  
  void set_mass(Mass const& mass);
  Mass const& mass() const;
  void set_centre_of_mass(Position<EccentricPart> const& centre_of_mass);
  
  
  RigidMotion<RigidPart, EccentricPart> MakeRigidToEccentricMotion() const;
  void set_inertia_tensor(InertiaTensor<RigidPart> const& inertia_tensor);
  InertiaTensor<RigidPart> const& inertia_tensor() const;
  
  
  void set_is_solid_rocket_motor(bool is_solid_rocket_motor);
  bool is_solid_rocket_motor() const;

  
  Mass const& mass_change() const;

  
  
  void clear_intrinsic_force();
  void apply_intrinsic_force(
      Vector<Force, Barycentric> const& intrinsic_force);
  Vector<Force, Barycentric> const& intrinsic_force() const;

  void clear_intrinsic_torque();
  void apply_intrinsic_torque(
      Bivector<Torque, Barycentric> const& intrinsic_torque);
  Bivector<Torque, Barycentric> const& intrinsic_torque() const;

  void ApplyIntrinsicForceWithLeverArm(
      Vector<Force, Barycentric> const& intrinsic_force,
      Displacement<Barycentric> const& lever_arm);

  
  void set_rigid_motion(
      RigidMotion<RigidPart, Barycentric> const& rigid_motion);
  RigidMotion<RigidPart, Barycentric> const& rigid_motion() const;

  
  
  
  DiscreteTrajectory<Barycentric>::iterator history_begin();
  DiscreteTrajectory<Barycentric>::iterator history_end();
  DiscreteTrajectory<Barycentric>::iterator psychohistory_begin();
  DiscreteTrajectory<Barycentric>::iterator psychohistory_end();

  
  
  
  
  
  
  void AppendToHistory(
      Instant const& time,
      DegreesOfFreedom<Barycentric> const& degrees_of_freedom);
  void AppendToPsychohistory(
      Instant const& time,
      DegreesOfFreedom<Barycentric> const& degrees_of_freedom);

  
  void ClearHistory();

  
  void set_containing_pile_up(not_null<std::shared_ptr<PileUp>> const& pile_up);

  
  PileUp* containing_pile_up() const;

  
  
  bool is_piled_up() const;

  
  
  void reset_containing_pile_up();

  void WriteToMessage(not_null<serialization::Part*> message,
                      PileUp::SerializationIndexForPileUp const&
                          serialization_index_for_pile_up) const;
  static not_null<std::unique_ptr<Part>> ReadFromMessage(
      serialization::Part const& message,
      std::function<void()> deletion_callback);
  void FillContainingPileUpFromMessage(
      serialization::Part const& message,
      PileUp::PileUpForSerializationIndex const&
          pile_up_for_serialization_index);

  
  std::string ShortDebugString() const;

  
  
  
  
  static Bivector<Torque, RigidPart> DragTorqueFromAngularVelocity(
      Inverse<Time> const& angular_drag,
      Time const& Δt,
      AngularVelocity<RigidPart> const& angular_velocity,
      InertiaTensor<RigidPart> const& inertia_tensor);

 private:
  Part(PartId part_id,
       std::string name,
       bool truthful,
       Mass const& mass,
       InertiaTensor<RigidPart> const& inertia_tensor,
       RigidMotion<RigidPart, Barycentric> rigid_motion,
       std::function<void()> deletion_callback);

  static RigidMotion<RigidPart, EccentricPart> MakeRigidToEccentricMotion(
      Position<EccentricPart> const& centre_of_mass);

  PartId const part_id_;
  std::string const name_;
  bool truthful_;
  Mass mass_;
  Position<EccentricPart> centre_of_mass_ = EccentricPart::origin;
  
  
  
  
  Mass mass_change_;
  bool is_solid_rocket_motor_ = false;
  InertiaTensor<RigidPart> inertia_tensor_;
  Vector<Force, Barycentric> intrinsic_force_;
  Bivector<Torque, Barycentric> intrinsic_torque_;

  std::shared_ptr<PileUp> containing_pile_up_;

  RigidMotion<RigidPart, Barycentric> rigid_motion_;

  

  
  
  DiscreteTrajectory<Barycentric> trajectory_;

  
  
  DiscreteTrajectorySegmentIterator<Barycentric> history_;

  
  
  DiscreteTrajectorySegmentIterator<Barycentric> psychohistory_;

  
  not_null<std::unique_ptr<Subset<Part>::Node>> const subset_node_;
  friend class Subset<Part>::Node;

  
  std::function<void()> deletion_callback_;
};



InertiaTensor<RigidPart> MakeWaterSphereInertiaTensor(Mass const& mass);

std::ostream& operator<<(std::ostream& out, Part const& part);

}  

using internal::Part;
using internal::MakeWaterSphereInertiaTensor;

}  
}  

namespace base {
namespace _disjoint_sets {
namespace internal {

using namespace cosmos_logic::ksp_plugin::_part;

template<>
inline not_null<Subset<Part>::Node*> Subset<Part>::Node::Get(Part& element) {
  return element.subset_node_.get();
}

}  
}  
}  
}  
