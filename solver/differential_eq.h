#pragma once

#include <functional>
#include <vector>

#include "absl/status/status.h"
#include "base/algebra.hpp"
#include "base/not_null.hpp"
#include "geometry/direct_sum.hpp"
#include "geometry/instant.hpp"
#include "numerics/double_precision.hpp"
#include "quantities/quantities.hpp"
#include "serialization/integrators.pb.h"

namespace principia {
namespace integrators {

namespace _ordinary_differential_equations {
namespace internal {

using namespace principia::base::_algebra;
using namespace principia::base::_not_null;
using namespace principia::geometry::_direct_sum;
using namespace principia::geometry::_instant;
using namespace principia::numerics::_double_precision;
using namespace principia::quantities::_quantities;







namespace termination_condition {

constexpr absl::StatusCode Done = absl::StatusCode::kOk;


constexpr absl::StatusCode ReachedMaximalStepCount =
    absl::StatusCode::kResourceExhausted;

constexpr absl::StatusCode VanishingStepSize =
    absl::StatusCode::kFailedPrecondition;

}  



template<typename IndependentVariable_, typename... DependentVariable>
struct ExplicitFirstOrderOrdinaryDifferentialEquation final {
  static constexpr std::int64_t order = 1;
  using IndependentVariable = IndependentVariable_;
  using IndependentVariableDifference = Difference<IndependentVariable>;
  using DependentVariables = DirectSum<DependentVariable...>;
  using DependentVariableDifferences =
      DirectSum<Difference<DependentVariable>...>;
  using DependentVariableDerivatives = DirectSum<
      Derivative<DependentVariable, IndependentVariable>...>;

  
  using RightHandSideComputation =
      std::function<absl::Status(IndependentVariable const& s,
                                 DependentVariables const& y,
                                 DependentVariableDerivatives& yʹ)>;

  struct State final {
    using Error = DependentVariableDifferences;

    State() = default;
    State(IndependentVariable const& s, DependentVariables const& y);

    DoublePrecision<IndependentVariable> s;
    DirectSum<DoublePrecision<DependentVariable>...> y;

    friend bool operator==(State const& lhs, State const& rhs) = default;
    friend bool operator!=(State const& lhs, State const& rhs) = default;

    void WriteToMessage(not_null<serialization::State*> message) const;
    static State ReadFromMessage(serialization::State const& message);
  };

  RightHandSideComputation compute_derivative;
};




template<typename... DependentVariable>
struct DecomposableFirstOrderDifferentialEquation final {
  static constexpr std::int64_t order = 1;
  using IndependentVariable = Instant;
  using IndependentVariableDifference = Time;
  using DependentVariables = DirectSum<std::vector<DependentVariable>...>;
  using DependentVariableDifferences =
      DirectSum<std::vector<Difference<DependentVariable>>...>;

  using Flow =
      std::function<absl::Status(IndependentVariable const& t_initial,
                                 IndependentVariable const& t_final,
                                 DependentVariables const& initial_state,
                                 DependentVariables& final_state)>;

  struct State final {
    using Error = DependentVariableDifferences;

    State() = default;
    State(IndependentVariable const& t, DependentVariables const& y);

    DoublePrecision<Instant> time;
    DirectSum<std::vector<DoublePrecision<DependentVariable>>...> y;

    friend bool operator==(State const& lhs, State const& rhs) = default;
    friend bool operator!=(State const& lhs, State const& rhs) = default;
  };

  
  
  
  
  Flow left_flow;
  Flow right_flow;
};



template<typename DependentVariable_>
struct ExplicitSecondOrderOrdinaryDifferentialEquation final {
  static constexpr std::int64_t order = 2;
  using IndependentVariable = Instant;
  using IndependentVariableDifference = Time;
  using DependentVariable = DependentVariable_;
  
  using DependentVariableDifference = Difference<DependentVariable>;
  
  using DependentVariableDerivative =
      Derivative<DependentVariable, IndependentVariable>;
  
  using DependentVariableDerivative2 =
      Derivative<DependentVariable, IndependentVariable, 2>;
  using DependentVariables = std::vector<DependentVariable>;
  using DependentVariableDifferences = std::vector<DependentVariableDifference>;
  using DependentVariableDerivatives = std::vector<DependentVariableDerivative>;
  using DependentVariableDerivatives2 =
      std::vector<DependentVariableDerivative2>;

  
  
  
  
  using RightHandSideComputation =
      std::function<absl::Status(IndependentVariable const& t,
                                 DependentVariables const& positions,
                                 DependentVariableDerivatives const& velocities,
                                 DependentVariableDerivatives2& accelerations)>;

  struct State final {
    struct Error final {
      DependentVariableDifferences position_error;
      DependentVariableDerivatives velocity_error;
    };

    State() = default;
    State(IndependentVariable const& t,
          DependentVariables const& q,
          DependentVariableDerivatives const& v);

    DoublePrecision<IndependentVariable> time;
    std::vector<DoublePrecision<DependentVariable>> positions;
    std::vector<
        DoublePrecision<Derivative<DependentVariable, IndependentVariable>>>
        velocities;

    friend bool operator==(State const& lhs, State const& rhs) = default;
    friend bool operator!=(State const& lhs, State const& rhs) = default;

    void WriteToMessage(not_null<serialization::State*> message) const;
    static State ReadFromMessage(serialization::State const& message);
  };

  RightHandSideComputation compute_acceleration;
};



template<typename DependentVariable_>
struct SpecialSecondOrderDifferentialEquation final {
  static constexpr std::int64_t order = 2;
  using IndependentVariable = Instant;
  using IndependentVariableDifference = Time;
  using DependentVariable = DependentVariable_;
  
  using DependentVariableDifference = Difference<DependentVariable>;
  
  using DependentVariableDerivative =
      Derivative<DependentVariable, IndependentVariable>;
  
  using DependentVariableDerivative2 =
      Derivative<DependentVariable, IndependentVariable, 2>;
  using DependentVariables = std::vector<DependentVariable>;
  using DependentVariableDifferences = std::vector<DependentVariableDifference>;
  using DependentVariableDerivatives2 =
      std::vector<DependentVariableDerivative2>;

  using RightHandSideComputation =
      std::function<absl::Status(IndependentVariable const& t,
                                 DependentVariables const& positions,
                                 DependentVariableDerivatives2& accelerations)>;

  using State = typename ExplicitSecondOrderOrdinaryDifferentialEquation<
      DependentVariable>::State;

  
  
  
  
  RightHandSideComputation compute_acceleration;
};


template<typename ODE>
struct InitialValueProblem final {
  ODE equation;
  typename ODE::State initial_state;
};

}  

using internal::DecomposableFirstOrderDifferentialEquation;
using internal::ExplicitFirstOrderOrdinaryDifferentialEquation;
using internal::ExplicitSecondOrderOrdinaryDifferentialEquation;
using internal::InitialValueProblem;
using internal::SpecialSecondOrderDifferentialEquation;
namespace termination_condition = internal::termination_condition;

}  
}  
}  

#include "integrators/ordinary_differential_equations_body.hpp"
