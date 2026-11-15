#include "ISScene.h"
#include "ISEarth.h"
#include "ISSim.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Components/LineBatchComponent.h"
#include "Components/SkyAtmosphereComponent.h"

namespace
{
	const FLinearColor KeepMat(0, 0, 0, 0);
	template <typename T> T* Load(const TCHAR* Path) { return LoadObject<T>(nullptr, Path); }
	const TCHAR* TEX_EARTH = TEXT("/Game/Infinisat/Textures/T_Earth.T_Earth");
	const TCHAR* TEX_MOON = TEXT("/Game/Infinisat/Textures/T_Moon2.T_Moon2");
	const TCHAR* TEX_JUP = TEXT("/Game/Infinisat/Textures/T_Jupiter2.T_Jupiter2");
	FString ModelPath(const TCHAR* N) { return FString::Printf(TEXT("/Game/Infinisat/Models/%s/SM_%s.SM_%s"), N, N, N); }
}

AISSpaceScene::AISSpaceScene()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	SatRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SatRoot"));
	SatRoot->SetupAttachment(SceneRoot);
	RealRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RealRoot"));
	RealRoot->SetupAttachment(SceneRoot);
	BitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BitRoot"));
	BitRoot->SetupAttachment(SceneRoot);

	auto MakeSM = [this](const TCHAR* Name) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(SceneRoot);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};
	Sky = MakeSM(TEXT("Sky"));
	Sky->CastShadow = false;
	Planet = MakeSM(TEXT("Planet"));
	PlanetAtmo = MakeSM(TEXT("PlanetAtmo"));
	PlanetAtmo->CastShadow = false;
	Moon = MakeSM(TEXT("Moon"));
	Sun = MakeSM(TEXT("Sun"));
	Sun->CastShadow = false;

	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(SceneRoot);
	FillLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(SceneRoot);
}

void AISSpaceScene::BeginPlay()
{
	Super::BeginPlay();
	if (FParse::Param(FCommandLine::Get(), TEXT("ISRocketTest"))) { extern void ISRocketSelfTest(); ISRocketSelfTest(); }
	if (FParse::Param(FCommandLine::Get(), TEXT("ISNBodyTest"))) { extern void ISNBodySelfTest(); ISNBodySelfTest(); }
	if (FParse::Param(FCommandLine::Get(), TEXT("ISTrajectoryTest"))) { extern void ISTrajectorySelfTest(); ISTrajectorySelfTest(); }
	MatColor = Load<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Color.M_IS_Color"));
	MatUnlit = Load<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Unlit.M_IS_Unlit"));
	MatPlanet = Load<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Planet.M_IS_Planet"));
	MatSky = Load<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Sky.M_IS_Sky"));
	MatAtmo = Load<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Atmo.M_IS_Atmo"));
	MCube = Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube"));
	MSphere = Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	MCyl = Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	MCone = Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Cone.Cone"));

	Sky->SetStaticMesh(MSphere);
	Sky->SetMaterial(0, MatSky);
	Sky->SetWorldScale3D(FVector(40000.f));
	Sky->SetWorldRotation(FRotator(0, 0, 60)); // tilt the Milky Way

	for (UStaticMeshComponent* C : { Planet, Moon, Sun, PlanetAtmo }) C->SetStaticMesh(MSphere);
	if (UMaterialInstanceDynamic* AtmoM = UMaterialInstanceDynamic::Create(MatAtmo, this))
	{
		AtmoM->SetScalarParameterValue(TEXT("Intensity"), 0.55f);
		PlanetAtmo->SetMaterial(0, AtmoM);
	}
	if (UMaterialInstanceDynamic* SunM = UMaterialInstanceDynamic::Create(MatUnlit, this))
	{
		SunM->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.85f, 0.5f));
		SunM->SetScalarParameterValue(TEXT("Intensity"), 8.f);
		Sun->SetMaterial(0, SunM);
	}

	SunLight->SetIntensity(4.5f);
	SunLight->SetLightColor(FLinearColor(1.f, 0.97f, 0.92f));
	SunLight->SetWorldRotation(FRotator(-25.f, -60.f, 0.f));
	SunLight->SetCastShadows(true);
	FillLight->SetIntensity(1.2f);
	FillLight->SetLightColor(FLinearColor(0.45f, 0.55f, 1.f));
	FillLight->SetWorldRotation(FRotator(20.f, 120.f, 0.f));
	FillLight->SetCastShadows(false);
	SunLight->SetForwardShadingPriority(1);
	FillLight->SetForwardShadingPriority(0);

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Camera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
	Camera->GetCameraComponent()->SetFieldOfView(50.f);
	Camera->GetCameraComponent()->bConstrainAspectRatio = false;

	PPV = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, SP);
	if (PPV)
	{
		PPV->bUnbound = true;
		FPostProcessSettings& S = PPV->Settings;
		S.bOverride_AutoExposureMethod = true; S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true; S.AutoExposureApplyPhysicalCameraExposure = false;
		S.bOverride_AutoExposureBias = true; S.AutoExposureBias = 0.f;
		S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.6f;
		S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.45f;
		S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0.f;
	}

	// physical sky (Rayleigh + Mie) around the true-scale Earth: blue at the pad, black in orbit, glowing limb from space
	SkyAtmo = NewObject<USkyAtmosphereComponent>(this);
	SkyAtmo->TransformMode = ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform;
	SkyAtmo->BottomRadius = 6368.f;   // the real Earth is flattened: at Mahia (39 S) sea level is 6,369.5 km out; a 6,371 km sphere would bury the pad, a much smaller one gives a thin high-altitude sky at the pad
	SkyAtmo->AtmosphereHeight = 100.f;
	SkyAtmo->SetupAttachment(SceneRoot);
	SkyAtmo->RegisterComponent();
	SkyAtmo->SetVisibility(false);
	SunLight->SetAtmosphereSunLight(true);
	FillLight->SetAtmosphereSunLight(false);   // only the real sun lights the sky (two "suns" gave a twilight sky and dimmed the sunlight)
	// the sky dims sunlight by the air it passes through; measure that at each pixel, not at the world origin
	// (in the Earth-centred views the world origin is the centre of the Earth, under 6,370 km of rock: no light)
	SunLight->bPerPixelAtmosphereTransmittance = true;
	SunLight->MarkRenderStateDirty();

	Lines = NewObject<ULineBatchComponent>(this);
	Lines->SetupAttachment(SceneRoot);
	Lines->RegisterComponent();
	CloudShell = NewObject<UStaticMeshComponent>(this);
	CloudShell->SetupAttachment(SceneRoot);
	CloudShell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CloudShell->CastShadow = false;
	CloudShell->RegisterComponent();
	CloudShell->SetVisibility(false);
	BuildBit();
	ShowTitle();
}

// ------------------------------------------------------------------------------------------------ helpers
UStaticMeshComponent* AISSpaceScene::AddMesh(USceneComponent* Parent, UStaticMesh* Mesh, const FTransform& T, const FLinearColor& Color, bool bUnlit, float Emissive, TArray<UStaticMeshComponent*>* Track)
{
	if (!Mesh) return nullptr;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetupAttachment(Parent ? Parent : SceneRoot);
	C->SetRelativeTransform(T);
	C->RegisterComponent();
	if (Color.A > 0.f)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(bUnlit ? MatUnlit : MatColor, C);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		if (bUnlit) MID->SetScalarParameterValue(TEXT("Intensity"), Emissive > 0 ? Emissive : 2.f);
		else
		{
			MID->SetScalarParameterValue(TEXT("Emissive"), Emissive);
			MID->SetScalarParameterValue(TEXT("Metallic"), 0.3f);
			MID->SetScalarParameterValue(TEXT("Roughness"), 0.45f);
		}
		for (int32 i = 0; i < C->GetNumMaterials(); ++i) C->SetMaterial(i, MID);
		if (bUnlit) C->CastShadow = false;
	}
	if (Track) Track->Add(C); else Extras.Add(C);
	return C;
}

UStaticMeshComponent* AISSpaceScene::AddMeshPath(USceneComponent* Parent, const FString& Path, const FTransform& T, const FLinearColor& Color, TArray<UStaticMeshComponent*>* Track)
{
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Path);
	if (!M) return nullptr;
	const bool bBasic = Path.StartsWith(TEXT("/Engine/"));
	return AddMesh(Parent, M, T, bBasic ? Color : KeepMat, false, 0.f, Track);
}

UTextRenderComponent* AISSpaceScene::AddLabel(const FString& Text, const FVector& Pos, const FColor& Color)
{
	UTextRenderComponent* L = NewObject<UTextRenderComponent>(this);
	L->SetupAttachment(SceneRoot);
	L->SetText(FText::FromString(Text));
	L->SetTextRenderColor(Color);
	L->SetHorizontalAlignment(EHTA_Center);
	L->SetVerticalAlignment(EVRTA_TextBottom);
	L->SetWorldLocation(Pos);
	L->SetCastShadow(false);
	if (UMaterialInterface* TM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/MI_IS_Text.MI_IS_Text"))) L->SetTextMaterial(TM);
	L->RegisterComponent();
	Labels.Add(L);
	return L;
}

UInstancedStaticMeshComponent* AISSpaceScene::AddDots(UStaticMesh* Mesh, const FLinearColor& Color, float Emissive)
{
	UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this);
	I->SetStaticMesh(Mesh);
	I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	I->CastShadow = false;
	I->SetupAttachment(SceneRoot);
	I->RegisterComponent();
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MatUnlit, I);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	MID->SetScalarParameterValue(TEXT("Intensity"), Emissive);
	I->SetMaterial(0, MID);
	DotSets.Add(I);
	return I;
}

