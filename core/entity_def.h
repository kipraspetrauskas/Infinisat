#ifndef PRINCIPIA_PHYSICS_BODY_HPP_
#define PRINCIPIA_PHYSICS_BODY_HPP_

#include <memory>

#include "base/not_constructible.hpp"
#include "base/not_null.hpp"
#include "serialization/physics.pb.h"

namespace principia {
namespace physics {
namespace _body {
namespace internal {

using namespace principia::base::_not_constructible;
using namespace principia::base::_not_null;

class Body {
 public:
  virtual ~Body() = default;

  
  virtual bool is_massless() const = 0;

  
  virtual bool is_oblate() const = 0;

  
  
  
  template<typename Frame>
  bool is_compatible_with() const;

  virtual void WriteToMessage(not_null<serialization::Body*> message) const = 0;

  
  
  static not_null<std::unique_ptr<Body>> ReadFromMessage(
      serialization::Body const& message);

 protected:
  Body() = default;

 private:
  
  
  
  template<typename Frame, bool is_inertial>
  struct CompatibilityHelper : not_constructible {
     static bool is_compatible_with(not_null<Body const*> body);
  };
};

}  

using internal::Body;

}  
}  
}  

#include "physics/body_body.hpp"

#endif  
