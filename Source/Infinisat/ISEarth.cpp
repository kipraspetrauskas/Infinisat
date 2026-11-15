// Infinisat - the real Earth (Cesium for Unreal). See ISEarth.h.
#include "ISEarth.h"
#include "CesiumGeoreference.h"
#include "Cesium3DTileset.h"
#include "CesiumIonRasterOverlay.h"
#include "OriginPlacement.h"
#include "CesiumSampleHeightResult.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/World.h"
#include "ISParts3D.h"

AISEarth::AISEarth()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FString AISEarth::Token() const
{
	// the free Cesium ion token lives in Config/DefaultGame.ini ([Infinisat] CesiumIonToken=...), not in the code
	FString T;
	GConfig->GetString(TEXT("Infinisat"), TEXT("CesiumIonToken"), T, GGameIni);
	return T;
}

void AISEarth::BeginPlay()
{
	Super::BeginPlay();
	const FString T = Token();
	if (T.IsEmpty()) { UE_LOG(LogTemp, Warning, TEXT("[EARTH] no Cesium ion token: the real Earth is off")); return; }
	Geo = ACesiumGeoreference::GetDefaultGeoreference(this);
	if (!Geo) return;
	Geo->SetOriginPlacement(EOriginPlacement::CartographicOrigin);
	Geo->SetOriginLongitudeLatitudeHeight(FVector(SiteLon, SiteLat, SiteHeight));

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Terrain = GetWorld()->SpawnActor<ACesium3DTileset>(ACesium3DTileset::StaticClass(), FTransform::Identity, SP);
	if (!Terrain) return;
	Terrain->SetGeoreference(Geo);
	Terrain->SetTilesetSource(ETilesetSource::FromCesiumIon);
	Terrain->SetIonAccessToken(T);
	Terrain->SetIonAssetID(1);                                  // Cesium World Terrain
	Terrain->SetCreatePhysicsMeshes(true);                      // walk on the real hills beyond the site
	Terrain->SetMaximumScreenSpaceError(ISQuality::Level <= 0 ? 32.0 : ISQuality::Level == 1 ? 20.0 : 12.0);
	UCesiumIonRasterOverlay* Img = NewObject<UCesiumIonRasterOverlay>(Terrain, TEXT("Imagery"));
	Img->IonAssetID = 2;                                         // Bing Maps aerial imagery
	Img->IonAccessToken = T;
	Terrain->AddInstanceComponent(Img);
	Img->RegisterComponent();
	Img->AddToTileset();
	UE_LOG(LogTemp, Log, TEXT("[EARTH] Cesium terrain + imagery at Mahia (%.4f, %.4f)"), SiteLat, SiteLon);
	if (FParse::Param(FCommandLine::Get(), TEXT("ISSiteSurvey"))) Survey();
}

void AISEarth::Tick(float Dt)
{
	Super::Tick(Dt);
	// level the site with the real ground: once the terrain under the origin has loaded, move the georeference so
	// that the real surface at the pad is exactly the site floor (Z = 0)
	if (!Terrain || bHeightFixed || bEarthOrigin) return;
	FixT += Dt;
	if (FixT < 1.f) return;
	FixT = 0.f;
	FHitResult H;
	FCollisionQueryParams Q(TEXT("ISEarthLevel"), true);
	if (Terrain->ActorLineTraceSingle(H, FVector(0, 0, 500000.f), FVector(0, 0, -500000.f), ECC_Visibility, Q))
	{
		SiteHeight += H.ImpactPoint.Z / 100.0;
		Geo->SetOriginLongitudeLatitudeHeight(FVector(SiteLon, SiteLat, SiteHeight));
		bHeightFixed = FMath::Abs(H.ImpactPoint.Z) < 20.f;
		UE_LOG(LogTemp, Log, TEXT("[EARTH] site ground %.1f m above the ellipsoid"), SiteHeight);
	}
}

void AISEarth::UseSiteOrigin()
{
	if (!Geo) return;
	bEarthOrigin = false;
	Geo->SetOriginPlacement(EOriginPlacement::CartographicOrigin);
	Geo->SetOriginLongitudeLatitudeHeight(FVector(SiteLon, SiteLat, SiteHeight));
}

void AISEarth::UseEarthOrigin()
{
	if (!Geo) return;
	bEarthOrigin = true;
	Geo->SetOriginPlacement(EOriginPlacement::TrueOrigin);
}

void AISEarth::SetVisible(bool bOn)
{
	if (Terrain && Terrain->IsHidden() == bOn) Terrain->SetActorHiddenInGame(!bOn);
}

