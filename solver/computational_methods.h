#ifndef COSMOS_INTEGRATORS_INTEGRATORS_HPP_
#define COSMOS_INTEGRATORS_INTEGRATORS_HPP_

#include <functional>
#include <memory>
#include <string>

#include "absl/status/status.h"
#include "base/concepts.hpp"
#include "base/not_null.hpp"
#include "geometry/instant.hpp"
#include "integrators/ordinary_differential_equations.hpp"
#include "numerics/double_precision.hpp"
#include "quantities/quantities.hpp"
#include "serialization/integrators.pb.h"

namespace cosmos_logic {
namespace integrators {
namespace _integrators {
namespace internal {

using namespace cosmos_logic::base::_concepts;
using namespace cosmos_logic::base::_not_null;
using namespace cosmos_logic::geometry::_instant;
using namespace cosmos_logic::integrators::_ordinary_differential_equations;
using namespace cosmos_logic::numerics::_double_precision;
using namespace cosmos_logic::quantities::_quantities;


template<typename ODE_>
class Integrator {
 public:
  using ODE = ODE_;
  using IndependentVariable = typename ODE::IndependentVariable;
  using AppendState =
      std::function<void(typename ODE::State const& state)>;

  
  
  class Instance {
   public:
    Instance(InitialValueProblem<ODE> const& problem,
             AppendState const& append_state);
    virtual ~Instance() = default;

    
    
    virtual absl::Status Solve(IndependentVariable const& s_final) = 0;

    
    ODE const& equation() const;

    
    AppendState const& append_state() const;

    
    DoublePrecision<Instant> const& time() const;

    
    typename ODE::State const& state() const;
    typename ODE::State& state();

    
    virtual not_null<std::unique_ptr<Instance>> Clone() const = 0;

    
    virtual void WriteToMessage(
        not_null<serialization::IntegratorInstance*> message) const;

   protected:
    
    Instance();

    
    
    ODE const equation_;
    typename ODE::State current_state_;
    AppendState const append_state_;
  };

  virtual ~Integrator() = default;
};


template<typename ODE_>
class FixedStepSizeIntegrator : public Integrator<ODE_> {
 public:
  using ODE = ODE_;
  using typename Integrator<ODE>::AppendState;

  
  
  
  
  class Instance : public Integrator<ODE>::Instance {
   public:
    
    Time const& step() const;

    
    virtual FixedStepSizeIntegrator const& integrator() const = 0;

    void WriteToMessage(
        not_null<serialization::IntegratorInstance*> message) const override;
    static not_null<std::unique_ptr<typename Integrator<ODE>::Instance>>
    ReadFromMessage(serialization::IntegratorInstance const& message,
                    ODE const& equation,
                    AppendState const& append_state)
      requires serializable<typename ODE_::State>;

   protected:
    Instance(InitialValueProblem<ODE> const& problem,
             AppendState const& append_state,
             Time const& step);

    Time const step_;
  };

  
  
  virtual not_null<std::unique_ptr<typename Integrator<ODE>::Instance>>
  NewInstance(InitialValueProblem<ODE> const& problem,
              AppendState const& append_state,
              Time const& step) const = 0;

  virtual void WriteToMessage(
      not_null<serialization::FixedStepSizeIntegrator*> message) const = 0;
  static FixedStepSizeIntegrator const& ReadFromMessage(
      serialization::FixedStepSizeIntegrator const& message);

 protected:
  FixedStepSizeIntegrator() = default;
};

template<typename Equation>
FixedStepSizeIntegrator<Equation> const&
ParseFixedStepSizeIntegrator(std::string const& integrator_kind);


template<typename ODE_>
class AdaptiveStepSizeIntegrator : public Integrator<ODE_> {
 public:
  using ODE = ODE_;
  using IndependentVariableDifference =
      typename ODE::IndependentVariableDifference;
  using typename Integrator<ODE>::AppendState;

  
  
  
  
  
  
  
  using ToleranceToErrorRatio = std::function<double(
      IndependentVariableDifference const& current_step_size,
      typename ODE::State const& state,
      typename ODE::State::Error const& error)>;

  struct Parameters final {
    Parameters(IndependentVariableDifference const& first_step,
               double safety_factor,
               std::int64_t max_steps,
               bool last_step_is_exact);

    
    Parameters(IndependentVariableDifference const& first_step,
               double safety_factor);

    void WriteToMessage(
        not_null<serialization::AdaptiveStepSizeIntegratorInstance::
                     Parameters*> message) const;
    static Parameters ReadFromMessage(
        serialization::AdaptiveStepSizeIntegratorInstance::Parameters const&
            message);

    
    
    IndependentVariableDifference const first_step;
    
    
    double const safety_factor;
    
    
    std::int64_t const max_steps;
    
    
    
    bool const last_step_is_exact;
  };

  
  class Instance : public Integrator<ODE>::Instance {
   public:
    
    virtual AdaptiveStepSizeIntegrator const& integrator() const = 0;

    void WriteToMessage(
        not_null<serialization::IntegratorInstance*> message) const override;
    static not_null<std::unique_ptr<typename Integrator<ODE>::Instance>>
    ReadFromMessage(serialization::IntegratorInstance const& message,
                    ODE const& equation,
                    AppendState const& append_state,
                    ToleranceToErrorRatio const& tolerance_to_error_ratio)
      requires serializable<typename ODE_::State>;

   protected:
    Instance(InitialValueProblem<ODE> const& problem,
             AppendState const& append_state,
             ToleranceToErrorRatio tolerance_to_error_ratio,
             Parameters const& parameters,
             IndependentVariableDifference const& step,
             bool first_use);

    ToleranceToErrorRatio const tolerance_to_error_ratio_;
    Parameters const parameters_;
    IndependentVariableDifference step_;
    bool first_use_;
  };

  
  
  virtual not_null<std::unique_ptr<typename Integrator<ODE>::Instance>>
  NewInstance(InitialValueProblem<ODE> const& problem,
              typename Integrator<ODE>::AppendState const& append_state,
              ToleranceToErrorRatio const& tolerance_to_error_ratio,
              Parameters const& parameters) const = 0;

  virtual void WriteToMessage(
      not_null<serialization::AdaptiveStepSizeIntegrator*> message) const = 0;
  static AdaptiveStepSizeIntegrator const& ReadFromMessage(
      serialization::AdaptiveStepSizeIntegrator const& message);

 protected:
  AdaptiveStepSizeIntegrator() = default;
};

template<typename Equation>
AdaptiveStepSizeIntegrator<Equation> const& ParseAdaptiveStepSizeIntegrator(
    std::string const& integrator_kind);

}  

using internal::AdaptiveStepSizeIntegrator;
using internal::FixedStepSizeIntegrator;
using internal::Integrator;
using internal::ParseAdaptiveStepSizeIntegrator;
using internal::ParseFixedStepSizeIntegrator;

}  
}  
}  

#include "integrators/integrators_body.hpp"

#endif  