void AISSpaceScene::AddOrbitRing(float R, float IncDeg, float RaanDeg, const FLinearColor& Color, int32 N, float DotScale)
{
	UInstancedStaticMeshComponent* I = AddDots(MSphere, Color, 1.5f);
	const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(RaanDeg)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(IncDeg));
	for (int32 i = 0; i < N; ++i)
	{
		const float A = 2.f * PI * i / N;
		I->AddInstance(FTransform(FRotator::ZeroRotator, Q.RotateVector(FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0)), FVector(DotScale)));
	}
}

void AISSpaceScene::SetPlanet(UStaticMeshComponent* C, const TCHAR* Tex, float RadiusUU, const FVector& Pos)
{
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MatPlanet, C);
	if (UTexture* T = LoadObject<UTexture>(nullptr, Tex)) MID->SetTextureParameterValue(TEXT("Tex"), T);
	C->SetMaterial(0, MID);
	C->SetWorldScale3D(FVector(RadiusUU / 50.f));
	C->SetWorldLocation(Pos);
	C->SetWorldRotation(FRotator::ZeroRotator);
	C->SetVisibility(true);
}

void AISSpaceScene::SetEarth(UStaticMeshComponent* C, float RadiusUU, const FVector& Pos, bool bClouds)
{
	if (UStaticMesh* Hi = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi"))) C->SetStaticMesh(Hi);
	UMaterialInterface* EM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Earth.M_IS_Earth"));
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(EM ? EM : MatPlanet, C);
	MID->SetVectorParameterValue(TEXT("SunDir"), FLinearColor(-SunLight->GetForwardVector()));
	C->SetMaterial(0, MID);
	C->SetWorldScale3D(FVector(RadiusUU / 50.f));
	C->SetWorldLocation(Pos);
	C->SetWorldRotation(FRotator(0, 200.f, 0));
	C->SetVisibility(true);
	C->SetCastShadow(false);
	PlanetAtmo->SetCastShadow(false);
	if (UStaticMesh* HiA = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi"))) { PlanetAtmo->SetStaticMesh(HiA); PlanetAtmo->SetMaterial(0, PlanetAtmo->GetMaterial(0)); }
	if (CloudShell && bClouds)
	{
		if (UStaticMesh* Hi = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi"))) CloudShell->SetStaticMesh(Hi);
		if (UMaterialInterface* CM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Clouds.M_IS_Clouds"))) CloudShell->SetMaterial(0, CM);
		CloudShell->SetCastShadow(false);   // a shell around the whole planet would put everything under it in shadow
		CloudShell->SetWorldScale3D(FVector(RadiusUU * 1.004f / 50.f));
		CloudShell->SetWorldLocation(Pos);
		CloudShell->SetVisibility(true);
	}
}

void AISSpaceScene::ClearView()
{
	ClearCosmos();
	bFlight = false;
	if (RocketRoot) RocketRoot->SetVisibility(false, true);
	StageVis.Reset(); BoosterVis.Reset(); FairingVis.Reset(); PlumeCore.Reset(); PlumeBoost.Reset();
	bTrueScale = false;
	bRealEarth = false;
	bGlobe = false;
	if (SunLight) SunLight->SetCastShadows(true);
	if (Earth) Earth->SetVisible(false);
	TruePts.Reset();
	if (Lines) Lines->Flush();
	if (CloudShell) CloudShell->SetVisibility(false);
	Sky->SetWorldScale3D(FVector(40000.f));
	Sky->SetWorldLocation(FVector::ZeroVector);
	Sky->SetVisibility(true);
	for (UStaticMeshComponent* C : SatParts) if (C) C->DestroyComponent();
	for (UStaticMeshComponent* C : RealParts) if (C) C->DestroyComponent();
	for (UStaticMeshComponent* C : Extras) if (C) C->DestroyComponent();
	for (UInstancedStaticMeshComponent* C : DotSets) if (C) C->DestroyComponent();
	for (UTextRenderComponent* C : Labels) if (C) C->DestroyComponent();
	SatParts.Reset(); RealParts.Reset(); Extras.Reset(); DotSets.Reset(); Labels.Reset(); SwarmSats.Reset();
	Swarm = nullptr; PathDots = nullptr;
	Planet->SetVisibility(false); PlanetAtmo->SetVisibility(false); Moon->SetVisibility(false); Sun->SetVisibility(false);
	Sky->SetMaterial(0, MatSky);
	SatRoot->SetRelativeTransform(FTransform::Identity);
	RealRoot->SetRelativeTransform(FTransform::Identity);
	BitRoot->SetVisibility(false, true);
	SunLight->SetWorldRotation(FRotator(-25.f, -60.f, 0.f));
	// voyage: the flight plan survives switching between the mission view and the solar system (a new launch resets it);
	// only the scene objects go
	VoyCamMode = -1;
	BodyC = FollowN = FVector::ZeroVector;
	MoonLabel = nullptr;
	if (SatThruster) { SatThruster->DestroyComponent(); SatThruster = nullptr; }
	if (SepRoot) SepRoot->SetVisibility(false, true);
	RevealT = -1.f;
	for (UStaticMeshComponent* C : VoyMoons) if (C) C->DestroyComponent();
	VoyMoons.Reset();
	bJupiterScene = false;
}

void AISSpaceScene::SetCameraTarget(const FVector& Focus, float Dist, float Yaw, float Pitch, float MinD, float MaxD)
{
	CamFocusGoal = Focus; CamDistGoal = Dist; CamYaw = Yaw; CamPitch = Pitch; CamMin = MinD; CamMax = MaxD;
	CamFocus = Focus; CamDist = Dist;
}

void AISSpaceScene::OrbitCamera(float DYaw, float DPitch)
{
	if (RevealT >= 0.f) return;   // the orbit reveal shot is directed
	if (View == EISView::Orbit && bTrueScale && bFollow)
	{
		FollowYaw += DYaw;
		bFlightCamUser = true;
		FollowPitch = FMath::Clamp(FollowPitch + DPitch, -80.f, 80.f);
		return;
	}
	CamYaw += DYaw;
	CamPitch = FMath::Clamp(CamPitch + DPitch, -85.f, 85.f);
}

void AISSpaceScene::ZoomCamera(float Steps)
{
	if (RevealT >= 0.f) return;
	const float Old = CamDistGoal;
	// one continuous zoom: spacecraft close-up -> whole Earth with the orbit -> (at the far limit) the solar system
	if (View == EISView::Orbit && bTrueScale && bGlobe)
	{
		// globe: zoom on the height above the ground; close enough and you are back on the ground at the site
		const float R = PlanetRUU;
		const float H = FMath::Max(CamDistGoal - R, 1000.f) * FMath::Pow(0.7f, Steps);
		if (Steps > 0 && H < 150000.f * 100.f) { bLand = true; return; }
		CamDistGoal = FMath::Clamp(R + H, CamMin, CamMax);
		const bool bAtMax = Steps < 0 && Old >= CamMax * 0.995f;
		if (bAtMax) { if (++ZoomPastCount >= 2) { ZoomPast = -1; ZoomPastCount = 0; } } else ZoomPastCount = 0;
		return;
	}
	if (View == EISView::Orbit && bTrueScale && !bFlight)
	{
		const float Alt = FMath::Max(1.f, OrbitRadiusUU - PlanetRUU);
		if (bFollow && Steps < 0 && CamDistGoal * FMath::Pow(0.75f, Steps) > FMath::Max(Alt * 0.3f, 5.0e6f))
		{
			bFollow = false;
			CamMin = PlanetRUU * 0.2f;
			CamPitch = -20.f;
			CamYaw = (SatRoot->GetComponentLocation().Rotation().Yaw) + 180.f;   // look at Earth from the satellite's side
			CamDistGoal = OrbitRadiusUU * 1.6f;
			ZoomPastCount = 0;
			return;
		}
		if (!bFollow && Steps > 0 && CamDistGoal * FMath::Pow(0.75f, Steps) < OrbitRadiusUU * 1.12f)
		{
			bFollow = true;
			CamMin = LastSatSize * 1.2f;
			CamDistGoal = LastSatSize * 40.f;
			FollowPitch = -28.f;
			ZoomPastCount = 0;
			return;
		}
	}
	// cosmos zooms in bigger steps so the whole Solar System is a few wheel notches away (Universe Sandbox feel)
	const float Rate = bTrueScale ? 0.75f : (View == EISView::Cosmos ? 0.7f : 0.88f);
	if (View == EISView::Cosmos && CosmosBodies.IsValidIndex(CosmosSel))
	{
		// zoom on the height above the surface, not the distance to the centre: every notch halves the gap, so the
		// camera can skim a planet's surface (or a spacecraft) and still reach the whole system in a few notches
		const float R = CosmosBodies[CosmosSel].Radius;
		const float H = FMath::Max(CamDistGoal - R, R * 0.02f) * FMath::Pow(Rate, Steps);
		CamDistGoal = FMath::Clamp(R + H, CamMin, CamMax);
	}
	else CamDistGoal = FMath::Clamp(CamDistGoal * FMath::Pow(Rate, Steps), CamMin, CamMax);
	// pushing on against a limit (2 notches) asks the UI to move to the next scale: mission <-> solar system
	const bool bAtMax = Steps < 0 && Old >= CamMax * 0.995f, bAtMin = Steps > 0 && Old <= CamMin * 1.005f;
	if (bAtMax || bAtMin)
	{
		const int32 Dir = bAtMax ? -1 : 1;
		ZoomPastCount = (FMath::Sign(ZoomPastCount) == Dir) ? ZoomPastCount + Dir : Dir;
		if (FMath::Abs(ZoomPastCount) >= 2) { ZoomPast = Dir; ZoomPastCount = 0; }
	}
	else ZoomPastCount = 0;
}

UTexture2D* AISSpaceScene::CosmosThumb(int32 I) const
{
	if (!CosmosBodies.IsValidIndex(I)) return nullptr;
	if (CosmosBodies[I].Thumb) return CosmosBodies[I].Thumb;
	if (CosmosBodies[I].Name == TEXT("Earth")) return LoadObject<UTexture2D>(nullptr, TEXT("/Game/Infinisat/Textures/T_EarthDay.T_EarthDay"));
	if (CosmosBodies[I].Parent < 0) return LoadObject<UTexture2D>(nullptr, TEXT("/Game/Infinisat/Textures/T_Sun.T_Sun"));
	return nullptr;
}

// ------------------------------------------------------------------------------------------------ BIT
void AISSpaceScene::BuildBit()
{
	const FLinearColor Shell(0.86f, 0.87f, 0.9f), Dark(0.08f, 0.09f, 0.12f), Band(0.55f, 0.58f, 0.64f);
	TArray<UStaticMeshComponent*>* Keep = nullptr; // BIT parts live in Extras-independent list
	auto Part = [&](USceneComponent* P, UStaticMesh* M, const FTransform& T, const FLinearColor& C, bool bUnlit = false, float E = 0.f) {
		UStaticMeshComponent* X = NewObject<UStaticMeshComponent>(this);
		X->SetStaticMesh(M); X->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		X->SetupAttachment(P); X->SetRelativeTransform(T); X->RegisterComponent();
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(bUnlit ? MatUnlit : MatColor, X);
		MID->SetVectorParameterValue(TEXT("Color"), C);
		if (bUnlit) MID->SetScalarParameterValue(TEXT("Intensity"), E);
		else { MID->SetScalarParameterValue(TEXT("Roughness"), 0.35f); MID->SetScalarParameterValue(TEXT("Metallic"), 0.1f); }
		X->SetMaterial(0, MID);
		return X;
	};
	(void)Keep;
	// body
	Part(BitRoot, MSphere, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.f)), Shell);
	// equator band
	Part(BitRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.035f, 1.035f, 0.13f)), Band);
	// eye: dark socket, glowing iris ring, pupil (faces +X)
	const FRotator Face(90.f, 0.f, 0.f);
	Part(BitRoot, MCyl, FTransform(Face, FVector(44.f, 0, 0), FVector(0.58f, 0.58f, 0.08f)), Dark);
	BitIris = Part(BitRoot, MCyl, FTransform(Face, FVector(48.f, 0, 0), FVector(0.42f, 0.42f, 0.04f)), FLinearColor(0.1f, 0.85f, 1.f), true, 4.f);
	BitIrisMID = Cast<UMaterialInstanceDynamic>(BitIris->GetMaterial(0));
	Part(BitRoot, MCyl, FTransform(Face, FVector(50.5f, 0, 0), FVector(0.17f, 0.17f, 0.03f)), Dark);
	// four panel slashes
	const FVector SlashPos[] = { FVector(30, -28, 26), FVector(30, 28, 26), FVector(30, -28, -26), FVector(30, 28, -26) };
	const float SlashRoll[] = { -30.f, 30.f, 30.f, -30.f };
	for (int32 i = 0; i < 4; ++i)
		Part(BitRoot, MCube, FTransform(FRotator(0, 0, SlashRoll[i]), SlashPos[i] * 1.12f, FVector(0.05f, 0.2f, 0.03f)), Dark);
	// floating hands (palm + 5 fingertip spheres)
	BitHandL = NewObject<USceneComponent>(this); BitHandL->SetupAttachment(BitRoot); BitHandL->RegisterComponent();
	BitHandR = NewObject<USceneComponent>(this); BitHandR->SetupAttachment(BitRoot); BitHandR->RegisterComponent();
	for (USceneComponent* H : { BitHandL, BitHandR })
	{
		Part(H, MSphere, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.3f, 0.3f, 0.26f)), Shell);
		for (int32 f = 0; f < 5; ++f)
		{
			const float A = FMath::DegreesToRadians(-70.f + f * 35.f);
			Part(H, MSphere, FTransform(FRotator::ZeroRotator, FVector(8.f, 24.f * FMath::Sin(A), 22.f * FMath::Cos(A) + 4.f), FVector(0.085f)), Shell);
		}
	}
	BitRoot->SetVisibility(false, true);
}

