#include "ISWorld.h"
#include "ISBit.h"
#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FVector GarageMin(-420, -320, 0), GarageMax(420, 320, 320);
	const FVector PadLocation(800.f, 9500.f, 0.f);
	const float Rho = 1.2f, G = 9.81f;
	FString TexPath(const TCHAR* T) { return FString::Printf(TEXT("/Game/Infinisat/PHTex/%s_diff.%s_diff"), T, T); }
	const TArray<FName>& CanSatTray()
	{
		static TArray<FName> T = { "bat_aa", "cpu_ard", "com_433", "pl_temp", "ex_wires", "ex_chute_l", "ex_buzzer", "pl_gnss", "pl_cam_mini", "bat_cube", "ex_chute_s", "adcs_mag", "pwr_body" };
		return T;
	}
	int32 StackOrder(EISCat C)
	{
		switch (C)
		{
		case EISCat::Battery: return 0; case EISCat::Computer: return 1; case EISCat::ADCS: return 2; case EISCat::Comms: return 3;
		case EISCat::Payload: return 4; case EISCat::Extra: return 6; default: return 5;
		}
	}
}

AISHomeWorld::AISHomeWorld()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);
	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(Root);
	CanRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CanRoot"));
	CanRoot->SetupAttachment(Root);
	RocketRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RocketRoot"));
	RocketRoot->SetupAttachment(Root);
	ChuteRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ChuteRoot"));
	ChuteRoot->SetupAttachment(CanRoot);
	BenchHands = CreateDefaultSubobject<USceneComponent>(TEXT("BenchHands"));
	BenchHands->SetupAttachment(Root);
}

// =====================================================================================================
//  construction
// =====================================================================================================
UStaticMeshComponent* AISHomeWorld::Block(const FVector& C, const FVector& S, const TCHAR* Tex, float Tile, const FLinearColor& Tint, bool bCollide)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(ISMat::Cube());
	M->SetupAttachment(Root);
	M->SetRelativeLocation(C);
	M->SetRelativeScale3D(S / 100.f);
	M->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	M->SetCollisionResponseToAllChannels(ECR_Block);
	M->RegisterComponent();
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Tex ? ISMat::World() : ISMat::Color(), M);
	if (Tex)
	{
		if (UTexture* T = LoadObject<UTexture>(nullptr, *TexPath(Tex))) MID->SetTextureParameterValue(TEXT("Tex"), T);
		MID->SetScalarParameterValue(TEXT("Tile"), Tile);
		MID->SetVectorParameterValue(TEXT("Tint"), Tint);
	}
	else
	{
		MID->SetVectorParameterValue(TEXT("Color"), Tint);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.7f);
		MID->SetScalarParameterValue(TEXT("Metallic"), 0.f);
	}
	M->SetMaterial(0, MID);
	AllComps.Add(M);
	return M;
}

UStaticMeshComponent* AISHomeWorld::Place(const TCHAR* Id, const FVector& Pos, float Yaw, float Scale, bool bCollide)
{
	UStaticMesh* Mesh = ISMat::Mesh(ISMat::PH(Id));
	if (!Mesh) return nullptr;
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Mesh);
	M->SetupAttachment(Root);
	const FBox B = Mesh->GetBoundingBox();
	M->SetRelativeTransform(FTransform(FRotator(0, Yaw, 0), Pos - FVector(0, 0, B.Min.Z * Scale), FVector(Scale)));
	M->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	M->SetCollisionResponseToAllChannels(ECR_Block);
	M->RegisterComponent();
	AllComps.Add(M);
	return M;
}

UBoxComponent* AISHomeWorld::AddInteract(const FVector& Center, const FVector& Extent, FName Action)
{
	UBoxComponent* B = NewObject<UBoxComponent>(this);
	B->SetupAttachment(Root);
	B->SetBoxExtent(Extent);
	B->SetRelativeLocation(Center);
	B->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	B->SetCollisionResponseToAllChannels(ECR_Ignore);
	B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	B->SetHiddenInGame(true);
	B->RegisterComponent();
	Actions.Add(B, Action);
	return B;
}

UTextRenderComponent* AISHomeWorld::AddSign(const FString& Text, const FVector& Pos, float Yaw, float Size, const FColor& Color)
{
	UTextRenderComponent* L = NewObject<UTextRenderComponent>(this);
	L->SetupAttachment(Root);
	L->SetText(FText::FromString(Text));
	L->SetTextRenderColor(Color);
	L->SetHorizontalAlignment(EHTA_Center);
	L->SetWorldSize(Size);
	L->SetRelativeLocation(Pos);
	L->SetRelativeRotation(FRotator(0, Yaw, 0));
	L->RegisterComponent();
	AllComps.Add(L);
	return L;
}

void AISHomeWorld::BuildSky()
{
	Sun->SetRelativeRotation(FRotator(-38.f, 125.f, 0.f));
	Sun->SetIntensity(10.f);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetCastShadows(true);
	Sun->SetDynamicShadowCascades(4);
	Sun->SetForwardShadingPriority(2);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLight->SetIntensity(1.f);
	Fog->SetFogDensity(0.006f);
	Fog->SetFogHeightFalloff(0.25f);
	if (UMaterialInterface* CM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")))
		Clouds->SetMaterial(CM);

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PPV = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, SP);
	PPV->bUnbound = true;
	PPV->Priority = 10.f;
	FPostProcessSettings& S = PPV->Settings;
	S.bOverride_AutoExposureMethod = true; S.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true; S.AutoExposureApplyPhysicalCameraExposure = false;
	S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = -1.5f;
	S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 10.f;
	S.bOverride_AutoExposureBias = true; S.AutoExposureBias = 0.3f;
	S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.4f;
	S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.3f;
	S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0.f;
	// colour grade: warm afternoon, richer colours, soft contrast (cozy, not grey)
	S.bOverride_WhiteTemp = true; S.WhiteTemp = 6000.f;
	S.bOverride_ColorSaturation = true; S.ColorSaturation = FVector4(1.18f, 1.18f, 1.18f, 1.f);
	S.bOverride_ColorContrast = true; S.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.f);
	S.bOverride_ColorGain = true; S.ColorGain = FVector4(1.03f, 1.01f, 0.97f, 1.f);
	S.bOverride_SceneFringeIntensity = true; S.SceneFringeIntensity = 0.f;
	S.bOverride_AmbientOcclusionIntensity = true; S.AmbientOcclusionIntensity = 0.6f;
}

