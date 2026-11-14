#include "ISBit.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

AISBitCharacter::AISBitCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(30.f, 80.f);
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(GetCapsuleComponent());
	Cam->SetRelativeLocation(FVector(0, 0, EyeHeight));
	Cam->bUsePawnControlRotation = true;
	Cam->SetFieldOfView(90.f);
	// hands are no longer drawn in first person; the components stay as attach points
	HandL = CreateDefaultSubobject<USceneComponent>(TEXT("HandL"));
	HandL->SetupAttachment(Cam);
	HandR = CreateDefaultSubobject<USceneComponent>(TEXT("HandR"));
	HandR->SetupAttachment(Cam);
	HoldRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HoldRoot"));
	HoldRoot->SetupAttachment(Cam);
	HoldRoot->SetRelativeLocation(FVector(34.f, 8.f, -12.f));

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->MaxWalkSpeed = WalkSpeed;
	M->MaxAcceleration = 2600.f;            // reach full speed in ~0.15 s
	M->BrakingDecelerationWalking = 2600.f; // stop crisply, no ice-skating
	M->GroundFriction = 9.f;
	M->BrakingFrictionFactor = 1.f;
	M->JumpZVelocity = 380.f;
	M->AirControl = 0.35f;
	M->GravityScale = 1.f;
	M->MaxStepHeight = 35.f;
	M->bCanWalkOffLedgesWhenCrouching = true;
	M->NavAgentProps.bCanCrouch = true;
	M->SetCrouchedHalfHeight(55.f);
	M->MaxWalkSpeedCrouched = 220.f;
	M->bUseSeparateBrakingFriction = false;
	JumpMaxHoldTime = 0.15f;
}

void AISBitCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AISBitCharacter::MoveInput(float Forward, float Right, bool bSprint)
{
	GetCharacterMovement()->MaxWalkSpeed = bSprint ? SprintSpeed : WalkSpeed;
	const FRotator Yaw(0, GetControlRotation().Yaw, 0);
	FVector Dir = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X) * Forward + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * Right;
	if (!Dir.IsNearlyZero()) AddMovementInput(Dir.GetClampedToMaxSize(1.f));
}

void AISBitCharacter::LookInput(float DYaw, float DPitch)
{
	// apply directly in degrees: no engine input scaling or smoothing, so the mouse feels 1:1
	if (AController* C = GetController())
	{
		FRotator R = C->GetControlRotation();
		R.Yaw += DYaw;
		R.Pitch = FMath::ClampAngle(FRotator::NormalizeAxis(R.Pitch) + DPitch, -88.f, 88.f);
		C->SetControlRotation(R);
	}
}

void AISBitCharacter::Hover() { Jump(); }

UPrimitiveComponent* AISBitCharacter::TraceView(float Range, FVector* OutHit) const
{
	FHitResult H;
	const FVector S = Cam->GetComponentLocation();
	const FVector E = S + Cam->GetForwardVector() * Range;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ISInteract), true, this);
	if (GetWorld()->LineTraceSingleByChannel(H, S, E, ECC_Visibility, Q))
	{
		if (OutHit) *OutHit = H.ImpactPoint;
		return H.GetComponent();
	}
	return nullptr;
}

void AISBitCharacter::Tick(float Dt)
{
	Super::Tick(Dt);
	T += Dt;
	ReachT = FMath::Max(0.f, ReachT - Dt * 2.2f);
	// BIT hovers: a very gentle float instead of a walking head-bob
	const float Speed = FMath::Clamp(GetVelocity().Size2D() / WalkSpeed, 0.f, 1.5f);
	const float Sway = FMath::Sin(T * 1.6f) * 0.6f + FMath::Sin(T * 7.f) * 0.25f * Speed;
	const float Base = bIsCrouched ? EyeHeight - 25.f : EyeHeight;
	CamZ = FMath::FInterpTo(CamZ <= 0.f ? Base : CamZ, Base, Dt, 10.f);
	Cam->SetRelativeLocation(FVector(0, 0, CamZ + Sway));
	// slight roll when strafing feels like a floating robot
	const float Strafe = FVector::DotProduct(GetVelocity(), GetActorRightVector()) / FMath::Max(1.f, SprintSpeed);
	Roll = FMath::FInterpTo(Roll, Strafe * 2.5f, Dt, 6.f);
	Cam->SetRelativeRotation(FRotator(0, 0, Roll));
	HoldRoot->SetRelativeLocation(FVector(34.f + FMath::Sin(ReachT * PI) * 8.f, 8.f, -12.f));
}
