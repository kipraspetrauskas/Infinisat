#include "ISScene.h"
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
	float SqrtAU(float AU) { return 2500.f * FMath::Sqrt(AU); }
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
	SkyAtmo->BottomRadius = 6371.f;
	SkyAtmo->AtmosphereHeight = 100.f;
	SkyAtmo->SetupAttachment(SceneRoot);
	SkyAtmo->RegisterComponent();
	SkyAtmo->SetVisibility(false);
	SunLight->SetAtmosphereSunLight(true);

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
}

void AISSpaceScene::SetCameraTarget(const FVector& Focus, float Dist, float Yaw, float Pitch, float MinD, float MaxD)
{
	CamFocusGoal = Focus; CamDistGoal = Dist; CamYaw = Yaw; CamPitch = Pitch; CamMin = MinD; CamMax = MaxD;
	CamFocus = Focus; CamDist = Dist;
}

void AISSpaceScene::OrbitCamera(float DYaw, float DPitch)
{
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
	const float Old = CamDistGoal;
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
	SetCameraTarget(FVector(0, 0, 40), 900.f, 205.f, -14.f, 220.f, 2400.f);
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
	else if (Dest == EISDest::Interstellar)
	{
		ViewK = 1.f;
		Sun->SetWorldScale3D(FVector(12.f)); Sun->SetWorldLocation(FVector::ZeroVector); Sun->SetVisibility(true);
		struct FPl { const TCHAR* N; float AU; FLinearColor C; } Pl[] = {
			{ TEXT("Mercury"), 0.39f, FLinearColor(0.6f, 0.55f, 0.5f) }, { TEXT("Venus"), 0.72f, FLinearColor(0.9f, 0.8f, 0.5f) },
			{ TEXT("Earth"), 1.f, FLinearColor(0.3f, 0.5f, 1.f) }, { TEXT("Mars"), 1.52f, FLinearColor(0.9f, 0.4f, 0.2f) },
			{ TEXT("Jupiter"), 5.2f, FLinearColor(0.9f, 0.7f, 0.5f) }, { TEXT("Saturn"), 9.5f, FLinearColor(0.9f, 0.8f, 0.55f) },
			{ TEXT("Uranus"), 19.2f, FLinearColor(0.6f, 0.9f, 0.95f) }, { TEXT("Neptune"), 30.1f, FLinearColor(0.3f, 0.45f, 1.f) } };
		FRandomStream R(7);
		for (const FPl& X : Pl)
		{
			const float Rr = SqrtAU(X.AU);
			AddOrbitRing(Rr, 0.f, 0.f, X.C * 0.6f, 160, 0.9f);
			const float A = R.FRandRange(0.f, 2.f * PI);
			const FVector Pos(Rr * FMath::Cos(A), Rr * FMath::Sin(A), 0);
			AddMesh(SceneRoot, MSphere, FTransform(FRotator::ZeroRotator, Pos, FVector(X.AU > 4 ? 5.f : 3.f)), X.C, true, 2.f, nullptr);
			AddLabel(FString::Printf(TEXT("%s %s AU"), X.N, *ISSim::Fmt(X.AU, 1)), Pos + FVector(0, 0, 500), FColor::White);
		}
		AddOrbitRing(SqrtAU(94.f), 0.f, 0.f, FLinearColor(0.4f, 0.8f, 1.f), 240, 1.4f);
		AddLabel(TEXT("Termination shock ~94 AU"), FVector(0, -SqrtAU(94.f), 900), FColor(130, 200, 255));
		AddOrbitRing(SqrtAU(120.f), 0.f, 0.f, FLinearColor(0.8f, 0.4f, 1.f), 300, 2.f);
		AddLabel(TEXT("HELIOPAUSE ~120 AU: interstellar space begins"), FVector(0, SqrtAU(120.f), 1200), FColor(220, 150, 255));
		AddLabel(TEXT("(distances drawn on a square-root scale)"), FVector(0, 0, -3000), FColor(160, 160, 170));
		PathDots = AddDots(MSphere, Mine, 2.f);
		const FVector Dir = FVector(FMath::Cos(0.5f), FMath::Sin(0.5f), 0.08f).GetSafeNormal();
		for (int32 i = 1; i <= 120; ++i) PathDots->AddInstance(FTransform(FRotator::ZeroRotator, Dir * SqrtAU((float)i), FVector(0.8f)));
		SatSize = 1200.f;
		SetCameraTarget(FVector(0, 0, 0), 62000.f, 210.f, -38.f, 5000.f, 150000.f);
	}
	else if (Dest == EISDest::Jupiter)
	{
		ViewK = 0.05f;
		const float RJ = 69911.f * ViewK;
		SetPlanet(Planet, TEX_JUP, RJ, FVector::ZeroVector);
		const float Rp = (69911.f + AltKm) * ViewK, Ra = 12.f * RJ;
		OrbitRadiusUU = Rp;
		const float Aa = (Rp + Ra) * 0.5f, E = (Ra - Rp) / (Ra + Rp), Bb = Aa * FMath::Sqrt(1 - E * E);
		PathDots = AddDots(MSphere, Mine, 2.f);
		for (int32 i = 0; i < 200; ++i)
		{
			const float T = 2.f * PI * i / 200.f;
			PathDots->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Aa * FMath::Cos(T) - (Aa - Rp), 0, Bb * FMath::Sin(T)), FVector(4.f)));
		}
		AddOrbitRing(421700.f * ViewK, 0.f, 0.f, FLinearColor(0.9f, 0.85f, 0.4f), 120, 3.f);
		AddLabel(TEXT("Io"), FVector(421700.f * ViewK, 0, 1500), FColor(255, 230, 120));
		AddOrbitRing(671000.f * ViewK, 0.f, 0.f, FLinearColor(0.7f, 0.85f, 1.f), 140, 3.f);
		AddLabel(TEXT("Europa (ocean under the ice!)"), FVector(0, 671000.f * ViewK, 1500), FColor(170, 210, 255));
		AddOrbitRing(2.5f * RJ, 0.f, 0.f, FLinearColor(0.6f, 0.08f, 0.08f), 90, 3.f);
		AddOrbitRing(3.2f * RJ, 0.f, 0.f, FLinearColor(0.6f, 0.08f, 0.08f), 110, 3.f);
		AddLabel(TEXT("Radiation belts (deadly!)"), FVector(-3.2f * RJ, 0, 1200), FColor(255, 110, 110));
		AddLabel(TEXT("Jupiter: 11x Earth's diameter"), FVector(0, 0, RJ * 1.15f), FColor::White);
		SatSize = 900.f;
		SetCameraTarget(FVector(-Ra * 0.12f, 0, 0), 62000.f, 235.f, -14.f, 6000.f, 200000.f);
	}
	else
	{
		// TRUE SCALE: 1 unit = 1 cm, real planet sizes and orbit radii (Unreal large-world coordinates)
		const bool bMoon = Dest == EISDest::Moon;
		bTrueScale = true;
		ViewK = 1.0e5f;                                  // uu per km
		const double K = ViewK;
		const double Rb = bMoon ? 1737.4 : 6371.0;
		const double Mu = bMoon ? 4902.8 : 398600.4418;
		const double Rorb = Rb + AltKm;
		PeriodS = 2.0 * PI * FMath::Sqrt(Rorb * Rorb * Rorb / Mu);
		Sky->SetWorldScale3D(FVector(1.0e9f));          // star sphere follows the camera, radius 500,000 km
		if (bMoon)
		{
			SetPlanet(Moon, TEX_MOON, (float)(Rb * K), FVector::ZeroVector);
			if (UStaticMesh* Hi = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi"))) Moon->SetStaticMesh(Hi);
			SetEarth(Planet, (float)(6371.0 * K), FVector(-384400.0 * K, 0, 0), true);
			PlanetAtmo->SetWorldScale3D(FVector((float)(6371.0 * 1.015 * K / 50.0))); PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation()); PlanetAtmo->SetVisibility(true);
			AddLabel(TEXT("Earth: 384,400 km away (1.3 light-seconds)"), FVector(-384400.0 * K, 0, 9000.0 * K), FColor::White);
			AddLabel(TEXT("South pole: permanently shadowed craters (ice?)"), FVector(0, 0, -Rb * 1.25 * K), FColor(170, 210, 255));
		}
		else
		{
			SetEarth(Planet, (float)(Rb * K), FVector::ZeroVector, true);
			PlanetAtmo->SetWorldScale3D(FVector((float)(Rb * 1.015 * K / 50.0))); PlanetAtmo->SetWorldLocation(FVector::ZeroVector); PlanetAtmo->SetVisibility(true);
			SetPlanet(Moon, TEX_MOON, (float)(1737.4 * K), FVector(0, 384400.0 * K, 0));
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
			AddLabel(TEXT("Moon"), FVector(0, 384400.0 * K, 4000.0 * K), FColor::White);
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
	SetMissionTime(0.0, 1.0, 100.0, true);
	LastSatSize = SatSize;
	MapDist = CamDistGoal;
	if (!bTrueScale) PlanetRUU = Dest == EISDest::Moon ? 1737.4f : 6371.f;
	const bool bNear = Dest != EISDest::Backyard && Dest != EISDest::Interstellar && Dest != EISDest::Jupiter;
	bFollow = bNear;
	if (bFollow) { CamMin = SatSize * 1.2f; CamDistGoal = SatSize * 5.f; CamDist = CamDistGoal; CamPitch = -12.f; CamFocus = CamFocusGoal = SatRoot->GetComponentLocation(); }
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
	if (View != EISView::Orbit || !bAlive) return;
	const double Frac = EndDay > 0 ? FMath::Clamp(Day / EndDay, 0.0, 1.0) : 0.0;
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
	case EISDest::Interstellar:
	{
		const FVector Dir = FVector(FMath::Cos(0.5f), FMath::Sin(0.5f), 0.08f).GetSafeNormal();
		Pos = Dir * SqrtAU(FMath::Max(1.f, (float)(Frac * 120.0)));
		break;
	}
	case EISDest::Jupiter:
	{
		const float RJ = 69911.f * ViewK;
		const float Rp = OrbitRadiusUU, Ra = 12.f * RJ;
		const float Aa = (Rp + Ra) * 0.5f, E = (Ra - Rp) / (Ra + Rp), Bb = Aa * FMath::Sqrt(1 - E * E);
		const float T = Ang * 0.5f;
		Pos = FVector(Aa * FMath::Cos(T) - (Aa - Rp), 0, Bb * FMath::Sin(T));
		break;
	}
	default:
	{
		const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(20.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(OrbitInc));
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
			CamDistGoal = LastSatSize * 5.f;
		}
	}
	else if (View == EISView::Orbit && bFollow && ViewDest != EISDest::Backyard && ViewDest != EISDest::Interstellar && ViewDest != EISDest::Jupiter)
		CamFocusGoal = SatRoot->GetComponentLocation();
	else if (View == EISView::Orbit && ViewDest != EISDest::Backyard && ViewDest != EISDest::Interstellar && ViewDest != EISDest::Jupiter && CamDistGoal < OrbitRadiusUU * 0.9f)
		CamFocusGoal = SatRoot->GetComponentLocation();
	else if (View == EISView::Orbit && ViewDest != EISDest::Backyard) CamFocusGoal = ViewDest == EISDest::Jupiter ? CamFocusGoal : FVector::ZeroVector;
	FRotator CR(CamPitch, CamYaw, 0.f);
	FVector CamLoc = CamFocus - CR.Vector() * CamDist;
	if (View == EISView::Orbit && bTrueScale && bFollow)
	{
		// follow camera in the satellite's local frame: behind it along its path, Earth's horizon below
		const FVector SatPos = LaunchT >= 0.f && RocketRoot ? RocketRoot->GetComponentLocation() + (bFlight ? RocketRoot->GetUpVector() * RocketLenCm * 0.45f : FVector::ZeroVector) : SatRoot->GetComponentLocation();
		const FVector Up = SatPos.GetSafeNormal();
		const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(20.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(OrbitInc));
		const FVector N = Q.RotateVector(FVector::UpVector);
		const FVector Fwd = FVector::CrossProduct(N, Up).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Up, Fwd);
		const FVector L = FRotator(FollowPitch, FollowYaw, 0.f).Vector();
		const FVector Dir = (Fwd * L.X + Right * L.Y + Up * L.Z).GetSafeNormal();
		CamLoc = SatPos - Dir * CamDist;
		CR = FRotationMatrix::MakeFromXZ(Dir, Up).Rotator();
	}
	if (bFlight && FlightShake > 0.01f)
	{
		// camera shake: felt strongest close to the rocket
		const float T = (float)FPlatformTime::Seconds(), K = FlightShake * FMath::Clamp(3000.f / FMath::Max(300.f, CamDist), 0.15f, 1.f);
		CR.Pitch += K * 0.6f * (FMath::Sin(T * 31.f) * 0.6f + FMath::Sin(T * 17.f) * 0.4f);
		CR.Yaw += K * 0.6f * (FMath::Sin(T * 27.f + 2.f) * 0.6f + FMath::Sin(T * 13.f) * 0.4f);
	}
	if (Camera) Camera->SetActorLocationAndRotation(CamLoc, CR);

	if (SkyAtmo)
	{
		const bool bAir = bTrueScale && View == EISView::Orbit && ViewDest != EISDest::Moon;
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
		L->SetVisibility(!(View == EISView::Orbit && bFollow) && (View != EISView::Cosmos || bCosmosLabels));   // follow view: clean screen, map view: labels
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
	Lines->Flush();
	const double T = MissionDay * 86400.0;
	// Earth turns once per sidereal day
	if (Planet->IsVisible() && ViewDest != EISDest::Moon) Planet->SetWorldRotation(FRotator(0, 200.f + (float)FMath::Fmod(T / 86164.0 * 360.0, 360.0), 0));
	if (CloudShell && CloudShell->IsVisible()) CloudShell->SetWorldRotation(FRotator(0, 200.f + (float)FMath::Fmod(T / 80000.0 * 360.0, 360.0), 0));
	// your orbit
	const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(20.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(OrbitInc));
	const int32 N = 256;
	FVector Prev = Q.RotateVector(FVector(OrbitRadiusUU, 0, 0));
	for (int32 i = 1; i <= N; ++i)
	{
		const float A = 2.f * PI * i / N;
		const FVector P = Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * OrbitRadiusUU);
		Lines->DrawLine(Prev, P, FLinearColor(1.f, 0.55f, 0.15f), SDPG_World, 0.f, 0.f);
		Prev = P;
	}
	// everyone else: points that keep a constant size on screen
	const bool bMap = !bFollow;
	for (const FTruePt& P : TruePts)
	{
		const double A = P.Phase + P.Rate * T;
		const FQuat Qp = FQuat(FVector::UpVector, FMath::DegreesToRadians((float)P.Raan)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians((float)P.Inc));
		const FVector Pos = Qp.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * P.R);
		Lines->DrawPoint(Pos, FLinearColor(P.C), bMap ? P.Size : P.Size * 0.6f, SDPG_World, 0.f);
	}
	// your spacecraft marker in the map
	if (bMap) Lines->DrawPoint(SatRoot->GetComponentLocation(), FLinearColor(1.f, 0.55f, 0.15f), 10.f, SDPG_Foreground, 0.f);
}
