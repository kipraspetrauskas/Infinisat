// Infinisat - stylised orbit view (demo look).
// The satellite stays at the world origin (no precision issues at any distance or warp) and a low-poly Earth at 1/100
// scale turns underneath it: radius 63.71 km (6,371,000 uu), orbit altitudes scaled the same way (LEO 400 km -> 4 km,
// GNSS 20,200 km -> 202 km). A Sky Atmosphere sized to that planet gives the blue rim, low-poly cloud puffs turn a bit
// faster than the ground, the user's 360 space panorama is the sky, and one strong sun draws a clear day/night line.
#include "ISScene.h"
#include "ISWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace ISLP
{
	constexpr double Scale = 0.01;              // 1/100 of the real Earth
	constexpr double EarthKm = 6371.0;
	constexpr float VisualBoost = 6.f;          // the ground turns 6x faster than the real orbit rate at 1x (it reads as motion)
	UStaticMesh* Mesh(const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Infinisat/LP/%s/SM_%s.SM_%s"), Name, Name, Name)); }
}

bool AISSpaceScene::StylizedAvailable()
{
	// parked: opt-in only until it is finished (-ISStylizedOrbit); the normal orbit view is unchanged
	return FParse::Param(FCommandLine::Get(), TEXT("ISStylizedOrbit")) && ISLP::Mesh(TEXT("LowPolyEarth")) != nullptr;
}

void AISSpaceScene::BuildStylizedEarth(float AltKm, float IncDeg)
{
	bStylized = true;
	LPRadius = (float)(ISLP::EarthKm * ISLP::Scale * 1.0e5);            // 6,371,000 uu
	LPAlt = (float)(FMath::Max(150.0, (double)AltKm) * ISLP::Scale * 1.0e5);
	const FVector Center(0.f, 0.f, -(LPRadius + LPAlt));
	const double Mu = 398600.4418, Rorb = ISLP::EarthKm + AltKm;
	PeriodS = 2.0 * PI * FMath::Sqrt(Rorb * Rorb * Rorb / Mu);
	LPAngle = 0.0; LPCloudAngle = 0.0;

	if (!LPPivot)
	{
		LPPivot = NewObject<USceneComponent>(this);
		LPPivot->SetupAttachment(SceneRoot);
		LPPivot->RegisterComponent();
		LPSpin = NewObject<USceneComponent>(this);
		LPSpin->SetupAttachment(LPPivot);
		LPSpin->RegisterComponent();
		LPEarth = NewObject<UStaticMeshComponent>(this);
		LPEarth->SetStaticMesh(ISLP::Mesh(TEXT("LowPolyEarth")));
		LPEarth->SetupAttachment(LPSpin);
		LPEarth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LPEarth->SetCastShadow(false);
		if (UMaterialInterface* VC = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_VertexColor.M_IS_VertexColor")))
			LPEarth->SetMaterial(0, VC);
		LPEarth->RegisterComponent();
		LPCloudSpin = NewObject<USceneComponent>(this);
		LPCloudSpin->SetupAttachment(LPPivot);
		LPCloudSpin->RegisterComponent();
		LPClouds = NewObject<UInstancedStaticMeshComponent>(this);
		LPClouds->SetStaticMesh(ISLP::Mesh(TEXT("CloudPuff")));
		LPClouds->SetupAttachment(LPCloudSpin);
		LPClouds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LPClouds->SetCastShadow(false);
		if (UMaterialInterface* VC = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_VertexColor.M_IS_VertexColor")))
			LPClouds->SetMaterial(0, VC);
		LPClouds->RegisterComponent();
		// cloud systems: clusters of flattened low-poly puffs on a shell above the highest land (land is raised ~1.8%)
		FRandomStream R(2026);
		const float Shell = 1.035f * 100.f;   // mesh units: the Earth mesh has radius 100 uu before scaling
		for (int32 c = 0; c < 150; ++c)
		{
			const FVector Dir = R.GetUnitVector();
			if (FMath::Abs(Dir.Z) > 0.93f) continue;    // no clouds right on the poles
			const int32 N = R.RandRange(3, 9);
			for (int32 k = 0; k < N; ++k)
			{
				const FVector D = (Dir + R.GetUnitVector() * 0.045f).GetSafeNormal();
				const FQuat Q = FQuat::FindBetweenNormals(FVector::UpVector, D) * FQuat(FVector::UpVector, R.FRandRange(0.f, 2.f * PI));
				const float S = R.FRandRange(0.012f, 0.03f);
				LPClouds->AddInstance(FTransform(Q, D * Shell, FVector(S, S, S * 0.8f)));
			}
		}
	}
	LPPivot->SetWorldLocation(Center);
	LPPivot->SetWorldRotation(FRotator::ZeroRotator);
	const float MeshScale = LPRadius / 100.f;
	LPEarth->SetRelativeScale3D(FVector(MeshScale));
	LPEarth->SetRelativeRotation(FRotator(0.f, 30.f, IncDeg));   // the equator is tilted against the ground track by the inclination
	LPClouds->SetRelativeScale3D(FVector(MeshScale));
	LPSpin->SetRelativeRotation(FRotator::ZeroRotator);
	LPCloudSpin->SetRelativeRotation(FRotator::ZeroRotator);
	for (USceneComponent* C : { (USceneComponent*)LPEarth, (USceneComponent*)LPClouds }) C->SetVisibility(true);

	// atmosphere sized to this planet: thin shell, blue rim from space (relief is exaggerated, so 2 km instead of 0.6)
	if (SkyAtmo)
	{
		SkyAtmo->SetWorldLocation(Center);
		SkyAtmo->BottomRadius = LPRadius / 1.0e5f;
		SkyAtmo->AtmosphereHeight = 2.f;
		SkyAtmo->RayleighExponentialDistribution = 0.27f;
		SkyAtmo->MieExponentialDistribution = 0.04f;
		SkyAtmo->MarkRenderStateDirty();
		SkyAtmo->SetVisibility(true);
	}
	// one strong sun with a clear terminator across the visible half, weak blue earthshine from below
	SunLight->SetWorldRotation(FRotator(-6.f, 115.f, 0.f));
	SunLight->SetIntensity(6.f);
	SunLight->SetAtmosphereSunLight(true);
	FillLight->SetIntensity(0.35f);
	FillLight->SetLightColor(FLinearColor(0.35f, 0.55f, 1.f));
	FillLight->SetWorldRotation(FRotator(70.f, 0.f, 0.f));
	// the user's 360 panorama as the sky, far beyond the planet; the point star field is not needed
	if (UMaterialInterface* SP = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_SkyPano.M_IS_SkyPano")))
	{
		Sky->SetMaterial(0, SP);
		Sky->SetWorldScale3D(FVector(2.0e7f));   // radius 1e9 uu
		Sky->SetCastShadow(false);
	}
	if (StarField) StarField->SetVisibility(false, true);
	Planet->SetVisibility(false); PlanetAtmo->SetVisibility(false); Moon->SetVisibility(false);
	PlanetRUU = LPRadius;
	OrbitRadiusUU = LPRadius + LPAlt;
	OrbitInc = IncDeg;
	UE_LOG(LogTemp, Log, TEXT("[ORBIT-LP] planet R=%.0f uu (%.2f km), altitude %.0f uu (%.2f km), period %.0f s"), LPRadius, LPRadius / 1.0e5f, LPAlt, LPAlt / 1.0e5f, PeriodS);
}

void AISSpaceScene::TickStylizedEarth(float Dt, const FVector& CamLoc)
{
	// the ground slides under the satellite: rotate the planet about the axis across the ground track (Y)
	const double W = (2.0 * PI / FMath::Max(60.0, PeriodS)) * ISLP::VisualBoost * FMath::Max(0.f, LPWarp);
	LPAngle += FMath::RadiansToDegrees(W) * Dt;
	LPCloudAngle += FMath::RadiansToDegrees(W) * Dt * 1.08;
	LPSpin->SetRelativeRotation(FRotator((float)FMath::Fmod(LPAngle, 360.0), 0.f, 0.f));
	LPCloudSpin->SetRelativeRotation(FRotator((float)FMath::Fmod(LPCloudAngle, 360.0), 0.f, 0.f));
	Sky->SetWorldLocation(CamLoc);
}

void AISSpaceScene::HideStylizedEarth()
{
	bStylized = false;
	if (LPEarth) LPEarth->SetVisibility(false);
	if (LPClouds) LPClouds->SetVisibility(false);
	if (StarField) StarField->SetVisibility(true, true);
	if (SkyAtmo)
	{
		SkyAtmo->BottomRadius = 6371.f; SkyAtmo->AtmosphereHeight = 100.f;
		SkyAtmo->RayleighExponentialDistribution = 8.f; SkyAtmo->MieExponentialDistribution = 1.2f;
		SkyAtmo->SetWorldLocation(FVector::ZeroVector);
		SkyAtmo->MarkRenderStateDirty();
	}
	SunLight->SetIntensity(4.5f);
	FillLight->SetIntensity(1.2f);
	FillLight->SetLightColor(FLinearColor(0.45f, 0.55f, 1.f));
	FillLight->SetWorldRotation(FRotator(20.f, 120.f, 0.f));
}