void AISSpaceScene::SetBitMood(int32 Mood)
{
	BitMood = Mood;
	if (!BitIrisMID) return;
	const FLinearColor C = Mood == 1 ? FLinearColor(0.2f, 1.f, 0.35f) : Mood == 2 ? FLinearColor(1.f, 0.45f, 0.1f) : FLinearColor(0.1f, 0.85f, 1.f);
	BitIrisMID->SetVectorParameterValue(TEXT("Color"), C);
}

void AISSpaceScene::PokeBit() { PokeT = 1.f; }

void AISSpaceScene::SetActive(bool bOn)
{
	SetActorHiddenInGame(!bOn);
	SetActorTickEnabled(bOn);
	SunLight->SetVisibility(bOn);
	FillLight->SetVisibility(bOn);
	if (PPV) PPV->bEnabled = bOn;
}

// ------------------------------------------------------------------------------------------------ satellite assembly
// ------------------------------------------------------------------------------------------------ views
void AISSpaceScene::ShowTitle()
{
	ClearView();
	View = EISView::Title;
	SetEarth(Planet, 6000.f, FVector(0, 0, -6650.f), true);
	PlanetAtmo->SetWorldScale3D(FVector(6150.f / 50.f));
	PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation());
	PlanetAtmo->SetVisibility(true);
	BitRoot->SetVisibility(true, true);
	BitHome = FVector(0, -170, 30); BitBaseScale = 1.1f;
	AddMeshPath(RealRoot, ModelPath(TEXT("Voyager")), FTransform(FRotator(10, 30, 0), FVector(-600, -560, 260), FVector(3.f)), KeepMat, &RealParts);
	SetCameraTarget(FVector(0, 30, 40), 900.f, 180.f, -6.f, 400.f, 3000.f);
}

