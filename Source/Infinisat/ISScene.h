// Infinisat - everything 3D: workshop, planets, orbits, satellite populations, BIT the robot.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISData.h"
#include "ISRocket.h"
#include "ISNBody.h"
#include "ISTrajectory.h"
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

enum class EISView : uint8 { Title, Workshop, Orbit, Compare, Cosmos };
class UTexture2D;
class UPointLightComponent;

UCLASS()
class AISSpaceScene : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY() class AISEarth* Earth = nullptr;   // the real globe (set by the player controller)
	AISSpaceScene();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetActive(bool bOn);
	// launch animation + follow camera
	void StartLaunch(bool bHeavy);
	// real rocket flight (ISRocket.h): physics ascent the player can fly; replaces the scripted launch for orbital vehicles
	bool StartFlight(FName VehicleId, double PayloadKg, double TargetAltKm, double IncDeg);
	// where the bought launch flies from (the real pad of that vehicle; Mahia by default). Call before ShowOrbit:
	// the orbital plane and the morning sun are placed over this pad.
	void SetLaunchSite(FName VehicleId);
	double PadLon = 177.8649, PadLat = -39.2615, PadH = 122.0;
	bool bPadMahia = true;
	bool bPadLevelled = false;
	bool IsFlightActive() const { return bFlight; }
	FISRocketSim* Flight() { return bFlight ? &Sim : nullptr; }
	void SetFlightWarp(float W) { FlightWarp = W; }
	float GetFlightWarp() const { return FlightWarp; }
	void RetryFlight();
	void EndFlight();
	void SkipLaunch();
	bool IsLaunching() const { return LaunchT >= 0.f; }
	FString LaunchCaption() const;
	void ToggleMap();
	bool IsFollowing() const { return bFollow; }
	void ShowTitle();
	void ShowWorkshop(const FISDesign& D);
	void ShowOrbit(EISDest Dest, float AltKm, float IncDeg, const FISDesign& D, int32 Fleet);
	void ShowCompare(const FISDesign& D, const FString& RealMesh, const FString& RealName);
	void ShowModelOnly(const FString& MeshPath, const FString& Label);

	// Cosmos explorer (ISCosmos.cpp): solar system, exoplanet systems, black hole, nebulae, galaxies.
	void ShowCosmos(const FISDesign& D);
	void FocusCosmos(int32 Idx);
	int32 CosmosCount() const { return CosmosBodies.Num(); }
	int32 CosmosFocusIdx() const { return CosmosSel; }
	FString CosmosName(int32 I) const { return CosmosBodies.IsValidIndex(I) ? CosmosBodies[I].Name : FString(); }
	FString CosmosFacts(int32 I) const { return CosmosBodies.IsValidIndex(I) ? CosmosBodies[I].Facts : FString(); }
	FString CosmosGroup(int32 I) const { return CosmosBodies.IsValidIndex(I) ? CosmosBodies[I].Group : FString(); }
	int32 CosmosDepth(int32 I) const;
	void SetCosmosRate(float DaysPerSecond) { CosmosRate = DaysPerSecond; }
	float GetCosmosRate() const { return CosmosRate; }
	double GetCosmosDays() const { return CosmosDays; }
	void ToggleCosmosLabels() { bCosmosLabels = !bCosmosLabels; }
	bool CosmosLabelsOn() const { return bCosmosLabels; }
	UTexture2D* CosmosThumb(int32 I) const;
	FLinearColor CosmosColor(int32 I) const { return CosmosBodies.IsValidIndex(I) ? FLinearColor(CosmosBodies[I].LabelColor) : FLinearColor::White; }
	int32 CosmosFind(const FString& Name) const { for (int32 i = 0; i < CosmosBodies.Num(); ++i) if (CosmosBodies[i].Name == Name) return i; return -1; }
	// the player pushed the wheel past the zoom limit (-1 out, +1 in): the UI moves between the mission view and the solar system
	int32 TakeZoomPast() { const int32 Z = ZoomPast; ZoomPast = 0; return Z; }
	// the globe view: the real Earth with your space centre, reached by zooming out from the ground
	void ShowGlobe(float AltM);
	FVector SatMountWorld(int32 Mount) const;  // where a mounting place of the satellite on the stand is, for the build UI
	bool bGlobe = false, bLand = false;
	bool TakeLand() { const bool B = bLand; bLand = false; return B; }
	int32 ZoomPast = 0, ZoomPastCount = 0;
	FVector CosmosPos(int32 I) const;
	int32 CosmosParent(int32 I) const { return CosmosBodies.IsValidIndex(I) ? CosmosBodies[I].Parent : -1; }
	float CosmosRadius(int32 I) const { return CosmosBodies.IsValidIndex(I) ? CosmosBodies[I].Radius : 0.f; }
	FString CosmosLabelText(int32 I) const { return CosmosBodies.IsValidIndex(I) && CosmosAlive(I) ? CosmosBodies[I].Label : FString(); }
	FColor CosmosLabelColor(int32 I) const { return CosmosBodies.IsValidIndex(I) ? CosmosBodies[I].LabelColor : FColor::White; }
	FVector GetCameraLocation() const;
	// sandbox (solar system runs on real mutual gravity, ISNBody.h); i = cosmos body index
	bool CosmosIsSim(int32 I) const { return CosmosBodies.IsValidIndex(I) && CosmosBodies[I].Sim >= 0; }
	bool CosmosAlive(int32 I) const { return !CosmosIsSim(I) || NBody.B[CosmosBodies[I].Sim].bAlive; }
	void CosmosScaleMass(int32 I, double F);
	void CosmosNudge(int32 I, double F);     // multiply the speed relative to what it orbits
	void CosmosAddBody(int32 Kind);          // 0 moon around the selection, 1 rogue Jupiter, 2 impactor aimed at the selection
	void CosmosResetSim();
	FString CosmosSimInfo(int32 I) const;    // live mass / orbit line for the selection card
	FString CosmosEvent() const
	{
		if (CosmosEventT > 0.f) return CosmosEventText;
		if (bCosmosMission) return VoyageCaption();   // the mission's own news: cruise, burns, distance from the Sun
		return NBody.bLimited ? FString(TEXT("Simulation slowed: too many close encounters for this speed.")) : FString();
	}

	// Operations: place the player's spacecraft along its path (0..1 of mission, orbit phase in days)
	void SetMissionTime(double Day, double EndDay, double PeriodMin, bool bAlive);
	// deep-space voyage (ISVoyage.cpp): the flight plan made from the parking orbit the rocket reached
	float WarpCap() const;                                   // 0 = no limit; otherwise the mission clock must not run faster
	FString VoyageCaption() const;                           // "Slowing down for the burn" etc. (empty = nothing to say)
	bool TakeVoyagePlanned(double& OutDepartDay) { const bool b = bVoyPlannedNew; bVoyPlannedNew = false; OutDepartDay = Voy.DepartT / 86400.0; return b; }
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
	bool bRealEarth = false;            // the true-scale Earth is the real Cesium globe (Earth-centred origin)
	FQuat OrbitQuat() const;            // the orbital plane: through the real launch site at the real launch heading
	FVector FlightBase = FVector::ZeroVector;   // shifts the flight (which starts at radius Re) onto the real pad
	FVector SatHalf = FVector(50.f);           // half size of the bus being built (model units)
	float RealSatCm = 200.f;
	double PeriodS = 5600.0;
	UPROPERTY() ULineBatchComponent* Lines = nullptr;
	UPROPERTY() UStaticMeshComponent* CloudShell = nullptr;
	struct FTruePt { double R, Inc, Raan, Phase, Rate; FColor C; float Size; };
	TArray<FTruePt> TruePts;
	void SetEarth(UStaticMeshComponent* C, float RadiusUU, const FVector& Pos, bool bClouds);
	void TickTrueScale(float Dt);
	void BuildCleanRoom();
	// the lab from the Kenney Space Station Kit (ISLab.cpp)
	static bool LabKitAvailable();
	void BuildLabKit();
	UStaticMeshComponent* LabPiece(const TCHAR* Name, const FVector& P, float Yaw, float Scale);
	float LaunchT = -1.f;
	float RevealT = -1.f;   // orbit reached: seconds into the pull-back to the whole-planet shot (-1 = off)
	// payload separation: a copy of the satellite rides on the upper stage, springs off and unfolds its solar wings
	struct FISWingPanel { int32 First = 0, Num = 0, K = 0, Wing = 0; FVector Out = FVector::ZeroVector, Hinge = FVector::ZeroVector; float Pitch = 0.f, Half = 0.f; };
	TArray<FISWingPanel> SatWings;      // filled by BuildSatellite for SatParts: which parts form each deployable panel
	float SatHalfH = 50.f;              // half the bus height in model units (adapter ring sits below it)
	UPROPERTY() USceneComponent* SepRoot = nullptr;
	UPROPERTY() TArray<UStaticMeshComponent*> SepParts;   // index-aligned with SatParts
	TArray<FTransform> SepFinal;        // deployed (as built) relative transforms
	float SepT = -1.f;
	TArray<UStaticMeshComponent*> TrenchFx;   // flame-trench jets + deluge steam (also in PadVis for clean-up)
	// ---- voyage (ISVoyage.cpp)
	bool bVoyage = false;               // deep-space mission view: the satellite follows Voy, not a fixed orbit
	FISTrajectory Voy;
	float VoyAltKm = 0.f;
	double VoyEndDay = 1.0, VoyBurnDur = 300.0, VoyInsDur = 1200.0;
	bool bVoyPlannedNew = false, bVoySepDone = false;
	FVector VoyCamOff = FVector::ZeroVector;   // stack shot: from the satellite to the middle of stage + satellite
	double VoySepStartT = -1.0;
	FQuat VoyAtt = FQuat::Identity;     // the free satellite's attitude after separation
	int32 VoyCamMode = 0;               // 0 follow (stack / satellite), 1 map (Earth + Moon + path)
	FVector VoyFocus = FVector::ZeroVector, BodyC = FVector::ZeroVector, FollowN = FVector::ZeroVector;
	UPROPERTY() UStaticMeshComponent* SatThruster = nullptr;
	UPROPERTY() UTextRenderComponent* MoonLabel = nullptr;
	UPROPERTY() TArray<UStaticMeshComponent*> VoyMoons;   // Jupiter scene: Io, Europa, Ganymede, Callisto
	bool bJupiterScene = false;         // the mission view is centred on Jupiter (arrival + science orbit), not on Earth
	bool bCosmosMission = false;        // the solar-system view shows the mission (clock locked, probe drawn)
	int32 CosmosProbe = -1;
	double CosmosMissionT = 0.0;        // mission second the Cosmos N-body sim stands at
	void PlanVoyage();
	void TickVoyage(float Dt);
	void EnsureThruster();
	void BuildJupiterScene();
	void TickCosmosMission(TArray<FVector>& SimPos, TArray<uint8>& SimDone);
	UTexture2D* MoonTexture(const FString& Name);
	int32 VoyCentre() const;            // ISOrbit body the mission view is centred on
