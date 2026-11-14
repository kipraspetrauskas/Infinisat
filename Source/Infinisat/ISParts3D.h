// Infinisat - procedural 3D models assembled from simple shapes: CanSat electronics, rocket, BIT hands.
// Built in code so every part is detailed, consistent and needs no binary assets.
#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
class UMaterialInterface;
class UObject;

namespace ISMat
{
	UMaterialInterface* Color();
	UMaterialInterface* Unlit();
	UMaterialInterface* Glass();
	UMaterialInterface* World();
	UStaticMesh* Cube();
	UStaticMesh* Sphere();
	UStaticMesh* Cyl();
	UStaticMesh* Cone();
	UStaticMesh* Mesh(const FString& Path);
	FString PH(const TCHAR* Id);   // Poly Haven mesh path
}

// Collects every component spawned for a model so it can be destroyed or hidden as a group.
struct FISModel
{
	TArray<UStaticMeshComponent*> Parts;
	void Destroy();
	void SetVisible(bool b);
};

namespace ISBuild
{
	// Low-level: add a primitive with a colour (metal/rough), relative to Parent. Units: cm.
	UStaticMeshComponent* Prim(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& SizeCm,
		const FLinearColor& Color, FISModel* Into, const FRotator& Rot = FRotator::ZeroRotator, float Metal = 0.2f, float Rough = 0.5f, float Emissive = 0.f);
	UStaticMeshComponent* Unlit(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& SizeCm,
		const FLinearColor& Color, float Intensity, FISModel* Into, const FRotator& Rot = FRotator::ZeroRotator);
	UStaticMeshComponent* Glass(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& SizeCm,
		const FLinearColor& Color, float Opacity, FISModel* Into, const FRotator& Rot = FRotator::ZeroRotator);
	UStaticMeshComponent* Asset(UObject* Owner, USceneComponent* Parent, const FString& Path, const FVector& Pos, float Scale, FISModel* Into,
		const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = false);

	// Detailed CanSat-scale parts (real size, cm). Returns height of the part (for stacking).
	float CanSatPart(UObject* Owner, USceneComponent* Parent, FName PartId, const FVector& Base, FISModel* Into);
	// The can shell itself; bOpen = translucent cut-away so the inside is visible.
	void CanShell(UObject* Owner, USceneComponent* Parent, const FVector& Base, bool bOpen, FISModel* Into);
	// Hobby rocket ~1.3 m (nose cone, body tube, fins, motor); returns component to animate.
	void HobbyRocket(UObject* Owner, USceneComponent* Parent, FISModel* Into);
	void LaunchPad(UObject* Owner, USceneComponent* Parent, FISModel* Into);
	void Parachute(UObject* Owner, USceneComponent* Parent, float AreaM2, FISModel* Into);
	// BIT's floating hand (palm + five fingertips), ~ palm 14 cm
	void Hand(UObject* Owner, USceneComponent* Parent, float Scale, FISModel* Into);
}