void AISSpaceScene::BuildCleanRoom()
{
	// Integration hall (ISO 8 clean room) around the satellite. Units: the bus is 100 units wide.
	const FLinearColor Floor(0.8f, 0.82f, 0.85f), WallC(0.93f, 0.94f, 0.95f), Blue(0.0f, 0.33f, 0.62f), Yellow(1.f, 0.78f, 0.05f), Steel(0.62f, 0.64f, 0.67f);
	auto B = [&](const FVector& P, const FVector& S, const FLinearColor& C, float Metal = 0.f, float Rough = 0.5f)
	{
		UStaticMeshComponent* M = AddMesh(SceneRoot, MCube, FTransform(FRotator::ZeroRotator, P, S / 100.f), C, false, 0.f, nullptr);
		if (M) if (UMaterialInstanceDynamic* I = Cast<UMaterialInstanceDynamic>(M->GetMaterial(0))) { I->SetScalarParameterValue(TEXT("Metallic"), Metal); I->SetScalarParameterValue(TEXT("Roughness"), Rough); }
		return M;
	};
	const float HX = 1800.f, HY = 1300.f, Hz = 1000.f, FloorZ = -260.f;
	B(FVector(0, 0, FloorZ - 5.f), FVector(2 * HX, 2 * HY, 10.f), Floor, 0.f, 0.18f);                     // epoxy floor
	for (int32 i = -7; i <= 7; ++i) B(FVector(i * 240.f, 0, FloorZ + 0.3f), FVector(1.5f, 2 * HY, 0.5f), Floor * 0.9f, 0.f, 0.3f); // ESD grid
	for (int32 s : { -1, 1 })
	{
		B(FVector(s * 380.f, 0, FloorZ + 0.6f), FVector(14.f, 760.f, 0.8f), Yellow, 0.f, 0.4f);      // safety box
		B(FVector(0, s * 380.f, FloorZ + 0.6f), FVector(760.f, 14.f, 0.8f), Yellow, 0.f, 0.4f);
		// walls with a blue band
		B(FVector(s * HX, 0, FloorZ + Hz * 0.5f), FVector(20.f, 2 * HY, Hz), WallC, 0.f, 0.6f);
		B(FVector(0, s * HY, FloorZ + Hz * 0.5f), FVector(2 * HX, 20.f, Hz), WallC, 0.f, 0.6f);
		B(FVector(s * (HX - 11.f), 0, FloorZ + 260.f), FVector(2.f, 2 * HY, 40.f), Blue, 0.f, 0.4f);
		B(FVector(0, s * (HY - 11.f), FloorZ + 260.f), FVector(2 * HX, 2.f, 40.f), Blue, 0.f, 0.4f);
		// gantry crane rails
		B(FVector(0, s * 900.f, FloorZ + Hz - 120.f), FVector(2 * HX - 40.f, 30.f, 40.f), Yellow * 0.9f, 0.3f, 0.5f);
	}
	B(FVector(0, 0, FloorZ + Hz + 10.f), FVector(2 * HX, 2 * HY, 20.f), WallC * 0.95f, 0.f, 0.8f);           // ceiling
	// bridge crane + hoist above the stand
	B(FVector(-250.f, 0, FloorZ + Hz - 90.f), FVector(60.f, 1860.f, 50.f), Yellow, 0.3f, 0.45f);
	B(FVector(-250.f, 0, FloorZ + Hz - 140.f), FVector(70.f, 90.f, 60.f), FLinearColor(0.2f, 0.2f, 0.22f), 0.5f, 0.5f);
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(-250.f, 0, FloorZ + Hz - 320.f), FVector(0.03f, 0.03f, 3.2f)), FLinearColor(0.15f, 0.15f, 0.15f), false, 0.f, nullptr);
	// ceiling light panels + HEPA grilles
	for (int32 ix = -3; ix <= 3; ++ix) for (int32 iy = -2; iy <= 2; ++iy)
	{
		const bool bLight = (ix + iy) % 2 == 0;
		AddMesh(SceneRoot, MCube, FTransform(FRotator::ZeroRotator, FVector(ix * 480.f, iy * 480.f, FloorZ + Hz - 1.f), FVector(2.4f, 1.2f, 0.02f)),
			bLight ? FLinearColor(1.f, 0.98f, 0.95f) : FLinearColor(0.7f, 0.72f, 0.75f), bLight, bLight ? 3.f : 0.f, nullptr);
	}
	// integration stand: base, column, rotating table, adapter ring
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, FloorZ + 15.f), FVector(2.2f, 2.2f, 0.3f)), Steel, false, 0.f, nullptr);
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, FloorZ + 100.f), FVector(0.5f, 0.5f, 1.7f)), Steel, false, 0.f, nullptr);
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, -120.f), FVector(1.4f, 1.4f, 0.12f)), FLinearColor(0.25f, 0.27f, 0.3f), false, 0.f, nullptr);
	// equipment: test racks with status LEDs, workstations, parts store doors
	for (int32 k = 0; k < 4; ++k)
	{
		const FVector R(-HX + 60.f, -700.f + k * 140.f, FloorZ + 110.f);
		B(R, FVector(80.f, 110.f, 220.f), FLinearColor(0.12f, 0.13f, 0.15f), 0.4f, 0.5f);
		for (int32 l = 0; l < 6; ++l)
			AddMesh(SceneRoot, MCube, FTransform(FRotator::ZeroRotator, R + FVector(41.f, -30.f + (l % 3) * 30.f, 60.f - (l / 3) * 30.f), FVector(0.02f, 0.06f, 0.04f)),
				(l + k) % 3 ? FLinearColor(0.1f, 1.f, 0.3f) : FLinearColor(1.f, 0.6f, 0.1f), true, 6.f, nullptr);
	}
	for (int32 k = 0; k < 3; ++k)
	{
		const FVector Dsk(600.f + k * 320.f, -HY + 120.f, FloorZ + 45.f);
		B(Dsk, FVector(220.f, 90.f, 90.f), WallC * 0.9f, 0.f, 0.5f);
		AddMesh(SceneRoot, MCube, FTransform(FRotator(0, 0, 0), Dsk + FVector(0, -25.f, 85.f), FVector(1.1f, 0.04f, 0.6f)), FLinearColor(0.15f, 0.45f, 0.75f), true, 1.2f, nullptr);
	}
	const TCHAR* Stores[] = { TEXT("OPTICS & CAMERAS"), TEXT("AVIONICS & PROCESSORS"), TEXT("POWER & BATTERIES"), TEXT("COMMS & ANTENNAS"), TEXT("PROPULSION") };
	for (int32 k = 0; k < 5; ++k)
	{
		const float X = -1300.f + k * 650.f;
		B(FVector(X, HY - 12.f, FloorZ + 150.f), FVector(240.f, 6.f, 300.f), FLinearColor(0.25f, 0.3f, 0.36f), 0.6f, 0.35f);
		AddLabel(Stores[k], FVector(X, HY - 30.f, FloorZ + 340.f), FColor(30, 60, 110));
	}
	AddLabel(TEXT("INTEGRATION HALL  -  CLEAN ROOM ISO 8"), FVector(HX - 30.f, 0, FloorZ + 520.f), FColor(30, 60, 110));
	// the hall must not shadow the key light: only the spacecraft casts shadows (onto the floor)
	for (UStaticMeshComponent* C : Extras) if (C) { C->SetCastShadow(false); }
}

void AISSpaceScene::ShowWorkshop(const FISDesign& D)
{
	ClearView();
	View = EISView::Workshop;
	Sky->SetVisibility(false);
	BuildCleanRoom();
	BuildSatellite(SatRoot, D, SatParts);
	BitRoot->SetVisibility(false, true);   // you are BIT: no robot in your own view
	SetCameraTarget(FVector(0, -60, 30), 640.f, 215.f, -16.f, 220.f, 2400.f);   // close: the satellite fills the space right of the catalogue
}

void AISSpaceScene::ShowCompare(const FISDesign& D, const FString& RealMesh, const FString& RealName)
{
	ClearView();
	View = EISView::Compare;
	SetEarth(Planet, 6000.f, FVector(-1500, 0, -7000.f), true);
	PlanetAtmo->SetWorldScale3D(FVector(6150.f / 50.f));
	PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation());
	PlanetAtmo->SetVisibility(true);
	const bool bReal = !RealMesh.IsEmpty();
	SatRoot->SetRelativeLocation(FVector(0, bReal ? -300.f : 0.f, 0));
	SatRoot->SetRelativeScale3D(FVector(0.6f));
	BuildSatellite(SatRoot, D, SatParts);
	AddLabel(TEXT("YOUR DESIGN"), FVector(0, bReal ? -300.f : 0.f, 190.f), FColor(120, 220, 255));
	if (bReal)
	{
		AddMeshPath(RealRoot, RealMesh, FTransform(FRotator::ZeroRotator, FVector(0, 300, 0), FVector(2.8f)), KeepMat, &RealParts);
		AddLabel(RealName + TEXT(" (NASA model)"), FVector(0, 300, 190.f), FColor(255, 210, 120));
	}
	SetCameraTarget(FVector(0, 0, 20), 1900.f, 180.f, -8.f, 300.f, 5000.f);
}

void AISSpaceScene::ShowModelOnly(const FString& MeshPath, const FString& Label)
{
	ClearView();
	View = EISView::Compare;
	SetEarth(Planet, 6000.f, FVector(-1500, 0, -7000.f), true);
	PlanetAtmo->SetWorldScale3D(FVector(6150.f / 50.f));
	PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation());
	PlanetAtmo->SetVisibility(true);
	if (!MeshPath.IsEmpty()) AddMeshPath(RealRoot, MeshPath, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(3.5f)), FLinearColor(0.8f, 0.8f, 0.8f), &RealParts);
	if (!Label.IsEmpty()) AddLabel(Label, FVector(0, 0, 210.f), FColor(255, 210, 120));
	SetCameraTarget(FVector(0, 0, 20), 900.f, 180.f, -8.f, 250.f, 4000.f);
}

