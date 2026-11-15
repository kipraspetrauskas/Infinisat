// Infinisat - set dressing: lived-in clutter from Poly Haven (CC0) models.
// Rooms are built in ISHouse.cpp; this file fills them so they look used by a family with a space-mad kid.
// Every prop is real-world scale. The Poly Haven meshes come in without collision, so solid pieces get an invisible
// box collider and shelves get one thin collider per board (measured in Blender): the player can't walk through
// them and SettleProps() can drop clutter / hunt items onto the real surfaces.
#include "ISWorld.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/CollisionProfile.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace ISDressK { constexpr float WallT = 14.f, WallH = 270.f; }   // same as the house shell (ISHouse.cpp)

UBoxComponent* AISHomeWorld::Collider(const FBox& B)
{
	UBoxComponent* C = NewObject<UBoxComponent>(this);
	C->SetupAttachment(Root);
	C->SetBoxExtent(B.GetExtent());
	C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	C->SetHiddenInGame(true);
	C->RegisterComponent();
	C->SetWorldLocation(B.GetCenter());
	return C;
}

static bool HasCollision(const UStaticMeshComponent* M)
{
	const UStaticMesh* S = M ? M->GetStaticMesh() : nullptr;
	return S && S->GetBodySetup() && S->GetBodySetup()->AggGeom.GetElementCount() > 0;
}

UStaticMeshComponent* AISHomeWorld::Prop(const TCHAR* Id, const FVector& Pos, float Yaw, float Scale, bool bSolid, bool bSettle)
{
	UStaticMeshComponent* M = Place(Id, Pos, Yaw, Scale, bSolid);
	if (!M) { UE_LOG(LogTemp, Warning, TEXT("[DRESS] missing model %s"), Id); return nullptr; }
	M->SetCullDistance(bSolid ? 0.f : 6000.f);   // small clutter fades out far away (Phase 4b: dithered fade)
	if (bSolid && !HasCollision(M)) Collider(M->Bounds.GetBox());
	FISPlaced P; P.Name = Id; P.C = M; P.bSurface = bSolid; P.bOnTop = bSettle;
	Furniture.Add(P);
	return M;
}

// Back flat against a wall, facing into the room (same convention as the KW helper in BuildHome).
UStaticMeshComponent* AISHomeWorld::PropWall(const TCHAR* Id, bool bAlongX, float WallC, float Side, float Along, float Z, float YawOffset, float Scale, bool bSolid)
{
	const float Face = (bAlongX ? (Side > 0 ? 90.f : -90.f) : (Side > 0 ? 0.f : 180.f)) + YawOffset;
	const FVector Guess = bAlongX ? FVector(Along, WallC + Side * 60.f, Z) : FVector(WallC + Side * 60.f, Along, Z);
	UStaticMeshComponent* M = Prop(Id, Guess, Face, Scale, false, false);   // collider added after the move
	if (!M) return nullptr;
	const FBox B = M->Bounds.GetBox();
	const float Depth = bAlongX ? B.GetSize().Y : B.GetSize().X;
	const float Want = WallC + Side * (ISDressK::WallT * 0.5f + 1.5f + Depth * 0.5f);
	const float Have = bAlongX ? B.GetCenter().Y : B.GetCenter().X;
	M->AddWorldOffset(bAlongX ? FVector(0, Want - Have, 0) : FVector(Want - Have, 0, 0));
	M->SetCullDistance(0.f);
	if (bSolid)
	{
		if (HasCollision(M)) { M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); M->SetCollisionResponseToAllChannels(ECR_Block); }
		else Collider(M->Bounds.GetBox());
	}
	return M;
}

// Hanging on a wall/pegboard: Pos is the centre of the prop, any rotation, no settling, no collision.
UStaticMeshComponent* AISHomeWorld::Hang(const TCHAR* Id, const FVector& Pos, const FRotator& Rot, float Scale)
{
	UStaticMesh* Mesh = ISMat::Mesh(ISMat::PH(Id));
	if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("[DRESS] missing model %s"), Id); return nullptr; }
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Mesh);
	M->SetupAttachment(Root);
	const FVector C = Mesh->GetBoundingBox().GetCenter() * Scale;
	M->SetRelativeTransform(FTransform(Rot, Pos - Rot.RotateVector(C), FVector(Scale)));
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCullDistance(5000.f);
	M->RegisterComponent();
	AllComps.Add(M);
	return M;
}

