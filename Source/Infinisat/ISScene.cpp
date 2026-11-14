#include "ISScene.h"
#include "ISSim.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor KeepMat(0, 0, 0, 0);
	template <typename T> T* Load(const TCHAR* Path) { return LoadObject<T>(nullptr, Path); }
	const TCHAR* TEX_EARTH = TEXT("/Game/Infinisat/Textures/T_Earth.T_Earth");
	const TCHAR* TEX_MOON = TEXT("/Game/Infinisat/Textures/T_Moon.T_Moon");
	const TCHAR* TEX_JUP = TEXT("/Game/Infinisat/Textures/T_Jupiter.T_Jupiter");
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

void AISSpaceScene::ClearView()
{
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
	CamYaw += DYaw;
	CamPitch = FMath::Clamp(CamPitch + DPitch, -85.f, 85.f);
}

void AISSpaceScene::ZoomCamera(float Steps)
{
	CamDistGoal = FMath::Clamp(CamDistGoal * FMath::Pow(0.88f, Steps), CamMin, CamMax);
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
void AISSpaceScene::BuildSatellite(USceneComponent* Root, const FISDesign& D, TArray<UStaticMeshComponent*>& Track)
{
	TArray<const FISPart*> P;
	for (const FName& Id : D.Parts) if (const FISPart* X = ISData::FindPart(Id)) P.Add(X);
	const FISPart* Bus = nullptr;
	bool bMLI = false;
	for (const FISPart* X : P) { if (X->Cat == EISCat::Bus) Bus = X; if (X->Tag == ISTag::Heaters) bMLI = true; }

	// bus body (100 uu reference size)
	FVector BusExt(50, 50, 60);
	if (Bus)
	{
		const bool bCube = Bus->MeshPath.Contains(TEXT("Cube"));
		const bool bCyl = Bus->MeshPath.Contains(TEXT("Cylinder"));
		const FLinearColor C = bMLI ? FLinearColor(0.95f, 0.72f, 0.18f) : Bus->Color;
		if (bCube) { AddMesh(Root, MCube, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.f, 1.f, 1.2f)), C, false, 0.f, &Track); }
		else if (bCyl)
		{
			const bool bCan = Bus->Id == "bus_can";
			BusExt = bCan ? FVector(33, 33, 58) : FVector(55, 55, 45);
			AddMesh(Root, MCyl, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, bCan ? FVector(0.66f, 0.66f, 1.15f) : FVector(1.1f, 1.1f, 0.9f)), C, false, 0.f, &Track);
			if (!bCan) AddMesh(Root, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, 47), FVector(1.15f, 1.15f, 0.06f)), FLinearColor(0.2f, 0.2f, 0.22f), false, 0.f, &Track);
		}
		else
		{
			// NASA satellite-kit body (normalized to 1 m = 100 uu)
			AddMeshPath(Root, Bus->MeshPath, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.2f)), KeepMat, &Track);
			AddMesh(Root, MCube, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.7f, 0.7f, 0.9f)), FLinearColor(0.7f, 0.7f, 0.75f), false, 0.f, &Track);
			BusExt = FVector(40, 40, 50);
		}
	}

	int32 PayloadIdx = 0, ExtraIdx = 0;
	for (const FISPart* X : P)
	{
		switch (X->Cat)
		{
		case EISCat::Power:
			if (X->SolarW > 0)
			{
				const float L = FMath::Clamp(30.f * FMath::Sqrt(X->SolarW / 20.f), 30.f, 260.f);
				const float H = FMath::Clamp(L * 0.35f, 30.f, 110.f);
				if (X->Id == "pwr_body")
				{
					for (int32 s : { -1, 1 })
						AddMesh(Root, MCube, FTransform(FRotator::ZeroRotator, FVector(0, s * (BusExt.Y + 1.f), 0), FVector(BusExt.X * 1.6f / 100.f, 0.02f, BusExt.Z * 1.6f / 100.f)), FLinearColor(0.05f, 0.08f, 0.35f), false, 0.f, &Track);
					break;
				}
				const int32 Wings = X->Id == "pwr_juno" ? 3 : 2;
				for (int32 w = 0; w < Wings; ++w)
				{
					const float Yaw = Wings == 3 ? w * 120.f : w * 180.f;
					const FRotator R(0, Yaw, 0);
					const FVector Dir = R.RotateVector(FVector(0, 1, 0));
					const FVector Boom = Dir * (BusExt.Y + 12.f);
					AddMesh(Root, MCyl, FTransform(FRotator(0, Yaw, 90), Boom, FVector(0.05f, 0.05f, 0.25f)), FLinearColor(0.6f, 0.6f, 0.62f), false, 0.f, &Track);
					AddMesh(Root, MCube, FTransform(R, Dir * (BusExt.Y + 25.f + L * 0.5f), FVector(0.03f, L / 100.f, H / 100.f)), FLinearColor(0.1f, 0.2f, 0.65f), false, 0.08f, &Track);
					// cell grid lines
					for (int32 g = 1; g < 4; ++g)
						AddMesh(Root, MCube, FTransform(R, Dir * (BusExt.Y + 25.f + L * g / 4.f) + FVector(0.f), FVector(0.035f, 0.01f, H / 100.f)), FLinearColor(0.75f, 0.75f, 0.8f), false, 0.f, &Track);
				}
			}
			else if (X->RtgW > 0)
			{
				const int32 N = X->Id == "pwr_gphs" ? 3 : 1;
				for (int32 i = 0; i < N; ++i)
				{
					const FVector Pos(-BusExt.X - 60.f - i * 45.f, -50.f, -10.f);
					AddMesh(Root, MCyl, FTransform(FRotator(90, 0, 0), Pos, FVector(0.22f, 0.22f, 0.4f)), FLinearColor(0.25f, 0.25f, 0.27f), false, 0.f, &Track);
					for (int32 f = 0; f < 4; ++f)
						AddMesh(Root, MCube, FTransform(FRotator(0, 0, f * 45.f), Pos, FVector(0.38f, 0.02f, 0.4f)), FLinearColor(0.3f, 0.3f, 0.32f), false, 0.f, &Track);
				}
				AddMesh(Root, MCyl, FTransform(FRotator(90, 0, 0), FVector(-BusExt.X - 30.f, -50.f, -10.f), FVector(0.04f, 0.04f, 0.6f)), FLinearColor(0.6f, 0.6f, 0.6f), false, 0.f, &Track);
			}
			break;
		case EISCat::Comms:
			if (X->DishM > 1.f || X->Tag == ISTag::HGA)
			{
				const float S = 0.6f + X->DishM * 0.35f;
				AddMesh(Root, MSphere, FTransform(FRotator::ZeroRotator, FVector(0, 0, BusExt.Z + 18.f), FVector(S, S, 0.22f)), FLinearColor(0.93f, 0.93f, 0.95f), false, 0.f, &Track);
				AddMesh(Root, MCone, FTransform(FRotator::ZeroRotator, FVector(0, 0, BusExt.Z + 40.f), FVector(0.12f, 0.12f, 0.35f)), FLinearColor(0.85f, 0.85f, 0.85f), false, 0.f, &Track);
			}
			else if (X->RadioMbps < 0.1f)
			{
				for (int32 s : { -1, 1 })
					AddMesh(Root, MCyl, FTransform(FRotator(0, 0, s * 35.f), FVector(0, s * 20.f, BusExt.Z + 30.f), FVector(0.015f, 0.015f, 0.7f)), FLinearColor(0.85f, 0.85f, 0.85f), false, 0.f, &Track);
			}
			else
			{
				if (!X->MeshPath.IsEmpty()) AddMeshPath(Root, X->MeshPath, FTransform(FRotator::ZeroRotator, FVector(0, 0, BusExt.Z + 20.f), FVector(0.45f)), KeepMat, &Track);
				AddMesh(Root, MCone, FTransform(FRotator(180, 0, 0), FVector(BusExt.X * 0.4f, 0, -BusExt.Z - 10.f), FVector(0.2f, 0.2f, 0.15f)), FLinearColor(0.9f, 0.9f, 0.9f), false, 0.f, &Track);
			}
			break;
		case EISCat::ADCS:
			if (X->PointingDeg > 0 && X->PointingDeg < 0.1f)
				for (int32 s : { -1, 1 })
					AddMesh(Root, MCyl, FTransform(FRotator(0, 0, s * 30.f), FVector(-BusExt.X * 0.6f, s * BusExt.Y * 0.6f, BusExt.Z + 6.f), FVector(0.1f, 0.1f, 0.18f)), FLinearColor(0.1f, 0.1f, 0.1f), false, 0.f, &Track);
			break;
		case EISCat::Propulsion:
			if (X->Tag == ISTag::SolarSail)
			{
				const float Side = 220.f * (1.f + FMath::LogX(10.f, X->SailM2 / 80.f + 1.f));
				const FString SailPath = ModelPath(TEXT("SolarSail"));
				if (!AddMeshPath(Root, SailPath, FTransform(FRotator(0, 0, 0), FVector(-BusExt.X - 40.f, 0, 0), FVector(Side / 100.f)), KeepMat, &Track))
					AddMesh(Root, MCube, FTransform(FRotator::ZeroRotator, FVector(-BusExt.X - 40.f, 0, 0), FVector(0.01f, Side / 100.f, Side / 100.f)), FLinearColor(0.9f, 0.9f, 0.95f), false, 0.3f, &Track);
			}
			else if (X->IspS > 1000.f)
			{
				AddMesh(Root, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, -BusExt.Z - 8.f), FVector(0.3f, 0.3f, 0.16f)), FLinearColor(0.3f, 0.3f, 0.35f), false, 0.f, &Track);
				AddMesh(Root, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, -BusExt.Z - 17.f), FVector(0.24f, 0.24f, 0.02f)), FLinearColor(0.3f, 0.5f, 1.f), true, 6.f, &Track);
			}
			else if (X->PropKg > 0)
			{
				const float S = FMath::Clamp(0.15f + FMath::Pow(X->PropKg, 0.33f) * 0.04f, 0.12f, 0.7f);
				AddMesh(Root, MCone, FTransform(FRotator::ZeroRotator, FVector(0, 0, -BusExt.Z - S * 60.f), FVector(S * 0.8f, S * 0.8f, S)), FLinearColor(0.35f, 0.33f, 0.32f), false, 0.f, &Track);
			}
			break;
		case EISCat::Payload:
		{
			const float S = FMath::Clamp(0.12f + FMath::Pow(X->MassKg, 0.33f) * 0.06f, 0.12f, 0.75f);
			const int32 Col = PayloadIdx % 3, Row = PayloadIdx / 3;
			const FVector Pos(BusExt.X + S * 30.f, (Col - 1) * BusExt.Y * 0.6f, BusExt.Z * 0.5f - Row * 40.f);
			if (X->Tag == ISTag::EntryProbe)
				AddMesh(Root, MCone, FTransform(FRotator(-90, 0, 0), FVector(BusExt.X + 45.f, 0, -BusExt.Z * 0.6f), FVector(0.8f, 0.8f, 0.45f)), X->Color, false, 0.f, &Track);
			else if (X->Tag == ISTag::Record)
				AddMesh(Root, MCyl, FTransform(FRotator(0, 0, 90), FVector(0, BusExt.Y + 2.f, -BusExt.Z * 0.4f), FVector(0.3f, 0.3f, 0.02f)), X->Color, false, 0.4f, &Track);
			else
			{
				UStaticMesh* M = X->MeshPath.Contains(TEXT("Cube")) ? MCube : X->MeshPath.Contains(TEXT("Cone")) ? MCone : MCyl;
				AddMesh(Root, M, FTransform(FRotator(90, 0, 0), Pos, FVector(S, S, S * 1.2f)), X->Color, false, 0.f, &Track);
				if (X->Tag == ISTag::Imager || X->Tag == ISTag::Camera || X->Tag == ISTag::WeatherImager)
					AddMesh(Root, MCyl, FTransform(FRotator(90, 0, 0), Pos + FVector(S * 61.f, 0, 0), FVector(S * 0.6f, S * 0.6f, 0.01f)), FLinearColor(0.1f, 0.3f, 0.6f), true, 1.5f, &Track);
			}
			++PayloadIdx;
			break;
		}
		case EISCat::Extra:
			if (X->Tag == ISTag::Parachute)
				AddMesh(Root, MCone, FTransform(FRotator::ZeroRotator, FVector(0, 0, BusExt.Z + 35.f), FVector(X->SailM2 > 0.1f ? 1.0f : 0.6f, X->SailM2 > 0.1f ? 1.0f : 0.6f, 0.35f)), FLinearColor(1.f, 0.35f, 0.1f), false, 0.f, &Track);
			else if (X->Tag == ISTag::Vault)
				AddMesh(Root, MCube, FTransform(FRotator::ZeroRotator, FVector(0, 0, BusExt.Z * 0.2f), FVector(BusExt.X * 1.9f / 100.f, BusExt.Y * 1.9f / 100.f, 0.4f)), FLinearColor(0.5f, 0.5f, 0.55f), false, 0.f, &Track);
			else if (X->Tag == ISTag::DragSail)
				AddMesh(Root, MCube, FTransform(FRotator::ZeroRotator, FVector(-BusExt.X - 8.f, 0, BusExt.Z * 0.5f), FVector(0.12f, 0.25f, 0.25f)), FLinearColor(0.8f, 0.8f, 0.85f), false, 0.f, &Track);
			else if (X->Tag == ISTag::Retro)
				AddMesh(Root, MCyl, FTransform(FRotator(90, 0, 0), FVector(BusExt.X + 2.f, BusExt.Y * 0.7f, -BusExt.Z * 0.7f), FVector(0.2f, 0.2f, 0.03f)), FLinearColor(0.1f, 0.1f, 0.1f), false, 0.f, &Track);
			++ExtraIdx;
			break;
		default: break;
		}
	}
}

