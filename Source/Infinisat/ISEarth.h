// Infinisat - the real Earth (Cesium for Unreal): one globe for the whole game.
// Walking: Mahia (Rocket Lab Launch Complex 1, New Zealand) is the Unreal origin, Z is local up, so the house,
// the space centre and the pad stand on the real coast. Space views: the Earth's centre is the Unreal origin with
// Z = the Earth's axis (the frame the orbit code already uses), so orbits and launches happen above the same globe.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISEarth.generated.h"

class ACesiumGeoreference;
class ACesium3DTileset;

UCLASS()
class INFINISAT_API AISEarth : public AActor
{
	GENERATED_BODY()
public:
	AISEarth();
	virtual void BeginPlay() override;

	// the site: Rocket Lab LC-1, Mahia Peninsula (WGS84)
	static constexpr double SiteLon = 177.86722;   // the flat 141 m plateau 1.2 km north of the pads (Cesium height survey)
	static constexpr double SiteLat = -39.25069;
	double SiteHeight = 141.0;  // ellipsoid height of the site floor, refined from the terrain once it has loaded

	bool IsReady() const { return Terrain != nullptr; }
	static bool Enabled();     // a Cesium ion token is configured: the game is on the real Earth
	// the levelled site plate (our own ground) in site coordinates, cm
	static bool InSite(const FVector& P) { return P.X > -10000.f && P.X < 25000.f && P.Y > -6000.f && P.Y < 7500.f; }
	void UseSiteOrigin();      // walking: the site at the origin, local up = +Z
	void UseEarthOrigin();     // space: the Earth's centre at the origin
	bool IsEarthOrigin() const { return bEarthOrigin; }
	void SetVisible(bool bOn);

	// Earth-centred (Unreal, cm) direction of the site's local up, for launches from the real pad
	FVector SiteUpEarthFrame() const;
	FVector SiteEastEarthFrame() const;
	FVector SiteNorthEarthFrame() const;
	// Rocket Lab Launch Complex 1 in site coordinates (1.2 km south, 200 m west, 19 m lower than the site floor)
	static FVector LC1Site() { return FVector(-20000.f, 119900.f, -1900.f); }
	// convert a site-frame point (walking origin) to the Earth-centred frame and back
	FVector SiteToEarth(const FVector& P) const;
	FVector EarthToSite(const FVector& P) const;

	virtual void Tick(float Dt) override;
	void Survey();

	UPROPERTY() ACesiumGeoreference* Geo = nullptr;
	UPROPERTY() ACesium3DTileset* Terrain = nullptr;

private:
	bool bEarthOrigin = false;
	UPROPERTY() UMaterialInterface* StyleMat = nullptr;      // stylised terrain for the walking world (flat colours, faceted)
	UPROPERTY() UMaterialInterface* PhotoMat = nullptr;      // Cesium's default (real imagery) for the views from space
	bool bStyled = false;
	void SetStyled(bool bOn);
	bool bHeightFixed = false;
	float FixT = 0.f;
	FString Token() const;
};