// Lines each board of a shelf unit with items that fit under the board above. Levels = board tops in cm above the
// unit's bottom (measured from the mesh in Blender); each board also gets a thin invisible collider so SettleProps
// and the player see real surfaces. Fill = share of the board length used. Returns the number of items placed.
int32 AISHomeWorld::FillShelf(UStaticMeshComponent* Shelf, const TArray<float>& Levels, const TArray<const TCHAR*>& Ids, int32 Seed, float Fill)
{
	if (!Shelf) return 0;
	const FBox B = Shelf->Bounds.GetBox();
	const bool bLongX = B.GetSize().X >= B.GetSize().Y;
	const float Len = bLongX ? B.GetSize().X : B.GetSize().Y, Depth = bLongX ? B.GetSize().Y : B.GetSize().X;
	FRandomStream R(Seed);
	const float Facing = bLongX ? 90.f : 0.f;
	int32 N = 0;
	for (int32 i = 0; i < Levels.Num(); ++i)
	{
		const float Z = B.Min.Z + Levels[i];
		Collider(FBox(FVector(B.Min.X + 2.f, B.Min.Y + 2.f, Z - 2.f), FVector(B.Max.X - 2.f, B.Max.Y - 2.f, Z)));
		const float Clear = (i + 1 < Levels.Num() ? Levels[i + 1] - Levels[i] : 40.f) - 5.f;
		if (Clear < 10.f || Ids.Num() == 0) continue;
		float S = -Len * 0.5f * Fill;
		for (int32 Guard = 0; Guard < 40 && S < Len * 0.5f * Fill; ++Guard)
		{
			const TCHAR* Id = Ids[R.RandRange(0, Ids.Num() - 1)];
			UStaticMesh* Mesh = ISMat::Mesh(ISMat::PH(Id));
			if (!Mesh) continue;
			const FVector Sz = Mesh->GetBoundingBox().GetSize();
			const float W = FMath::Max(Sz.X, Sz.Y), D = FMath::Min(Sz.X, Sz.Y);
			if (Sz.Z > Clear || D > Depth + 4.f || S + W > Len * 0.5f * Fill + 6.f) continue;
			if (R.FRand() < 0.08f) { S += W * 0.6f; continue; }   // gaps: real shelves are never packed evenly
			const float At = S + W * 0.5f;
			const float Off = (R.FRand() - 0.5f) * FMath::Max(0.f, Depth - D) * 0.6f;
			const FVector P = B.GetCenter() + (bLongX ? FVector(At, Off, 0) : FVector(Off, At, 0));
			if (Prop(Id, FVector(P.X, P.Y, Z + 0.3f), Facing + (Sz.X > Sz.Y ? 0.f : 90.f) + R.FRandRange(-14.f, 14.f), 1.f, false, false)) ++N;
			S += W + R.FRandRange(2.f, 8.f);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[DRESS] shelf %s: %d boards, %d items"), *Shelf->GetStaticMesh()->GetName(), Levels.Num(), N);
	return N;
}

// Floor clutter: random spots in Area that keep clear of furniture, doorways and each other. Cycles through a shuffled
// copy of Ids so the same model doesn't repeat until all have been used.
int32 AISHomeWorld::Litter(const TArray<const TCHAR*>& Ids, const FBox2D& Area, float Z, int32 Count, int32 Seed, float MinGap)
{
	if (Ids.Num() == 0) return 0;
	FRandomStream R(Seed);
	TArray<const TCHAR*> Order = Ids;
	for (int32 i = Order.Num() - 1; i > 0; --i) Order.Swap(i, R.RandRange(0, i));
	TArray<FBox2D> Taken;
	for (const FISPlaced& P : Furniture) if (P.C) { const FBox b = P.C->Bounds.GetBox(); if (b.Min.Z < Z + 40.f) Taken.Add(FBox2D(FVector2D(b.Min), FVector2D(b.Max))); }
	for (const FBox& O : Openings) Taken.Add(FBox2D(FVector2D(O.Min), FVector2D(O.Max)));
	int32 N = 0, K = 0;
	for (int32 Try = 0; Try < Count * 12 && N < Count; ++Try)
	{
		const TCHAR* Id = Order[K++ % Order.Num()];
		UStaticMesh* Mesh = ISMat::Mesh(ISMat::PH(Id));
		if (!Mesh) continue;
		const FVector Sz = Mesh->GetBoundingBox().GetSize();
		const float Rad = FMath::Max(Sz.X, Sz.Y) * 0.5f + MinGap;
		if (Area.GetSize().X < Rad * 2.f || Area.GetSize().Y < Rad * 2.f) continue;
		const FVector2D P(R.FRandRange(Area.Min.X + Rad, Area.Max.X - Rad), R.FRandRange(Area.Min.Y + Rad, Area.Max.Y - Rad));
		const FBox2D Me(P - FVector2D(Rad), P + FVector2D(Rad));
		if (Taken.ContainsByPredicate([&](const FBox2D& T) { return T.Intersect(Me); })) continue;
		if (Prop(Id, FVector(P.X, P.Y, Z), R.FRandRange(0.f, 360.f), 1.f, false, true)) { Taken.Add(Me); ++N; }
	}
	return N;
}


// ---------------------------------------------------------------------------------------------------------------------
//  Handmade kit pieces (Kenney Building Kit "KB", KayKit "KK"/"KD", nature "KF"/"QN", Poly Pizza "LP", vehicles "VEH").
//  Path: /Game/Infinisat/<Kit>/<Name>/SM_<Name> (dashes in the source names became underscores on import).
// ---------------------------------------------------------------------------------------------------------------------
UStaticMesh* AISHomeWorld::KitMeshAsset(const TCHAR* Kit, const TCHAR* Name)
{
	return ISMat::Mesh(FString::Printf(TEXT("/Game/Infinisat/%s/%s/SM_%s.SM_%s"), Kit, Name, Name, Name));
}

bool AISHomeWorld::KitReady()
{
	static const bool bReady = KitMeshAsset(TEXT("KB"), TEXT("wall")) != nullptr && !FParse::Param(FCommandLine::Get(), TEXT("ISNoKit"));
	return bReady;
}

// Places a kit mesh so that the centre of its footprint and its bottom land on BottomCentre (any pivot, any scale).
UStaticMeshComponent* AISHomeWorld::KitPiece(const TCHAR* Kit, const TCHAR* Name, const FVector& BottomCentre, float Yaw, const FVector& Scale, bool bShadow)
{
	UStaticMesh* Mesh = KitMeshAsset(Kit, Name);
	if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("[KIT] missing %s/%s"), Kit, Name); return nullptr; }
	const FBox B = Mesh->GetBoundingBox();
	const FVector Pivot(B.GetCenter().X, B.GetCenter().Y, B.Min.Z);
	const FRotator R(0.f, Yaw, 0.f);
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Mesh);
	M->SetupAttachment(Root);
	M->SetRelativeTransform(FTransform(R, BottomCentre - R.RotateVector(Pivot * Scale), Scale));
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCastShadow(bShadow);
	M->RegisterComponent();
	AllComps.Add(M);
	return M;
}