// ------------------------------------------------------------------------------------------------ views
void AISSpaceScene::ShowTitle()
{
	ClearView();
	View = EISView::Title;
	SetPlanet(Planet, TEX_EARTH, 6000.f, FVector(0, 0, -6650.f));
	PlanetAtmo->SetWorldScale3D(FVector(6150.f / 50.f));
	PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation());
	PlanetAtmo->SetVisibility(true);
	BitRoot->SetVisibility(true, true);
	BitHome = FVector(0, -170, 30); BitBaseScale = 1.1f;
	AddMeshPath(RealRoot, ModelPath(TEXT("Voyager")), FTransform(FRotator(10, 30, 0), FVector(-600, -560, 260), FVector(3.f)), KeepMat, &RealParts);
	SetCameraTarget(FVector(0, 30, 40), 900.f, 180.f, -6.f, 400.f, 3000.f);
}

void AISSpaceScene::ShowWorkshop(const FISDesign& D)
{
	ClearView();
	View = EISView::Workshop;
	SetPlanet(Planet, TEX_EARTH, 6000.f, FVector(-1500, 0, -7000.f));
	PlanetAtmo->SetWorldScale3D(FVector(6150.f / 50.f));
	PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation());
	PlanetAtmo->SetVisibility(true);
	BuildSatellite(SatRoot, D, SatParts);
	BitRoot->SetVisibility(true, true);
	BitHome = FVector(80, -220, 110); BitBaseScale = 0.7f;
	SetCameraTarget(FVector(0, -60, 20), 1000.f, 205.f, -12.f, 250.f, 4000.f);
}

