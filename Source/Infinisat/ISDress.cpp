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

// The garage: x -760..0, y 0..700. Door (open arch) on y=0 between x -680 and -80, back wall y=700 with the workbench
// in the middle and a window at x -700..-560, the house on the x=0 side. The walking line door -> bench stays clear.
void AISHomeWorld::DressGarage()
{
	using namespace ISDressK;
	// --- surfaces: stained concrete floor, bare block wainscot (the painted plaster above), so it reads as a garage
	Block(FVector(-380.f, 350.f, 2.1f), FVector(756.f, 696.f, 0.2f), TEXT("concrete_floor_02"), 220.f, FLinearColor(0.95f, 0.94f, 0.9f), false);
	const FLinearColor BlockTint(0.9f, 0.89f, 0.86f);
	Block(FVector(-760.f + WallT * 0.5f + 2.5f, 350.f, 62.f), FVector(1.f, 686.f, 120.f), TEXT("brushed_concrete"), 140.f, BlockTint, false);
	Block(FVector(-380.f, 700.f - WallT * 0.5f - 2.5f, 62.f), FVector(746.f, 1.f, 120.f), TEXT("brushed_concrete"), 140.f, BlockTint, false);
	Block(FVector(-720.f, WallT * 0.5f + 2.5f, 62.f), FVector(66.f, 1.f, 120.f), TEXT("brushed_concrete"), 140.f, BlockTint, false);

	const float BT = BenchTop.Z;
	const float WallIn = 700.f - WallT * 0.5f;   // back wall inside face
	// --- workbench: pegboard with hanging tools behind it, tools and junk on the top (centre kept free for the CanSat)
	Block(FVector(BenchTop.X, WallIn - 5.f, BT + 78.f), FVector(200.f, 3.f, 110.f), TEXT("brown_planks_05"), 60.f, FLinearColor(0.78f, 0.64f, 0.48f), false);   // pegboard
	struct FPeg { const TCHAR* Id; float X, Z, Roll; };
	const FPeg Pegs[] = {
		{ TEXT("cross_pein_hammer"), -80.f, 110.f, 90.f }, { TEXT("handsaw_wood"), -42.f, 100.f, 90.f }, { TEXT("adjustable_wrench"), -5.f, 118.f, 90.f },
		{ TEXT("combination_wrench"), 10.f, 116.f, 90.f }, { TEXT("pipe_wrench"), 32.f, 104.f, 90.f }, { TEXT("rusted_hacksaw"), 72.f, 64.f, 0.f },
		{ TEXT("tongue_groove_pliers"), -15.f, 74.f, 90.f }, { TEXT("flathead_screwdriver"), 5.f, 76.f, 90.f }, { TEXT("garden_gloves_01"), -75.f, 62.f, 0.f },
		{ TEXT("ratchet_wrench"), 55.f, 112.f, 90.f }, { TEXT("wooden_hammer_01"), 85.f, 108.f, 90.f }, { TEXT("bolt_cutters_01"), -60.f, 74.f, 90.f },
	};
	for (const FPeg& P : Pegs) Hang(P.Id, FVector(BenchTop.X + P.X, WallIn - 9.f, BT + P.Z), FRotator(0.f, 90.f, P.Roll));
	Prop(TEXT("bench_vice_01"), BenchTop + FVector(-88, -12, 1), 0.f);
	Prop(TEXT("retro_multimeter"), BenchTop + FVector(60, -18, 1), -25.f);
	Prop(TEXT("screwdrivers_02"), BenchTop + FVector(-60, 18, 1), 15.f);
	Prop(TEXT("small_oil_can_01"), BenchTop + FVector(85, 22, 1), 0.f);
	Prop(TEXT("lubricant_spray"), BenchTop + FVector(95, 5, 1), 30.f);
	Prop(TEXT("office_notepads"), BenchTop + FVector(-45, -22, 1), -8.f);
	Prop(TEXT("circuit_board"), BenchTop + FVector(42, -25, 1), 20.f);
	Prop(TEXT("lightbulb_led"), BenchTop + FVector(70, -30, 1), 0.f);
	Prop(TEXT("measuring_tape_01"), BenchTop + FVector(-30, -30, 1), 50.f);
	for (float Y : { 200.f, 520.f }) Hang(TEXT("mounted_fluorescent_lights"), FVector(-380.f, Y, WallH - 6.f), FRotator::ZeroRotator);
	Prop(TEXT("metal_stool_01"), FVector(BenchTop.X + 35.f, 560.f, 2.f), 20.f, 1.f, true, false);

	// --- back wall either side of the bench: tool cart (under the window), tool chest with the drill press on top
	Prop(TEXT("tool_cart"), FVector(-630.f, 620.f, 2.f), 8.f, 1.f, true, false);
	Prop(TEXT("vintage_spacecraft_instrument"), FVector(-640.f, 615.f, 92.f), 25.f);
	Prop(TEXT("chemistry_set"), FVector(-600.f, 625.f, 92.f), -10.f);
	if (UStaticMeshComponent* Chest = PropWall(TEXT("metal_tool_chest"), true, 700.f, -1.f, -150.f, 2.f))
		Prop(TEXT("drill_press_01"), FVector(-150.f, Chest->Bounds.GetBox().GetCenter().Y, Chest->Bounds.GetBox().Max.Z + 1.f), 180.f);

	// --- left wall (x=-760): steel shelving packed with junk either side of the old shelf, a barrel in the corner
	const TArray<const TCHAR*> ShelfJunk = { TEXT("plastic_crate_01"), TEXT("plastic_crate_02"), TEXT("cardboard_box_01"), TEXT("cleaner_tin_01"), TEXT("all_purpose_cleaner"),
		TEXT("bleach_bottle"), TEXT("multi_cleaner_5_litre"), TEXT("oil_tin"), TEXT("spray_paint_bottles_02"), TEXT("spray_paint_bottles"), TEXT("can_rusted"), TEXT("plastic_container"),
		TEXT("medical_box"), TEXT("binder_notebook"), TEXT("boombox"), TEXT("portable_cassette_player"), TEXT("metal_toolbox"), TEXT("Drill_01"), TEXT("vintage_hand_drill"),
		TEXT("hand_plane_no4"), TEXT("tire_pump"), TEXT("watering_can_metal_01"), TEXT("lightbulb_01"), TEXT("plastic_jerrycan"), TEXT("vintage_microscope") };
	const TArray<float> Steel02 = { 14.f, 64.f, 114.f, 166.f, 214.f }, Rack = { 44.f, 92.f, 142.f, 190.f };
	int32 Seed = 11;
	for (float Y : { 470.f, 532.f, 594.f })
		if (UStaticMeshComponent* S = PropWall(TEXT("steel_frame_shelves_02"), false, -760.f, 1.f, Y, 2.f, 0.f, 1.f, false)) FillShelf(S, Steel02, ShelfJunk, Seed++);
	if (UStaticMeshComponent* S = PropWall(TEXT("worn_metal_rack"), false, -760.f, 1.f, 140.f, 2.f, 0.f, 1.f, false)) FillShelf(S, Rack, ShelfJunk, 23);
	Prop(TEXT("Barrel_02"), FVector(-715.f, 660.f, 2.f), 30.f, 1.f, true, false);
	Prop(TEXT("compost_bags"), FVector(-700.f, 45.f, 2.f), 75.f, 1.f, true, false);

	// --- right wall (x=0, the house side): ladder, garden tools leaning, hose reel, tyre, kid's sports stuff
	Hang(TEXT("wooden_ladder"), FVector(-28.f, 420.f, 150.f), FRotator(-12.f, 90.f, 0.f));
	Hang(TEXT("rusted_spade_01"), FVector(-14.f, 250.f, 60.f), FRotator(-14.f, 180.f, 0.f));
	Hang(TEXT("plastic_broom"), FVector(-14.f, 280.f, 70.f), FRotator(-10.f, 180.f, 4.f));
	Hang(TEXT("garden_hose_wall_mounted_01"), FVector(-14.f, 140.f, 120.f), FRotator(0.f, 180.f, 0.f));
	Prop(TEXT("old_tyre"), FVector(-70.f, 560.f, 2.f), 0.f, 1.f, true, false);
	Prop(TEXT("baseball_bat"), FVector(-30.f, 330.f, 2.f), 80.f);
	Prop(TEXT("korean_fire_extinguisher_01"), FVector(-25.f, 60.f, 2.f), 180.f, 1.f, true, false);
	Prop(TEXT("metal_jerrycan"), FVector(-50.f, 95.f, 2.f), 95.f, 1.f, true, false);

	// --- floor junk along the walls (the centre stays walkable), plus a bag of trash next to the bin
	const TArray<const TCHAR*> Floor = { TEXT("dirty_football"), TEXT("can_rusted"), TEXT("plastic_crate_03"), TEXT("cardboard_box_01"), TEXT("dustpan"),
		TEXT("watering_can_metal_01"), TEXT("baseball_01"), TEXT("rubber_duck_toy"), TEXT("trowel_01"), TEXT("plastic_jerrycan"), TEXT("wooden_crate_01"), TEXT("football") };
	Litter(Floor, FBox2D(FVector2D(-745.f, 20.f), FVector2D(-560.f, 690.f)), 2.f, 6, 101);   // left strip
	Litter(Floor, FBox2D(FVector2D(-200.f, 20.f), FVector2D(-15.f, 690.f)), 2.f, 5, 202);    // right strip
	Prop(TEXT("trashbag"), FVector(-100.f, 655.f, 2.f), 40.f, 1.f, false, false);
}