// Tiles a kit piece (native footprint TileX x TileY cm) over a rectangle at height Z, stretched to fit whole tiles.
void AISHomeWorld::KitTile(const TCHAR* Kit, const TCHAR* Name, const FBox2D& Area, float Z, float TileX, float TileY, float ScaleZ)
{
	const FVector2D Sz = Area.GetSize();
	const int32 NX = FMath::Max(1, FMath::RoundToInt(Sz.X / TileX)), NY = FMath::Max(1, FMath::RoundToInt(Sz.Y / TileY));
	const float SX = Sz.X / NX, SY = Sz.Y / NY;
	for (int32 i = 0; i < NX; ++i)
		for (int32 j = 0; j < NY; ++j)
			KitPiece(Kit, Name, FVector(Area.Min.X + (i + 0.5f) * SX, Area.Min.Y + (j + 0.5f) * SY, Z), 0.f, FVector(SX / TileX, SY / TileY, ScaleZ));
}

// ---------------------------------------------------------------------------------------------------------------------
//  Set dressing with handmade low-poly pieces. Deco() registers every piece; ValidateDress() (after SettleProps) logs
//  [DRESS-OVERLAP] for two dressed pieces that intersect and [DRESS-FLOAT] for a settled piece with nothing under it.
// ---------------------------------------------------------------------------------------------------------------------
namespace ISDressK2 { constexpr float KK = 0.65f; }   // KayKit Furniture Bits is ~1.5x real size

UStaticMeshComponent* AISHomeWorld::Deco(const TCHAR* Kit, const TCHAR* Name, const FVector& Pos, float Yaw, float Scale, bool bSolid, bool bSettle)
{
	UStaticMeshComponent* M = KitPiece(Kit, Name, Pos, Yaw, FVector(Scale));
	if (!M) return nullptr;
	M->SetCullDistance(bSolid ? 0.f : 7000.f);
	if (bSolid) Collider(M->Bounds.GetBox());
	FISPlaced P; P.Name = FString::Printf(TEXT("%s/%s"), Kit, Name); P.C = M; P.bSurface = bSolid; P.bOnTop = bSettle;
	Furniture.Add(P);
	Dressed.Add(P);
	return M;
}