void AISSpaceScene::ShowCompare(const FISDesign& D, const FString& RealMesh, const FString& RealName)
{
	ClearView();
	View = EISView::Compare;
	SetPlanet(Planet, TEX_EARTH, 6000.f, FVector(-1500, 0, -7000.f));
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
	SetPlanet(Planet, TEX_EARTH, 6000.f, FVector(-1500, 0, -7000.f));
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
		const bool bMoon = Dest == EISDest::Moon;
		ViewK = 1.f;
		const float Rb = bMoon ? 1737.4f : 6371.f;
		if (bMoon)
		{
			SetPlanet(Moon, TEX_MOON, Rb, FVector::ZeroVector);
			SetPlanet(Planet, TEX_EARTH, 6371.f, FVector(-384400.f, 0, 0));
			PlanetAtmo->SetWorldScale3D(FVector(6500.f / 50.f)); PlanetAtmo->SetWorldLocation(Planet->GetComponentLocation()); PlanetAtmo->SetVisibility(true);
			AddLabel(TEXT("Earth: 384,400 km away (1.3 light-seconds)"), FVector(-384400.f, 0, 9000.f), FColor::White);
			AddLabel(TEXT("South pole: permanently shadowed craters (ice?)"), FVector(0, 0, -Rb * 1.25f), FColor(170, 210, 255));
			for (int32 i = 0; i < 6; ++i)
			{
				const float A = FMath::DegreesToRadians(i * 60.f);
				AddMesh(SceneRoot, MSphere, FTransform(FRotator::ZeroRotator, FVector(FMath::Cos(A) * 150.f, FMath::Sin(A) * 150.f, -Rb * 0.99f), FVector(0.8f)), FLinearColor(0.2f, 1.f, 0.4f), true, 4.f, nullptr);
			}
			AddLabel(TEXT("Candidate landing sites"), FVector(0, 0, -Rb * 1.12f), FColor(120, 255, 150));
			SatSize = 140.f;
		}
		else
		{
			SetPlanet(Planet, TEX_EARTH, Rb, FVector::ZeroVector);
			PlanetAtmo->SetWorldScale3D(FVector(Rb * 1.02f / 50.f)); PlanetAtmo->SetWorldLocation(FVector::ZeroVector); PlanetAtmo->SetVisibility(true);
			BuildEarthPopulation(1.f);
			SatSize = FMath::Clamp(AltKm * 0.04f, 250.f, 1200.f);
		}
		const float R = Rb + AltKm;
		OrbitRadiusUU = R;
		OrbitInc = IncDeg;
		AddOrbitRing(R, IncDeg, 20.f, Mine, 220, bMoon ? 0.2f : FMath::Clamp(R / 6000.f, 0.6f, 4.f));
		if (Fleet > 1)
		{
			const int32 Planes = FMath::Clamp(Fleet / 12, 2, 12);
			UInstancedStaticMeshComponent* I = AddDots(MCube, Mine, 3.f);
			for (int32 i = 0; i < Fleet; ++i)
			{
				const int32 Pn = i % Planes;
				const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(Pn * 360.f / Planes)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(IncDeg));
				const float A = 2.f * PI * (i / Planes) / FMath::Max(1, Fleet / Planes) + Pn * 0.3f;
				I->AddInstance(FTransform(FRotator::ZeroRotator, Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0) * R), FVector(1.6f)));
			}
			AddLabel(FString::Printf(TEXT("Your fleet: %d satellites in %d planes"), Fleet, Planes), FVector(0, 0, R * 1.15f), FColor(255, 160, 60));
		}
		const float Dist = bMoon ? 7000.f : FMath::Max(24000.f, R * 3.2f);
		SetCameraTarget(FVector::ZeroVector, Dist, 210.f, -22.f, bMoon ? 2500.f : 9000.f, bMoon ? 450000.f : 200000.f);
	}

	// the player's spacecraft, scaled up so it is visible (not to scale!)
	BuildSatellite(SatRoot, D, SatParts);
	SatRoot->SetRelativeScale3D(FVector(SatSize / 150.f));
	AddMesh(SatRoot, MSphere, FTransform(FRotator::ZeroRotator, FVector(0, 0, 120.f), FVector(0.3f)), FLinearColor(1.f, 0.55f, 0.15f, 1.f), true, 6.f, &SatParts);
	SetMissionTime(0.0, 1.0, 100.0, true);
	LastSatSize = SatSize;
	MapDist = CamDistGoal;
	PlanetRUU = Dest == EISDest::Moon ? 1737.4f : 6371.f;
	const bool bNear = Dest != EISDest::Backyard && Dest != EISDest::Interstellar && Dest != EISDest::Jupiter;
	bFollow = bNear;
	if (bFollow) { CamMin = SatSize * 1.2f; CamDistGoal = SatSize * 5.f; CamDist = CamDistGoal; CamPitch = -12.f; }
}