// Cesium's true-origin frame: Unreal = ECEF metres x 100 with Y mirrored (Unreal is left-handed)
static FVector EcefToEarthFrame(const FVector& E) { return FVector(E.X, -E.Y, E.Z) * 100.0; }
static FVector EarthFrameToEcef(const FVector& U) { return FVector(U.X, -U.Y, U.Z) / 100.0; }

FVector AISEarth::SiteUpEarthFrame() const
{
	const double La = FMath::DegreesToRadians(SiteLat), Lo = FMath::DegreesToRadians(SiteLon);
	return EcefToEarthFrame(FVector(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La))).GetSafeNormal();
}

FVector AISEarth::SiteEastEarthFrame() const
{
	const double Lo = FMath::DegreesToRadians(SiteLon);
	return EcefToEarthFrame(FVector(-FMath::Sin(Lo), FMath::Cos(Lo), 0.0)).GetSafeNormal();
}

FVector AISEarth::SiteToEarth(const FVector& P) const
{
	// site frame (Cesium cartographic origin): +X east, +Y south, +Z up, centimetres
	const double La = FMath::DegreesToRadians(SiteLat), Lo = FMath::DegreesToRadians(SiteLon);
	const FVector E(-FMath::Sin(Lo), FMath::Cos(Lo), 0.0);
	const FVector N(-FMath::Sin(La) * FMath::Cos(Lo), -FMath::Sin(La) * FMath::Sin(Lo), FMath::Cos(La));
	const FVector U(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La));
	const double A = 6378137.0, F = 1.0 / 298.257223563, E2 = F * (2.0 - F);
	const double Nr = A / FMath::Sqrt(1.0 - E2 * FMath::Square(FMath::Sin(La)));
	const FVector O((Nr + SiteHeight) * FMath::Cos(La) * FMath::Cos(Lo), (Nr + SiteHeight) * FMath::Cos(La) * FMath::Sin(Lo), (Nr * (1.0 - E2) + SiteHeight) * FMath::Sin(La));
	const FVector Ecef = O + (E * P.X - N * P.Y + U * P.Z) / 100.0;
	return EcefToEarthFrame(Ecef);
}

FVector AISEarth::EarthToSite(const FVector& P) const
{
	const double La = FMath::DegreesToRadians(SiteLat), Lo = FMath::DegreesToRadians(SiteLon);
	const FVector E(-FMath::Sin(Lo), FMath::Cos(Lo), 0.0);
	const FVector N(-FMath::Sin(La) * FMath::Cos(Lo), -FMath::Sin(La) * FMath::Sin(Lo), FMath::Cos(La));
	const FVector U(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La));
	const FVector O = EarthFrameToEcef(SiteToEarth(FVector::ZeroVector));
	const FVector D = EarthFrameToEcef(P) - O;
	return FVector(FVector::DotProduct(D, E), -FVector::DotProduct(D, N), FVector::DotProduct(D, U)) * 100.0;
}

bool AISEarth::Enabled()
{
	FString T;
	GConfig->GetString(TEXT("Infinisat"), TEXT("CesiumIonToken"), T, GGameIni);
	return !T.IsEmpty();
}

// dev: -ISSiteSurvey writes a 4 x 4 km height grid around the pad (100 m spacing) to Saved/site_survey.txt,
// used to pick a flat piece of real land for the site
void AISEarth::Survey()
{
	if (!Terrain) return;
	TArray<FVector> Pts;
	const double DLon = 100.0 / (111320.0 * FMath::Cos(FMath::DegreesToRadians(SiteLat))), DLat = 100.0 / 110950.0;
	for (int32 j = -20; j <= 20; ++j) for (int32 i = -20; i <= 20; ++i) Pts.Add(FVector(SiteLon + i * DLon, SiteLat + j * DLat, 0.0));
	Terrain->SampleHeightMostDetailed(Pts, FCesiumSampleHeightMostDetailedCallback::CreateLambda([](ACesium3DTileset*, const TArray<FCesiumSampleHeightResult>& R, const TArray<FString>&)
	{
		FString S = TEXT("rows: north (+lat) last; cols: east (+lon) right; metres above ellipsoid, -999 = failed\n");
		for (int32 j = 0; j < 41; ++j)
		{
			for (int32 i = 0; i < 41; ++i) { const FCesiumSampleHeightResult& H = R[j * 41 + i]; S += FString::Printf(TEXT("%5.0f "), H.SampleSuccess ? H.LongitudeLatitudeHeight.Z : -999.0); }
			S += TEXT("\n");
		}
		FFileHelper::SaveStringToFile(S, *(FPaths::ProjectSavedDir() / TEXT("site_survey.txt")));
		UE_LOG(LogTemp, Log, TEXT("[EARTH] site survey written"));
	}));
}
