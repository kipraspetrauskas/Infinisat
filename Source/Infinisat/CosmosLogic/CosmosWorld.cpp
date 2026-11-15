#include "CosmosWorld.h"
#include "CelestialEntity.h"
#include "Spacecraft.h"
#include "MotionSolver.h"

ACosmosWorld::ACosmosWorld()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACosmosWorld::BeginPlay()
{
	Super::BeginPlay();
	InitializeCosmos();
}

void ACosmosWorld::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ACosmosWorld::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatePhysics(DeltaTime * SimulationSpeed);
	HandleCollisions();
}

void ACosmosWorld::InitializeCosmos()
{
	// Load cosmos data from database
	// Initialize celestial entities and spacecraft
}

void ACosmosWorld::AddCelestialEntity(ACelestialEntity* Entity)
{
	if (Entity && !CelestialEntities.Contains(Entity))
	{
		CelestialEntities.Add(Entity);
	}
}

void ACosmosWorld::AddSpacecraft(ASpacecraft* Craft)
{
	if (Craft && !Spacecraft.Contains(Craft))
	{
		Spacecraft.Add(Craft);
	}
}

void ACosmosWorld::UpdatePhysics(float DeltaTime)
{
	if (!bUseNBodyPhysics)
		return;

	// Update all celestial entities using N-body physics
	for (ACelestialEntity* Entity : CelestialEntities)
	{
		if (Entity)
		{
			Entity->UpdateOrbit(DeltaTime);
		}
	}

	// Update all spacecraft
	for (ASpacecraft* Craft : Spacecraft)
	{
		if (Craft)
		{
			Craft->UpdateTrajectory(DeltaTime);
		}
	}
}

void ACosmosWorld::HandleCollisions()
{
	// Check for collisions between entities
	for (int32 i = 0; i < CelestialEntities.Num(); i++)
	{
		for (int32 j = i + 1; j < CelestialEntities.Num(); j++)
		{
			if (CelestialEntities[i] && CelestialEntities[j])
			{
				FVector Dist = CelestialEntities[i]->GetActorLocation() - CelestialEntities[j]->GetActorLocation();
				float MinDist = CelestialEntities[i]->Radius + CelestialEntities[j]->Radius;

				if (Dist.Size() < MinDist)
				{
					// Handle collision
					CelestialEntities[i]->OnCollision(CelestialEntities[j]);
				}
			}
		}
	}
}