void AISHomeWorld::ValidateDress()
{
	int32 Over = 0, Float = 0;
	for (int32 i = 0; i < Dressed.Num(); ++i)
	{
		const FISPlaced& A = Dressed[i];
		if (!A.C) continue;
		const FBox BA = A.C->Bounds.GetBox().ExpandBy(-1.5f);
		for (int32 j = i + 1; j < Dressed.Num(); ++j)
		{
			const FISPlaced& B = Dressed[j];
			if (!B.C) continue;
			const FBox BB = B.C->Bounds.GetBox().ExpandBy(-1.5f);
			if (!BA.Intersect(BB)) continue;
			if (FMath::Abs(BA.Min.Z - BB.Max.Z) < 4.f || FMath::Abs(BB.Min.Z - BA.Max.Z) < 4.f) continue;   // resting on top
			if ((A.bSurface && !B.bSurface && BA.IsInsideXY(BB.GetCenter())) || (B.bSurface && !A.bSurface && BB.IsInsideXY(BA.GetCenter()))) continue;   // on a shelf board
			++Over;
			UE_LOG(LogTemp, Warning, TEXT("[DRESS-OVERLAP] %s at (%.0f,%.0f,%.0f) x %s at (%.0f,%.0f,%.0f)"), *A.Name, BA.GetCenter().X, BA.GetCenter().Y, BA.Min.Z, *B.Name, BB.GetCenter().X, BB.GetCenter().Y, BB.Min.Z);
		}
		for (const FISPlaced& K : Furniture)
		{
			if (!K.C || Dressed.ContainsByPredicate([&](const FISPlaced& D) { return D.C == K.C; })) continue;
			const FBox BK = K.C->Bounds.GetBox().ExpandBy(-1.5f);
			if (!BA.Intersect(BK) || FMath::Abs(BA.Min.Z - BK.Max.Z) < 4.f || FMath::Abs(BK.Min.Z - BA.Max.Z) < 4.f) continue;
			if (BK.IsInsideXY(BA.GetCenter()) && BA.Min.Z > BK.Min.Z + 20.f) continue;   // on a shelf / in a cabinet
			++Over;
			UE_LOG(LogTemp, Warning, TEXT("[DRESS-OVERLAP] %s at (%.0f,%.0f,%.0f) x furniture %s"), *A.Name, BA.GetCenter().X, BA.GetCenter().Y, BA.Min.Z, *K.Name);
		}
		if (!A.bOnTop) continue;
		FHitResult H;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ISDressFloat), true);
		Q.AddIgnoredComponent(A.C);
		const FVector Start(BA.GetCenter().X, BA.GetCenter().Y, BA.Min.Z + 3.f);
		if (!GetWorld()->LineTraceSingleByChannel(H, Start, Start - FVector(0, 0, 10.f), ECC_Visibility, Q))
		{
			++Float;
			UE_LOG(LogTemp, Warning, TEXT("[DRESS-FLOAT] %s at (%.0f,%.0f,%.0f)"), *A.Name, Start.X, Start.Y, BA.Min.Z);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[DRESS-CHECK] %d pieces, %d overlaps, %d floating"), Dressed.Num(), Over, Float);
}