void AISHomeWorld::BuildGround()
{
	Block(FVector(5000, 0, -50), FVector(900000, 900000, 100), TEXT("leafy_grass"), 250.f, FLinearColor(0.85f, 0.95f, 0.8f));
	// dirt path from the garage door to the launch field
	const FVector A(450, 0, 0), B = PadLocation;
	const FVector Mid = (A + B) * 0.5f;
	const float Len = FVector::Dist2D(A, B);
	UStaticMeshComponent* Path = Block(Mid + FVector(0, 0, 0.6f), FVector(Len, 180, 1), TEXT("dirt_floor"), 200.f, FLinearColor::White, false);
	Path->SetRelativeRotation(FRotator(0, (B - A).Rotation().Yaw, 0));
	Block(FVector(800, 0, 0.8f), FVector(800, 600, 1), TEXT("brushed_concrete"), 300.f);   // driveway
}

void AISHomeWorld::BuildGarage()
{
	const TCHAR* Wall = TEXT("painted_plaster_wall");
	Block(FVector(0, 0, 0.5f), FVector(840, 640, 1), TEXT("concrete_floor_02"), 250.f);
	Block(FVector(-410, 0, 160), FVector(20, 640, 320), Wall, 250.f, FLinearColor(0.95f, 0.93f, 0.88f));
	Block(FVector(0, 310, 160), FVector(840, 20, 320), Wall, 250.f, FLinearColor(0.95f, 0.93f, 0.88f));
	Block(FVector(0, -310, 160), FVector(840, 20, 320), Wall, 250.f, FLinearColor(0.95f, 0.93f, 0.88f));
	Block(FVector(410, -250, 160), FVector(20, 140, 320), Wall, 250.f, FLinearColor(0.95f, 0.93f, 0.88f));
	Block(FVector(410, 250, 160), FVector(20, 140, 320), Wall, 250.f, FLinearColor(0.95f, 0.93f, 0.88f));
	Block(FVector(410, 0, 290), FVector(20, 360, 60), Wall, 250.f, FLinearColor(0.95f, 0.93f, 0.88f));
	Block(FVector(0, 0, 330), FVector(900, 700, 20), TEXT("corrugated_iron"), 200.f, FLinearColor(0.7f, 0.72f, 0.75f));
	Block(FVector(0, 0, 318), FVector(840, 640, 4), nullptr, 0, FLinearColor(0.85f, 0.85f, 0.85f), false);   // ceiling
	// rolled-up garage door above the opening
	Place(TEXT("rollershutter_door"), FVector(415, 0, 255), 90.f, 1.f);

	// ceiling lights
	for (float X : { -200.f, 150.f })
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetupAttachment(Root);
		L->SetRelativeLocation(FVector(X, 0, 290));
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(2400.f);
		L->SetAttenuationRadius(1200.f);
		L->SetLightColor(FLinearColor(1.f, 0.96f, 0.9f));
		L->SetCastShadows(true);
		L->RegisterComponent();
		Place(TEXT("mounted_fluorescent_lights"), FVector(X, 0, 300), 0.f);
	}

	// workbench along the back (-Y) wall
	UStaticMeshComponent* Bench = Place(TEXT("WoodenTable_03"), FVector(-60, -235, 1), 0.f, 1.f, true);
	if (Bench)
	{
		const FBox B = Bench->Bounds.GetBox();
		BenchTop = FVector(B.GetCenter().X, B.GetCenter().Y, B.Max.Z);
	}
	else
	{
		Block(FVector(-60, -235, 45), FVector(180, 80, 90), TEXT("brown_planks_05"), 100.f);
		BenchTop = FVector(-60, -235, 90);
	}
	AddInteract(BenchTop + FVector(0, 0, 10), FVector(60, 30, 20), "bench");
	AddSign(TEXT("WORKBENCH"), FVector(BenchTop.X, -298, BenchTop.Z + 70), 90.f, 12.f, FColor(40, 40, 40));
	Place(TEXT("desk_lamp_arm_01"), BenchTop + FVector(70, -22, 0), 200.f);
	Place(TEXT("bench_vice_01"), BenchTop + FVector(-95, -10, 0), 0.f);
	Place(TEXT("retro_multimeter"), BenchTop + FVector(55, 8, 0), -20.f);
	Place(TEXT("screwdriver"), BenchTop + FVector(-35, 20, 0), 40.f);
	Place(TEXT("pliers"), BenchTop + FVector(-48, 25, 0), -10.f);
	UStaticMeshComponent* Laptop = Place(TEXT("classic_laptop"), BenchTop + FVector(-75, -15, 0), 20.f);
	AddInteract(BenchTop + FVector(-75, -15, 12), FVector(22, 18, 14), "laptop");
	AddSign(TEXT("ORDERS"), BenchTop + FVector(-75, -30, 38), 90.f, 7.f, FColor(0, 120, 200));
	if (!Laptop) Block(BenchTop + FVector(-75, -15, 2), FVector(32, 22, 3), nullptr, 0, FLinearColor(0.1f, 0.1f, 0.12f), false);
	Place(TEXT("vintage_radio_transceiver"), BenchTop + FVector(95, -18, 0), -10.f);
	AddInteract(BenchTop + FVector(95, -18, 15), FVector(20, 18, 18), "radio");

	// bench lamp light
	UPointLightComponent* BL = NewObject<UPointLightComponent>(this);
	BL->SetupAttachment(Root);
	BL->SetRelativeLocation(BenchTop + FVector(10, 0, 60));
	BL->SetIntensityUnits(ELightUnits::Lumens);
	BL->SetIntensity(500.f);
	BL->SetAttenuationRadius(300.f);
	BL->RegisterComponent();

	// shelves, tool chest, misc
	Place(TEXT("steel_frame_shelves_01"), FVector(-370, 120, 1), 90.f);
	Place(TEXT("cardboard_box_01"), FVector(-350, 230, 1), 15.f);
	Place(TEXT("metal_tool_chest"), FVector(-330, -230, 1), 0.f, 1.f, true);
	Place(TEXT("Drill_01"), FVector(-330, -230, 105), 30.f);
	Place(TEXT("ladder_sectioned_01"), FVector(250, 290, 1), 0.f);
	Place(TEXT("power_box_01"), FVector(200, -298, 150), 0.f);
	Place(TEXT("dartboard"), FVector(-398, -40, 170), 90.f);
	Place(TEXT("plastic_monobloc_chair_01"), FVector(-40, -120, 1), 160.f);
	Place(TEXT("spray_paint_bottles"), FVector(-370, 60, 101), 0.f);
	Place(TEXT("metal_toolbox"), FVector(-370, 160, 1), 90.f);
	Place(TEXT("measuring_tape_01"), BenchTop + FVector(30, 25, 0), 70.f);
	Place(TEXT("magnifying_glass_01"), BenchTop + FVector(40, 30, 0), 10.f);
}

