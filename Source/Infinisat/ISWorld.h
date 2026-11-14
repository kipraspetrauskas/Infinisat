// Infinisat - the open world: home garage, yard, launch field, sky. Chapter 0 gameplay lives here:
// build a CanSat on the workbench, carry it to the pad, launch, then find it with radio tracking.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISData.h"
#include "ISSim.h"
#include "ISParts3D.h"
#include "ISWorld.generated.h"

class UBoxComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UVolumetricCloudComponent;
class UPointLightComponent;
class UTextRenderComponent;
class ACameraActor;
class APostProcessVolume;
class AISBitCharacter;

enum class EISStage : uint8 { NoOrder, Build, Sealed, Carry, OnPad, Countdown, Boost, Coast, Descent, Landed, Recovered };

struct FISTelemetry { float T, AltM, TempC, PressHPa; };

struct FISDoor
{
	USceneComponent* Hinge = nullptr;
	float Angle = 0.f, Target = 0.f, OpenAngle = 95.f;
	bool bLocked = false;
	FString LockMsg;
};

// A component BIT has to find around the house (chapter 0 scavenger hunt)
struct FISHuntItem
{
	FName Part;
	FString Name;
	FString Where;      // hint
	FVector Pos = FVector::ZeroVector;
	bool bFound = false;
	bool bRequired = true;
	FISModel Model;
	UPrimitiveComponent* Hit = nullptr;
};

struct FISOpening { float From, To, Bottom, Top; uint8 Kind; };   // Kind: 0 door, 1 window, 2 open arch

struct FISFlightReport
{
	float ApogeeM = 0, MaxSpeed = 0, LandingSpeed = 0, DriftM = 0, FlightS = 0, SearchS = 0, TelemetryPct = 0;
	bool bBroken = false, bFound = false, bBeaconAlive = true, bChute = false;
	int32 Samples = 0;
};

UCLASS()
class AISHomeWorld : public AActor
{
	GENERATED_BODY()
public:
	AISHomeWorld();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	void SetActive(bool bOn);
	FTransform SpawnPoint() const;

	// --- order / design
	void StartCanSatOrder(int32 MissionIndex);
	FISDesign Design;
	FISEval Eval;
	int32 MissionIdx = 0;
	EISStage Stage = EISStage::NoOrder;
	int32 DesignVersion = 0;   // bumps whenever the CanSat design changes (UI refresh)

	// --- interaction
	FName ActionFor(UPrimitiveComponent* C) const;
	FString PromptFor(FName Action) const;
	void Interact(FName Action, AISBitCharacter* Bit);
	FString Objective() const;
	TArray<FString> Toasts;   // messages for the HUD (BIT says...)

	// --- scavenger hunt
	TArray<FISHuntItem> Hunt;
	int32 FoundCount(bool bRequiredOnly) const;
	int32 RequiredCount() const;
	bool UsesHunt() const { return MissionIdx == 0; }
	float ScanT = 0.f;          // > 0 while BIT's scanner shows nearby parts
	void Scan() { ScanT = 5.f; }

	// --- bench mode
	ACameraActor* BenchCamera() const { return BenchCam; }
	FName PartUnderCursor(UPrimitiveComponent* C) const;
	void BenchToggle(FName PartId);
	void BenchSeal();
	FName BenchInfoPart;
	TArray<FName> TrayParts;

