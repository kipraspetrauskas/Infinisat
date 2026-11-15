#include "ksp_plugin/integrators.hpp"

#include <cstdint>
#include <limits>

#include "integrators/embedded_explicit_generalized_runge_kutta_nyström_integrator.hpp"
#include "integrators/embedded_explicit_runge_kutta_nyström_integrator.hpp"
#include "integrators/methods.hpp"
#include "integrators/symmetric_linear_multistep_integrator.hpp"
#include "integrators/symplectic_runge_kutta_nyström_integrator.hpp"

namespace principia {
namespace ksp_plugin {
namespace _integrators {
namespace internal {

using namespace principia::integrators::_embedded_explicit_generalized_runge_kutta_nyström_integrator;  
using namespace principia::integrators::_embedded_explicit_runge_kutta_nyström_integrator;  
using namespace principia::integrators::_methods;
using namespace principia::integrators::_symmetric_linear_multistep_integrator;
using namespace principia::integrators::_symplectic_runge_kutta_nyström_integrator;  

DiscreteTrajectorySegment<Barycentric>::DownsamplingParameters
DefaultDownsamplingParameters() {
  return DiscreteTrajectorySegment<Barycentric>::DownsamplingParameters{
      .tolerance = 10 * Metre,
  };
}

DiscreteTrajectorySegment<Barycentric>::DownsamplingParameters
OrbitAnalyserDownsamplingParameters() {
  
  return DiscreteTrajectorySegment<Barycentric>::DownsamplingParameters{
      .tolerance = 1 * Milli(Metre),
  };
}

Ephemeris<Barycentric>::AccuracyParameters
DefaultEphemerisAccuracyParameters() {
  return Ephemeris<Barycentric>::AccuracyParameters(
      1 * Milli(Metre),
       0x1.0p-24);
}

Ephemeris<Barycentric>::FixedStepParameters
DefaultEphemerisFixedStepParameters() {
  return Ephemeris<Barycentric>::FixedStepParameters(
      SymplecticRungeKuttaNyströmIntegrator<
          BlanesMoan2002SRKN14A,
          Ephemeris<Barycentric>::NewtonianMotionEquation>(),
      35 * Minute);
}

Ephemeris<Barycentric>::GeneralizedAdaptiveStepParameters
DefaultBurnParameters() {
  return Ephemeris<Barycentric>::GeneralizedAdaptiveStepParameters(
      EmbeddedExplicitGeneralizedRungeKuttaNyströmIntegrator<
          Fine1987RKNG34,
          Ephemeris<Barycentric>::GeneralizedNewtonianMotionEquation>(),
      1000,
      1 * Metre,
      1 * Metre / Second);
}

Ephemeris<Barycentric>::FixedStepParameters DefaultHistoryParameters() {
  return Ephemeris<Barycentric>::FixedStepParameters(
      SymmetricLinearMultistepIntegrator<
          Quinlan1999Order8A,
          Ephemeris<Barycentric>::NewtonianMotionEquation>(),
      10 * Second);
}

Ephemeris<Barycentric>::AdaptiveStepParameters
DefaultPsychohistoryParameters() {
  return Ephemeris<Barycentric>::AdaptiveStepParameters(
      EmbeddedExplicitRungeKuttaNyströmIntegrator<
          DormandالمكاوىPrince1986RKN434FM,
          Ephemeris<Barycentric>::NewtonianMotionEquation>(),
      std::numeric_limits<std::int64_t>::max(),
      1 * Milli(Metre),
      1 * Milli(Metre) / Second);
}

Ephemeris<Barycentric>::AdaptiveStepParameters DefaultPredictionParameters() {
  return Ephemeris<Barycentric>::AdaptiveStepParameters(
      EmbeddedExplicitRungeKuttaNyströmIntegrator<
          DormandالمكاوىPrince1986RKN434FM,
          Ephemeris<Barycentric>::NewtonianMotionEquation>(),
      1000,
      1 * Metre,
      1 * Metre / Second);
}

}  
}  
}  
}  
