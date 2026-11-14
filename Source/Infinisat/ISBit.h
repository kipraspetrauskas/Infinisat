// Infinisat - BIT, the player: first-person floating robot. Only its floating hands are visible.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ISParts3D.h"
#include "ISBit.generated.h"

class UCameraComponent;

UCLASS()
class AISBitCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AISBitCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	void MoveInput(float Forward, float Right, bool bSprint);
	void LookInput(float DYaw, float DPitch);
	void Hover();
	void Reach() { ReachT = 1.f; }
	// Trace along the view; returns the component under the crosshair (complex trace)
	UPrimitiveComponent* TraceView(float RangeCm, FVector* OutHit = nullptr) const;

	UPROPERTY() UCameraComponent* Cam;
	UPROPERTY() USceneComponent* HandL;
	UPROPERTY() USceneComponent* HandR;
	UPROPERTY() USceneComponent* HoldRoot;   // held objects attach here (between the hands)
	bool bHolding = false;

private:
	FISModel HandModel;
	float T = 0.f, ReachT = 0.f;
};