public:
	// 0 near Earth (launch / departure), 1 cruise between planets, 2 arrival at the target (Jupiter)
	int32 VoyagePhase() const;
	void SetCosmosMission(bool bOn);
private:
	FVector PadSite = FVector::ZeroVector, PadAxis = FVector::RightVector;
	void StartSeparation();
	void TickSeparation(float Dt);
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
	float FlightShake = 0.f;      // 0..1 vibration from the engines and the air (lift-off rumble, max-Q buffeting)
	bool bFlightCamUser = false;   // the player turned the launch camera: stop steering it automatically

	// ---- cosmos explorer
	struct FCosmosBody
	{
		FString Name, Facts, Group;
		int32 Parent = -1;            // orbits this body (-1: fixed at Center)
		FVector Center = FVector::ZeroVector;
		float OrbitR = 0.f, PeriodDays = 1.f, Phase = 0.f, Inc = 0.f, Node = 0.f;
		float Radius = 100.f, SpinDegPerDay = 0.f, Tilt = 0.f;
		float Angle = 0.f, SpinAngle = 0.f;   // accumulated (visual speed is capped so fast moons stay watchable)
		FString Label; FColor LabelColor = FColor::White; FLinearColor OrbitColor = FLinearColor::Transparent;
		USceneComponent* Pivot = nullptr;   // moves along the orbit, holds everything that travels with the body
		USceneComponent* Spinner = nullptr; // rotates (surface)
		UMaterialInstanceDynamic* SunDirMID = nullptr; // Earth day/night material needs the sun direction
		int32 Star = -1;                    // which star lights it
		UTexture2D* Thumb = nullptr;        // surface texture, used as the picture in the body strip
		int32 Sim = -1;                     // index in NBody (-1: still on rails)
		double RealR0 = 1.0;                // real radius (km) when built, to scale the mesh after merges / mass edits
	};
	TArray<FCosmosBody> CosmosBodies;
	UPROPERTY() TArray<USceneComponent*> CosmosComps;
	UPROPERTY() TArray<USceneComponent*> CosmosBillboards;
	UPROPERTY() TMap<FString, UTexture2D*> CosmosTexCache;
	UPROPERTY() USceneComponent* CosmosBackdrop = nullptr;
	UPROPERTY() USceneComponent* StarField = nullptr;   // 3D star field used by every space view (replaces the flat sky picture)
	void BuildStarField();
	void TickStarField(const FVector& CamLoc);
	void BuildCosmosBackdropOld();
	UPROPERTY() USceneComponent* SkyPlanets = nullptr;
	UPROPERTY() TArray<UTextRenderComponent*> SkyPlanetLabels;
	UPROPERTY() ULineBatchComponent* CosmosLines = nullptr;
	UPROPERTY() ULineBatchComponent* FlightLines = nullptr;   // flown path (orange) + predicted path (cyan)
	TArray<FVector2D> FlightPred;
	UPROPERTY() UMaterialInterface* MatGlow = nullptr;
	UPROPERTY() UMaterialInterface* MatRing = nullptr;
	int32 CosmosSel = 0;
	double CosmosDays = 0.0;
	float CosmosRate = 1.f;
	bool bCosmosLabels = true;
	void TickCosmos(float Dt, const FVector& CamLoc);
	void ClearCosmos();
	UTexture2D* CosmosTex(const FString& Key, int32 W, int32 H, TFunctionRef<FLinearColor(float U, float V)> F);
	int32 AddCosmosBody(const FString& Name, const FString& Group, int32 Parent, float OrbitR, float PeriodDays, float Radius, float SpinDegPerDay, const FString& Facts);
	USceneComponent* CosmosNode(USceneComponent* Parent);
	UStaticMeshComponent* CosmosSphere(USceneComponent* Parent, float Radius, UMaterialInterface* Mat, bool bHi = true);
	void CosmosPlanetTex(int32 Body, UTexture2D* Tex, float Tilt, float Ambient = 0.03f);
	void CosmosAtmo(int32 Body, const FLinearColor& C, float Scale, float Intensity);
	void CosmosRing(USceneComponent* Parent, float R0, float R1, UTexture2D* Tex, bool bGlow, float Brightness, float Tilt);
	UStaticMeshComponent* CosmosSprite(USceneComponent* Parent, const FVector& Loc, float Size, UTexture2D* Tex, const FLinearColor& C, float Intensity, bool bFaceCamera);
	void CosmosOrbitDots(int32 Body, const FLinearColor& C);
	int32 CosmosStar(const FString& Name, const FVector& Center, float Radius, const FLinearColor& C, float Light, const FString& Facts);
	void CosmosLabel(int32 Body, const FString& Text, const FColor& C);
	void BuildSolarSystem(const FISDesign& D);
	void BuildExoSystem(int32 Seed, const FString& StarName, const FVector& Center, const FLinearColor& StarC, float StarR, const FString& StarFacts);
	void BuildBlackHole(const FVector& Center);
	void BuildCosmosBackdrop();
	// sandbox: real physics in km, drawn on the compressed layout (CompressKm maps a real distance to scene units per parent)
	FISNBodySim NBody;
	TArray<int32> SimToCosmos;
	TMap<int32, TArray<FVector2D>> CosmosScale;   // per sim parent: sorted (log real km, log scene units) of its original children
	int32 CosmosBodyBase = 0, CosmosCompBase = 0;  // everything after these was added by the player (removed on reset)
	FString CosmosEventText;
	float CosmosEventT = 0.f;
	int32 CosmosAdded = 0;
	void SeedCosmosSim(double T0 = 0.0);   // ISOrbit time the planets start at
	float CosmosDispR(int32 Cosmos) const;
	float CompressKm(int32 SimParent, double Km, float Touch) const;
	FVector CosmosSimPos(int32 Sim, TArray<FVector>& Memo, TArray<uint8>& Done) const;
	void CosmosNotice(const FString& S) { CosmosEventText = S; CosmosEventT = 5.f; }

	// ---- rocket flight
	FISRocketSim Sim;
	bool bFlight = false;
	float FlightWarp = 1.f, FlightEndT = 0.f;
	FName FlightVehicle;
	double FlightPayload = 0, FlightAlt = 0, FlightInc = 0;
	FVector FlightU = FVector::UpVector, FlightDn = FVector::ForwardVector;
	TArray<TArray<UStaticMeshComponent*>> StageVis;
	TArray<UStaticMeshComponent*> BoosterVis, FairingVis, PlumeCore, PlumeBoost, PadVis;
	TArray<float> StageBaseZ;
	float RocketLenCm = 7000.f;
	UPROPERTY() class USkyAtmosphereComponent* SkyAtmo = nullptr;
	// ---- stylised orbit (ISStylizedOrbit.cpp): satellite at the origin, a 1/100-scale low-poly Earth turning below it
	bool bStylized = false;
	float LPRadius = 0.f, LPAlt = 0.f, LPWarp = 1.f;
	double LPAngle = 0.0, LPCloudAngle = 0.0;
	UPROPERTY() USceneComponent* LPPivot = nullptr;
	UPROPERTY() USceneComponent* LPSpin = nullptr;
	UPROPERTY() USceneComponent* LPCloudSpin = nullptr;
	UPROPERTY() UStaticMeshComponent* LPEarth = nullptr;
	UPROPERTY() UInstancedStaticMeshComponent* LPClouds = nullptr;
	static bool StylizedAvailable();
	void BuildStylizedEarth(float AltKm, float IncDeg);
	void TickStylizedEarth(float Dt, const FVector& CamLoc);
	void HideStylizedEarth();
	void BuildFlightRocket();
	void TickFlight(float Dt);

	// camera state
	FVector CamFocus = FVector::ZeroVector, CamFocusGoal = FVector::ZeroVector;
	float CamDist = 800.f, CamDistGoal = 800.f, CamYaw = 200.f, CamPitch = -15.f, CamMin = 100.f, CamMax = 5000.f;
};
