// Infinisat - the integration lab (Ch.1+ design room) built from the Kenney Space Station Kit (handmade low-poly).
// Hall 30 x 24 m, 6 m high, floor at z -260; the satellite sits on the integration stand at the origin.
// Kit grid is 1 m; walls x3 (3 m panels), furniture x2.2. The old box-built hall stays as the fallback.
#include "ISScene.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace ISLabK
{
	const FString Path(const TCHAR* Name)
	{
		const FString N = FString(Name).Replace(TEXT("-"), TEXT("_"));
		return FString::Printf(TEXT("/Game/Infinisat/KS/%s/SM_%s.SM_%s"), *N, *N, *N);
	}
}

bool AISSpaceScene::LabKitAvailable()
{
	return LoadObject<UStaticMesh>(nullptr, *ISLabK::Path(TEXT("wall"))) != nullptr;
}

// Places a kit piece so the centre of its footprint and its bottom land on P (any pivot), keeping its own materials.
UStaticMeshComponent* AISSpaceScene::LabPiece(const TCHAR* Name, const FVector& P, float Yaw, float Scale)
{
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *ISLabK::Path(Name));
	if (!M) { UE_LOG(LogTemp, Warning, TEXT("[LAB] missing %s"), Name); return nullptr; }
	const FBox B = M->GetBoundingBox();
	const FRotator R(0.f, Yaw, 0.f);
	const FVector Pivot = FVector(B.GetCenter().X, B.GetCenter().Y, B.Min.Z) * Scale;
	return AddMesh(SceneRoot, M, FTransform(R, P - R.RotateVector(Pivot), FVector(Scale)), FLinearColor(0, 0, 0, 0), false, 0.f, nullptr);
}

