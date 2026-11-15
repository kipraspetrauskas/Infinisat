

#ifndef PRINCIPIA_PHYSICS_BODY_HPP_
#include "physics/body.hpp"
#endif  
#ifndef PRINCIPIA_PHYSICS_MASSIVE_BODY_HPP_
#define PRINCIPIA_PHYSICS_MASSIVE_BODY_HPP_

#include <memory>
#include <string>

#include "base/not_null.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace physics {
namespace _massive_body {
namespace internal {

using namespace principia::base::_not_null;
using namespace principia::physics::_body;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;

class MassiveBody : public Body {
 public:
  
  
  
  class Parameters final {
   public:
    
    Parameters(
        GravitationalParameter const& gravitational_parameter);  
    Parameters(std::string name,
               GravitationalParameter const& gravitational_parameter);
    Parameters(Mass const& mass);  
    Parameters(std::string name,
               Mass const& mass);

    
    GravitationalParameter const& gravitational_parameter() const;

   private:
    std::string const name_;
    GravitationalParameter const gravitational_parameter_;
    Mass const mass_;
    friend class MassiveBody;
  };

  explicit MassiveBody(Parameters parameters);

  
  std::string const& name() const;
  GravitationalParameter const& gravitational_parameter() const;
  Mass const& mass() const;

  
  virtual Length min_radius() const;
  virtual Length mean_radius() const;
  virtual Length max_radius() const;

  
  bool is_massless() const override;

  
  bool is_oblate() const override;

  
  
  void WriteToMessage(not_null<serialization::Body*> message) const override;

  
  
  virtual void WriteToMessage(
      not_null<serialization::MassiveBody*> message) const;

  
  static not_null<std::unique_ptr<MassiveBody>> ReadFromMessage(
      serialization::Body const& message);

  static not_null<std::unique_ptr<MassiveBody>> ReadFromMessage(
      serialization::MassiveBody const& message);

 private:
  Parameters const parameters_;
};

}  

using internal::MassiveBody;

}  
}  
}  

#include "physics/massive_body_body.hpp"

#endif  
