#include "ISBit.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

AISBitCharacter::AISBitCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(32.f, 80.f);
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(GetCapsuleComponent());
	Cam->SetRelativeLocation(FVector(0, 0, 62.f));   // eye ~1.42 m: BIT hovers at workbench height
	Cam->bUsePawnControlRotation = true;
	Cam->SetFieldOfView(80.f);
	HandL = CreateDefaultSubobject<USceneComponent>(TEXT("HandL"));
	HandL->SetupAttachment(Cam);
	HandR = CreateDefaultSubobject<USceneComponent>(TEXT("HandR"));
	HandR->SetupAttachment(Cam);
	HoldRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HoldRoot"));
	HoldRoot->SetupAttachment(Cam);
	HoldRoot->SetRelativeLocation(FVector(38.f, 0.f, -14.f));

	bUseControllerRotationYaw = true;
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->MaxWalkSpeed = 420.f;
	M->JumpZVelocity = 520.f;
	M->AirControl = 0.6f;
	M->GravityScale = 0.8f;   // BIT floats a little
	M->MaxStepHeight = 40.f;
	JumpMaxHoldTime = 0.35f;
}

void AISBitCharacter::BeginPlay()
{
	Super::BeginPlay();
	ISBuild::Hand(this, HandL, 1.f, &HandModel);
	ISBuild::Hand(this, HandR, 1.f, &HandModel);
	for (UStaticMeshComponent* C : HandModel.Parts)
	{
		C->SetCastShadow(false);
		C->bOnlyOwnerSee = false;
	}
	HandL->SetRelativeScale3D(FVector(1, -1, 1)); // mirror left hand
}

void AISBitCharacter::MoveInput(float Forward, float Right, bool bSprint)
{
	GetCharacterMovement()->MaxWalkSpeed = bSprint ? 950.f : 420.f;
	const FRotator Yaw(0, GetControlRotation().Yaw, 0);
	if (Forward != 0) AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Forward);
	if (Right != 0) AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Right);
}

void AISBitCharacter::LookInput(float DYaw, float DPitch)
{
	AddControllerYawInput(DYaw);
	AddControllerPitchInput(DPitch);
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
	const float Speed = GetVelocity().Size2D() / 420.f;
	const float Bob = FMath::Sin(T * (2.f + Speed * 6.f)) * (0.6f + Speed * 1.2f);
	const float R = FMath::Sin(ReachT * PI);
	const float Hold = bHolding ? 1.f : 0.f;
	// floating hands: resting low at the sides, move together when holding something, reach forward on interact
	HandR->SetRelativeLocation(FVector(34.f + R * 22.f, FMath::Lerp(17.f, 9.f, Hold), -21.f + Bob + R * 6.f + Hold * 5.f));
	HandL->SetRelativeLocation(FVector(34.f + R * 8.f, FMath::Lerp(-17.f, -9.f, Hold), -21.f - Bob * 0.8f + Hold * 5.f));
	HandR->SetRelativeRotation(FRotator(-10.f + R * 20.f, -15.f + Hold * 40.f, FMath::Sin(T * 1.3f) * 6.f));
	HandL->SetRelativeRotation(FRotator(-10.f, 15.f - Hold * 40.f, FMath::Sin(T * 1.1f) * 6.f));
}