void AISSpaceScene::BuildEarthPopulation(float K)
{
	FRandomStream R(1972);
	// GNSS constellations in MEO: GPS-like 6 planes x 4, Galileo-like 3 planes x 8
	{
		UInstancedStaticMeshComponent* I = AddDots(MCube, FLinearColor(1.f, 0.85f, 0.3f), 3.f);
		for (int32 p = 0; p < 6; ++p) for (int32 s = 0; s < 4; ++s)
		{
			const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(p * 60.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(55.f));
			const float A = FMath::DegreesToRadians(s * 90.f + p * 15.f);
			I->AddInstance(FTransform(FRotator::ZeroRotator, Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * 26571.f * K), FVector(2.2f)));
		}
		for (int32 p = 0; p < 3; ++p) for (int32 s = 0; s < 8; ++s)
		{
			const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(p * 120.f + 30.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(56.f));
			const float A = FMath::DegreesToRadians(s * 45.f);
			I->AddInstance(FTransform(FRotator::ZeroRotator, Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * 29600.f * K), FVector(2.2f)));
		}
		AddLabel(TEXT("GNSS: GPS 20,200 km / Galileo 23,222 km"), FVector(0, 0, 27500.f * K), FColor(255, 215, 80));
	}
	// GEO belt
	{
		UInstancedStaticMeshComponent* I = AddDots(MCube, FLinearColor(0.9f, 0.9f, 1.f), 2.5f);
		for (int32 i = 0; i < 400; ++i)
		{
			const float A = R.FRandRange(0.f, 2.f * PI);
			I->AddInstance(FTransform(FRotator::ZeroRotator, FVector(FMath::Cos(A), FMath::Sin(A), R.FRandRange(-0.01f, 0.01f)) * 42164.f * K, FVector(1.8f)));
		}
		AddLabel(TEXT("GEO belt 35,786 km: weather & TV satellites"), FVector(42164.f * K, 0, 2500.f * K), FColor(230, 230, 255));
		AddMeshPath(RealRoot, ModelPath(TEXT("GOES")), FTransform(FRotator::ZeroRotator, FVector(0, 42164.f * K, 0), FVector(14.f)), KeepMat, &RealParts);
		AddLabel(TEXT("GOES (weather, GEO)"), FVector(0, 42164.f * K, 1300.f), FColor(255, 210, 120));
		AddMeshPath(RealRoot, ModelPath(TEXT("TDRS")), FTransform(FRotator::ZeroRotator, FVector(-42164.f * K * 0.7f, -42164.f * K * 0.7f, 0), FVector(14.f)), KeepMat, &RealParts);
		AddLabel(TEXT("TDRS relay (GEO)"), FVector(-42164.f * K * 0.7f, -42164.f * K * 0.7f, 1300.f), FColor(255, 210, 120));
	}
	// LEO mega-constellation shells (animated)
	{
		Swarm = AddDots(MCube, FLinearColor(0.45f, 0.75f, 1.f), 2.f);
		const float Incs[] = { 43.f, 53.f, 70.f, 97.f };
		for (int32 i = 0; i < 1600; ++i)
		{
			FSwarmSat S;
			S.R = (6371.f + R.FRandRange(530.f, 570.f)) * K;
			S.Inc = Incs[R.RandRange(0, 3)];
			S.Raan = R.FRandRange(0.f, 360.f);
			S.Phase = R.FRandRange(0.f, 2.f * PI);
			S.Rate = 2.f * PI / 20.f;
			SwarmSats.Add(S);
			Swarm->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.45f)));
		}
		AddLabel(TEXT("LEO: 11,000+ Starlink sats (43/53/70/97 deg shells)"), FVector(0, -7600.f * K, 1500.f * K), FColor(130, 200, 255));
		AddMeshPath(RealRoot, ModelPath(TEXT("ISS")), FTransform(FRotator::ZeroRotator, FVector(6790.f * K, 0, 0), FVector(6.f)), KeepMat, &RealParts);
		AddLabel(TEXT("ISS 420 km"), FVector(6790.f * K, 0, 500.f), FColor(255, 210, 120));
	}
}

void AISSpaceScene::ShowOrbit(EISDest Dest, float AltKm, float IncDeg, const FISDesign& D, int32 Fleet)
{
	ClearView();
	View = EISView::Orbit;
	ViewDest = Dest;
	const FLinearColor Mine(1.f, 0.55f, 0.15f);
	float SatSize = 400.f;

	if (Dest == EISDest::Backyard)
	{
		ViewK = 1.f;
		UMaterialInstanceDynamic* SkyM = UMaterialInstanceDynamic::Create(MatUnlit, Sky);
		SkyM->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.5f, 0.9f));
		SkyM->SetScalarParameterValue(TEXT("Intensity"), 0.6f);
		Sky->SetMaterial(0, SkyM);
		const float Top = (AltKm > 5 ? 30.f : 1.f) * 1000.f;
		AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, -50.f), FVector(800.f, 800.f, 1.f)), FLinearColor(0.2f, 0.45f, 0.15f), false, 0.f, nullptr);
		PathDots = AddDots(MSphere, Mine, 2.f);
		for (int32 i = 0; i <= 40; ++i) PathDots->AddInstance(FTransform(FRotator::ZeroRotator, FVector(i * Top * 0.004f, 0, Top * i / 40.f), FVector(Top / 4000.f)));
		AddLabel(FString::Printf(TEXT("Apogee %s km"), *ISSim::Fmt(Top / 1000.f, 0)), FVector(0, 0, Top * 1.03f), FColor::White);
		OrbitRadiusUU = Top;
		SatSize = FMath::Max(60.f, Top * 0.06f);
		BitRoot->SetVisibility(true, true);
		BitHome = FVector(Top * 0.12f, -Top * 0.1f, 40.f); BitBaseScale = FMath::Max(0.6f, Top / 2500.f);
		SetCameraTarget(FVector(0, 0, Top * 0.45f), Top * 2.2f, 200.f, -4.f, Top * 0.3f, Top * 8.f);
	}
	else if (ISSim::IsDeepSpace(Dest) && Voy.bValid && VoyagePhase() == 2)
	{
		// arrival: the view moves to the destination (Jupiter, true scale) for the insertion burn and the science orbit
		bVoyage = true;
		VoyAltKm = AltKm;
		BuildJupiterScene();
	}
	else
	{
		// TRUE SCALE: 1 unit = 1 cm, real planet sizes and orbit radii (Unreal large-world coordinates). Deep-space missions
		// start here too: they launch from Earth like any other; ISVoyage.cpp then flies the real transfer.
		bVoyage = ISSim::IsDeepSpace(Dest);
		VoyAltKm = AltKm;
		bTrueScale = true;
		ViewK = 1.0e5f;                                  // uu per km
		const double K = ViewK;
		const double Rb = 6371.0;
		const double Mu = 398600.4418;
		const double Rorb = Rb + (bVoyage ? 250.0 : AltKm);   // deep space: the parking orbit
		PeriodS = 2.0 * PI * FMath::Sqrt(Rorb * Rorb * Rorb / Mu);
		Sky->SetWorldScale3D(FVector(bVoyage ? 1.0e10f : 1.0e9f));   // star sphere follows the camera: radius 500,000 km (5 million on voyages, the Moon must fit inside)
		{
			SetEarth(Planet, (float)(Rb * K), FVector::ZeroVector, true);
			if (Earth && Earth->IsReady())
			{
				// the real globe (terrain + imagery) replaces the textured sphere: same size, same place, real surface
				Earth->UseEarthOrigin();
				Earth->SetVisible(true);
				bRealEarth = true;
			}
			PlanetAtmo->SetWorldScale3D(FVector((float)(Rb * 1.015 * K / 50.0))); PlanetAtmo->SetWorldLocation(FVector::ZeroVector); PlanetAtmo->SetVisibility(true);
			if (bRealEarth)
			{
				Planet->SetVisibility(false); PlanetAtmo->SetVisibility(false);
				if (CloudShell) CloudShell->SetVisibility(false);   // the real imagery; a painted cloud sphere would cover it
				// 6,371 km from the origin the sun's shadow maps lose precision and put everything in shadow: no sun shadows here
				SunLight->SetCastShadows(false);
				// the same time of day as the walking world: mid-morning at Mahia, the sun 50 deg up in the north-east
				const FVector ToSun = (Earth->SiteUpEarthFrame() * 0.77f + Earth->SiteEastEarthFrame() * 0.5f + Earth->SiteNorthEarthFrame() * 0.4f).GetSafeNormal();
				SunLight->SetWorldRotation((-ToSun).Rotation());
			}
			SetPlanet(Moon, TEX_MOON, (float)(1737.4 * K), FVector(0, 384400.0 * K, 0));
			if (UStaticMesh* Hi = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi"))) Moon->SetStaticMesh(Hi);   // seen up close on lunar missions
			// real populations as screen-size points: GPS + Galileo (MEO), GEO belt, Starlink-like LEO shells, ISS
			FRandomStream R(1972);
			auto Add = [this](double Rkm, double Inc, double Raan, double Ph, double MuL, FColor C, float Sz)
			{
				FTruePt P; P.R = Rkm * 1.0e5; P.Inc = Inc; P.Raan = Raan; P.Phase = Ph; P.Rate = FMath::Sqrt(MuL / (Rkm * Rkm * Rkm)); P.C = C; P.Size = Sz; TruePts.Add(P);
			};
			for (int32 p = 0; p < 6; ++p) for (int32 k = 0; k < 4; ++k) Add(26571.0, 55.0, p * 60.0, FMath::DegreesToRadians(k * 90.0 + p * 15.0), Mu, FColor(255, 215, 80), 5.f);
			for (int32 p = 0; p < 3; ++p) for (int32 k = 0; k < 8; ++k) Add(29600.0, 56.0, p * 120.0 + 30.0, FMath::DegreesToRadians(k * 45.0), Mu, FColor(255, 190, 60), 5.f);
			for (int32 i = 0; i < 400; ++i) Add(42164.0, R.FRandRange(-0.1f, 0.1f), 0.0, R.FRandRange(0.f, 2.f * PI), Mu, FColor(230, 230, 255), 3.f);
			const double Incs[] = { 43.0, 53.0, 70.0, 97.6 };
			for (int32 i = 0; i < 1800; ++i) Add(6371.0 + R.FRandRange(530.f, 570.f), Incs[R.RandRange(0, 3)], R.FRandRange(0.f, 360.f), R.FRandRange(0.f, 2.f * PI), Mu, FColor(120, 190, 255), 2.f);
			Add(6371.0 + 420.0, 51.6, 40.0, 0.5, Mu, FColor(255, 255, 255), 7.f);
			AddLabel(TEXT("GNSS: GPS 20,200 km / Galileo 23,222 km"), FVector(0, 0, 27500.0 * K), FColor(255, 215, 80));
			AddLabel(TEXT("GEO belt 35,786 km"), FVector(42164.0 * K, 0, 3000.0 * K), FColor(230, 230, 255));
			AddLabel(TEXT("LEO: 11,000+ Starlink satellites"), FVector(0, -7800.0 * K, 2000.0 * K), FColor(130, 200, 255));
			MoonLabel = AddLabel(TEXT("Moon"), FVector(0, 384400.0 * K, 4000.0 * K), FColor::White);
		}
		OrbitRadiusUU = (float)(Rorb * K);
		OrbitInc = IncDeg;
		PlanetRUU = (float)(Rb * K);
		const double Dist = Rorb * K * 3.0;
		SetCameraTarget(FVector::ZeroVector, (float)Dist, 210.f, -22.f, 50.f, (float)(Rorb * K * 40.0));
	}

	// the player's spacecraft, scaled up so it is visible (not to scale!)
	BuildSatellite(SatRoot, D, SatParts);
	if (bTrueScale)
	{
		// real size of the bus (the model's bus is 100 units across)
		RealSatCm = 150.f;
		for (const FName& Id : D.Parts)
		{
			const FString S = Id.ToString();
			if (S == TEXT("bus_can")) RealSatCm = 6.6f; else if (S == TEXT("bus_1u")) RealSatCm = 10.f; else if (S == TEXT("bus_3u") || S == TEXT("bus_6u")) RealSatCm = 10.f;
			else if (S == TEXT("bus_small")) RealSatCm = 70.f; else if (S == TEXT("bus_med")) RealSatCm = 150.f; else if (S == TEXT("bus_geo")) RealSatCm = 250.f;
			else if (S == TEXT("bus_deep")) RealSatCm = 250.f; else if (S == TEXT("bus_ultra")) RealSatCm = 50.f;
		}
		SatRoot->SetRelativeScale3D(FVector(RealSatCm / 100.f));
		SatSize = RealSatCm * 1.6f;
	}
	else
	{
		SatRoot->SetRelativeScale3D(FVector(SatSize / 150.f));
		AddMesh(SatRoot, MSphere, FTransform(FRotator::ZeroRotator, FVector(0, 0, 120.f), FVector(0.3f)), FLinearColor(1.f, 0.55f, 0.15f, 1.f), true, 6.f, &SatParts);
	}
	if (!bVoyage) SetMissionTime(0.0, 1.0, 100.0, true);   // a voyage keeps its mission clock across view changes
	LastSatSize = SatSize;
	MapDist = CamDistGoal;
	if (!bTrueScale) PlanetRUU = Dest == EISDest::Moon ? 1737.4f : 6371.f;
	const bool bNear = Dest != EISDest::Backyard;
	bFollow = bNear;
	if (bFollow) { FollowPitch = -28.f; FollowYaw = 25.f; CamMin = SatSize * 1.2f; CamDistGoal = SatSize * 5.f; CamDist = CamDistGoal; CamPitch = -12.f; CamFocus = CamFocusGoal = SatRoot->GetComponentLocation(); }
}

