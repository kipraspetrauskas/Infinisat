#pragma once

#include "CoreMinimal.h"

class COSMOSLOGIC_API FMotionSolver
{
public:
	// Runge-Kutta 4th order integration
	static FVector RK4Integrate(const FVector& Position, const FVector& Velocity, 
	                           float DeltaTime, const TFunction<FVector(const FVector&)>& AccelerationFunc);

	// Verlet integration for better energy conservation
	static FVector VerletIntegrate(const FVector& OldPosition, const FVector& CurrentPosition,
	                              float DeltaTime, const TFunction<FVector(const FVector&)>& AccelerationFunc);

	// N-body gravitational force calculation
	static FVector CalculateNBodyForce(const FVector& Position, float Mass, 
	                                   const TArray<TPair<FVector, float>>& Bodies);

	// Orbital mechanics helpers
	static FVector CalculateOrbitalVelocity(float SemiMajorAxis, float Eccentricity, float GM);
	static FVector CalculateEscapeVelocity(float Distance, float CentralMass);
};