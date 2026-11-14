// Infinisat - everything 3D: workshop, planets, orbits, satellite populations, BIT the robot.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISData.h"
#include "ISScene.generated.h"

struct FISDesign;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UStaticMesh;
class ACameraActor;
class UDirectionalLightComponent;
class UMaterialInstanceDynamic;
class ULineBatchComponent;

enum class EISView : uint8 { Title, Workshop, Orbit, Compare };

UCLASS()
class AISSpaceScene : public AActor
{
	GENERATED_BODY()
public:
	AISSpaceScene();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetActive(bool bOn);
	// launch animation + follow camera
	void StartLaunch(bool bHeavy);
	void SkipLaunch() { if (LaunchT >= 0.f) LaunchT = 0.999f; }
	bool IsLaunching() const { return LaunchT >= 0.f; }
	FString LaunchCaption() const;
	void ToggleMap();
	bool IsFollowing() const { return bFollow; }
	void ShowTitle();
	void ShowWorkshop(const FISDesign& D);
	void ShowOrbit(EISDest Dest, float AltKm, float IncDeg, const FISDesign& D, int32 Fleet);
	void ShowCompare(const FISDesign& D, const FString& RealMesh, const FString& RealName);
	void ShowModelOnly(const FString& MeshPath, const FString& Label);

	// Operations: place the player's spacecraft along its path (0..1 of mission, orbit phase in days)
	void SetMissionTime(double Day, double EndDay, double PeriodMin, bool bAlive);
	void SetBitMood(int32 Mood); // 0 neutral, 1 happy, 2 worried
	void PokeBit();

	// Camera control (driven by the player controller)
	void OrbitCamera(float DYaw, float DPitch);
	void ZoomCamera(float Steps);
	ACameraActor* GetCamera() const { return Camera; }

private:
	// helpers
	UStaticMeshComponent* AddMesh(USceneComponent* Parent, UStaticMesh* Mesh, const FTransform& T, const FLinearColor& Color, bool bUnlit = false, float Emissive = 0.f, TArray<UStaticMeshComponent*>* Track = nullptr);
	UStaticMeshComponent* AddMeshPath(USceneComponent* Parent, const FString& Path, const FTransform& T, const FLinearColor& Color, TArray<UStaticMeshComponent*>* Track);
	UTextRenderComponent* AddLabel(const FString& Text, const FVector& Pos, const FColor& Color);
	UInstancedStaticMeshComponent* AddDots(UStaticMesh* Mesh, const FLinearColor& Color, float Emissive);
	void BuildSatellite(USceneComponent* Root, const FISDesign& D, TArray<UStaticMeshComponent*>& Track);
	void ClearView();
	void BuildBit();
	void SetCameraTarget(const FVector& Focus, float Dist, float Yaw, float Pitch, float MinD, float MaxD);
	void AddOrbitRing(float RadiusUU, float IncDeg, float RaanDeg, const FLinearColor& Color, int32 N, float DotScale);
	void BuildEarthPopulation(float K);
	void SetPlanet(UStaticMeshComponent* C, const TCHAR* Tex, float RadiusUU, const FVector& Pos);

	UPROPERTY() USceneComponent* SceneRoot;
	UPROPERTY() USceneComponent* SatRoot;
	UPROPERTY() USceneComponent* RealRoot;
	UPROPERTY() USceneComponent* BitRoot;
	UPROPERTY() UStaticMeshComponent* Sky;
	UPROPERTY() UStaticMeshComponent* Planet;
	UPROPERTY() UStaticMeshComponent* PlanetAtmo;
	UPROPERTY() UStaticMeshComponent* Moon;
	UPROPERTY() UStaticMeshComponent* Sun;
	UPROPERTY() UDirectionalLightComponent* SunLight;
	UPROPERTY() UDirectionalLightComponent* FillLight;
	UPROPERTY() ACameraActor* Camera;
	UPROPERTY() class APostProcessVolume* PPV = nullptr;

	UPROPERTY() TArray<UStaticMeshComponent*> SatParts;
	UPROPERTY() TArray<UStaticMeshComponent*> RealParts;
	UPROPERTY() TArray<UStaticMeshComponent*> Extras;
	UPROPERTY() TArray<UInstancedStaticMeshComponent*> DotSets;
	UPROPERTY() TArray<UTextRenderComponent*> Labels;
	UPROPERTY() UInstancedStaticMeshComponent* Swarm = nullptr;
	UPROPERTY() UInstancedStaticMeshComponent* PathDots = nullptr;

	// BIT parts
	UPROPERTY() UStaticMeshComponent* BitIris;
	UPROPERTY() USceneComponent* BitHandL;
	UPROPERTY() USceneComponent* BitHandR;
	UPROPERTY() UMaterialInstanceDynamic* BitIrisMID;

	UPROPERTY() UMaterialInterface* MatColor;
	UPROPERTY() UMaterialInterface* MatUnlit;
	UPROPERTY() UMaterialInterface* MatPlanet;
	UPROPERTY() UMaterialInterface* MatSky;
	UPROPERTY() UMaterialInterface* MatAtmo;
	UPROPERTY() UStaticMesh* MCube;
	UPROPERTY() UStaticMesh* MSphere;
	UPROPERTY() UStaticMesh* MCyl;
	UPROPERTY() UStaticMesh* MCone;

	struct FSwarmSat { float R, Inc, Raan, Phase, Rate; };
	TArray<FSwarmSat> SwarmSats;

	EISView View = EISView::Title;
	bool bFollow = true;
	bool bTrueScale = false;            // Earth/Moon views: real sizes and distances (cm)
	float RealSatCm = 200.f;
	double PeriodS = 5600.0;
	UPROPERTY() ULineBatchComponent* Lines = nullptr;
	UPROPERTY() UStaticMeshComponent* CloudShell = nullptr;
	struct FTruePt { double R, Inc, Raan, Phase, Rate; FColor C; float Size; };
	TArray<FTruePt> TruePts;
	void SetEarth(UStaticMeshComponent* C, float RadiusUU, const FVector& Pos, bool bClouds);
	void TickTrueScale(float Dt);
	void BuildCleanRoom();
	float LaunchT = -1.f;
	float LastSatSize = 400.f, MapDist = 30000.f, PlanetRUU = 6371.f;
	FVector LaunchSite = FVector::ZeroVector, RocketPrev = FVector::ZeroVector;
	UPROPERTY() USceneComponent* RocketRoot = nullptr;
	EISDest ViewDest = EISDest::LEO;
	float ViewK = 1.f;          // uu per km in orbit views
	float OrbitRadiusUU = 0.f;
	float OrbitInc = 0.f;
	double SimTime = 0.0;
	float PokeT = 0.f;
	int32 BitMood = 0;
	float BitBaseScale = 1.f;
	FVector BitHome = FVector::ZeroVector;
	double MissionDay = 0.0;
	float FollowYaw = 25.f, FollowPitch = -18.f;

	// camera state
	FVector CamFocus = FVector::ZeroVector, CamFocusGoal = FVector::ZeroVector;
	float CamDist = 800.f, CamDistGoal = 800.f, CamYaw = 200.f, CamPitch = -15.f, CamMin = 100.f, CamMax = 5000.f;
};