void AISHomeWorld::BuildHouse()
{
	const FLinearColor Plaster(0.93f, 0.9f, 0.82f);
	Block(FVector(-50, 1050, 300), FVector(1300, 800, 600), TEXT("white_stucco"), 300.f, Plaster);
	// gable roof
	for (int32 s : { -1, 1 })
	{
		UStaticMeshComponent* R = Block(FVector(-50, 1050 + s * 210, 720), FVector(1400, 470, 20), TEXT("grey_roof_tiles"), 250.f, FLinearColor(0.75f, 0.45f, 0.4f));
		R->SetRelativeRotation(FRotator(0, 0, -s * 32.f));
	}
	// windows and door
	for (float X : { -500.f, -150.f, 250.f })
		for (float Z : { 150.f, 430.f })
		{
			Block(FVector(X, 647, Z), FVector(140, 6, 120), nullptr, 0, FLinearColor(0.05f, 0.08f, 0.12f), false);
			Block(FVector(X, 649, Z), FVector(150, 4, 8), nullptr, 0, FLinearColor(0.95f, 0.95f, 0.95f), false);
		}
	Block(FVector(450, 647, 110), FVector(110, 8, 220), TEXT("brown_planks_05"), 120.f);
	Place(TEXT("planter_box_01"), FVector(300, 700, 1), 0.f);
	Place(TEXT("garden_gnome"), FVector(560, 520, 1), -60.f);
	Place(TEXT("wooden_picnic_table"), FVector(1100, 900, 1), 20.f, 1.f, true);
	Place(TEXT("outdoor_table_chair_set_01"), FVector(-700, 1700, 1), 0.f);
	// fence along the garden
	for (int32 i = 0; i < 12; ++i)
		Place(TEXT("modular_chainlink_fence"), FVector(-900 + i * 250.f, 1650, 1), 0.f);
}

void AISHomeWorld::BuildField()
{
	PadPos = PadLocation;
	PadModel.Destroy();
	USceneComponent* PadRoot = NewObject<USceneComponent>(this);
	PadRoot->SetupAttachment(Root);
	PadRoot->SetRelativeLocation(PadPos);
	PadRoot->RegisterComponent();
	ISBuild::LaunchPad(this, PadRoot, &PadModel);
	AddInteract(PadPos + FVector(0, 0, 100), FVector(60, 60, 100), "pad");
	AddInteract(PadPos + FVector(-250, 120, 10), FVector(25, 20, 15), "pad");
	AddSign(TEXT("LAUNCH PAD"), PadPos + FVector(-260, 120, 60), 180.f, 25.f, FColor(255, 200, 40));
	// safety perimeter markers
	for (int32 i = 0; i < 16; ++i)
	{
		const FVector D = FRotator(0, i * 22.5f, 0).Vector() * 1500.f;
		ISBuild::Prim(this, PadRoot, ISMat::Cone(), D + FVector(0, 0, 18), FVector(20, 20, 36), FLinearColor(1.f, 0.35f, 0.05f), &PadModel);
	}
	Place(TEXT("wooden_picnic_table"), PadPos + FVector(-700, 400, 0), 70.f, 1.f, true);
	Place(TEXT("binoculars"), PadPos + FVector(-700, 400, 78), 10.f);
	RocketRoot->SetRelativeLocation(PadPos + FVector(0, 0, 40));
	RocketModel.Destroy();
	ISBuild::HobbyRocket(this, RocketRoot, &RocketModel);
	RocketModel.SetVisible(true);
}

