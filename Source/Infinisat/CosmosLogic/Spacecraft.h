#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spacecraft.generated.h"

UENUM(BlueprintType)
enum class ESpacecraftType : uint8
{
	Probe,
	Satellite,
	Rocket,
	Capsule,
	Telescope
};

UCLASS()
class COSMOSLOGIC_API ASpacecraft : public AActor
{
	GENERATED_BODY()

public:	
	ASpacecraft();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spacecraft")
	ESpacecraftType SpacecraftType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spacecraft")
	float Mass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spacecraft")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spacecraft")
	TArray<FVector> Stages; // Rocket stages for staging logic

	UFUNCTION(BlueprintCallable, Category = "Spacecraft")
	void UpdateTrajectory(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Spacecraft")
	void ApplyThrust(FVector Direction, float Force);

	UFUNCTION(BlueprintCallable, Category = "Spacecraft")
	void StageRocket();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	FVector CalculateGravitationalForces();
};