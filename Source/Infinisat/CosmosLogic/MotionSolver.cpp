#include "MotionSolver.h"
#include "Math/UnrealMathUtility.h"

FVector FMotionSolver::RK4Integrate(const FVector& Position, const FVector& Velocity, 
                                   float DeltaTime, const TFunction<FVector(const FVector&)>& AccelerationFunc)
{
	// k1
	FVector k1_v = AccelerationFunc(Position) * DeltaTime;
	FVector k1_p = Velocity * DeltaTime;

	// k2
	FVector k2_v = AccelerationFunc(Position + k1_p * 0.5f) * DeltaTime;
	FVector k2_p = (Velocity + k1_v * 0.5f) * DeltaTime;

	// k3
	FVector k3_v = AccelerationFunc(Position + k2_p * 0.5f) * DeltaTime;
	FVector k3_p = (Velocity + k2_v * 0.5f) * DeltaTime;

	// k4
	FVector k4_v = AccelerationFunc(Position + k3_p) * DeltaTime;
	FVector k4_p = (Velocity + k3_v) * DeltaTime;

	// New position
	return Position + (k1_p + 2.0f * k2_p + 2.0f * k3_p + k4_p) / 6.0f;
}

FVector FMotionSolver::VerletIntegrate(const FVector& OldPosition, const FVector& CurrentPosition,
                                      float DeltaTime, const TFunction<FVector(const FVector&)>& AccelerationFunc)
{
	FVector Acceleration = AccelerationFunc(CurrentPosition);
	FVector NewPosition = 2.0f * CurrentPosition - OldPosition + Acceleration * DeltaTime * DeltaTime;
	return NewPosition;
}

FVector FMotionSolver::CalculateNBodyForce(const FVector& Position, float Mass, 
                                          const TArray<TPair<FVector, float>>& Bodies)
{
	FVector TotalForce = FVector::ZeroVector;
	const float G = 6.674e-11f * 1e20f; // Scaled gravitational constant

	for (const auto& Body : Bodies)
	{
		FVector Direction = Body.Key - Position;
		float Distance = Direction.Size();

		if (Distance < 1.0f)
			continue;

		Direction.Normalize();
		float ForceMagnitude = G * Mass * Body.Value / (Distance * Distance);
		TotalForce += Direction * ForceMagnitude;
	}

	return TotalForce;
}

FVector FMotionSolver::CalculateOrbitalVelocity(float SemiMajorAxis, float Eccentricity, float GM)
{
	// Vis-viva equation: v^2 = GM(2/r - 1/a)
	float r = SemiMajorAxis * (1.0f - Eccentricity); // Perihelion distance
	float vSquared = GM * (2.0f / r - 1.0f / SemiMajorAxis);
	float Velocity = FMath::Sqrt(FMath::Max(0.0f, vSquared));

	return FVector(Velocity, 0.0f, 0.0f);
}

FVector FMotionSolver::CalculateEscapeVelocity(float Distance, float CentralMass)
{
	// Escape velocity: v = sqrt(2GM/r)
	const float G = 6.674e-11f * 1e20f;
	float vSquared = 2.0f * G * CentralMass / Distance;
	float Velocity = FMath::Sqrt(FMath::Max(0.0f, vSquared));

	return FVector(Velocity, 0.0f, 0.0f);
}