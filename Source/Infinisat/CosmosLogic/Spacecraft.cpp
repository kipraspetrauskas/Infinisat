#include "Spacecraft.h"
#include "CosmosWorld.h"
#include "CelestialEntity.h"

ASpacecraft::ASpacecraft()
{
	PrimaryActorTick.bCanEverTick = true;
	SpacecraftType = ESpacecraftType::Probe;
	Mass = 1.0f;
	Velocity = FVector::ZeroVector;
}

void ASpacecraft::BeginPlay()
{
	Super::BeginPlay();
}

void ASpacecraft::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTrajectory(DeltaTime);
}

void ASpacecraft::UpdateTrajectory(float DeltaTime)
{
	// Calculate gravitational forces from all celestial bodies
	FVector TotalForce = CalculateGravitationalForces();
	
	// Update velocity and position
	FVector Acceleration = TotalForce / Mass;
	Velocity += Acceleration * DeltaTime;
	AddActorLocalOffset(Velocity * DeltaTime, true);
}

void ASpacecraft::ApplyThrust(FVector Direction, float Force)
{
	Direction.Normalize();
	Velocity += Direction * (Force / Mass);
}

void ASpacecraft::StageRocket()
{
	if (!Stages.IsEmpty())
	{
		// Remove the last stage
		Stages.RemoveAt(Stages.Num() - 1);
		
		// Update mass based on remaining stages
		Mass *= 0.8f; // Simplified mass reduction
		
		UE_LOG(LogTemp, Log, TEXT("Rocket staged! Remaining stages: %d"), Stages.Num());
	}
}

FVector ASpacecraft::CalculateGravitationalForces()
{
	FVector TotalForce = FVector::ZeroVector;
	
	if (ACosmosWorld* CosmosWorld = Cast<ACosmosWorld>(GetWorld()))
	{
		for (ACelestialEntity* Entity : CosmosWorld->CelestialEntities)
		{
			if (Entity)
			{
				FVector Direction = Entity->GetActorLocation() - GetActorLocation();
				float Distance = Direction.Size();
				
				if (Distance < 1.0f)
					continue;

				Direction.Normalize();
				
				const float G = 6.674e-11f * 1e20f;
				float ForceMagnitude = G * Mass * Entity->Mass / (Distance * Distance);
				
				TotalForce += Direction * ForceMagnitude;
			}
		}
	}
	
	return TotalForce;
}