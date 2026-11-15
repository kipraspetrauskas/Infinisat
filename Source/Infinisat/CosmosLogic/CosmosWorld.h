#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmosWorld.generated.h"

UCLASS()
class COSMOSLOGIC_API ACosmosWorld : public AActor
{
	GENERATED_BODY()

public:	
	ACosmosWorld();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmos")
	TArray<class ACelestialEntity*> CelestialEntities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmos")
	TArray<class ASpacecraft*> Spacecraft;

	UPROPERTY(EditDefaultsOnly, Category = "Simulation")
	float SimulationSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Simulation")
	bool bUseNBodyPhysics = true;

	UFUNCTION(BlueprintCallable, Category = "Cosmos")
	void InitializeCosmos();

	UFUNCTION(BlueprintCallable, Category = "Cosmos")
	void AddCelestialEntity(class ACelestialEntity* Entity);

	UFUNCTION(BlueprintCallable, Category = "Cosmos")
	void AddSpacecraft(class ASpacecraft* Craft);

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	void UpdatePhysics(float DeltaTime);
	void HandleCollisions();
};