void AISHomeWorld::BuildNature()
{
	FRandomStream R(42);
	auto Scatter = [&](const TCHAR* Id, int32 N, float MinR, float MaxR, const FVector& Center, float MinS, float MaxS, float Cull)
	{
		UStaticMesh* Mesh = ISMat::Mesh(ISMat::PH(Id));
		if (!Mesh) return;
		UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		H->SetStaticMesh(Mesh);
		H->SetupAttachment(Root);
		H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		H->SetCullDistances(0, (int32)Cull);
		H->RegisterComponent();
		const float Base = -Mesh->GetBoundingBox().Min.Z;
		N = FMath::Max(1, FMath::RoundToInt(N * ISQuality::Density()));
		int32 Added = 0, Tries = 0;
		while (Added < N && Tries++ < N * 20)
		{
			const float A = R.FRandRange(0.f, 2.f * PI);
			const float D = FMath::Sqrt(R.FRandRange(MinR * MinR, MaxR * MaxR));
			const FVector P = Center + FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, 0);
			// keep the house, path and launch field clear
			if (FVector::Dist2D(P, FVector(0, 600, 0)) < 1500.f) continue;
			if (FVector::Dist2D(P, PadLocation) < 4500.f) continue;
			const FVector Seg = PadLocation - FVector(450, 0, 0);
			const float T = FMath::Clamp(FVector::DotProduct(P - FVector(450, 0, 0), Seg) / Seg.SizeSquared(), 0.f, 1.f);
			if (FVector::Dist2D(P, FVector(450, 0, 0) + Seg * T) < 500.f) continue;
			const float S = R.FRandRange(MinS, MaxS);
			H->AddInstance(FTransform(FRotator(0, R.FRandRange(0.f, 360.f), 0), P + FVector(0, 0, Base * S - 5.f), FVector(S)));
			++Added;
		}
		AllComps.Add(H);
	};
	const FVector C(7500, 1200, 0);
	Scatter(TEXT("fir_tree_01"), 150, 3000.f, 60000.f, C, 0.8f, 1.3f, 120000.f);
	Scatter(TEXT("pine_tree_01"), 70, 3000.f, 60000.f, C, 0.8f, 1.2f, 120000.f);
	Scatter(TEXT("tree_small_02"), 90, 1600.f, 25000.f, C, 0.9f, 1.4f, 60000.f);
	Scatter(TEXT("shrub_01"), 150, 1500.f, 30000.f, C, 0.8f, 1.5f, 20000.f);
	Scatter(TEXT("shrub_02"), 150, 1500.f, 30000.f, C, 0.8f, 1.5f, 20000.f);
	Scatter(TEXT("boulder_01"), 25, 2000.f, 40000.f, C, 0.6f, 1.6f, 40000.f);
	Scatter(TEXT("grass_medium_01"), 3500, 0.f, 22000.f, C, 0.8f, 1.4f, 6000.f);
	Scatter(TEXT("grass_medium_02"), 3500, 0.f, 22000.f, C, 0.8f, 1.4f, 6000.f);
	Scatter(TEXT("dandelion_01"), 1200, 0.f, 18000.f, C, 0.9f, 1.2f, 3000.f);
}

void AISHomeWorld::BeginPlay()
{
	Super::BeginPlay();
	BuildSky();
	BuildGround2();
	BuildHome();
	BuildNeighbourhood();
	BuildField();
	BuildBackyard();
	SpawnHunt();

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BenchCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
	BenchCam->SetActorLocation(BenchTop + FVector(0, 62, 48));
	BenchCam->SetActorRotation(FRotator(-38.f, -90.f, 0.f));
	BenchCam->GetCameraComponent()->SetFieldOfView(60.f);
	BenchCam->GetCameraComponent()->bConstrainAspectRatio = false;
	ChaseCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
	ChaseCam->GetCameraComponent()->SetFieldOfView(70.f);
	ChaseCam->GetCameraComponent()->bConstrainAspectRatio = false;

	SetActive(false);
}

void AISHomeWorld::SetActive(bool bOn)
{
	SetActorHiddenInGame(!bOn);
	SetActorEnableCollision(bOn);
	Sun->SetVisibility(bOn);
	SkyLight->SetVisibility(bOn);
	Atmosphere->SetVisibility(bOn);
	Fog->SetVisibility(bOn);
	Clouds->SetVisibility(bOn);
	if (PPV) PPV->bEnabled = bOn;
}

FTransform AISHomeWorld::SpawnPoint() const
{
	return FTransform(FRotator(0, 110.f, 0), FVector(400.f, 820.f, 100.f));
}

// =====================================================================================================
//  CanSat on the bench
// =====================================================================================================
void AISHomeWorld::StartCanSatOrder(int32 Index)
{
	MissionIdx = Index;
	Design = FISDesign();
	for (const char* Id : { "bus_can", "pwr_none", "adcs_none", "prop_none", "thm_passive", "gnd_yard" }) Design.Parts.Add(FName(Id));
	Design.Vehicle = "lv_hobby";
	Design.Dest = EISDest::Backyard;
	Eval = ISSim::Evaluate(Design, ISData::Missions()[Index]);
	Stage = EISStage::Build;
	Telemetry.Reset(); Report = FISFlightReport(); bDebriefReady = false;
	CanRoot->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
	CanRoot->SetWorldLocationAndRotation(BenchTop + FVector(0, -8, 0), FRotator::ZeroRotator);
	ChuteModel.Destroy();
	RocketRoot->SetRelativeLocation(PadPos + FVector(0, 0, 40));
	RocketRoot->SetRelativeRotation(FRotator::ZeroRotator);
	RebuildCan();
	RebuildTray();
	++DesignVersion;
	Toast(TEXT("New order: build a CanSat! Go to the WORKBENCH and press E."));
}

