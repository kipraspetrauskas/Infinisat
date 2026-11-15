#pragma once

#include <list>

#include "base/disjoint_sets.hpp"  
#include "geometry/instant.hpp"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/pile_up.hpp"
#include "physics/ephemeris.hpp"




namespace principia {

FORWARD_DECLARE(class, Part, FROM(ksp_plugin, part), INTO(base, disjoint_sets));

namespace base {
namespace _disjoint_sets {
namespace internal {

using namespace principia::geometry::_instant;
using namespace principia::ksp_plugin::_frames;
using namespace principia::ksp_plugin::_pile_up;
using namespace principia::physics::_ephemeris;







template<>
class Subset<Part>::Properties final {
  using PileUps = std::list<PileUp*>;

 public:
  
  explicit Properties(not_null<Part*> part);

  
  
  
  
  
  void MergeWith(Properties& other);

  
  
  
  
  void Ground();
  bool grounded() const;

  
  
  
  
  
  
  
  
  void Collect(
      PileUps& pile_ups,
      Instant const& t,
      Ephemeris<Barycentric>::AdaptiveStepParameters const&
          adaptive_step_parameters,
      Ephemeris<Barycentric>::FixedStepParameters const&
          fixed_step_parameters,
      not_null<Ephemeris<Barycentric>*> ephemeris);

 private:
  
  
  
  static bool SubsetsOfSamePileUp(Properties const& left,
                                  Properties const& right);
  
  
  bool EqualsExistingPileUp() const;
  
  
  
  bool SubsetOfExistingPileUp() const;
  
  
  bool StrictSubsetOfExistingPileUp() const;

  
  bool collected_ = false;
  
  
  int missing_;
  
  std::list<not_null<Part*>> parts_;
  
  bool grounded_ = false;
};

}  
}  
}  
}  
