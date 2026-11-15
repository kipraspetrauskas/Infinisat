#include "CelestialEntity.h"
#include "CosmosWorld.h"

ACelestialEntity::ACelestialEntity()
{
	PrimaryActorTick.bCanEverTick = true;
	CelestialType = ECelestialType::Planet;
	Mass = 1.0f;
	Radius = 1.0f;
	Velocity = FVector::ZeroVector;
	OrbitalElements = FVector(1.0f, 0.0f, 0.0f);
}

void ACelestialEntity::BeginPlay()
{
	Super::BeginPlay();
}

void ACelestialEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateOrbit(DeltaTime);
}

void ACelestialEntity::UpdateOrbit(float DeltaTime)
{
	// Apply gravitational forces from other celestial bodies
	FVector TotalForce = FVector::ZeroVector;
	
	if (ACosmosWorld* CosmosWorld = Cast<ACosmosWorld>(GetWorld()))
	{
		for (ACelestialEntity* Other : CosmosWorld->CelestialEntities)
	{
		for (ACelestialEntity* Other : World->CelestialEntities)
		{
			if (Other && Other != this)
			{
				TotalForce += CalculateGravitationalForce(Other);
			}
		}
	}

	// Update velocity and position
	FVector Acceleration = TotalForce / Mass;
	Velocity += Acceleration * DeltaTime;
	AddActorLocalOffset(Velocity * DeltaTime, true);
}

FVector ACelestialEntity::CalculateGravitationalForce(const ACelestialEntity* Other)
{
	if (!Other || Other == this)
		return FVector::ZeroVector;

	FVector Direction = Other->GetActorLocation() - GetActorLocation();
	float Distance = Direction.Size();
	
	if (Distance < 1.0f) // Prevent division by zero
		return FVector::ZeroVector;

	Direction.Normalize();
	
	// Gravitational constant (scaled for game)
	const float G = 6.674e-11f * 1e20f; 
	float ForceMagnitude = G * Mass * Other->Mass / (Distance * Distance);
	
	return Direction * ForceMagnitude;
}