void AISHomeWorld::RebuildCan()
{
	CanModel.Destroy();
	if (Stage == EISStage::NoOrder) return;
	const bool bOpen = Stage == EISStage::Build;
	ISBuild::CanShell(this, CanRoot, FVector::ZeroVector, bOpen, &CanModel);
	TArray<const FISPart*> Inside;
	for (const FName& Id : Design.Parts)
		if (const FISPart* P = ISData::FindPart(Id))
			if (CanSatTray().Contains(Id)) Inside.Add(P);
	Inside.Sort([](const FISPart& A, const FISPart& B) { return StackOrder(A.Cat) < StackOrder(B.Cat); });
	float Z = 0.4f;
	for (const FISPart* P : Inside)
	{
		const float H = ISBuild::CanSatPart(this, CanRoot, P->Id, FVector(0, 0, Z), &CanModel);
		Z += H + (H > 0 ? 0.25f : 0.f);
	}
	if (Z > 11.5f) Toast(FString::Printf(TEXT("Warning: the stack is %.1f cm tall but the can is only 11.5 cm! Real CanSats must fit the can volume."), Z));
	if (!bOpen)
	{
		// sealed can: lid, beacon LED and antenna stub visible outside
		ISBuild::Unlit(this, CanRoot, ISMat::Sphere(), FVector(0, 2.2f, 11.8f), FVector(0.6f), FLinearColor(1.f, 0.1f, 0.05f), 10.f, &CanModel);
	}
	if (IsValid(CanHit)) { Actions.Remove(CanHit); CanHit->DestroyComponent(); CanHit = nullptr; }
	if (Stage == EISStage::Sealed)
	{
		CanHit = AddInteract(CanRoot->GetComponentLocation() + FVector(0, 0, 6), FVector(8, 8, 9), "cansat");
	}
}

void AISHomeWorld::RebuildTray()
{
	for (auto& KV : TrayModels) KV.Value.Destroy();
	TrayModels.Reset();
	for (auto& KV : TrayHits) if (KV.Key.IsValid()) KV.Key->DestroyComponent();
	TrayHits.Reset();
	TrayParts.Reset();
	if (Stage != EISStage::Build) return;
	int32 i = 0;
	for (const FName& Id : CanSatTray())
	{
		const int32 Row = i / 7, Col = i % 7;
		++i;
		if (Design.Has(Id)) continue;
		if (UsesHunt() && !IsFound(Id)) continue;
		TrayParts.Add(Id);
		const FVector Pos = BenchTop + FVector(-48.f + Col * 16.f, 14.f + Row * 13.f, 0.2f);
		USceneComponent* Slot = NewObject<USceneComponent>(this);
		Slot->SetupAttachment(Root);
		Slot->SetRelativeLocation(Pos);
		Slot->RegisterComponent();
		FISModel& M = TrayModels.Add(Id);
		ISBuild::Prim(this, Slot, ISMat::Cube(), FVector(0, 0, 0.1f), FVector(11.f, 10.f, 0.2f), FLinearColor(0.12f, 0.12f, 0.14f), &M);
		ISBuild::CanSatPart(this, Slot, Id, FVector(0, 0, 0.2f), &M);
		UBoxComponent* B = NewObject<UBoxComponent>(this);
		B->SetupAttachment(Slot);
		B->SetBoxExtent(FVector(5.5f, 5.f, 3.f));
		B->SetRelativeLocation(FVector(0, 0, 3));
		B->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		B->SetCollisionResponseToAllChannels(ECR_Ignore);
		B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		B->RegisterComponent();
		TrayHits.Add(B, Id);
	}
	// the can itself is clickable (click an installed part in the list to remove it)
}

FName AISHomeWorld::PartUnderCursor(UPrimitiveComponent* C) const
{
	if (const FName* N = TrayHits.Find(C)) return *N;
	return NAME_None;
}

void AISHomeWorld::BenchToggle(FName Id)
{
	const FISPart* P = ISData::FindPart(Id);
	if (!P || Stage != EISStage::Build) return;
	BenchInfoPart = Id;
	const bool bHad = Design.Has(Id);
	if (P->Cat == EISCat::Payload || P->Cat == EISCat::Extra) Design.Toggle(Id);
	else if (bHad)
	{
		const FName None = P->Cat == EISCat::Power ? FName("pwr_none") : P->Cat == EISCat::ADCS ? FName("adcs_none") : NAME_None;
		Design.SetSingle(P->Cat, None);
	}
	else Design.SetSingle(P->Cat, Id);
	Eval = ISSim::Evaluate(Design, ISData::Missions()[MissionIdx]);
	BenchHandGoal = bHad ? BenchTop + FVector(0, 25, 8) : CanRoot->GetComponentLocation() + FVector(0, 6, 9);
	BenchHandT = 1.f;
	RebuildCan();
	RebuildTray();
	++DesignVersion;
}

void AISHomeWorld::BenchSeal()
{
	if (!Design.GetSingle(EISCat::Battery).IsNone() && !Design.GetSingle(EISCat::Computer).IsNone() && !Design.GetSingle(EISCat::Comms).IsNone() && (!UsesHunt() || Design.HasTag(ISTag::Wiring)))
	{
		Stage = EISStage::Sealed;
		++DesignVersion;
		RebuildCan();
		RebuildTray();
		Toast(TEXT("Can sealed! Pick it up (E) and carry it out to the LAUNCH PAD in the field (follow the dirt path)."));
	}
	else Toast(TEXT("A CanSat needs at least a battery, a computer (ESP32), a radio and wires to connect them before you can seal it."));
}

// =====================================================================================================
//  interaction
// =====================================================================================================
FName AISHomeWorld::ActionFor(UPrimitiveComponent* C) const
{
	if (const FName* N = Actions.Find(C)) return *N;
	return NAME_None;
}