// The house (floor top z=10): living room x 0..750 / y 0..600 (TV on the y=600 wall, sofa at y=140), kitchen + dining
// x 750..1500 / y 0..600 (counter run on the y=600 wall), BIT's room / office x 0..500 / y 760..1050 (desk on the
// y=1050 wall), bedroom x 500..1000, mudroom x 1250..1500. Small things are dropped from above onto the furniture.
void AISHomeWorld::DressHouse()
{
	const float FZ = 10.f, Drop = FZ + 130.f;   // spawn above the furniture, SettleProps drops them onto it
	// --- living room: a game left mid-match, a chess board, a guitar against the sofa, photos on the shelf
	Prop(TEXT("gamepad"), FVector(315.f, 300.f, Drop), 25.f);
	Prop(TEXT("chess_set"), FVector(355.f, 325.f, Drop), 10.f);
	Prop(TEXT("gaming_console"), FVector(400.f, 570.f, Drop), 180.f);
	Prop(TEXT("Ukulele_01"), FVector(430.f, 105.f, FZ + 2.f), 70.f);
	Prop(TEXT("standing_picture_frame_01"), FVector(30.f, 230.f, Drop + 90.f), 90.f);
	Prop(TEXT("alarm_clock_01"), FVector(490.f, 150.f, Drop), 200.f);
	Prop(TEXT("dirty_football"), FVector(560.f, 420.f, FZ + 2.f), 0.f);
	Prop(TEXT("wicker_basket_01"), FVector(80.f, 520.f, FZ + 2.f), 15.f);
	for (int32 i = 0; i < 3; ++i)   // a row of photos on the wall the sofa backs onto (y=0 side wall, facing +Y)
		Hang(i == 1 ? TEXT("hanging_picture_frame_02") : (i == 0 ? TEXT("hanging_picture_frame_01") : TEXT("hanging_picture_frame_03")), FVector(240.f + i * 80.f, 12.f, FZ + 175.f + (i == 1 ? 12.f : 0.f)), FRotator(0.f, 90.f, 0.f));

	// --- kitchen: kettle, chopping board, fruit; dining table with a fruit bowl, a vase and the kid's homework
	Prop(TEXT("vintage_electric_kettle"), FVector(1060.f, 565.f, Drop), 200.f);
	Prop(TEXT("wooden_cutting_board"), FVector(1210.f, 568.f, Drop), 0.f);
	Prop(TEXT("bananas"), FVector(1290.f, 560.f, Drop), 30.f);
	Prop(TEXT("wooden_bowl_01"), FVector(1150.f, 250.f, Drop), 0.f);
	Prop(TEXT("food_apple_01"), FVector(1150.f, 252.f, Drop + 10.f), 0.f);
	Prop(TEXT("ceramic_vase_01"), FVector(1110.f, 230.f, Drop), 0.f);
	Prop(TEXT("office_notepads"), FVector(1185.f, 270.f, Drop), -20.f);
	Prop(TEXT("tea_set_01"), FVector(1115.f, 275.f, Drop), 40.f);

	// --- BIT's room / office: space kid's desk, chalkboard with orbits, camera and binoculars, a ball on the floor
	const float DX = DeskBox.GetCenter().X, DY = DeskBox.GetCenter().Y;
	Prop(TEXT("vintage_binocular"), FVector(DX - 60.f, DY - 10.f, Drop), 120.f);
	Prop(TEXT("Camera_01"), FVector(DeskBox.Max.X - 20.f, DY, Drop), 200.f);
	Prop(TEXT("binder_notebook"), FVector(DX + 10.f, DY - 25.f, Drop), 10.f);
	Prop(TEXT("standing_chalkboard_01"), FVector(460.f, 830.f, FZ + 2.f), 225.f, 1.f, true, false);
	Prop(TEXT("football"), FVector(120.f, 820.f, FZ + 2.f), 0.f);
	Prop(TEXT("vintage_video_camera"), FVector(60.f, 1000.f, Drop), 140.f);

	// --- bedroom + mudroom: suitcase still half unpacked, boots and a basket by the back door
	Prop(TEXT("vintage_suitcase"), FVector(880.f, 800.f, FZ + 2.f), 75.f);
	Prop(TEXT("rubber_boots"), FVector(1300.f, 790.f, FZ + 2.f), 30.f);
	Prop(TEXT("wicker_basket_01"), FVector(1440.f, 790.f, FZ + 2.f), 0.f);
}