void AISSpaceScene::ToggleMap()
{
	if (View != EISView::Orbit) return;
	bFollow = !bFollow;
	CamDistGoal = bFollow ? LastSatSize * 5.f : MapDist;
	CamMin = bFollow ? LastSatSize * 1.2f : (bTrueScale ? PlanetRUU * 0.2f : 2000.f);
}

FString AISSpaceScene::LaunchCaption() const
{
	if (bFlight) return FString();   // the flight HUD shows the real numbers
	if (LaunchT < 0.f) return FString();
	if (LaunchT < 0.12f) return TEXT("LIFT-OFF!  The engines push about 1.3x the rocket's own weight.");
	if (LaunchT < 0.30f) return TEXT("GRAVITY TURN  -  tilting east to build sideways speed: low orbit needs about 7.8 km/s.");
	if (LaunchT < 0.45f) return TEXT("MAX-Q  -  the moment of maximum air pressure on the rocket.");
	if (LaunchT < 0.60f) return TEXT("STAGE SEPARATION  -  dropping empty tanks saves mass (the rocket equation!).");
	if (LaunchT < 0.75f) return TEXT("FAIRING SEPARATION  -  above the atmosphere the nose cone is no longer needed.");
	return TEXT("ORBIT INSERTION  -  engine cut-off... spacecraft separation!");
}

void AISSpaceScene::StartLaunch(bool bHeavy)
{
	if (View != EISView::Orbit || ViewDest == EISDest::Backyard || ViewDest == EISDest::Jupiter || ViewDest == EISDest::Interstellar || ViewDest == EISDest::Moon) return;
	if (!RocketRoot)
	{
		RocketRoot = NewObject<USceneComponent>(this);
		RocketRoot->SetupAttachment(SceneRoot);
		RocketRoot->RegisterComponent();
	}
	RocketRoot->SetVisibility(true, true);
	const float Size = LastSatSize * 1.6f;
	UStaticMesh* RM = LoadObject<UStaticMesh>(nullptr, *ModelPath(bHeavy ? TEXT("SaturnV") : TEXT("AresI")));
	UStaticMeshComponent* Body = RM ? AddMesh(RocketRoot, RM, FTransform::Identity, KeepMat, false, 0.f, nullptr)
		: AddMesh(RocketRoot, MCyl, FTransform::Identity, FLinearColor(0.95f, 0.95f, 0.95f), false, 0.f, nullptr);
	// imported models have arbitrary axes/origins: stand the long axis up (+Z = flight direction), base at the origin
	float BaseR = Size * 0.06f;
	if (Body && Body->GetStaticMesh())
	{
		const FBox LB = Body->GetStaticMesh()->GetBoundingBox();
		const FVector Ext = LB.GetSize();
		const FRotator Rot = (Ext.X >= Ext.Y && Ext.X >= Ext.Z) ? FRotator(90.f, 0.f, 0.f) : (Ext.Y >= Ext.Z ? FRotator(0.f, 0.f, -90.f) : FRotator::ZeroRotator);
		const float K = Size / FMath::Max(1.f, (float)Ext.GetMax());
		const FVector Sc = RM ? FVector(K) : FVector(Size / 900.f, Size / 900.f, Size / 100.f);
		const FBox B = LB.TransformBy(FTransform(Rot, FVector::ZeroVector, Sc));
		Body->SetRelativeTransform(FTransform(Rot, FVector(-B.GetCenter().X, -B.GetCenter().Y, -B.Min.Z), Sc));
		BaseR = FMath::Max(Size * 0.03f, (float)FMath::Min(B.GetSize().X, B.GetSize().Y) * 0.3f);
	}
	// exhaust plume at the nozzles: white-hot core inside a longer orange glow (additive, blooms)
	if (!MatGlow) MatGlow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Glow.M_IS_Glow"));
	UTexture2D* White = CosmosTex(TEXT("white"), 4, 4, [](float, float) { return FLinearColor::White; });
	auto Plume = [&](float Len, float R, const FLinearColor& C, float I)
	{
		UStaticMeshComponent* P = AddMesh(RocketRoot, MCone, FTransform(FRotator(180, 0, 0), FVector(0, 0, -Len * 0.5f), FVector(R / 50.f, R / 50.f, Len / 100.f)), FLinearColor(0, 0, 0, 0), true, 0.f, nullptr);
		if (P && MatGlow)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatGlow, P);
			M->SetTextureParameterValue(TEXT("Tex"), White);
			M->SetVectorParameterValue(TEXT("Color"), C);
			M->SetScalarParameterValue(TEXT("Intensity"), I);
			P->SetMaterial(0, M);
			P->CastShadow = false;
		}
	};
	Plume(Size * 0.55f, BaseR * 1.5f, FLinearColor(1.f, 0.45f, 0.1f), 2.5f);
	Plume(Size * 0.22f, BaseR * 0.8f, FLinearColor(1.f, 0.9f, 0.7f), 6.f);
	const FVector Sat = SatRoot->GetRelativeLocation();
	// launch from the sunlit side of Earth
	const FVector ToSun = -SunLight->GetForwardVector();
	FVector Dir = FQuat(FVector::UpVector, FMath::DegreesToRadians(-50.f)).RotateVector(Sat.GetSafeNormal());
	if (FVector::DotProduct(Dir, ToSun) < 0.f) Dir = FQuat(FVector::UpVector, FMath::DegreesToRadians(50.f)).RotateVector(Sat.GetSafeNormal());
	if (FVector::DotProduct(Dir, ToSun) < 0.f) Dir = (Dir + ToSun).GetSafeNormal();
	LaunchSite = Dir * PlanetRUU;
	RocketPrev = LaunchSite;
	RocketRoot->SetRelativeLocation(LaunchSite);
	LaunchT = 0.f;
	SatRoot->SetVisibility(false, true);
	bFollow = true;
	CamDistGoal = Size * 4.f; CamDist = CamDistGoal; CamPitch = -8.f;
}

void AISSpaceScene::SetMissionTime(double Day, double EndDay, double PeriodMin, bool bAlive)
{
	VoyEndDay = EndDay;
	if (bVoyage) { MissionDay = Day; return; }   // deep space: TickVoyage / the Cosmos view fly the real trajectory
	if (View != EISView::Orbit || !bAlive) return;
	FVector Pos = FVector::ZeroVector;
	const float Ang = (float)(SimTime * 2.0 * PI / 14.0);
	switch (ViewDest)
	{
	case EISDest::Backyard:
	{
		const double Land = FMath::Max(0.01, EndDay - 2.0 / 24.0);
		const double T = Day / Land;
		const double H = T < 0.12 ? T / 0.12 : FMath::Max(0.0, 1.0 - (T - 0.12) / 0.88);
		Pos = FVector(FMath::Min(T, 1.0) * OrbitRadiusUU * 0.16, 0, H * OrbitRadiusUU);
		break;
	}
	default:
	{
		const FQuat Q = OrbitQuat();
		const float A2 = bTrueScale ? (float)(2.0 * PI * FMath::Fmod(Day * 86400.0 / FMath::Max(1.0, PeriodS), 1.0)) : Ang;
		Pos = Q.RotateVector(FVector(FMath::Cos(A2), FMath::Sin(A2), 0) * OrbitRadiusUU);
		break;
	}
	}
	SatRoot->SetRelativeLocation(Pos);
	SatRoot->SetRelativeRotation((-Pos).Rotation());
	MissionDay = Day;
	if (ViewDest == EISDest::Backyard) BitRoot->SetWorldLocation(BitHome);
}

