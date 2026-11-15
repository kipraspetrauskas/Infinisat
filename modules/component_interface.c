#include "ksp_plugin/interface.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

#include "absl/log/check.h"
#include "absl/log/die_if_null.h"
#include "absl/log/log.h"
#include "base/algebra.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/r3x3_matrix.hpp"
#include "journal/method.hpp"
#include "journal/profiles.hpp"  
#include "ksp_plugin/identification.hpp"
#include "ksp_plugin/part.hpp"
#include "physics/tensors.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/si.hpp"

namespace cosmos_logic {
namespace interface {

using namespace cosmos_logic::base::_algebra;
using namespace cosmos_logic::geometry::_grassmann;
using namespace cosmos_logic::geometry::_r3x3_matrix;
using namespace cosmos_logic::journal::_method;
using namespace cosmos_logic::ksp_plugin::_identification;
using namespace cosmos_logic::ksp_plugin::_part;
using namespace cosmos_logic::physics::_tensors;
using namespace cosmos_logic::quantities::_named_quantities;
using namespace cosmos_logic::quantities::_si;

XYZ __cdecl cosmos_logic__PartAngularMomentumFromAngularVelocity(
    XYZ world_angular_velocity,
    XYZ moments_of_inertia_in_tonnes,
    WXYZ principal_axes_rotation,
    WXYZ part_rotation) {
  journal::Method<journal::PartAngularMomentumFromAngularVelocity> m(
      {world_angular_velocity,
       moments_of_inertia_in_tonnes,
       principal_axes_rotation,
       part_rotation});
  auto const angular_velocity =
      FromXYZ<AngularVelocity<World>>(world_angular_velocity);
  Rotation<RigidPart, World> const part_to_world(FromWXYZ(part_rotation));
  InertiaTensor<World> const inertia_tensor =
      part_to_world(FromMomentsOfInertia(moments_of_inertia_in_tonnes,
                                         principal_axes_rotation));

  Bivector<AngularMomentum, World> const angular_momentum =
      inertia_tensor * angular_velocity;

  return m.Return(ToXYZ(angular_momentum));
}

void __cdecl cosmos_logic__PartApplyIntrinsicForce(
    Plugin* const plugin,
    PartId const part_id,
    XYZ const force_in_kilonewtons) {
  journal::Method<journal::PartApplyIntrinsicForce> m(
      {plugin, part_id, force_in_kilonewtons});
  ABSL_DIE_IF_NULL(plugin)->ApplyPartIntrinsicForce(
      part_id,
      Vector<Force, World>(FromXYZ(force_in_kilonewtons) * Kilo(Newton)));
  return m.Return();
}

void __cdecl cosmos_logic__PartApplyIntrinsicForceAtPosition(
    Plugin* const plugin,
    PartId const part_id,
    XYZ const force_in_kilonewtons,
    XYZ const lever_arm) {
  journal::Method<journal::PartApplyIntrinsicForceAtPosition> m(
      {plugin,
       part_id,
       force_in_kilonewtons,
       lever_arm});
  ABSL_DIE_IF_NULL(plugin)->ApplyPartIntrinsicForceAtPosition(
      part_id,
      Vector<Force, World>(FromXYZ(force_in_kilonewtons) * Kilo(Newton)),
      Displacement<World>(FromXYZ(lever_arm) * Metre));
  return m.Return();
}

void __cdecl cosmos_logic__PartApplyIntrinsicTorque(
    Plugin* const plugin,
    PartId const part_id,
    XYZ const torque_in_kilonewton_metre) {
  journal::Method<journal::PartApplyIntrinsicTorque> m(
      {plugin, part_id, torque_in_kilonewton_metre});
  ABSL_DIE_IF_NULL(plugin)->ApplyPartIntrinsicTorque(
      part_id,
      Bivector<Torque, World>(FromXYZ(torque_in_kilonewton_metre) *
                              Kilo(Newton) * Metre * Radian));
  return m.Return();
}

XYZ __cdecl cosmos_logic__PartDragTorqueFromAngularVelocity(
    double const angular_drag_per_second,
    double const delta_t,
    XYZ const world_angular_velocity,
    XYZ const moments_of_inertia_in_tonnes,
    WXYZ const principal_axes_rotation,
    WXYZ const part_rotation) {
  journal::Method<journal::PartDragTorqueFromAngularVelocity> m(
      {angular_drag_per_second,
       delta_t,
       world_angular_velocity,
       moments_of_inertia_in_tonnes,
       principal_axes_rotation,
       part_rotation});
  Time const Δt = delta_t * Second;
  
  
  Inverse<Time> const angular_drag = angular_drag_per_second / Second;

  Rotation<RigidPart, World> const part_to_world(FromWXYZ(part_rotation));
  Rotation<World, RigidPart> const world_to_part = part_to_world.Inverse();

  AngularVelocity<RigidPart> const angular_velocity =
      world_to_part(FromXYZ<AngularVelocity<World>>(world_angular_velocity));
  InertiaTensor<RigidPart> const inertia_tensor = FromMomentsOfInertia(
      moments_of_inertia_in_tonnes, principal_axes_rotation);

  return m.Return(ToXYZ(part_to_world(Part::DragTorqueFromAngularVelocity(
      angular_drag, Δt, angular_velocity, inertia_tensor))));
}

QPRW __cdecl cosmos_logic__PartGetActualRigidMotion(
    Plugin const* const plugin,
    PartId const part_id,
    Origin const origin) {
  journal::Method<journal::PartGetActualRigidMotion> m(
      {plugin, part_id, origin});
  CHECK(plugin != nullptr);
  RigidMotion<EccentricPart, World> const part_motion =
      plugin->GetPartActualMotion(
          part_id,
          plugin->BarycentricToWorld(
              origin.reference_part_is_unmoving,
              origin.reference_part_id,
              origin.reference_part_is_at_origin
                  ? std::nullopt
                  : std::make_optional(FromXYZ<Position<World>>(
                        origin.main_body_centre_in_world))));
  DegreesOfFreedom<World> const part_dof =
      part_motion({EccentricPart::origin, EccentricPart::unmoving});
  Rotation<EccentricPart, World> const part_orientation =
      part_motion.orthogonal_map().AsRotation();
  AngularVelocity<World> const part_angular_velocity =
      part_motion.angular_velocity_of<EccentricPart>();
  return m.Return(
      {ToQP(part_dof),
       ToWXYZ(part_orientation.quaternion()),
       ToXYZ(part_angular_velocity.coordinates() / (Radian / Second))});
}

bool __cdecl cosmos_logic__PartIsTruthful(
    Plugin const* const plugin,
    uint32_t const part_id) {
  journal::Method<journal::PartIsTruthful> m({plugin, part_id});
  CHECK(plugin != nullptr);
  return m.Return(plugin->PartIsTruthful(part_id));
}

void __cdecl cosmos_logic__PartSetApparentRigidMotion(
    Plugin* const plugin,
    PartId const part_id,
    QP const degrees_of_freedom,
    WXYZ const rotation,
    XYZ const angular_velocity) {
  journal::Method<journal::PartSetApparentRigidMotion> m(
      {plugin,
       part_id,
       degrees_of_freedom,
       rotation,
       angular_velocity});
  CHECK(plugin != nullptr);
  plugin->SetPartApparentRigidMotion(
      part_id,
      MakePartApparentRigidMotion(
          degrees_of_freedom, rotation, angular_velocity));
  return m.Return();
}

}  
}  
