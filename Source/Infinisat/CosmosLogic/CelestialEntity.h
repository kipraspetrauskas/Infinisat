#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CelestialEntity.generated.h"

UENUM(BlueprintType)
enum class ECelestialType : uint8
{
	Star,
	Planet,
	Moon,
	Asteroid,
	Comet,
	BlackHole
};

UCLASS()
class COSMOSLOGIC_API ACelestialEntity : public AActor
{
	GENERATED_BODY()

public:	
	ACelestialEntity();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial")
	ECelestialType CelestialType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial")
	float Mass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial")
	float Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial")
	FVector OrbitalElements; // Semi-major axis, eccentricity, inclination

	UFUNCTION(BlueprintCallable, Category = "Celestial")
	void UpdateOrbit(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Celestial")
	void OnCollision(ACelestialEntity* Other);

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	FVector CalculateGravitationalForce(const ACelestialEntity* Other);
};