FString AISHomeWorld::PromptFor(FName A) const
{
	if (A == "laptop") return TEXT("E  Open order board");
	const FString AS = A.ToString();
	if (AS.StartsWith(TEXT("door:")))
	{
		const int32 I = FCString::Atoi(*AS.Mid(5));
		return Doors.IsValidIndex(I) && Doors[I].Target > 1.f ? TEXT("E  Close door") : TEXT("E  Open door");
	}
	if (AS.StartsWith(TEXT("item:")))
	{
		const FName P(*AS.Mid(5));
		for (const FISHuntItem& H : Hunt) if (H.Part == P && !H.bFound) return FString::Printf(TEXT("E  Take: %s"), *H.Name);
		return FString();
	}
	if (A == "radio") return TEXT("E  Ground-station radio (what is it?)");
	if (A == "bench") return Stage == EISStage::Build ? TEXT("E  Work at the bench (build the CanSat)") : Stage == EISStage::NoOrder ? TEXT("Workbench: take an order first (laptop)") : TEXT("Workbench");
	if (A == "cansat") return TEXT("E  Pick up the CanSat");
	if (A == "pad")
	{
		if (Stage == EISStage::Carry) return TEXT("E  Load the CanSat into the rocket");
		if (Stage == EISStage::OnPad) return TEXT("E  Start the launch countdown");
		return TEXT("Launch pad");
	}
	if (A == "landed") return TEXT("E  Recover the CanSat");
	return FString();
}