// ------------------------------------------------------------------------------------------------ tick
void AISSpaceScene::Tick(float Dt)
{
	Super::Tick(Dt);
	SimTime += Dt;
	if (bVoyage && !bFlight && View == EISView::Orbit) TickVoyage(Dt);   // places the craft, the Moon and the camera goals

	// camera
	CamDist = FMath::FInterpTo(CamDist, CamDistGoal, Dt, 6.f);
	CamFocus = FMath::VInterpTo(CamFocus, CamFocusGoal, Dt, 4.f);
	// launch sequence: the rocket climbs from the pad to the spacecraft's orbit
	if (bFlight && RocketRoot) { TickFlight(Dt); CamFocusGoal = RocketRoot->GetComponentLocation(); }
	else if (LaunchT >= 0.f && RocketRoot)
	{
		LaunchT = FMath::Min(1.f, LaunchT + Dt / 11.f);
		const FVector Target = SatRoot->GetRelativeLocation();
		const float H = FMath::Pow(LaunchT, 1.4f);
		const float S = FMath::SmoothStep(0.12f, 1.f, LaunchT);
		const FVector D = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(LaunchSite.GetSafeNormal(), Target.GetSafeNormal()), S).RotateVector(LaunchSite.GetSafeNormal());
		const FVector Pos = D * FMath::Lerp(PlanetRUU, Target.Size(), H);
		const FVector Vel = Pos - RocketPrev;
		if (Vel.SizeSquared() > 1.f) RocketRoot->SetRelativeRotation(FRotationMatrix::MakeFromZ(Vel.GetSafeNormal()).Rotator());
		RocketRoot->SetRelativeLocation(Pos);
		RocketPrev = Pos;
		CamFocusGoal = RocketRoot->GetComponentLocation();
		if (LaunchT >= 1.f)
		{
			LaunchT = -1.f;
			RocketRoot->SetVisibility(false, true);
			SatRoot->SetVisibility(true, true);
			CamDistGoal = LastSatSize * 5.f; FollowPitch = -28.f; FollowYaw = 25.f;
		}
	}
	else if (bVoyage && !bFollow) CamFocusGoal = VoyFocus;   // voyage map: Earth, Moon and the path in one frame
	else if (View == EISView::Orbit && bFollow && ViewDest != EISDest::Backyard)
		CamFocusGoal = SatRoot->GetComponentLocation();
	else if (View == EISView::Orbit && !bGlobe && ViewDest != EISDest::Backyard && ViewDest != EISDest::Interstellar && ViewDest != EISDest::Jupiter && CamDistGoal < OrbitRadiusUU * 0.9f)
		CamFocusGoal = SatRoot->GetComponentLocation();
	else if (View == EISView::Orbit && ViewDest != EISDest::Backyard) CamFocusGoal = FVector::ZeroVector;
	FRotator CR(CamPitch, CamYaw, 0.f);
	FVector CamLoc = CamFocus - CR.Vector() * CamDist;
	if (View == EISView::Orbit && bTrueScale && bFollow)
	{
		// follow camera in the satellite's local frame: behind it along its path, Earth's horizon below
		FVector SatPos = SepT >= 0.f && SepRoot ? SepRoot->GetComponentLocation()   // separating payload
			: LaunchT >= 0.f && RocketRoot ? RocketRoot->GetComponentLocation() + (bFlight ? RocketRoot->GetUpVector() * RocketLenCm * 0.45f : FVector::ZeroVector) : SatRoot->GetComponentLocation();
		SatPos += bVoyage ? VoyCamOff : FVector::ZeroVector;   // the stack shot frames stage + satellite
		const FVector Up = (SatPos - BodyC).GetSafeNormal();   // "down" is the body it orbits now (Earth, or the Moon on arrival)
		const FQuat Q = OrbitQuat();
		const FVector N = FollowN.IsNearlyZero() ? Q.RotateVector(FVector::UpVector) : FollowN;
		const FVector Fwd = FVector::CrossProduct(N, Up).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Up, Fwd);
		const FVector L = FRotator(FollowPitch, FollowYaw, 0.f).Vector();
		const FVector Dir = (Fwd * L.X + Right * L.Y + Up * L.Z).GetSafeNormal();
		CamLoc = SatPos - Dir * CamDist;
		CR = FRotationMatrix::MakeFromXZ(Dir, Up).Rotator();
		if (RevealT >= 0.f && bFlight)
		{
			// orbit reached: fly back from the rocket until the whole planet and the new orbit fill the screen (6 s), hold,
			// turn to the satellite, then hand over to the follow camera at exactly this pose - no cut anywhere
			RevealT += Dt;
			const FVector Earth = SceneRoot->GetComponentLocation();
			const FVector Out = (Up * 0.75f + N * 0.66f).GetSafeNormal();   // final camera side: above the orbit plane, over the rocket
			const float S = FMath::SmoothStep(0.f, 1.f, RevealT / 6.f);
			const float D0 = FMath::Max(CamDist, 1.f), D = FMath::Exp(FMath::Lerp(FMath::Loge(D0), FMath::Loge(FMath::Max(MapDist, D0)), S));   // log zoom: same feel at every scale
			const float W = FMath::Clamp((D - D0) / FMath::Max(MapDist - D0, 1.f), 0.f, 1.f);   // focus leaves the rocket only once far out
			const FVector Look = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Dir, -Out), S).RotateVector(Dir);
			CamLoc = FMath::Lerp(SatPos, Earth, W) - Look * D;
			// deep space: the satellite is still on the upper stage, so the shot ends on the stack
			const FVector SatW = bVoyage && RocketRoot ? RocketRoot->GetComponentLocation() + RocketRoot->GetUpVector() * ((StageBaseZ.Num() ? StageBaseZ.Last() * 100.f : 0.f) + RocketLenCm) * 0.5f : SatRoot->GetComponentLocation();
			const float Turn = FMath::SmoothStep(7.5f, 9.f, RevealT);
			const FVector Aim = FMath::Lerp(Look, (SatW - CamLoc).GetSafeNormal(), Turn).GetSafeNormal();
			CR = FRotationMatrix::MakeFromXZ(Aim, FMath::Lerp(Up, N, S).GetSafeNormal()).Rotator();
			if (RevealT >= 9.f)
			{
				EndFlight();
				// express the current view in the satellite's follow frame so the next frame starts exactly here
				const FVector SUp = (SatW - Earth).GetSafeNormal();
				const FVector SFwd = FVector::CrossProduct(N, SUp).GetSafeNormal(), SRight = FVector::CrossProduct(SUp, SFwd);
				const FVector V = SatW - CamLoc, VD = V.GetSafeNormal();
				FollowPitch = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FVector::DotProduct(VD, SUp), -1.0, 1.0)));
				FollowYaw = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(VD, SRight), FVector::DotProduct(VD, SFwd)));
				CamDist = V.Size();   // EndFlight set CamDistGoal to the satellite close-up: FInterpTo glides in from here
			}
		}
	}
	if (bFlight && FlightShake > 0.01f)
	{
		// camera shake: felt strongest close to the rocket
		const float T = (float)FPlatformTime::Seconds(), K = FlightShake * FMath::Clamp(3000.f / FMath::Max(300.f, CamDist), 0.15f, 1.f);
		CR.Pitch += K * 0.6f * (FMath::Sin(T * 31.f) * 0.6f + FMath::Sin(T * 17.f) * 0.4f);
		CR.Yaw += K * 0.6f * (FMath::Sin(T * 27.f + 2.f) * 0.6f + FMath::Sin(T * 13.f) * 0.4f);
	}
	TickStarField(CamLoc);
	if (Camera) Camera->SetActorLocationAndRotation(CamLoc, CR);

	if (SkyAtmo)
	{
		const bool bAir = bTrueScale && View == EISView::Orbit && !bJupiterScene;   // Earth's sky only around Earth
		if (SkyAtmo->IsVisible() != bAir) { SkyAtmo->SetVisibility(bAir); PlanetAtmo->SetVisibility(!bAir && PlanetAtmo->IsVisible()); }
	}
	if (bTrueScale) TickTrueScale(Dt);
	if (bTrueScale) Sky->SetWorldLocation(CamLoc);
	if (View == EISView::Cosmos) TickCosmos(Dt, CamLoc);

	// planets spin
	if (Planet->IsVisible() && !bTrueScale) Planet->AddLocalRotation(FRotator(0, Dt * (View == EISView::Orbit ? 3.f : 0.6f), 0));
	if (Moon->IsVisible()) Moon->AddLocalRotation(FRotator(0, Dt * 0.5f, 0));

	// labels face the camera and keep a readable size
	for (UTextRenderComponent* L : Labels)
	{
		if (!L) continue;
		// follow view: clean screen, map view: labels (not on voyages: at Earth-Moon distances the 3D text breaks up)
		L->SetVisibility(!(View == EISView::Orbit && bFollow) && (View != EISView::Cosmos || bCosmosLabels) && !(View == EISView::Orbit && bVoyage));
		const FVector P = L->GetComponentLocation();
		L->SetWorldRotation((CamLoc - P).Rotation());
		L->SetWorldSize(FMath::Max(8.f, FVector::Dist(CamLoc, P) * 0.014f));
	}

	// follow view and launch: only your spacecraft, no clutter (the orbit map shows everything)
	{
		const bool bClean = View == EISView::Orbit && (bFollow || LaunchT >= 0.f);
		for (UInstancedStaticMeshComponent* C : DotSets) if (C) C->SetVisibility(!bClean);
		for (UStaticMeshComponent* C : RealParts) if (C) C->SetVisibility(!bClean || View != EISView::Orbit);
	}

	// LEO swarm motion
	if (Swarm && SwarmSats.Num())
	{
		TArray<FTransform> X; X.Reserve(SwarmSats.Num());
		for (const FSwarmSat& S : SwarmSats)
		{
			const float A = S.Phase + S.Rate * (float)SimTime;
			const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(S.Raan)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(S.Inc));
			X.Add(FTransform(FRotator::ZeroRotator, Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * S.R), FVector(0.45f)));
		}
		Swarm->BatchUpdateInstancesTransforms(0, X, false, true, true);
	}

	// BIT animation
	if (BitRoot->IsVisible())
	{
		const float T = (float)SimTime;
		FVector P = BitHome + FVector(0, 0, FMath::Sin(T * 1.7f) * 8.f * BitBaseScale);
		if (ViewDest == EISDest::Backyard && View == EISView::Orbit) P = BitHome + FVector(0, 0, 60.f * BitBaseScale + FMath::Sin(T * 1.7f) * 8.f);
		BitRoot->SetWorldLocation(P);
		BitRoot->SetWorldScale3D(FVector(BitBaseScale));
		FRotator Look = (CamLoc - P).Rotation();
		Look.Pitch = FMath::Clamp(Look.Pitch, -25.f, 25.f);
		Look.Yaw += FMath::Sin(T * 0.6f) * 12.f;
		BitRoot->SetWorldRotation(Look);
		PokeT = FMath::Max(0.f, PokeT - Dt * 0.9f);
		const float Reach = FMath::Sin(PokeT * PI);
		BitHandL->SetRelativeLocation(FVector(25.f + Reach * 10.f, -98.f, -35.f + FMath::Sin(T * 2.1f + 1.f) * 10.f));
		BitHandR->SetRelativeLocation(FVector(25.f + Reach * 160.f, 98.f + Reach * 140.f, -35.f + Reach * 40.f + FMath::Sin(T * 2.1f) * 10.f));
		BitHandR->SetRelativeRotation(FRotator(0, Reach * -40.f, FMath::Sin(T * 3.f) * 10.f));
		BitHandL->SetRelativeRotation(FRotator(0, 0, FMath::Sin(T * 2.6f) * 10.f));
		// blink every ~4 s
		const float Blink = FMath::Fmod(T, 4.2f) < 0.12f ? 0.1f : 1.f;
		if (BitIris) BitIris->SetRelativeScale3D(FVector(0.42f, 0.42f * Blink, 0.04f));
	}
}