void AISSpaceScene::ToggleMap()
{
	if (View != EISView::Orbit) return;
	bFollow = !bFollow;
	CamDistGoal = bFollow ? LastSatSize * 5.f : MapDist;
	CamMin = bFollow ? LastSatSize * 1.2f : 2000.f;
}

FString AISSpaceScene::LaunchCaption() const
{
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
	if (RM) AddMesh(RocketRoot, RM, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(Size / 100.f)), KeepMat, false, 0.f, nullptr);
	else AddMesh(RocketRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(Size / 900.f, Size / 900.f, Size / 100.f)), FLinearColor(0.95f, 0.95f, 0.95f), false, 0.f, nullptr);
	AddMesh(RocketRoot, MCone, FTransform(FRotator(180, 0, 0), FVector(0, 0, -Size * 0.62f), FVector(Size / 500.f, Size / 500.f, Size / 220.f)), FLinearColor(1.f, 0.55f, 0.15f), true, 9.f, nullptr);
	const FVector Sat = SatRoot->GetRelativeLocation();
	const FVector Dir = FQuat(FVector::UpVector, FMath::DegreesToRadians(-50.f)).RotateVector(Sat.GetSafeNormal());
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
		Pos = Q.RotateVector(FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0) * OrbitRadiusUU);
		break;
	}
	}
	SatRoot->SetRelativeLocation(Pos);
	SatRoot->SetRelativeRotation((-Pos).Rotation());
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
	if (LaunchT >= 0.f && RocketRoot)
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
	const FRotator CR(CamPitch, CamYaw, 0.f);
	const FVector CamLoc = CamFocus - CR.Vector() * CamDist;
	if (Camera) Camera->SetActorLocationAndRotation(CamLoc, CR);

	// planets spin
	if (Planet->IsVisible()) Planet->AddLocalRotation(FRotator(0, Dt * (View == EISView::Orbit ? 3.f : 0.6f), 0));
	if (Moon->IsVisible()) Moon->AddLocalRotation(FRotator(0, Dt * 0.5f, 0));

	// labels face the camera and keep a readable size
	for (UTextRenderComponent* L : Labels)
	{
		if (!L) continue;
		L->SetVisibility(!(View == EISView::Orbit && bFollow));   // follow view: clean screen, map view: labels
		const FVector P = L->GetComponentLocation();
		L->SetWorldRotation((CamLoc - P).Rotation());
		L->SetWorldSize(FMath::Max(8.f, FVector::Dist(CamLoc, P) * 0.014f));
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