// Garage x -760..0, y 0..700 (floor ~2): open garage door on y=0 (x -680..-80), window on the back wall (x -700..-560),
// door to the house on x=0 (y 440..530), Kenney desk bench centred at x -380 on the back wall, hunt-item shelf on the
// left wall at y 330, junk box (-690,130). The walk line door -> bench stays clear.
void AISHomeWorld::DressGarage()
{
	using namespace ISDressK2;
	const float GZ = 2.f;
	// left wall: big steel shelving full of cans, toolboxes and tools (one invisible collider per board)
	if (UStaticMeshComponent* S = Deco(TEXT("LP"), TEXT("ShelfLarge"), FVector(-760.f + 7.f + 18.f, 520.f, GZ), 90.f, 1.f, true, false))
	{
		const FBox B = S->Bounds.GetBox();
		const float Lv[] = { 30.f, 58.f, 86.f, 114.f, 144.f, 172.f };
		const TCHAR* Items[] = { TEXT("CanRed"), TEXT("Toolbox"), TEXT("CanRed"), TEXT("Wrench"), TEXT("Mallet"), TEXT("Toolbox"), TEXT("CanRed"), TEXT("Screwdriver") };
		int32 k = 0;
		for (float L : Lv)
		{
			Collider(FBox(FVector(B.Min.X + 2.f, B.Min.Y + 2.f, B.Min.Z + L - 2.f), FVector(B.Max.X - 2.f, B.Max.Y - 2.f, B.Min.Z + L)));
			for (float Y : { -45.f, 0.f, 45.f })
			{
				++k;
				if ((k % 3) == 1 && L > 100.f) continue;   // gaps, fuller at the bottom
				Deco(TEXT("LP"), Items[k % UE_ARRAY_COUNT(Items)], FVector(B.GetCenter().X, B.GetCenter().Y + Y, B.Min.Z + L + 0.5f), 90.f + float(k * 23 % 30) - 15.f, 1.f, false, false);
			}
		}
	}
	Deco(TEXT("KD"), TEXT("barrel_small"), FVector(-712.f, 650.f, GZ), 20.f, 0.6f, true, false);
	Deco(TEXT("KD"), TEXT("crates_stacked"), FVector(-690.f, 230.f, GZ), 90.f, 0.5f, true, false);

	// back wall: wall shelf above the bench with the kid's model rocket and a globe; tyres + bucket by the bin
	if (UStaticMeshComponent* WS = Deco(TEXT("KK"), TEXT("shelf_A_big"), FVector(BenchTop.X, 700.f - 7.f - 17.f, BenchTop.Z + 70.f), 0.f, KK, true, false))
	{
		const float Top = WS->Bounds.GetBox().Max.Z + 0.5f;
		Deco(TEXT("LP"), TEXT("Rocket"), FVector(BenchTop.X - 45.f, 676.f, Top), 0.f, 1.f, false, false);
		Deco(TEXT("LP"), TEXT("Globe"), FVector(BenchTop.X + 45.f, 676.f, Top), 30.f, 1.f, false, false);
	}
	Deco(TEXT("LP"), TEXT("Tires"), FVector(-170.f, 640.f, GZ), 0.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("Bucket"), FVector(-110.f, 600.f, GZ), 0.f, 1.f, false, true);
	Deco(TEXT("LP"), TEXT("Planet"), FVector(-380.f, 420.f, 215.f), 0.f, 1.f, false, false);   // hanging solar-system mobile

	// right wall (house side; the door at y 440..530 stays clear): ladder, shovel + hoe against the wall, fuel can
	Deco(TEXT("LP"), TEXT("Ladder"), FVector(-45.f, 300.f, GZ), 90.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("Shovel"), FVector(-20.f, 190.f, GZ), 0.f, 1.f, false, true);
	Deco(TEXT("LP"), TEXT("Hoe"), FVector(-30.f, 130.f, GZ), 90.f, 1.f, false, true);
	Deco(TEXT("LP"), TEXT("GasCan"), FVector(-35.f, 50.f, GZ), 0.f, 1.f, false, true);
}