// True-scale overlay: orbit line + other satellites as screen-size points (KSP-style map), real orbital speeds.
void AISSpaceScene::TickTrueScale(float Dt)
{
	if (!Lines || View != EISView::Orbit) return;
	if (bRealEarth && Earth) Earth->SetVisible(true);   // the real globe stays on in every true-scale Earth view
	Lines->Flush();
	const double T = MissionDay * 86400.0;
	// Earth turns once per sidereal day
	const double Spin = bJupiterScene ? 35730.0 : 86164.0;   // Jupiter turns in 9 h 55 min
	if (Planet->IsVisible()) Planet->SetWorldRotation(FRotator(0, 200.f + (float)FMath::Fmod(T / Spin * 360.0, 360.0), 0));
	if (CloudShell && CloudShell->IsVisible()) CloudShell->SetWorldRotation(FRotator(0, 200.f + (float)FMath::Fmod(T / 80000.0 * 360.0, 360.0), 0));
	// your orbit (a voyage draws its real trajectory instead, in TickVoyage)
	const FQuat Q = OrbitQuat();
	const int32 N = bVoyage ? 0 : 256;
	FVector Prev = Q.RotateVector(FVector(OrbitRadiusUU, 0, 0));
	for (int32 i = 1; i <= N; ++i)
	{
		const float A = 2.f * PI * i / N;
		const FVector P = Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * OrbitRadiusUU);
		if (!bGlobe) Lines->DrawLine(Prev, P, FLinearColor(1.f, 0.55f, 0.15f), SDPG_World, 0.f, 0.f);
		Prev = P;
	}
	// everyone else: points that keep a constant size on screen
	const bool bMap = !bFollow;
	const bool bDaySky = bFlight && Sim.AltM < 60000.0;   // other satellites are invisible against a blue daytime sky
	for (const FTruePt& P : TruePts)
	{
		const double A = P.Phase + P.Rate * T;
		const FQuat Qp = FQuat(FVector::UpVector, FMath::DegreesToRadians((float)P.Raan)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians((float)P.Inc));
		const FVector Pos = Qp.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * P.R);
		if (!bDaySky) Lines->DrawPoint(Pos, FLinearColor(P.C), bMap ? P.Size : P.Size * 0.6f, SDPG_World, 0.f);
	}
	// your spacecraft marker in the map
	if (bGlobe && bRealEarth && Earth) Lines->DrawPoint(Earth->SiteToEarth(FVector::ZeroVector), FLinearColor(1.f, 0.55f, 0.15f), 10.f, SDPG_Foreground, 0.f);   // your space centre
	// your spacecraft marker whenever it is too small to see (map, or zoomed far out)
	if ((bMap || CamDist > LastSatSize * 30.f) && !bFlight && !bGlobe && LaunchT < 0.f) Lines->DrawPoint(SatRoot->GetComponentLocation(), FLinearColor(1.f, 0.55f, 0.15f), 8.f, SDPG_Foreground, 0.f);
}

FQuat AISSpaceScene::OrbitQuat() const
{
	if (!bRealEarth || !Earth)
		return FQuat(FVector::UpVector, FMath::DegreesToRadians(20.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(OrbitInc));
	// the real orbital plane: it passes over Mahia, heading where a launch to this inclination must go
	// (sin(azimuth) = cos(inclination) / cos(latitude); sun-synchronous and polar orbits leave toward the south)
	const FVector U = Earth->SiteUpEarthFrame(), E = Earth->SiteEastEarthFrame(), N = Earth->SiteNorthEarthFrame();
	const double CosLat = FMath::Cos(FMath::DegreesToRadians(FMath::Abs(AISEarth::SiteLat)));
	double Az = FMath::Asin(FMath::Clamp(FMath::Cos(FMath::DegreesToRadians((double)OrbitInc)) / CosLat, -1.0, 1.0));
	if (OrbitInc > 80.f) Az = PI - Az;
	const FVector Dn = (N * (float)FMath::Cos(Az) + E * (float)FMath::Sin(Az)).GetSafeNormal();
	return FQuat(FMatrix(U, Dn, FVector::CrossProduct(U, Dn), FVector::ZeroVector));
}

void AISSpaceScene::ShowGlobe(float AltM)
{
	FISDesign Empty;
	ShowOrbit(EISDest::LEO, 500.f, 39.f, Empty, 1);
	bGlobe = true;
	SatRoot->SetVisibility(false, true);
	bFollow = false;
	const FVector Up = bRealEarth && Earth ? Earth->SiteUpEarthFrame() : FVector::UpVector;
	const float R = bRealEarth && Earth ? (float)Earth->SiteToEarth(FVector::ZeroVector).Size() : PlanetRUU;
	// the same view the overview camera had: straight down onto the site from AltM
	const FRotator CR = (-Up).Rotation();
	CamPitch = CR.Pitch; CamYaw = CR.Yaw;
	CamFocus = CamFocusGoal = FVector::ZeroVector;
	CamDist = CamDistGoal = R + AltM * 100.f;
	CamMin = R + 100000.f * 100.f;
	CamMax = R * 60.f;
}

FVector AISSpaceScene::SatMountWorld(int32 M) const
{
	FVector L = FVector::ZeroVector;
	switch (M)
	{
	case 1: L = FVector(SatHalf.X * 1.35f, 0, 0); break;                    // nadir
	case 2: L = FVector(-SatHalf.X * 1.35f, 0, 0); break;                   // zenith
	case 3: L = FVector(0, SatHalf.Y + FMath::Max(60.f, SatHalf.Y * 1.6f), SatHalf.Z * 0.3f); break;   // wings
	case 4: L = FVector(0, 0, -SatHalf.Z * 1.3f); break;                    // base
	case 5: L = FVector(0, -SatHalf.Y * 1.35f, SatHalf.Z * 0.4f); break;     // side panels
	default: L = FVector(0, 0, SatHalf.Z * 0.15f); break;                   // inside
	}
	return SatRoot ? SatRoot->GetComponentTransform().TransformPosition(L) : L;
}