void AISSpaceScene::BuildLabKit()
{
	const float HX = 1500.f, HY = 1200.f, FloorZ = -260.f, W = 3.f, F = 2.2f, Panel = 100.f * W;
	// floor: kit floor panels (300 cm), a detailed tile here and there
	for (int32 ix = 0; ix < 10; ++ix)
		for (int32 iy = 0; iy < 8; ++iy)
			LabPiece(((ix * 3 + iy * 5) % 7 == 0) ? TEXT("floor-detail") : TEXT("floor-panel"), FVector(-HX + (ix + 0.5f) * Panel, -HY + (iy + 0.5f) * Panel, FloorZ - 30.f * W), 0.f, W);
	// walls: two rows of 3 m panels; pillars every third panel, banners on the upper row, a wide door in each short wall
	auto WallRun = [&](bool bAlongX, float Fixed, int32 N, float Yaw)
	{
		for (int32 i = 0; i < N; ++i)
		{
			const float U = (bAlongX ? -HX : -HY) + (i + 0.5f) * Panel;
			const FVector P = bAlongX ? FVector(U, Fixed, 0.f) : FVector(Fixed, U, 0.f);
			const bool bMid = i == N / 2 || i == N / 2 - 1;
			const TCHAR* Low = (!bAlongX && bMid && i == N / 2) ? TEXT("wall-door-wide") : (i % 3 == 0 ? TEXT("wall-pillar") : TEXT("wall"));
			LabPiece(Low, P + FVector(0, 0, FloorZ), Yaw, W);
			if (FCString::Strcmp(Low, TEXT("wall-door-wide")) == 0) LabPiece(TEXT("door-double-closed"), P + FVector(0, 0, FloorZ), Yaw, W);   // closed: no black void behind it
			LabPiece(i % 4 == 1 ? TEXT("wall-banner") : TEXT("wall"), P + FVector(0, 0, FloorZ + Panel), Yaw, W);
		}
	};
	WallRun(true, -HY, 10, 0.f);
	WallRun(true, HY, 10, 180.f);
	WallRun(false, -HX, 8, 90.f);
	WallRun(false, HX, 8, -90.f);
	// ceiling with light panels (the old hall's emissive panels, now at 6 m)
	AddMesh(SceneRoot, MCube, FTransform(FRotator::ZeroRotator, FVector(0, 0, FloorZ + 2.f * Panel + 10.f), FVector(2 * HX / 100.f, 2 * HY / 100.f, 0.2f)), FLinearColor(0.82f, 0.85f, 0.9f), false, 0.f, nullptr);
	for (int32 ix = -2; ix <= 2; ++ix) for (int32 iy = -1; iy <= 1; ++iy)
		AddMesh(SceneRoot, MCube, FTransform(FRotator::ZeroRotator, FVector(ix * 560.f, iy * 640.f, FloorZ + 2.f * Panel - 1.f), FVector(2.4f, 1.2f, 0.02f)), FLinearColor(1.f, 0.98f, 0.95f), true, 3.f, nullptr);

	// back wall (x = -HX, the camera looks at it): mission consoles with big wall screens and chairs
	for (int32 k = -1; k <= 1; ++k)
	{
		const float Y = k * 620.f;
		LabPiece(TEXT("computer-system"), FVector(-HX + 45.f + 80.f, Y, FloorZ), -90.f, F);
		LabPiece(TEXT("display-wall-wide"), FVector(-HX + 45.f + 45.f, Y, FloorZ + 330.f), -90.f, W);
		LabPiece(TEXT("chair-armrest-headrest"), FVector(-HX + 420.f, Y + 40.f, FloorZ), 90.f, F);
	}
	// side wall (y = -HY): engineering desks with monitors
	for (int32 k = 0; k < 3; ++k)
	{
		const float X = -400.f + k * 560.f;
		LabPiece(TEXT("table-large"), FVector(X, -HY + 45.f + 110.f, FloorZ), 0.f, F);
		LabPiece(TEXT("computer-screen"), FVector(X - 60.f, -HY + 45.f + 90.f, FloorZ + 40.f * F), 0.f, F * 0.8f);
		LabPiece(TEXT("computer"), FVector(X + 110.f, -HY + 45.f + 90.f, FloorZ + 40.f * F), 0.f, F * 0.8f);
		LabPiece(TEXT("chair-armrest-headrest"), FVector(X, -HY + 420.f, FloorZ), 180.f, F);
		LabPiece(TEXT("display-wall"), FVector(X, -HY + 45.f + 45.f, FloorZ + 380.f), 0.f, W);
	}
	// other side (y = +HY): parts store, crates and tall containers
	const TCHAR* Store[] = { TEXT("container-tall"), TEXT("container-wide"), TEXT("container"), TEXT("container-tall"), TEXT("container-wide"), TEXT("container-tall") };
	for (int32 k = 0; k < 6; ++k)
		LabPiece(Store[k], FVector(-1100.f + k * 430.f, HY - 45.f - 100.f, FloorZ), (k % 2) ? 10.f : -5.f, W * 0.95f);
	LabPiece(TEXT("container"), FVector(-1100.f + 2 * 430.f, HY - 45.f - 100.f, FloorZ + 60.f * W * 0.95f), 20.f, W * 0.8f);
	// front wall (x = +HX, behind the camera): a big screen over the door
	LabPiece(TEXT("display-wall-wide"), FVector(HX - 90.f, 0.f, FloorZ + 360.f), 90.f, W);

	// around the integration stand: safety rail square (gap toward the camera), planet hologram tables, a work table
	const float RS = 450.f;
	for (int32 i = -1; i <= 1; ++i)
	{
		LabPiece(TEXT("rail"), FVector(-RS, i * 300.f, FloorZ), 90.f, W);
		LabPiece(TEXT("rail"), FVector(i * 300.f, -RS, FloorZ), 0.f, W);
		if (i != 1) LabPiece(TEXT("rail"), FVector(i * 300.f, RS, FloorZ), 0.f, W);
	}
	LabPiece(TEXT("table-display-planet"), FVector(-700.f, -700.f, FloorZ), 30.f, W);
	LabPiece(TEXT("table-display-planet"), FVector(750.f, 650.f, FloorZ), -20.f, W * 0.8f);
	LabPiece(TEXT("table-display"), FVector(800.f, -700.f, FloorZ), 0.f, F);
	// pipes along the top of the long walls
	for (int32 i = 0; i < 10; ++i)
	{
		LabPiece(TEXT("pipe"), FVector(-HX + (i + 0.5f) * Panel, -HY + 60.f, FloorZ + 2.f * Panel - 160.f), 0.f, W);
		LabPiece(TEXT("pipe"), FVector(-HX + (i + 0.5f) * Panel, HY - 60.f, FloorZ + 2.f * Panel - 160.f), 180.f, W);
	}

	// integration stand (as before): base, column, adapter table
	const FLinearColor Steel(0.62f, 0.64f, 0.67f);
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, FloorZ + 15.f), FVector(2.2f, 2.2f, 0.3f)), Steel, false, 0.f, nullptr);
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, FloorZ + 100.f), FVector(0.5f, 0.5f, 1.7f)), Steel, false, 0.f, nullptr);
	AddMesh(SceneRoot, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, -120.f), FVector(1.4f, 1.4f, 0.12f)), FLinearColor(0.25f, 0.27f, 0.3f), false, 0.f, nullptr);
	// the hall must not shadow the key light: only the spacecraft casts shadows (onto the floor)
	for (UStaticMeshComponent* C : Extras) if (C) C->SetCastShadow(false);
}