	// --- flight / tracking
	bool bChaseCam = false;
	ACameraActor* ChaseCamera() const { return ChaseCam; }
	bool IsFlying() const { return Stage >= EISStage::Countdown && Stage <= EISStage::Descent; }
	bool IsTracking() const { return Stage >= EISStage::Boost && Stage < EISStage::Recovered; }
	float Countdown = 0.f;
	FVector CanPosM = FVector::ZeroVector, CanVelMs = FVector::ZeroVector;
	FVector WindMs = FVector::ZeroVector;
	TArray<FISTelemetry> Telemetry;
	FISFlightReport Report;
	// tracker readout for the HUD
	float TrackDistM = 0.f, TrackRssi = -200.f, TrackSens = -110.f, BeaconLeftS = 0.f;
	bool bTrackSignal = false, bHasGnss = false;
	FVector TrackDirWorld = FVector::ZeroVector;
	FVector PadWorld() const { return PadPos; }
	bool bDebriefReady = false;

private:
	void BuildSky();
	void BuildGround();
	void BuildGarage();
	void BuildHouse();
	// detailed home (ISHouse.cpp)
	void BuildHome();
	void BuildNeighbourhood();
	void BuildBackyard();
	void SpawnHunt();
	void Wall(const FVector2D& A, const FVector2D& B, const TArray<FISOpening>& Ops, const TCHAR* InTex, const FLinearColor& InTint,
		const TCHAR* OutTex = nullptr, const FLinearColor& OutTint = FLinearColor::White, float OutSide = 0.f, bool bBaseboard = true);
	void AddDoor(const FVector& HingePos, float Yaw, float Width, const FLinearColor& Color, bool bStartOpen, bool bLocked = false, const FString& LockMsg = FString());
	void Roof(const FVector& Center, float SizeX, float SizeY, float Pitch, float Overhang, const TCHAR* Tex, const FLinearColor& Tint, const TCHAR* GableTex, const FLinearColor& GableTint);
	void HouseShell(const FVector& Origin, float SX, float SY, float Yaw, const FLinearColor& Wall, const FLinearColor& Door, const TCHAR* RoofTex, const FLinearColor& RoofTint);
	UStaticMeshComponent* Slab(const FVector& Center, const FVector& Size, const TCHAR* Tex, float Tile, const FLinearColor& Tint, bool bCollide = true, float Yaw = 0.f);
	UStaticMeshComponent* Paint(const FVector& Center, const FVector& Size, const FLinearColor& C, float Rough = 0.6f, bool bCollide = true, float Yaw = 0.f, float Metal = 0.f);
	void RoomLight(const FVector& Pos, float Lumens, bool bFixture = true);
	TArray<FISDoor> Doors;
	// layout validation (dev: -ISValidate)
	struct FISPlaced { FString Name; UStaticMeshComponent* C = nullptr; bool bSurface = false; bool bOnTop = false; };
	TArray<FISPlaced> Furniture;
	TArray<FBox> Openings;      // keep-clear volumes in front of doors and windows
	void ValidateLayout();
	TArray<float> DoorYaw;
	float MudShelfTop = 100.f, GarageShelfTop = 120.f, OfficeShelfTop = 100.f, MonitorTop = 120.f, JunkBoxTop = 40.f, DeskTopZ = 90.f, DeskTopY = 1005.f, OfficeShelfX = 460.f, MudShelfX = 1290.f, GarageShelfX = -730.f;
	void BuildGround2();
	bool IsFound(FName Part) const;
	void BuildField();
	void BuildNature();
	void RebuildCan();
	void RebuildTray();
	UStaticMeshComponent* Block(const FVector& Center, const FVector& Size, const TCHAR* Tex, float Tile, const FLinearColor& Tint = FLinearColor::White, bool bCollide = true);
	UStaticMeshComponent* Place(const TCHAR* Id, const FVector& Pos, float Yaw, float Scale = 1.f, bool bCollide = false);
	UBoxComponent* AddInteract(const FVector& Center, const FVector& Extent, FName Action);
	UTextRenderComponent* AddSign(const FString& Text, const FVector& Pos, float Yaw, float Size, const FColor& Color);
	void Toast(const FString& S) { Toasts.Add(S); }
	void StepFlight(float Dt);
	void UpdateTracker(float Dt);
	FVector MToWorld(const FVector& M) const { return PadPos + M * 100.f; }

	UPROPERTY() USceneComponent* Root;
	UPROPERTY() UDirectionalLightComponent* Sun;
	UPROPERTY() USkyLightComponent* SkyLight;
	UPROPERTY() USkyAtmosphereComponent* Atmosphere;
	UPROPERTY() UExponentialHeightFogComponent* Fog;
	UPROPERTY() UVolumetricCloudComponent* Clouds;
	UPROPERTY() APostProcessVolume* PPV = nullptr;
	UPROPERTY() ACameraActor* BenchCam = nullptr;
	UPROPERTY() ACameraActor* ChaseCam = nullptr;
	UPROPERTY() USceneComponent* CanRoot;
	UPROPERTY() USceneComponent* RocketRoot;
	UPROPERTY() USceneComponent* ChuteRoot;
	UPROPERTY() USceneComponent* BenchHands;
	UPROPERTY() TArray<UPrimitiveComponent*> AllComps;

	TMap<TWeakObjectPtr<UPrimitiveComponent>, FName> Actions;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FName> TrayHits;
	TMap<FName, FISModel> TrayModels;
	FISModel CanModel, RocketModel, ChuteModel, BenchHandModel, PadModel;
	UPROPERTY() UBoxComponent* CanHit = nullptr;
	UPROPERTY() UBoxComponent* LandedHit = nullptr;

	FVector BenchTop = FVector::ZeroVector;   // world point at centre of bench top
	FVector PadPos = FVector::ZeroVector;
	FVector RocketPosM = FVector::ZeroVector, RocketVelM = FVector::ZeroVector;
	float FlightT = 0.f, StageT = 0.f, BenchHandT = 0.f, SearchT = 0.f;
	FVector BenchHandGoal = FVector::ZeroVector;
	bool bChuteOpen = false, bLandedBroken = false;
	int32 TelemetryTotal = 0, TelemetryGot = 0;
};