// House (floor top 10): living x 0..800 / y 0..600, kitchen x 800..1500 / y 0..600, hall y 600..760, BIT's room
// x 0..500 / y 760..1050 (desk on y=1050; BIT's dock at (380,830) stays clear), bedroom x 500..1000, bath, mudroom.
void AISHomeWorld::DressHouse()
{
	using namespace ISDressK2;
	const float F = 10.f, Drop = F + 120.f;
	// --- living room: pictures over the TV, pillows on the sofa, cactus in the empty corner, books on the coffee table
	Deco(TEXT("KK"), TEXT("pictureframe_small_A"), FVector(175.f, 586.f, F + 165.f), 180.f, KK, false, false);
	Deco(TEXT("KK"), TEXT("pictureframe_medium"), FVector(330.f, 586.f, F + 175.f), 180.f, KK, false, false);
	Deco(TEXT("KK"), TEXT("pictureframe_small_B"), FVector(490.f, 586.f, F + 165.f), 180.f, KK, false, false);
	Deco(TEXT("KK"), TEXT("pillow_A"), FVector(285.f, 135.f, Drop), 8.f, KK, false, true);
	Deco(TEXT("KK"), TEXT("pillow_B"), FVector(375.f, 135.f, Drop), -10.f, KK, false, true);
	Deco(TEXT("KK"), TEXT("book_set"), FVector(330.f, 300.f, Drop), 15.f, KK * 0.5f, false, true);
	Deco(TEXT("KK"), TEXT("cactus_medium_A"), FVector(755.f, 555.f, F), 0.f, KK, false, true);

	// --- kitchen + dining: bottles and plates on the counter, a meal on the table, picture by the window
	Deco(TEXT("KD"), TEXT("bottle_A_green"), FVector(1110.f, 282.f, Drop), 0.f, 0.3f, false, true);   // the counter run is full of appliances
	Deco(TEXT("KD"), TEXT("plate_food_A"), FVector(1120.f, 250.f, Drop), 0.f, 0.25f, false, true);
	Deco(TEXT("KD"), TEXT("plate_food_A"), FVector(1185.f, 250.f, Drop), 180.f, 0.25f, false, true);
	Deco(TEXT("KD"), TEXT("bottle_A_brown"), FVector(1150.f, 228.f, Drop), 0.f, 0.3f, false, true);
	Deco(TEXT("KK"), TEXT("pictureframe_large_A"), FVector(1486.f, 520.f, F + 160.f), -90.f, KK, false, false);

	// --- BIT's room: telescope at the window, globe on the desk, toy rocket, solar-system mobile, poster, rug
	Deco(TEXT("LP"), TEXT("Telescope"), FVector(85.f, 915.f, F), -90.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("Globe"), FVector(OfficeShelfX, 915.f, OfficeShelfTop + 0.5f), 200.f, 1.f, false, false);   // on top of the bookcase
	Deco(TEXT("LP"), TEXT("RocketToy"), FVector(455.f, 800.f, F), 0.f, 0.5f, false, true);
	Deco(TEXT("LP"), TEXT("Planet"), FVector(250.f, 900.f, F + 200.f), 0.f, 1.f, false, false);
	Deco(TEXT("KK"), TEXT("pictureframe_large_B"), FVector(410.f, 774.f, F + 165.f), 0.f, KK * 0.7f, false, false);
	Deco(TEXT("KK"), TEXT("rug_oval_A"), FVector(220.f, 880.f, F), 90.f, 0.4f, false, false);

	// --- bedroom: pillows, picture, plant; mudroom: a box
	Deco(TEXT("KK"), TEXT("pillow_A"), FVector(690.f, 1000.f, Drop), 0.f, KK, false, true);
	Deco(TEXT("KK"), TEXT("pillow_B"), FVector(750.f, 1000.f, Drop), 0.f, KK, false, true);
	Deco(TEXT("KK"), TEXT("pictureframe_medium"), FVector(986.f, 900.f, F + 160.f), -90.f, KK, false, false);
	Deco(TEXT("KK"), TEXT("cactus_small_B"), FVector(960.f, 795.f, F), 0.f, KK, false, true);
	Deco(TEXT("KD"), TEXT("box_small"), FVector(1310.f, 1005.f, F), 20.f, 0.5f, true, false);
}

// Launch field around Pad: handmade pad + camp (folding table with the controller box, chairs, coolers, grill), cones.
void AISHomeWorld::DressLaunchSite(const FVector& Pad)
{
	Deco(TEXT("LP"), TEXT("LaunchPad"), Pad, 0.f, 1.f, true, false);
	if (UStaticMeshComponent* T = Deco(TEXT("LP"), TEXT("FoldingTable"), Pad + FVector(-250.f, 120.f, 0.f), 0.f, 1.f, true, false))
		Deco(TEXT("LP"), TEXT("Toolbox"), Pad + FVector(-250.f, 120.f, T->Bounds.GetBox().Max.Z - Pad.Z + 0.5f), 90.f, 1.f, false, false);   // launch controller
	Deco(TEXT("LP"), TEXT("BeachChair"), Pad + FVector(-420.f, 40.f, 0.f), 200.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("BeachChair"), Pad + FVector(-430.f, 220.f, 0.f), 160.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("CoolerOpen"), Pad + FVector(-330.f, 280.f, 0.f), 30.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("CoolerBlue"), Pad + FVector(-560.f, 130.f, 0.f), 80.f, 1.f, true, false);
	Deco(TEXT("LP"), TEXT("Grill"), Pad + FVector(-680.f, 340.f, 0.f), 0.f, 1.f, true, false);
	for (int32 i = 0; i < 12; ++i)
	{
		const FVector D = FRotator(0, i * 30.f, 0).Vector() * 900.f;
		KitPiece(TEXT("K/road"), TEXT("construction_cone"), Pad + D, i * 30.f, FVector(2.f));
	}
}