void AISHomeWorld::Interact(FName A, AISBitCharacter* Bit)
{
	if (Bit) Bit->Reach();
	const FString AS = A.ToString();
	if (AS.StartsWith(TEXT("door:")))
	{
		const int32 I = FCString::Atoi(*AS.Mid(5));
		if (Doors.IsValidIndex(I))
		{
			if (Doors[I].bLocked) Toast(Doors[I].LockMsg);
			else Doors[I].Target = Doors[I].Target > 1.f ? 0.f : Doors[I].OpenAngle;
		}
		return;
	}
	if (AS.StartsWith(TEXT("item:")))
	{
		const FName P(*AS.Mid(5));
		for (FISHuntItem& H : Hunt)
			if (H.Part == P && !H.bFound)
			{
				H.bFound = true;
				H.Model.SetVisible(false);
				if (H.Hit) H.Hit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				const FISPart* Pt = ISData::FindPart(P);
				FString Edu = Pt ? Pt->Edu : FString();
				int32 Dot;
				if (Edu.FindChar(TEXT('.'), Dot) && Dot < 220) Edu = Edu.Left(Dot + 1);
				Toast(FString::Printf(TEXT("Found: %s! %s  (%d/%d required parts)"), *H.Name, *Edu, FoundCount(true), RequiredCount()));
				if (FoundCount(true) == RequiredCount()) Toast(TEXT("All required parts found! Take them to the WORKBENCH in the garage (E) and build your CanSat."));
				RebuildTray();
				++DesignVersion;
			}
		return;
	}
	if (A == "radio")
		Toast(TEXT("This radio listens on 433 MHz, the same band as your CanSat. You also carry a handheld receiver with a directional (Yagi) antenna: point it at the signal to make it stronger."));
	else if (A == "cansat" && Stage == EISStage::Sealed && Bit)
	{
		if (IsValid(CanHit)) { Actions.Remove(CanHit); CanHit->DestroyComponent(); CanHit = nullptr; }
		CanRoot->AttachToComponent(Bit->HoldRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CanRoot->SetRelativeLocation(FVector(0, 0, -6));
		Bit->bHolding = true;
		Stage = EISStage::Carry;
		Toast(TEXT("Got it. Walk to the launch pad (hold SHIFT to float faster)."));
	}
	else if (A == "pad" && Stage == EISStage::Carry && Bit)
	{
		CanRoot->AttachToComponent(RocketRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CanRoot->SetRelativeLocation(FVector(0, 0, 89));
		Bit->bHolding = false;
		Stage = EISStage::OnPad;
		Toast(FString::Printf(TEXT("CanSat loaded into the payload bay. Rocket + CanSat mass: %.2f kg. Press E at the pad to start the countdown, then stand back!"), 1.1f + Eval.TotalKg));
	}
	else if (A == "pad" && Stage == EISStage::OnPad)
	{
		Stage = EISStage::Countdown;
		Countdown = 5.f;
		FRandomStream R(FPlatformTime::Cycles());
		const float WindSpeed = R.FRandRange(2.5f, 6.5f);
		WindMs = FRotator(0, R.FRandRange(0.f, 360.f), 0).Vector() * WindSpeed;
		Toast(FString::Printf(TEXT("Countdown! Wind %.1f m/s. Watch the sky - press F for the chase camera."), WindSpeed));
	}
	else if (A == "landed" && Stage == EISStage::Landed)
	{
		Stage = EISStage::Recovered;
		Report.bFound = true;
		Report.SearchS = SearchT;
		Report.TelemetryPct = TelemetryTotal > 0 ? 100.f * TelemetryGot / TelemetryTotal : 0.f;
		Report.Samples = Telemetry.Num();
		if (IsValid(LandedHit)) { Actions.Remove(LandedHit); LandedHit->DestroyComponent(); LandedHit = nullptr; }
		CanRoot->AttachToComponent(Bit->HoldRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CanRoot->SetRelativeLocation(FVector(0, 0, -6));
		ChuteModel.Destroy();
		Bit->bHolding = true;
		bDebriefReady = true;
	}
}

FString AISHomeWorld::Objective() const
{
	switch (Stage)
	{
	case EISStage::NoOrder: return TEXT("Check the ORDER laptop on the workbench.");
	case EISStage::Build:
		if (UsesHunt() && FoundCount(true) < RequiredCount())
			return FString::Printf(TEXT("Find the CanSat parts around the house: %d/%d found.  Press Q to scan for parts nearby."), FoundCount(true), RequiredCount());
		return TEXT("Build the CanSat at the WORKBENCH in the garage (E). Then seal the can.");
	case EISStage::Sealed: return TEXT("Pick up the sealed CanSat.");
	case EISStage::Carry: return TEXT("Carry the CanSat to the LAUNCH PAD (follow the dirt path).");
	case EISStage::OnPad: return TEXT("Start the countdown at the pad (E).");
	case EISStage::Countdown: return FString::Printf(TEXT("T-%.0f ... stand back!"), FMath::CeilToFloat(Countdown));
	case EISStage::Boost: case EISStage::Coast: return TEXT("Liftoff! Watch the telemetry.");
	case EISStage::Descent: return TEXT("CanSat descending - note the wind drift. Get ready to track it!");
	case EISStage::Landed: return TEXT("Find the CanSat: use your receiver (point it toward the strongest signal).");
	case EISStage::Recovered: return TEXT("Recovered! Debrief time.");
	}
	return FString();
}

// =====================================================================================================
//  flight + tracking
// =====================================================================================================
void AISHomeWorld::Tick(float Dt)
{
	Super::Tick(Dt);
	if (IsHidden()) return;

	// doors swing smoothly
	for (int32 i = 0; i < Doors.Num(); ++i)
	{
		FISDoor& D = Doors[i];
		if (!D.Hinge || FMath::IsNearlyEqual(D.Angle, D.Target, 0.05f)) continue;
		D.Angle = FMath::FInterpTo(D.Angle, D.Target, Dt, 7.f);
		D.Hinge->SetRelativeRotation(FRotator(0, (DoorYaw.IsValidIndex(i) ? DoorYaw[i] : 0.f) + D.Angle, 0));
	}
	ScanT = FMath::Max(0.f, ScanT - Dt);

	// bench hands animation
	const bool bBench = Stage == EISStage::Build;
	BenchHandModel.SetVisible(false);
	if (bBench)
	{
		BenchHandT = FMath::Max(0.f, BenchHandT - Dt * 1.5f);
		const FVector Rest = BenchTop + FVector(18, 30, 14);
		const float R = FMath::Sin(BenchHandT * PI);
		BenchHands->SetWorldLocation(FMath::Lerp(Rest, BenchHandGoal, R) + FVector(0, 0, FMath::Sin(GetWorld()->TimeSeconds * 2.f) * 1.5f));
		BenchHands->SetWorldRotation(FRotator(-30, -90, 0));
	}

	if (Stage == EISStage::Countdown)
	{
		Countdown -= Dt;
		if (Countdown <= 0.f)
		{
			Stage = EISStage::Boost;
			FlightT = 0.f; StageT = 0.f;
			RocketPosM = FVector(0, 0, 0.4f); RocketVelM = FVector::ZeroVector;
			TelemetryTotal = TelemetryGot = 0;
			Telemetry.Reset();
			Report = FISFlightReport();
			Report.bChute = Design.HasTag(ISTag::Parachute);
			bHasGnss = Design.HasTag(ISTag::GNSSRx);
		}
	}
	if (Stage >= EISStage::Boost && Stage <= EISStage::Descent)
	{
		// sub-step physics for stability
		float Left = Dt;
		while (Left > 0.f) { const float H = FMath::Min(Left, 0.01f); StepFlight(H); Left -= H; }
	}
	if (IsTracking()) UpdateTracker(Dt);

	// chase camera
	if (ChaseCam)
	{
		const FVector Target = (Stage >= EISStage::Descent) ? CanRoot->GetComponentLocation() : RocketRoot->GetComponentLocation() + FVector(0, 0, 80);
		const FVector Want = Target + FVector(-600.f, -350.f, 150.f);
		ChaseCam->SetActorLocation(FMath::VInterpTo(ChaseCam->GetActorLocation(), Want, Dt, 3.f));
		ChaseCam->SetActorRotation((Target - ChaseCam->GetActorLocation()).Rotation());
	}
}

void AISHomeWorld::StepFlight(float Dt)
{
	FlightT += Dt; StageT += Dt;
	const float CanKg = FMath::Max(0.05f, Eval.TotalKg);
	auto RhoAt = [](float Z) { return Rho * FMath::Exp(-Z / 8500.f); };

	if (Stage == EISStage::Boost || Stage == EISStage::Coast)
	{
		const float Burn = 1.6f, Thrust = 200.f;
		const float Mass = 0.9f + CanKg + 0.2f * FMath::Max(0.f, 1.f - FlightT / Burn);
		const float A = PI * FMath::Square(0.038f);
		const FVector Rel = RocketVelM - WindMs * FMath::Min(1.f, RocketPosM.Z / 10.f);
		const FVector Drag = -0.5f * RhoAt(RocketPosM.Z) * 0.5f * A * Rel.Size() * Rel;
		const FVector Up = RocketVelM.Size() > 5.f ? RocketVelM.GetSafeNormal() : FVector::UpVector;
		const FVector Th = (FlightT < Burn) ? Up * Thrust : FVector::ZeroVector;
		RocketVelM += ((Th + Drag) / Mass + FVector(0, 0, -G)) * Dt;
		if (RocketPosM.Z <= 0.4f && RocketVelM.Z < 0) RocketVelM = FVector::ZeroVector;
		RocketPosM += RocketVelM * Dt;
		Report.MaxSpeed = FMath::Max(Report.MaxSpeed, (float)RocketVelM.Size());
		RocketRoot->SetRelativeLocation(MToWorld(RocketPosM) + FVector(0, 0, 40));
		if (RocketVelM.Size() > 3.f) RocketRoot->SetRelativeRotation(FRotationMatrix::MakeFromZ(RocketVelM.GetSafeNormal()).Rotator());
		if (Stage == EISStage::Boost && FlightT >= Burn) { Stage = EISStage::Coast; Toast(TEXT("Motor burnout. Coasting up...")); }
		CanPosM = RocketPosM + FVector(0, 0, 0.9f);
		CanVelMs = RocketVelM;
		if (Stage == EISStage::Coast && RocketVelM.Z < 0.f)
		{
			// apogee: eject the CanSat
			Report.ApogeeM = RocketPosM.Z;
			Stage = EISStage::Descent; StageT = 0.f;
			CanRoot->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
			CanRoot->SetWorldRotation(FRotator::ZeroRotator);
			Toast(FString::Printf(TEXT("Apogee %.0f m! CanSat ejected."), Report.ApogeeM));
		}
	}
	else if (Stage == EISStage::Descent)
	{
		// spent rocket falls under its streamer
		RocketVelM = FMath::VInterpTo(RocketVelM, FVector(WindMs.X, WindMs.Y, -18.f), Dt, 1.5f);
		if (RocketPosM.Z > 0.f) { RocketPosM += RocketVelM * Dt; RocketRoot->SetRelativeLocation(MToWorld(RocketPosM) + FVector(0, 0, 40)); }
		RocketRoot->SetRelativeRotation(FRotator(-80.f, 0, 0));

		const bool bChute = Report.bChute && StageT > 1.0f;
		if (bChute && !bChuteOpen)
		{
			bChuteOpen = true;
			ChuteModel.Destroy();
			float Area = 0.f;
			for (const FName& Id : Design.Parts) if (const FISPart* P = ISData::FindPart(Id)) if (P->Tag == ISTag::Parachute) Area += P->SailM2;
			ISBuild::Parachute(this, ChuteRoot, Area, &ChuteModel);
			Toast(TEXT("Parachute deployed!"));
		}
		float Area = 0.0034f, Cd = 0.8f;
		if (bChute)
		{
			Area = 0.f;
			for (const FName& Id : Design.Parts) if (const FISPart* P = ISData::FindPart(Id)) if (P->Tag == ISTag::Parachute) Area += P->SailM2;
		}
		const FVector Rel = CanVelMs - WindMs;
		const FVector Drag = -0.5f * RhoAt(CanPosM.Z) * Cd * Area * Rel.Size() * Rel;
		CanVelMs += (Drag / CanKg + FVector(0, 0, -G)) * Dt;
		CanPosM += CanVelMs * Dt;
		CanRoot->SetWorldLocation(MToWorld(CanPosM));
		if (CanPosM.Z <= 0.f)
		{
			CanPosM.Z = 0.f;
			Report.LandingSpeed = FMath::Abs(CanVelMs.Z);
			Report.bBroken = Report.LandingSpeed > 15.f;
			Report.DriftM = FVector::Dist2D(CanPosM, FVector::ZeroVector);
			Report.FlightS = FlightT;
			Stage = EISStage::Landed;
			SearchT = 0.f;
			CanRoot->SetWorldLocationAndRotation(MToWorld(CanPosM) + FVector(0, 0, 3), FRotator(0, 0, 75));
			ChuteRoot->SetRelativeScale3D(FVector(1, 1, 0.08f));
			RocketPosM.Z = 0.f;
			LandedHit = AddInteract(CanRoot->GetComponentLocation() + FVector(0, 0, 10), FVector(40, 40, 30), "landed");
			const double FlightH = FlightT / 3600.0;
			BeaconLeftS = FMath::Max(0.f, (float)((Eval.RuntimeH - FlightH) * 60.0)); // 1 game minute = 1 hour of battery
			Toast(FString::Printf(TEXT("Touchdown at %.1f m/s, %.0f m from the pad. Go find it!%s"), Report.LandingSpeed, Report.DriftM,
				Report.bBroken ? TEXT(" (That landing was hard: something probably broke.)") : TEXT("")));
		}
	}

	// telemetry downlink every 0.5 s while the beacon works and is in range of BIT's receiver
	static float Acc = 0.f;
	Acc += Dt;
	if (Acc >= 0.5f)
	{
		Acc = 0.f;
		++TelemetryTotal;
		const float Alt = Stage == EISStage::Descent ? CanPosM.Z : RocketPosM.Z;
		if (bTrackSignal)
		{
			++TelemetryGot;
			FRandomStream R(TelemetryTotal * 7919);
			FISTelemetry S;
			S.T = FlightT; S.AltM = Alt;
			S.TempC = 15.f - 6.5f * Alt / 1000.f + R.FRandRange(-0.3f, 0.3f);
			S.PressHPa = 1013.25f * FMath::Exp(-Alt / 8400.f);
			Telemetry.Add(S);
		}
	}
}

void AISHomeWorld::UpdateTracker(float Dt)
{
	APawn* P = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!P) return;
	if (Stage == EISStage::Landed) { SearchT += Dt; BeaconLeftS = FMath::Max(0.f, BeaconLeftS - Dt); }
	const FVector CanW = CanRoot->GetComponentLocation();
	const FVector Me = P->GetActorLocation();
	TrackDistM = FVector::Dist(Me, CanW) / 100.f;
	TrackDirWorld = (CanW - Me).GetSafeNormal();
	const FISPart* Radio = ISData::FindPart(Design.GetSingle(EISCat::Comms));
	const float RangeKm = Radio ? FMath::Max(0.1f, Radio->RadioRangeKm) : 0.f;
	const bool bAlive = Stage != EISStage::Landed || (BeaconLeftS > 0.f && !Report.bBroken);
	Report.bBeaconAlive = bAlive;
	// free-space path loss at 433 MHz; receiver sensitivity set so that the radio's rated range is the limit
	auto FSPL = [](float Km) { return 20.f * FMath::LogX(10.f, FMath::Max(0.001f, Km)) + 20.f * FMath::LogX(10.f, 433.f) + 32.44f; };
	TrackSens = 10.f - FSPL(RangeKm);
	// handheld Yagi: +7 dB pointing at the source, -10 dB pointing away
	FVector Fwd = P->GetControlRotation().Vector();
	const float Cos = FVector::DotProduct(Fwd.GetSafeNormal2D(), TrackDirWorld.GetSafeNormal2D());
	const float Gain = -1.5f + 8.5f * Cos;
	TrackRssi = 10.f - FSPL(TrackDistM / 1000.f) + Gain + FMath::FRandRange(-1.5f, 1.5f);
	bTrackSignal = Radio && bAlive && TrackRssi > TrackSens;
}
