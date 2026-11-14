// Infinisat - the player's home and neighbourhood (stylised, Astroneer-like palette).
// Architecture is built from coloured blocks + a procedural roof; furniture, houses, roads,
// trees and flowers are Kenney CC0 kits; electronics are KiCad parts converted in Fusion.
#include "ISWorld.h"
#include "ISBit.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	const float FZ = 10.f;      // house floor top
	const float WallH = 270.f;
	const float WallT = 14.f;
	const float KF = 2.1f;      // Kenney furniture kit scale (import already converts m -> cm)
	const float KCity = 10.f;   // Kenney city kits
	const float KNat = 4.5f;    // Kenney nature kit (trees)
	const float KYaw = -90.f;   // Kenney models face +Y in Unreal; -90 makes yaw 0 face +X

	// pastel palette
	const FLinearColor Cream(0.93f, 0.85f, 0.70f), Trim(0.96f, 0.95f, 0.92f), Coral(0.80f, 0.38f, 0.30f), Teal(0.10f, 0.55f, 0.58f);
	const FLinearColor WarmWhite(0.95f, 0.92f, 0.85f), Mint(0.72f, 0.88f, 0.79f), SoftBlue(0.70f, 0.80f, 0.93f), Lilac(0.82f, 0.76f, 0.91f);
	const FLinearColor Peach(0.97f, 0.84f, 0.73f), Aqua(0.78f, 0.92f, 0.95f), GarageGrey(0.72f, 0.73f, 0.76f), Wood(0.66f, 0.46f, 0.30f), TileCol(0.93f, 0.91f, 0.86f);

	FString KPath(const TCHAR* Kit, const TCHAR* Name)
	{
		FString S = FString(Name).Replace(TEXT("-"), TEXT("_"));
		return FString::Printf(TEXT("/Game/Infinisat/K/%s/%s/SM_%s.SM_%s"), Kit, *S, *S, *S);
	}
	FString CadPath(const TCHAR* Name) { return FString::Printf(TEXT("/Game/Infinisat/CAD/%s/SM_%s.SM_%s"), Name, Name, Name); }
}

// =====================================================================================================
//  building blocks
// =====================================================================================================
UStaticMeshComponent* AISHomeWorld::Paint(const FVector& C, const FVector& S, const FLinearColor& Col, float Rough, bool bCollide, float Yaw, float Metal)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(ISMat::Cube());
	M->SetupAttachment(Root);
	M->SetRelativeTransform(FTransform(FRotator(0, Yaw, 0), C, S / 100.f));
	M->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	M->SetCollisionResponseToAllChannels(ECR_Block);
	M->RegisterComponent();
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ISMat::Color(), M);
	MID->SetVectorParameterValue(TEXT("Color"), Col);
	MID->SetScalarParameterValue(TEXT("Roughness"), Rough);
	MID->SetScalarParameterValue(TEXT("Metallic"), Metal);
	M->SetMaterial(0, MID);
	AllComps.Add(M);
	return M;
}

UStaticMeshComponent* AISHomeWorld::Slab(const FVector& C, const FVector& S, const TCHAR* Tex, float Tile, const FLinearColor& Tint, bool bCollide, float Yaw)
{
	UStaticMeshComponent* M = Block(C, S, Tex, Tile, Tint, bCollide);
	if (M) M->SetRelativeRotation(FRotator(0, Yaw, 0));
	return M;
}

// One art style: every low-poly Kenney model is swapped for a realistic Poly Haven (CC0) model at real-world size.
// Name -> (Poly Haven id, scale). An empty id means "leave it out" (no realistic equivalent; the room works without it).
struct FISRealSwap { const TCHAR* Ph; float Scale; float Yaw = 0.f; };   // Yaw: extra turn where the model front differs
static const FISRealSwap* RealSwap(const FString& KenneyName)
{
	static const TMap<FString, FISRealSwap> M = {
		{ TEXT("loungeSofa"), { TEXT("sofa_02"), 1.f } }, { TEXT("tableCoffee"), { TEXT("modern_coffee_table_01"), 1.f } },
		{ TEXT("loungeChair"), { TEXT("modern_arm_chair_01"), 1.f } }, { TEXT("loungeChairRelax"), { TEXT("mid_century_lounge_chair"), 1.f } },
		{ TEXT("sideTable"), { TEXT("side_table_01"), 1.f } }, { TEXT("books"), { TEXT("book_encyclopedia_set_01"), 1.f } },
		{ TEXT("pottedPlant"), { TEXT("potted_plant_01"), 1.f } }, { TEXT("plantSmall2"), { TEXT("potted_plant_04"), 1.f } },
		{ TEXT("table"), { TEXT("dining_table"), 1.f } }, { TEXT("chair"), { TEXT("dining_chair_02"), 1.f } },
		{ TEXT("chairModernCushion"), { TEXT("plastic_monobloc_chair_01"), 1.f } }, { TEXT("tableRound"), { TEXT("round_wooden_table_01"), 1.f } },
		{ TEXT("trashcan"), { TEXT("metal_trash_can"), 0.6f } }, { TEXT("chairDesk"), { TEXT("painted_wooden_chair_01"), 1.f } },
		{ TEXT("laptop"), { TEXT("classic_laptop"), 1.f, -90.f } }, { TEXT("radio"), { TEXT("vintage_radio_transceiver"), 1.f } },
		{ TEXT("cardboardBoxOpen"), { TEXT("cardboard_box_01"), 1.f } }, { TEXT("cardboardBoxClosed"), { TEXT("cardboard_box_01"), 1.f } },
		{ TEXT("lampRoundTable"), { TEXT("desk_lamp_arm_01"), 1.f } }, { TEXT("lampSquareTable"), { TEXT("desk_lamp_arm_01"), 1.f } },
		{ TEXT("lampSquareCeiling"), { TEXT("modern_ceiling_lamp_01"), 1.f } },
		{ TEXT("cabinetTelevision"), { TEXT("painted_wooden_cabinet"), 1.f } }, { TEXT("televisionModern"), { TEXT("Television_01"), 1.f } },
		{ TEXT("speaker"), { TEXT(""), 0.f } }, { TEXT("toaster"), { TEXT(""), 0.f } },
		{ TEXT("bookcaseOpen"), { TEXT("wooden_display_shelves_01"), 1.f } }, { TEXT("bookcaseOpenLow"), { TEXT("drawer_cabinet"), 1.f } },
		{ TEXT("bookcaseClosedWide"), { TEXT("wooden_bookshelf_worn"), 1.f } }, { TEXT("bookcaseClosedDoors"), { TEXT("wooden_bookshelf_worn"), 1.f } },
		{ TEXT("cabinetBedDrawerTable"), { TEXT("painted_wooden_nightstand"), 1.f } }, { TEXT("bedDouble"), { TEXT("GothicBed_01"), 1.f } },
		{ TEXT("desk"), { TEXT("small_wooden_table_01"), 1.25f, 90.f } }, { TEXT("kitchenMicrowave"), { TEXT("vintage_microwave"), 1.f } },
		{ TEXT("computerScreen"), { TEXT(""), 0.f } }, { TEXT("computerKeyboard"), { TEXT(""), 0.f } },
		{ TEXT("lampRoundFloor"), { TEXT(""), 0.f } }, { TEXT("lampSquareFloor"), { TEXT(""), 0.f } },
		{ TEXT("rugRound"), { TEXT(""), 0.f } }, { TEXT("rugRectangle"), { TEXT(""), 0.f } },
		// garden and nature
		{ TEXT("tree_default"), { TEXT("island_tree_01"), 1.1f } }, { TEXT("tree_oak"), { TEXT("tree_small_02"), 1.6f } },
		{ TEXT("tree_fat"), { TEXT("island_tree_01"), 1.3f } }, { TEXT("tree_small"), { TEXT("tree_small_02"), 1.f } },
		{ TEXT("tree_pineRoundB"), { TEXT("pine_tree_01"), 1.f } },
		{ TEXT("plant_bushDetailed"), { TEXT("shrub_02"), 1.f } }, { TEXT("plant_bushLarge"), { TEXT("shrub_01"), 1.2f } },
		{ TEXT("flower_redA"), { TEXT("shrub_04"), 0.35f } }, { TEXT("flower_yellowB"), { TEXT("dandelion_01"), 1.f } },
		{ TEXT("flower_purpleA"), { TEXT("shrub_04"), 0.35f } }, { TEXT("flower_redC"), { TEXT("shrub_04"), 0.3f } },
		{ TEXT("flower_yellowA"), { TEXT("dandelion_01"), 1.f } }, { TEXT("flower_purpleC"), { TEXT("shrub_04"), 0.3f } },
		{ TEXT("sedan"), { TEXT("covered_car"), 1.f } },
		{ TEXT("plant_bush"), { TEXT("shrub_03"), 1.f } }, { TEXT("rock_largeA"), { TEXT("boulder_01"), 1.f } },
		{ TEXT("grass_large"), { TEXT("grass_medium_01"), 1.f } }, { TEXT("grass"), { TEXT("grass_medium_02"), 1.f } },
		{ TEXT("flower_purpleB"), { TEXT("shrub_04"), 0.3f } },
	};
	return M.Find(KenneyName);
}

static UStaticMeshComponent* PlaceMesh(AISHomeWorld* W, USceneComponent* Root, TArray<UPrimitiveComponent*>& All, const FString& InPath, const FVector& Center, float FaceYaw, float InScale, bool bCollide, bool bInKenney)
{
	FString Path = InPath;
	float Scale = InScale;
	bool bKenney = bInKenney;
	if (bKenney)
	{
		// "/Game/Infinisat/K/<kit>/<name>/SM_<name>..." -> realistic model if there is one
		FString Name = FPaths::GetBaseFilename(FPaths::GetPath(Path)).Replace(TEXT("_"), TEXT("-"));
		const FISRealSwap* Sw = RealSwap(Name);
		if (!Sw) Sw = RealSwap(Name.Replace(TEXT("-"), TEXT("_")));
		if (Sw)
		{
			if (!*Sw->Ph) return nullptr;
			Path = ISMat::PH(Sw->Ph);
			Scale = Sw->Scale;
			FaceYaw += Sw->Yaw;
			bKenney = false;
		}
	}
	UStaticMesh* Mesh = ISMat::Mesh(Path);
	if (!Mesh) return nullptr;
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(W);
	M->SetStaticMesh(Mesh);
	M->SetupAttachment(Root);
	const FBox B = Mesh->GetBoundingBox();
	const FRotator R(0, FaceYaw + (bKenney ? KYaw : 0.f), 0);
	const FVector C = B.GetCenter() * Scale;
	const FVector Off = R.RotateVector(FVector(C.X, C.Y, 0));
	M->SetRelativeTransform(FTransform(R, FVector(Center.X - Off.X, Center.Y - Off.Y, Center.Z - B.Min.Z * Scale), FVector(Scale)));
	M->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	M->SetCollisionResponseToAllChannels(ECR_Block);
	M->RegisterComponent();
	All.Add(M);
	return M;
}

void AISHomeWorld::RoomLight(const FVector& Pos, float Lumens, bool bFixture)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetupAttachment(Root);
	L->SetRelativeLocation(Pos);
	L->SetIntensityUnits(ELightUnits::Lumens);
	L->SetIntensity(Lumens);
	L->SetAttenuationRadius(900.f);
	L->SetLightColor(FLinearColor(1.f, 0.88f, 0.72f));   // warm 2700 K
	L->SetSourceRadius(15.f);
	L->SetCastShadows(ISQuality::Level >= 3 && RoomLights.Num() < 2);   // 6 shadow passes per shadowed point light: only 2, only on Epic
	L->RegisterComponent();
	RoomLights.Add(L);
	if (bFixture) PlaceMesh(this, Root, AllComps, KPath(TEXT("furn"), TEXT("lampSquareCeiling")), FVector(Pos.X, Pos.Y, FZ + WallH - 50.f), 0.f, KF, false, true);
}

// Axis-aligned wall from A to B with openings (positions along the wall from A, in cm).
void AISHomeWorld::Wall(const FVector2D& A, const FVector2D& B, const TArray<FISOpening>& Ops, const TCHAR* /*InTex*/, const FLinearColor& In,
	const TCHAR* /*OutTex*/, const FLinearColor& Out, float OutSide, bool bBaseboard)
{
	const bool bAlongX = FMath::Abs(B.X - A.X) > FMath::Abs(B.Y - A.Y);
	const float Len = bAlongX ? FMath::Abs(B.X - A.X) : FMath::Abs(B.Y - A.Y);
	const float U0 = bAlongX ? FMath::Min(A.X, B.X) : FMath::Min(A.Y, B.Y);
	const float Fixed = bAlongX ? A.Y : A.X;
	// world position for (u along wall, n across wall, z)
	auto P = [&](float U, float N, float Z) { return bAlongX ? FVector(U0 + U, Fixed + N, Z) : FVector(Fixed + N, U0 + U, Z); };
	auto S = [&](float SU, float SN, float SZ) { return bAlongX ? FVector(SU, SN, SZ) : FVector(SN, SU, SZ); };
	const FLinearColor CoreCol = OutSide == 0.f ? In : Out;

	auto Piece = [&](float U1, float U2, float Z1, float Z2)
	{
		if (U2 - U1 < 0.5f || Z2 - Z1 < 0.5f) return;
		// real surfaces (one art style): stucco outside, painted plaster inside, tinted with the room colour
		Block(P((U1 + U2) * 0.5f, 0, FZ + (Z1 + Z2) * 0.5f), S(U2 - U1, WallT, Z2 - Z1), OutSide != 0.f ? TEXT("white_stucco") : TEXT("painted_plaster_wall"), 250.f, CoreCol);
		if (OutSide != 0.f)   // interior plaster layer on the inside face
			Block(P((U1 + U2) * 0.5f, -OutSide * (WallT * 0.5f + 0.6f), FZ + (Z1 + Z2) * 0.5f), S(U2 - U1, 1.f, Z2 - Z1), TEXT("painted_plaster_wall"), 250.f, In, false);
	};
	auto Baseboard = [&](float U1, float U2)
	{
		if (!bBaseboard || U2 - U1 < 2.f) return;
		for (float Side : { -1.f, 1.f })
		{
			if (OutSide != 0.f && Side == OutSide) continue;   // no baseboard outside
			Paint(P((U1 + U2) * 0.5f, Side * (WallT * 0.5f + 1.5f), FZ + 6.f), S(U2 - U1, 2.f, 12.f), Trim, 0.5f, false);
		}
	};

	TArray<FISOpening> Sorted = Ops;
	Sorted.Sort([](const FISOpening& X, const FISOpening& Y) { return X.From < Y.From; });
	float Cur = 0.f;
	for (const FISOpening& O : Sorted)
	{
		Piece(Cur, O.From, 0.f, WallH);
		Baseboard(Cur, O.From);
		Piece(O.From, O.To, 0.f, O.Bottom);
		if (O.Bottom > 0.f) Baseboard(O.From, O.To);
		Piece(O.From, O.To, O.Top, WallH);
		const float W = O.To - O.From, Mid = (O.From + O.To) * 0.5f;
		{
			const float Depth = (O.Kind == 1) ? 35.f : 85.f;            // windows: shallow, doors/arches: walk-through space
			const float Z0 = (O.Kind == 1) ? O.Bottom - 5.f : 0.f;
			const FVector A3 = P(O.From, -Depth - WallT * 0.5f, FZ + Z0), B3 = P(O.To, Depth + WallT * 0.5f, FZ + O.Top);
			Openings.Add(FBox(FVector(FMath::Min(A3.X, B3.X), FMath::Min(A3.Y, B3.Y), A3.Z), FVector(FMath::Max(A3.X, B3.X), FMath::Max(A3.Y, B3.Y), B3.Z)));
		}
		// frame trims (both faces)
		const float FT = WallT + 3.f;
		Paint(P(O.From - 3.f, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(6.f, FT, O.Top - O.Bottom + 6.f), Trim, 0.5f, false);
		Paint(P(O.To + 3.f, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(6.f, FT, O.Top - O.Bottom + 6.f), Trim, 0.5f, false);
		Paint(P(Mid, 0, FZ + O.Top + 3.f), S(W + 12.f, FT, 6.f), Trim, 0.5f, false);
		if (O.Kind == 1)
		{
			// window: glass, cross mullions, sill
			UStaticMeshComponent* G = ISBuild::Glass(this, Root, ISMat::Cube(), P(Mid, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(W, 1.5f, O.Top - O.Bottom), FLinearColor(0.75f, 0.9f, 1.f), 0.18f, nullptr);
			if (G) { G->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); G->SetCollisionResponseToAllChannels(ECR_Block); AllComps.Add(G); }
			Paint(P(Mid, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(4.f, 4.f, O.Top - O.Bottom), Trim, 0.5f, false);
			Paint(P(Mid, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(W, 4.f, 4.f), Trim, 0.5f, false);
			Paint(P(Mid, 0, FZ + O.Bottom - 2.f), S(W + 16.f, WallT + 12.f, 4.f), Trim, 0.5f, false);
		}
		else if (O.Kind == 0 || O.Kind == 3)
		{
			// door leaf hinged at the "From" side, swinging into the side opposite the exterior
			const FVector Hinge = P(O.From, 0, FZ);
			const float Yaw = bAlongX ? 0.f : 90.f;
			const bool bExterior = O.Kind == 3;
			AddDoor(Hinge, Yaw, W, bExterior ? Teal : Trim, !bExterior);
		}
		Cur = O.To;
	}
	Piece(Cur, Len, 0.f, WallH);
	Baseboard(Cur, Len);
}

void AISHomeWorld::AddDoor(const FVector& HingePos, float Yaw, float Width, const FLinearColor& Color, bool bStartOpen, bool bLocked, const FString& LockMsg)
{
	USceneComponent* H = NewObject<USceneComponent>(this);
	H->SetupAttachment(Root);
	H->SetRelativeLocationAndRotation(HingePos, FRotator(0, Yaw, 0));
	H->RegisterComponent();
	FISModel M;
	UStaticMeshComponent* Leaf = ISBuild::Prim(this, H, ISMat::Cube(), FVector(Width * 0.5f, 0, 104.f), FVector(Width - 2.f, 4.f, 208.f), Color, &M, FRotator::ZeroRotator, 0.f, 0.6f);
	ISBuild::Prim(this, H, ISMat::Cube(), FVector(Width * 0.5f, 0, 160.f), FVector(Width - 22.f, 5.f, 70.f), Color * 0.92f, &M, FRotator::ZeroRotator, 0.f, 0.6f);
	ISBuild::Prim(this, H, ISMat::Cube(), FVector(Width * 0.5f, 0, 60.f), FVector(Width - 22.f, 5.f, 80.f), Color * 0.92f, &M, FRotator::ZeroRotator, 0.f, 0.6f);
	for (float Side : { -1.f, 1.f })
		ISBuild::Prim(this, H, ISMat::Sphere(), FVector(Width - 12.f, Side * 5.f, 100.f), FVector(6.f), FLinearColor(0.85f, 0.7f, 0.3f), &M, FRotator::ZeroRotator, 1.f, 0.2f);
	if (Leaf)
	{
		Leaf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Leaf->SetCollisionResponseToAllChannels(ECR_Block);
		Actions.Add(Leaf, FName(*FString::Printf(TEXT("door:%d"), Doors.Num())));
	}
	FISDoor D;
	D.Hinge = H;
	D.bLocked = bLocked;
	D.LockMsg = LockMsg;
	D.Target = D.Angle = bStartOpen ? D.OpenAngle : 0.f;
	H->SetRelativeRotation(FRotator(0, Yaw + D.Angle, 0));
	Doors.Add(D);
	DoorYaw.Add(Yaw);
}

// Gable roof as a procedural mesh (double-sided), ridge along X.
void AISHomeWorld::Roof(const FVector& C, float SX, float SY, float Pitch, float Ov, const TCHAR*, const FLinearColor& Tint, const TCHAR*, const FLinearColor& GableTint)
{
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(this);
	PM->SetupAttachment(Root);
	PM->RegisterComponent();
	const float H = SY * 0.5f * FMath::Tan(FMath::DegreesToRadians(Pitch));
	const float X0 = C.X - SX * 0.5f, X1 = C.X + SX * 0.5f, Y0 = C.Y - SY * 0.5f, Y1 = C.Y + SY * 0.5f, Z = C.Z;
	const float OvZ = Ov * FMath::Tan(FMath::DegreesToRadians(Pitch));
	auto Quad = [](TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, FVector A, FVector B, FVector Cc, FVector D)
	{
		const FVector Nrm = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const int32 I = V.Num();
			V.Append({ A, B, Cc, D });
			const FVector Nn = Side ? -Nrm : Nrm;
			N.Append({ Nn, Nn, Nn, Nn });
			if (Side) T.Append({ I, I + 1, I + 2, I, I + 2, I + 3 }); else T.Append({ I, I + 2, I + 1, I, I + 3, I + 2 });
		}
	};
	TArray<FVector> V, N; TArray<int32> T; TArray<FVector2D> UV;
	const FVector RidgeA(X0 - Ov, C.Y, Z + H + 4.f), RidgeB(X1 + Ov, C.Y, Z + H + 4.f);
	Quad(V, T, N, FVector(X0 - Ov, Y0 - Ov, Z - OvZ), FVector(X1 + Ov, Y0 - Ov, Z - OvZ), RidgeB, RidgeA);
	Quad(V, T, N, RidgeA, RidgeB, FVector(X1 + Ov, Y1 + Ov, Z - OvZ), FVector(X0 - Ov, Y1 + Ov, Z - OvZ));
	UV.SetNumZeroed(V.Num());
	PM->CreateMeshSection(0, V, T, N, UV, {}, {}, true);
	// gable triangles (house colour)
	TArray<FVector> V2, N2; TArray<int32> T2; TArray<FVector2D> UV2;
	for (float X : { X0, X1 })
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const int32 I = V2.Num();
			V2.Append({ FVector(X, Y0, Z), FVector(X, Y1, Z), FVector(X, C.Y, Z + H) });
			const FVector Nn(Side ? -1.f : 1.f, 0, 0);
			N2.Append({ Nn, Nn, Nn });
			if (Side) T2.Append({ I, I + 1, I + 2 }); else T2.Append({ I, I + 2, I + 1 });
		}
	UV2.SetNumZeroed(V2.Num());
	PM->CreateMeshSection(1, V2, T2, N2, UV2, {}, {}, true);
	UMaterialInstanceDynamic* RM = UMaterialInstanceDynamic::Create(ISMat::Color(), PM);
	RM->SetVectorParameterValue(TEXT("Color"), Tint); RM->SetScalarParameterValue(TEXT("Roughness"), 0.75f);
	UMaterialInstanceDynamic* GM = UMaterialInstanceDynamic::Create(ISMat::Color(), PM);
	GM->SetVectorParameterValue(TEXT("Color"), GableTint); GM->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
	PM->SetMaterial(0, RM);
	PM->SetMaterial(1, GM);
	// roof ridge cap + fascia
	Paint(FVector(C.X, C.Y, Z + H + 6.f), FVector(SX + 2 * Ov, 16.f, 8.f), Tint * 0.8f, 0.7f, false);
	AllComps.Add(PM);
}

// Neighbour house built exactly like ours (stucco walls, plaster inside, trimmed windows, tiled roof), door to the street.
void AISHomeWorld::HouseShell(const FVector& Origin, float SX, float SY, float Yaw, const FLinearColor& WallCol, const FLinearColor& DoorCol, const TCHAR*, const FLinearColor& RoofCol)
{
	const float X0 = Origin.X - SX * 0.5f, X1 = Origin.X + SX * 0.5f, Y0 = Origin.Y - SY * 0.5f, Y1 = Origin.Y + SY * 0.5f;
	const bool bDoorFront = Yaw < 0.f;          // -90: faces -Y (street below), +90: faces +Y
	Block(FVector(Origin.X, Origin.Y, FZ * 0.5f - 15.f), FVector(SX + 30.f, SY + 30.f, FZ + 30.f), TEXT("brushed_concrete"), 300.f, FLinearColor(0.85f, 0.84f, 0.82f));
	auto Win = [](float From) { return FISOpening{ From, From + 120.f, 90.f, 210.f, 1 }; };
	const float Mid = SX * 0.5f;
	TArray<FISOpening> Front = { Win(120.f), Win(SX - 240.f) }, Back = { Win(150.f), Win(SX - 270.f) };
	(bDoorFront ? Front : Back).Add(FISOpening{ Mid - 50.f, Mid + 50.f, 0.f, 215.f, 0 });
	Wall(FVector2D(X0, Y0), FVector2D(X1, Y0), Front, nullptr, Cream, nullptr, WallCol, -1.f);
	Wall(FVector2D(X0, Y1), FVector2D(X1, Y1), Back, nullptr, Cream, nullptr, WallCol, 1.f);
	Wall(FVector2D(X0, Y0), FVector2D(X0, Y1), { Win(SY * 0.5f - 60.f) }, nullptr, Cream, nullptr, WallCol, -1.f);
	Wall(FVector2D(X1, Y0), FVector2D(X1, Y1), { Win(SY * 0.5f - 60.f) }, nullptr, Cream, nullptr, WallCol, 1.f);
	// closed front door (the neighbours are not home)
	Paint(FVector(Origin.X, bDoorFront ? Y0 - 2.f : Y1 + 2.f, FZ + 107.f), FVector(98.f, 6.f, 214.f), DoorCol, 0.5f);
	Paint(FVector(Origin.X, bDoorFront ? Y0 - 60.f : Y1 + 60.f, FZ * 0.5f), FVector(160.f, 100.f, FZ), FLinearColor(0.75f, 0.74f, 0.72f), 0.9f);   // step
	Roof(FVector(Origin.X, Origin.Y, FZ + WallH + 10.f), SX, SY, 30.f, 50.f, nullptr, RoofCol, nullptr, WallCol);
}

// Wooden picket-rail fence from A to B: posts every 1.5 m and two rails, real plank texture.
void AISHomeWorld::WoodFence(const FVector& A, const FVector& B, float GapFrom, float GapTo)
{
	const FVector D = B - A;
	const float Len = D.Size2D();
	if (Len < 10.f) return;
	const FVector U = D.GetSafeNormal2D();
	const float Yaw = U.Rotation().Yaw;
	auto Plank = [this, Yaw](const FVector& C, const FVector& S) { Slab(C, S, TEXT("brown_planks_05"), 120.f, FLinearColor(0.85f, 0.75f, 0.62f), true, Yaw); };
	for (float T = 0.f; T <= Len + 1.f; T += 150.f)
	{
		if (T > GapFrom && T < GapTo) continue;
		Plank(A + U * T + FVector(0, 0, 55.f), FVector(10.f, 10.f, 110.f));
	}
	auto Rail = [&](float T0, float T1)
	{
		if (T1 - T0 < 5.f) return;
		for (float Z : { 40.f, 88.f }) Plank(A + U * ((T0 + T1) * 0.5f) + FVector(0, 0, Z), FVector(T1 - T0, 3.f, 12.f));
	};
	if (GapTo > GapFrom) { Rail(0.f, GapFrom); Rail(GapTo, Len); }
	else Rail(0.f, Len);
}

// =====================================================================================================
//  the home
// =====================================================================================================
void AISHomeWorld::BuildHome()
{
	auto K = [this](const TCHAR* Name, const FVector& C, float Face, float Scale = KF, bool bCollide = true)
	{
		UStaticMeshComponent* M = PlaceMesh(this, Root, AllComps, KPath(TEXT("furn"), Name), C, Face, Scale, bCollide, true);
		if (M)
		{
			FISPlaced P; P.Name = Name; P.C = M;
			P.bSurface = FString(Name).StartsWith(TEXT("rug"));
			P.bOnTop = C.Z > FZ + 20.f;   // placed on another piece (lamp on table, TV on cabinet...)
			Furniture.Add(P);
		}
		return M;
	};
	auto TopOf = [](UStaticMeshComponent* C, float Fallback) { return C ? C->Bounds.GetBox().Max.Z : Fallback; };

	// ---- foundations and floors
	Paint(FVector(750, 525, FZ * 0.5f - 15.f), FVector(1530, 1080, FZ + 30.f), FLinearColor(0.75f, 0.73f, 0.7f));      // plinth
	// floors: real wood planks / stone (textures instead of flat colour)
	Block(FVector(400, 300, FZ - 1.f), FVector(800, 600, 2), TEXT("brown_planks_05"), 200.f, FLinearColor(1.f, 0.95f, 0.9f));                // living
	Block(FVector(1150, 300, FZ - 1.f), FVector(700, 600, 2), TEXT("brushed_concrete"), 150.f, FLinearColor(1.15f, 1.13f, 1.08f));           // kitchen (pale polished concrete)
	Block(FVector(750, 680, FZ - 1.f), FVector(1500, 160, 2), TEXT("brown_planks_05"), 200.f, FLinearColor(0.95f, 0.9f, 0.85f));          // hall
	Block(FVector(250, 905, FZ - 1.f), FVector(500, 290, 2), TEXT("brown_planks_05"), 200.f, FLinearColor(1.f, 0.95f, 0.9f));              // office
	Block(FVector(750, 905, FZ - 1.f), FVector(500, 290, 2), TEXT("brown_planks_05"), 200.f, FLinearColor(1.f, 1.f, 0.95f));               // bedroom
	Paint(FVector(1125, 905, FZ - 1.f), FVector(250, 290, 2), Aqua, 0.3f);              // bath
	Paint(FVector(1375, 905, FZ - 1.f), FVector(250, 290, 2), GarageGrey, 0.8f);        // mudroom
	Paint(FVector(-380, 350, 1.f), FVector(760, 700, 2), GarageGrey * 0.85f, 0.9f);     // garage
	// front steps
	Paint(FVector(610, -45, 5.f), FVector(180, 70, 10), FLinearColor(0.8f, 0.78f, 0.74f));
	Paint(FVector(610, -20, FZ - 5.f), FVector(200, 40, FZ + 10.f), FLinearColor(0.8f, 0.78f, 0.74f));
	// ceilings
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Paint(FVector(750, 525, FZ + WallH + 5.f), FVector(1500, 1050, 10), FLinearColor(0.97f, 0.96f, 0.94f), 0.95f);
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Paint(FVector(-380, 350, WallH + 8.f), FVector(760, 700, 10), FLinearColor(0.9f, 0.9f, 0.9f), 0.95f);

	// ---- walls (openings: From, To, Bottom, Top, Kind 0=door 1=window 2=arch 3=exterior door)
	Wall({ 0, 0 }, { 1500, 0 }, { { 120, 420, 85, 220, 1 }, { 560, 660, 0, 212, 3 }, { 980, 1300, 95, 220, 1 } }, nullptr, WarmWhite, nullptr, Cream, -1.f);
	Wall({ 0, 1050 }, { 1500, 1050 }, { { 130, 380, 100, 220, 1 }, { 640, 800, 110, 220, 1 }, { 1080, 1180, 150, 220, 1 }, { 1290, 1380, 0, 212, 3 } }, nullptr, SoftBlue, nullptr, Cream, 1.f);
	Wall({ 0, 0 }, { 0, 1050 }, { { 440, 530, 0, 212, 0 }, { 830, 1000, 85, 220, 1 } }, nullptr, WarmWhite, nullptr, Cream, -1.f);
	Wall({ 1500, 0 }, { 1500, 1050 }, { { 160, 440, 95, 220, 1 } }, nullptr, Mint, nullptr, Cream, 1.f);
	Wall({ 0, 600 }, { 1500, 600 }, { { 640, 780, 0, 225, 2 }, { 830, 980, 0, 225, 2 } }, nullptr, Peach, nullptr, Peach, 0.f);
	Wall({ 0, 760 }, { 1500, 760 }, { { 200, 290, 0, 212, 0 }, { 880, 970, 0, 212, 0 }, { 1040, 1120, 0, 212, 0 }, { 1290, 1460, 0, 225, 2 } }, nullptr, Peach, nullptr, Peach, 0.f);
	Wall({ 800, 0 }, { 800, 600 }, { { 150, 450, 0, 225, 2 } }, nullptr, Mint, nullptr, Mint, 0.f);
	Wall({ 500, 760 }, { 500, 1050 }, {}, nullptr, Lilac, nullptr, Lilac, 0.f);
	Wall({ 1000, 760 }, { 1000, 1050 }, {}, nullptr, Aqua, nullptr, Aqua, 0.f);
	Wall({ 1250, 760 }, { 1250, 1050 }, {}, nullptr, GarageGrey, nullptr, GarageGrey, 0.f);
	// bathroom door: locked
	for (FISDoor& D : Doors)
		if (D.Hinge && FVector::Dist2D(D.Hinge->GetRelativeLocation(), FVector(1040, 760, 0)) < 5.f)
		{
			D.bLocked = true; D.Target = D.Angle = 0.f;
			D.Hinge->SetRelativeRotation(FRotator(0, 0, 0));
			D.LockMsg = TEXT("Occupied! (BIT: robots don't need the bathroom anyway.)");
		}
	// garage walls (front opening is the big garage door)
	Wall({ -760, 0 }, { 0, 0 }, { { 80, 680, 0, 235, 2 } }, nullptr, GarageGrey, nullptr, FLinearColor(0.62f, 0.75f, 0.86f), -1.f, false);
	Wall({ -760, 700 }, { 0, 700 }, { { 60, 200, 140, 220, 1 } }, nullptr, GarageGrey, nullptr, FLinearColor(0.62f, 0.75f, 0.86f), 1.f, false);
	Wall({ -760, 0 }, { -760, 700 }, {}, nullptr, GarageGrey, nullptr, FLinearColor(0.62f, 0.75f, 0.86f), -1.f, false);
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Paint(FVector(-380, 350, FZ + WallH + 22.f), FVector(800, 740, 18), FLinearColor(0.35f, 0.36f, 0.4f), 0.8f);   // garage flat roof
	Paint(FVector(-380, -4, FZ + 255.f), FVector(640, 30, 40), FLinearColor(0.55f, 0.66f, 0.76f), 0.6f);          // rolled-up door box

	// ---- roof + chimney
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Roof(FVector(750, 525, FZ + WallH + 10.f), 1500, 1050, 30.f, 55.f, nullptr, Coral, nullptr, Cream);
	Paint(FVector(1180, 760, FZ + WallH + 260.f), FVector(70, 70, 330), FLinearColor(0.62f, 0.3f, 0.25f), 0.9f);
	// porch canopy + light
	Paint(FVector(610, -60, FZ + 240.f), FVector(220, 120, 8), Trim, 0.6f, false);
	for (float X : { 520.f, 700.f }) Paint(FVector(X, -110, FZ + 120.f), FVector(10, 10, 240), Trim, 0.6f);
	RoomLight(FVector(610, -60, FZ + 225.f), 400.f, false);

	// ---- lights
	RoomLight(FVector(400, 300, FZ + WallH - 20.f), 1600.f);
	RoomLight(FVector(1150, 300, FZ + WallH - 20.f), 1600.f);
	RoomLight(FVector(400, 680, FZ + WallH - 20.f), 700.f);
	RoomLight(FVector(1100, 680, FZ + WallH - 20.f), 700.f);
	RoomLight(FVector(250, 905, FZ + WallH - 20.f), 1100.f);
	RoomLight(FVector(750, 905, FZ + WallH - 20.f), 1000.f);
	RoomLight(FVector(1375, 905, FZ + WallH - 20.f), 600.f);
	RoomLight(FVector(-380, 350, WallH - 20.f), 1800.f);

	// Place a piece with its back flat against a wall (real measured depth), facing into the room.
	// bAlongX: the wall runs along X (fixed Y). Side: +1 if the room is on the + side of the wall.
	auto KW = [&](const TCHAR* Name, bool bAlongX, float WallC, float Side, float Along, float Z = FZ, float Scale = KF, bool bCollide = true) -> UStaticMeshComponent*
	{
		const float Face = bAlongX ? (Side > 0 ? 90.f : -90.f) : (Side > 0 ? 0.f : 180.f);
		const FVector Guess = bAlongX ? FVector(Along, WallC + Side * 60.f, Z) : FVector(WallC + Side * 60.f, Along, Z);
		UStaticMeshComponent* M = K(Name, Guess, Face, Scale, bCollide);
		if (!M) return nullptr;
		if (Furniture.Num() && Z > FZ + 20.f) Furniture.Last().bOnTop = true;
		const FBox B = M->Bounds.GetBox();
		const float Depth = bAlongX ? B.GetSize().Y : B.GetSize().X;
		const float Want = WallC + Side * (WallT * 0.5f + 1.5f + Depth * 0.5f);
		const float Have = bAlongX ? B.GetCenter().Y : B.GetCenter().X;
		M->AddWorldOffset(bAlongX ? FVector(0, Want - Have, 0) : FVector(Want - Have, 0, 0));
		return M;
	};
	auto CenterOf = [](UStaticMeshComponent* C, const FVector& Fallback) { return C ? C->Bounds.GetBox().GetCenter() : Fallback; };

	// ---- living room (TV against the hall wall, sofa facing it, reading corner by the garage wall)
	K(TEXT("rugRectangle"), FVector(330, 300, FZ), 90.f, KF * 1.3f, false);
	UStaticMeshComponent* TVCab = KW(TEXT("cabinetTelevision"), true, 600, -1, 330);
	const FVector TVc = CenterOf(TVCab, FVector(330, 560, FZ));
	KW(TEXT("televisionModern"), true, 600, -1, 330, TopOf(TVCab, FZ + 60.f), KF, false);
	KW(TEXT("speaker"), true, 600, -1, 175);
	KW(TEXT("speaker"), true, 600, -1, 490);
	K(TEXT("loungeSofa"), FVector(330, 140, FZ), 90.f);
	K(TEXT("tableCoffee"), FVector(330, 310, FZ), 90.f);
	K(TEXT("loungeChair"), FVector(640, 300, FZ), 180.f);
	UStaticMeshComponent* Side = K(TEXT("sideTable"), FVector(490, 140, FZ), 90.f);
	K(TEXT("lampRoundTable"), FVector(490, 140, TopOf(Side, FZ + 55.f)), 90.f, KF, false);
	UStaticMeshComponent* Shelf = KW(TEXT("bookcaseOpen"), false, 0, 1, 260);
	K(TEXT("books"), FVector(CenterOf(Shelf, FVector(30, 260, 0)).X, 260, TopOf(Shelf, FZ + 180.f)), 0.f, KF, false);
	K(TEXT("pottedPlant"), FVector(745, 50, FZ), 0.f);
	K(TEXT("pottedPlant"), FVector(45, 565, FZ), 0.f);
	K(TEXT("lampRoundFloor"), FVector(55, 70, FZ), 0.f);
	(void)TVc;

	// ---- kitchen: counter run on the hall wall (right of the arch), fridge at the end, dining table by the windows
	const TCHAR* Run[] = { TEXT("kitchenCabinetDrawer"), TEXT("kitchenStoveElectric"), TEXT("kitchenSink"), TEXT("kitchenCabinet") };
	float CounterTop = FZ + 95.f;
	float RunX[UE_ARRAY_COUNT(Run)];
	for (int32 i = 0; i < UE_ARRAY_COUNT(Run); ++i)
	{
		RunX[i] = 1040.f + i * 90.f;
		UStaticMeshComponent* C = KW(Run[i], true, 600, -1, RunX[i]);
		if (i == 0 && C) CounterTop = TopOf(C, CounterTop);
		KW(TEXT("kitchenCabinetUpper"), true, 600, -1, RunX[i], FZ + 150.f, KF, false);
	}
	KW(TEXT("kitchenFridgeLarge"), true, 600, -1, 1425);
	KW(TEXT("toaster"), true, 600, -1, RunX[0], CounterTop, KF, false);
	KW(TEXT("kitchenMicrowave"), true, 600, -1, RunX[3], CounterTop, KF, false);
	// dining set: chairs placed from the table's real footprint, two per long side, facing it
	UStaticMeshComponent* Tbl = K(TEXT("table"), FVector(1150, 250, FZ), 0.f);
	if (Tbl)
	{
		const FBox TB = Tbl->Bounds.GetBox();
		const FVector Sz = TB.GetSize(), Ct = TB.GetCenter();
		const bool bLongX = Sz.X >= Sz.Y;
		const float Long = bLongX ? Sz.X : Sz.Y, Short = bLongX ? Sz.Y : Sz.X;
		for (int32 Side2 : { -1, 1 })
			for (int32 k = 0; k < 2; ++k)
			{
				const float Along = (k == 0 ? -0.25f : 0.25f) * Long;
				const float Off = Side2 * (Short * 0.5f + 40.f);
				const FVector Pc = bLongX ? FVector(Ct.X + Along, Ct.Y + Off, FZ) : FVector(Ct.X + Off, Ct.Y + Along, FZ);
				const float Face = bLongX ? (Side2 < 0 ? 90.f : -90.f) : (Side2 < 0 ? 0.f : 180.f);
				K(TEXT("chair"), Pc, Face);
			}
	}
	K(TEXT("pottedPlant"), FVector(1455, 50, FZ), 0.f);
	K(TEXT("trashcan"), FVector(1465, 470, FZ), 0.f);

	// ---- office (BIT's room): desk under the window with the order laptop, shelf on the side wall, charging dock
	UStaticMeshComponent* Desk = KW(TEXT("desk"), true, 1050, -1, 250);
	if (Desk) DeskBox = Desk->Bounds.GetBox();
	UE_LOG(LogTemp, Log, TEXT("[LAYOUT-INFO] desk %s"), *DeskBox.ToString());
	const float DeskTop = TopOf(Desk, FZ + 80.f);
	DeskTopZ = DeskTop;
	const float DeskY = CenterOf(Desk, FVector(250, 1005, 0)).Y;
	DeskTopY = DeskY;
	K(TEXT("chairDesk"), FVector(250, DeskY - 75.f, FZ), 90.f);
	MonitorTop = TopOf(K(TEXT("computerScreen"), FVector(237, DeskY + 15.f, DeskTop), -90.f, KF, false), DeskTop + 40.f);
	if (Furniture.Num()) Furniture.Last().bOnTop = true;
	K(TEXT("computerKeyboard"), FVector(237, DeskY - 12.f, DeskTop), -90.f, KF, false);
	K(TEXT("laptop"), FVector(DeskBox.GetCenter().X - 10.f, DeskY - 10.f, DeskTop), -90.f, KF, false);
	AddInteract(FVector(DeskBox.GetCenter().X - 10.f, DeskY - 10.f, DeskTop + 15.f), FVector(28, 25, 20), "laptop");
	K(TEXT("plantSmall2"), FVector(DeskBox.Min.X + 14.f, DeskY + 14.f, DeskTop), 0.f, KF, false);   // on the desk, at its real left end
	UStaticMeshComponent* OShelf = KW(TEXT("bookcaseClosedWide"), false, 500, -1, 915);
	OfficeShelfTop = TopOf(OShelf, FZ + 100.f);
	OfficeShelfX = CenterOf(OShelf, FVector(460, 915, 0)).X;
	K(TEXT("lampSquareFloor"), FVector(45, 795, FZ), 0.f);
	Paint(FVector(380, 830, FZ + 2.f), FVector(90, 90, 4), FLinearColor(0.2f, 0.22f, 0.26f), 0.4f, false);
	ISBuild::Unlit(this, Root, ISMat::Cyl(), FVector(380, 830, FZ + 4.5f), FVector(70, 70, 1), FLinearColor(0.1f, 0.85f, 1.f), 2.5f, nullptr);

	// ---- bedroom: bed head against the back wall, nightstands either side, wardrobe on the side wall
	K(TEXT("rugRound"), FVector(720, 850, FZ), 0.f, KF, false);
	UStaticMeshComponent* Bed = KW(TEXT("bedDouble"), true, 1050, -1, 720);
	const FBox BedB = Bed ? Bed->Bounds.GetBox() : FBox(FVector(620, 810, FZ), FVector(820, 1043, FZ + 80));
	BedTopZ = BedB.Max.Z - 6.f; BedX = BedB.GetCenter().X; BedY = BedB.Min.Y + BedB.GetSize().Y * 0.35f;   // near the foot of the bed
	UStaticMeshComponent* NS1 = KW(TEXT("cabinetBedDrawerTable"), true, 1050, -1, BedB.Min.X - 35.f);
	KW(TEXT("cabinetBedDrawerTable"), true, 1050, -1, BedB.Max.X + 35.f);
	K(TEXT("lampRoundTable"), FVector(BedB.Min.X - 35.f, CenterOf(NS1, FVector(0, 1015, 0)).Y, TopOf(NS1, FZ + 50.f)), -90.f, KF, false);
	if (Furniture.Num()) Furniture.Last().bOnTop = true;
	KW(TEXT("bookcaseClosedDoors"), false, 500, 1, 880);

	// ---- bathroom (door is locked; seen through the window)
	KW(TEXT("toilet"), true, 1050, -1, 1200);
	KW(TEXT("bathroomSink"), false, 1000, 1, 960);

	// ---- mudroom: washer in the corner, low shelf, coat rack
	KW(TEXT("washerDryerStacked"), false, 1500, -1, 985);
	UStaticMeshComponent* Mud = KW(TEXT("bookcaseOpenLow"), false, 1250, 1, 905);
	MudShelfTop = TopOf(Mud, FZ + 90.f);
	MudShelfX = CenterOf(Mud, FVector(1290, 880, 0)).X;
	MudShelfY = CenterOf(Mud, FVector(1290, 905, 0)).Y;

	// ---- garage: workbench on the back wall, shelf on the side wall, junk box by the door
	UStaticMeshComponent* Bench = KW(TEXT("desk"), true, 700, -1, -380, 2.f, KF * 1.2f);
	if (Bench)
	{
		const FBox B = Bench->Bounds.GetBox();
		BenchTop = FVector(B.GetCenter().X, B.GetCenter().Y, B.Max.Z);
	}
	else BenchTop = FVector(-380, 655, 90);
	AddInteract(BenchTop + FVector(0, 0, 10), FVector(70, 30, 20), "bench");
	K(TEXT("radio"), BenchTop + FVector(52, 10, 0), -90.f, KF, false);
	if (Furniture.Num()) Furniture.Last().bOnTop = true;
	AddInteract(BenchTop + FVector(52, 10, 15), FVector(25, 15, 18), "radio");
	K(TEXT("lampSquareTable"), BenchTop + FVector(-80, 15, 0), -90.f, KF, false);
	if (Furniture.Num()) Furniture.Last().bOnTop = true;
	UStaticMeshComponent* GShelf = KW(TEXT("bookcaseOpen"), false, -760, 1, 330, 2.f);
	if (GShelf) GarageShelfBox = GShelf->Bounds.GetBox();
	UE_LOG(LogTemp, Log, TEXT("[LAYOUT-INFO] garage shelf %s"), *GarageShelfBox.ToString());
	GarageShelfTop = TopOf(GShelf, 180.f) - 40.f;
	GarageShelfX = CenterOf(GShelf, FVector(-730, 330, 0)).X;
	JunkBoxTop = TopOf(K(TEXT("cardboardBoxOpen"), FVector(-690, 130, 2.f), 0.f), 40.f);
	K(TEXT("cardboardBoxClosed"), FVector(-690, 580, 2.f), 15.f);
	K(TEXT("trashcan"), FVector(-45, 650, 2.f), 0.f);
	AddSign(TEXT("WORKBENCH"), BenchTop + FVector(0, 30, 110), -90.f, 14.f, FColor(40, 40, 50));
}

// =====================================================================================================
//  street, neighbours, garden
// =====================================================================================================
void AISHomeWorld::BuildNeighbourhood()
{
	auto C = [this](const TCHAR* Kit, const TCHAR* Name, const FVector& P, float Face, float Scale, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(Kit, Name), P, Face, Scale, bCollide, true); };

	// road along X (asphalt-dark concrete), centre line, concrete sidewalks with kerbs
	Block(FVector(1000, -1800, 0.6f), FVector(19000, 960, 1.2f), TEXT("brushed_concrete"), 400.f, FLinearColor(0.28f, 0.28f, 0.29f));
	for (int32 i = -9; i <= 10; ++i) Paint(FVector(i * 1000.f + 250.f, -1800.f, 1.3f), FVector(300.f, 12.f, 0.4f), FLinearColor(0.9f, 0.88f, 0.8f), 0.8f, false);
	Block(FVector(1000, -1240, 4), FVector(19000, 160, 8), TEXT("brushed_concrete"), 250.f, FLinearColor(0.9f, 0.89f, 0.87f));
	Block(FVector(1000, -2360, 4), FVector(19000, 160, 8), TEXT("brushed_concrete"), 250.f, FLinearColor(0.9f, 0.89f, 0.87f));
	// our front garden: path, driveway, mailbox, flower beds, car
	Block(FVector(610, -700, 1.f), FVector(140, 1200, 2), TEXT("brushed_concrete"), 200.f, FLinearColor(0.95f, 0.94f, 0.92f));
	Block(FVector(-380, -650, 1.f), FVector(340, 1300, 2), TEXT("brushed_concrete"), 300.f, FLinearColor(0.85f, 0.84f, 0.82f));
	C(TEXT("car"), TEXT("sedan"), FVector(-380, -520, 2.f), -90.f, 1.6f);
	Paint(FVector(900, -1080, 55), FVector(8, 8, 110), Trim, 0.6f);
	Paint(FVector(900, -1080, 120), FVector(26, 45, 26), FLinearColor(0.2f, 0.45f, 0.75f), 0.5f);
	Paint(FVector(300, -90, 6), FVector(500, 110, 12), FLinearColor(0.42f, 0.28f, 0.2f), 0.95f, false);
	Paint(FVector(1150, -90, 6), FVector(600, 110, 12), FLinearColor(0.42f, 0.28f, 0.2f), 0.95f, false);
	FRandomStream R(7);
	const TCHAR* Flowers[] = { TEXT("flower_redA"), TEXT("flower_yellowB"), TEXT("flower_purpleA"), TEXT("flower_redC"), TEXT("flower_yellowA"), TEXT("flower_purpleC") };
	for (int32 i = 0; i < 26; ++i)
	{
		const float X = i < 12 ? 70.f + i * 40.f : 870.f + (i - 12) * 42.f;
		C(TEXT("nat"), Flowers[i % 6], FVector(X, -90.f + R.FRandRange(-30.f, 30.f), 10.f), R.FRandRange(0.f, 360.f), 1.6f, false);
	}
	C(TEXT("nat"), TEXT("plant_bushDetailed"), FVector(40, -120, 0), 0.f, 3.f, false);
	C(TEXT("nat"), TEXT("plant_bushDetailed"), FVector(1460, -120, 0), 0.f, 3.f, false);
	C(TEXT("nat"), TEXT("tree_oak"), FVector(1250, -700, 0), 30.f, KNat);
	C(TEXT("nat"), TEXT("tree_default"), FVector(-1100, 1400, 0), 0.f, KNat);

	// neighbours: built the same way as our house (same walls, windows, roof), each with its own colours
	struct FN { float X, Y, Face; FLinearColor Wall, Door, RoofC; };
	const FN Hs[] = {
		{ -2700, 500, -90, FLinearColor(0.93f, 0.9f, 0.82f), FLinearColor(0.2f, 0.3f, 0.45f), FLinearColor(0.35f, 0.3f, 0.28f) },
		{ 3600, 500, -90, FLinearColor(0.8f, 0.86f, 0.9f), FLinearColor(0.55f, 0.15f, 0.12f), FLinearColor(0.3f, 0.32f, 0.36f) },
		{ 6900, 500, -90, FLinearColor(0.95f, 0.93f, 0.9f), FLinearColor(0.15f, 0.35f, 0.2f), FLinearColor(0.5f, 0.28f, 0.22f) },
		{ -1600, -3700, 90, FLinearColor(0.88f, 0.82f, 0.72f), FLinearColor(0.3f, 0.2f, 0.12f), FLinearColor(0.32f, 0.3f, 0.3f) },
		{ 1800, -3700, 90, FLinearColor(0.92f, 0.92f, 0.92f), FLinearColor(0.1f, 0.25f, 0.4f), FLinearColor(0.55f, 0.3f, 0.24f) },
		{ 5200, -3700, 90, FLinearColor(0.85f, 0.88f, 0.8f), FLinearColor(0.5f, 0.35f, 0.15f), FLinearColor(0.3f, 0.3f, 0.33f) },
		{ -5200, -3700, 90, FLinearColor(0.94f, 0.88f, 0.86f), FLinearColor(0.2f, 0.2f, 0.22f), FLinearColor(0.45f, 0.27f, 0.22f) } };
	for (const FN& H : Hs)
	{
		HouseShell(FVector(H.X, H.Y, 0), 1100.f, 850.f, H.Face, H.Wall, H.Door, nullptr, H.RoofC);
		const float Dir = H.Y > 0 ? -1.f : 1.f;
		Block(FVector(H.X + 750.f, H.Y + Dir * 900.f, 1.f), FVector(320, 900, 2), TEXT("brushed_concrete"), 300.f, FLinearColor(0.85f, 0.84f, 0.82f));
		C(TEXT("nat"), R.RandRange(0, 1) ? TEXT("tree_default") : TEXT("tree_fat"), FVector(H.X - 800.f, H.Y + Dir * 800.f, 0), R.FRandRange(0.f, 360.f), KNat);
		WoodFence(FVector(H.X - 600.f, H.Y + Dir * 1150.f, 0), FVector(H.X + 500.f, H.Y + Dir * 1150.f, 0));
	}
}

void AISHomeWorld::BuildBackyard()
{
	auto C = [this](const TCHAR* Kit, const TCHAR* Name, const FVector& P, float Face, float Scale, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(Kit, Name), P, Face, Scale, bCollide, true); };
	// wooden fence around the back garden with a gate toward the field
	const float Y1 = 3400.f, X0 = -900.f, X1 = 2300.f;
	// one continuous plank fence; the gap lines up with the path to the launch field (gate at x 800)
	WoodFence(FVector(X0, Y1, 0), FVector(X1, Y1, 0), 800.f - X0 - 110.f, 800.f - X0 + 110.f);
	WoodFence(FVector(X0, 1100.f, 0), FVector(X0, Y1, 0));
	WoodFence(FVector(X1, 1100.f, 0), FVector(X1, Y1, 0));
	// patio + garden furniture
	Paint(FVector(750, 1350, 4), FVector(900, 500, 8), FLinearColor(0.78f, 0.74f, 0.68f), 0.9f);
	auto F = [this](const TCHAR* Name, const FVector& Pp, float Face) { return PlaceMesh(this, Root, AllComps, KPath(TEXT("furn"), Name), Pp, Face, KF, true, true); };
	F(TEXT("tableRound"), FVector(600, 1400, 8), 0.f);
	F(TEXT("chairModernCushion"), FVector(520, 1400, 8), 0.f);
	F(TEXT("chairModernCushion"), FVector(680, 1400, 8), 180.f);
	F(TEXT("loungeChairRelax"), FVector(1000, 1350, 8), 90.f);
	C(TEXT("nat"), TEXT("tree_oak"), FVector(1900, 2600, 0), 0.f, KNat);
	C(TEXT("nat"), TEXT("tree_small"), FVector(-500, 2900, 0), 0.f, KNat);
	C(TEXT("nat"), TEXT("plant_bushLarge"), FVector(2150, 1400, 0), 0.f, 3.f);
	C(TEXT("nat"), TEXT("plant_bushLarge"), FVector(-750, 1500, 0), 0.f, 3.f);
	// path from the gate to the launch field
	const FVector Gate(800, Y1, 0);
	const FVector Seg = PadPos - Gate;
	UStaticMeshComponent* Path = Paint((Gate + PadPos) * 0.5f + FVector(0, 0, 0.6f), FVector(Seg.Size2D(), 160, 1.2f), FLinearColor(0.55f, 0.42f, 0.3f), 0.95f, false);
	Path->SetRelativeRotation(FRotator(0, Seg.Rotation().Yaw, 0));

	// rolling hills on the horizon (soft Astroneer-like silhouettes)
	FRandomStream R(11);
	for (int32 i = 0; i < 14; ++i)
	{
		const float A = R.FRandRange(0.f, 2.f * PI);
		const float D = R.FRandRange(60000.f, 110000.f);
		const float Rad = R.FRandRange(15000.f, 40000.f);
		const FLinearColor Hc = FMath::Lerp(FLinearColor(0.32f, 0.58f, 0.25f), FLinearColor(0.5f, 0.68f, 0.32f), R.FRand());
		ISBuild::Prim(this, Root, ISMat::Sphere(), FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, -Rad * 0.75f), FVector(Rad * 2.f, Rad * 2.4f, Rad * 2.f), Hc, nullptr, FRotator(0, R.FRandRange(0.f, 180.f), 0), 0.f, 0.95f);
	}

	// instanced nature (trees, bushes, grass, flowers), density follows the graphics preset
	auto Scatter = [&](const TCHAR* Name, int32 N, float MinR, float MaxR, const FVector& Ctr, float MinS, float MaxS, float Cull)
	{
		// realistic model at real size (the old Kenney scale range becomes a +-20% size variation)
		const FISRealSwap* Sw = RealSwap(Name);
		UStaticMesh* Mesh = ISMat::Mesh(Sw && *Sw->Ph ? ISMat::PH(Sw->Ph) : KPath(TEXT("nat"), Name));
		if (!Mesh) return;
		if (Sw && *Sw->Ph) { const float MidS = (MinS + MaxS) * 0.5f; MinS = Sw->Scale * MinS / MidS; MaxS = Sw->Scale * MaxS / MidS; }
		N = FMath::Max(1, FMath::RoundToInt(N * ISQuality::Density()));
		UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		H->SetStaticMesh(Mesh);
		H->SetupAttachment(Root);
		H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		H->SetCullDistances(0, (int32)Cull);
		H->RegisterComponent();
		int32 Added = 0, Tries = 0;
		while (Added < N && Tries++ < N * 30)
		{
			const float A = R.FRandRange(0.f, 2.f * PI);
			const float Dd = FMath::Sqrt(R.FRandRange(MinR * MinR, MaxR * MaxR));
			const FVector P = Ctr + FVector(FMath::Cos(A) * Dd, FMath::Sin(A) * Dd, 0);
			// keep house, street, garden and launch field clear
			if (P.X > -1200 && P.X < 2600 && P.Y > -2600 && P.Y < 3500) continue;
			if (FMath::Abs(P.Y + 1800.f) < 900.f) continue;
			if (FVector::Dist2D(P, PadPos) < 4000.f) continue;
			const float S = R.FRandRange(MinS, MaxS);
			H->AddInstance(FTransform(FRotator(0, R.FRandRange(0.f, 360.f), 0), P - FVector(0, 0, 4.f), FVector(S)));
			++Added;
		}
		AllComps.Add(H);
	};
	const FVector Mid(800, 6000, 0);
	Scatter(TEXT("tree_default"), 160, 6000.f, 40000.f, Mid, 3.5f, 6.f, 150000.f);
	Scatter(TEXT("tree_oak"), 120, 6000.f, 40000.f, Mid, 3.5f, 5.5f, 150000.f);
	Scatter(TEXT("tree_pineRoundB"), 120, 9000.f, 45000.f, Mid, 4.f, 7.f, 150000.f);
	Scatter(TEXT("tree_fat"), 80, 7000.f, 40000.f, Mid, 3.5f, 5.5f, 150000.f);
	Scatter(TEXT("plant_bush"), 260, 2500.f, 30000.f, Mid, 2.5f, 4.5f, 40000.f);
	Scatter(TEXT("rock_largeA"), 40, 6000.f, 35000.f, Mid, 1.5f, 3.5f, 60000.f);
	Scatter(TEXT("grass_large"), 2800, 2500.f, 16000.f, Mid, 1.4f, 2.2f, 9000.f);
	Scatter(TEXT("grass"), 2800, 2500.f, 16000.f, Mid, 1.4f, 2.2f, 9000.f);
	Scatter(TEXT("flower_yellowA"), 500, 2500.f, 14000.f, Mid, 1.2f, 1.7f, 7000.f);
	Scatter(TEXT("flower_purpleB"), 500, 2500.f, 14000.f, Mid, 1.2f, 1.7f, 7000.f);
}

// =====================================================================================================
//  chapter 0: parts to find around the house
// =====================================================================================================
void AISHomeWorld::SpawnHunt()
{
	for (FISHuntItem& H : Hunt) { H.Model.Destroy(); if (H.Hit) { Actions.Remove(H.Hit); H.Hit->DestroyComponent(); } }
	Hunt.Reset();
	auto Add = [this](FName Part, const TCHAR* Name, const TCHAR* Where, const FVector& InPos, bool bReq, const TCHAR* Cad, float CadScale)
	{
		const FVector Pos = InPos;   // SettleProps() later drops it onto the real surface (physics is not queryable yet in BeginPlay)
		FISHuntItem I;
		I.Part = Part; I.Name = Name; I.Where = Where; I.Pos = Pos; I.bRequired = bReq;
		USceneComponent* S = NewObject<USceneComponent>(this);
		S->SetupAttachment(Root);
		S->SetRelativeLocation(Pos);
		S->RegisterComponent();
		// shown 1.6x real size: big enough to spot, small enough to sit on a shelf without hanging over the edge
		USceneComponent* Scaled = NewObject<USceneComponent>(this);
		Scaled->SetupAttachment(S);
		Scaled->SetRelativeScale3D(FVector(1.f));   // rescaled below once the model size is known
		Scaled->RegisterComponent();
		const FString CadP = Cad ? CadPath(Cad) : FString();
		if (Cad && ISMat::Mesh(CadP)) ISBuild::Asset(this, Scaled, CadP, FVector::ZeroVector, CadScale, &I.Model);
		else ISBuild::CanSatPart(this, Scaled, Part, FVector::ZeroVector, &I.Model);
		if (Part == "bus_can") ISBuild::CanShell(this, Scaled, FVector::ZeroVector, false, &I.Model);
		// visible size: the longest side at least ~12 cm (tiny modules like a pin header would be specks), max 3x real size
		{
			FBox Bx(ForceInit);
			for (UStaticMeshComponent* Pc : I.Model.Parts) if (Pc) { Pc->UpdateBounds(); Bx += Pc->Bounds.GetBox(); }
			const float Big = Bx.IsValid ? (float)Bx.GetSize().GetMax() : 10.f;
			Scaled->SetRelativeScale3D(FVector(FMath::Clamp(12.f / FMath::Max(0.5f, Big), 1.2f, 3.f)));
		}
		// rest the part ON the surface: lift/lower so the lowest point of the model touches Pos.Z (CAD origins vary)
		{
			FBox Bx(ForceInit);
			for (UStaticMeshComponent* Pc : I.Model.Parts) if (Pc) { Pc->UpdateBounds(); Bx += Pc->Bounds.GetBox(); }
			if (Bx.IsValid) Scaled->AddWorldOffset(FVector(0, 0, S->GetComponentLocation().Z - Bx.Min.Z + 0.1f));
		}
		UBoxComponent* B = NewObject<UBoxComponent>(this);
		B->SetupAttachment(S);
		B->SetBoxExtent(FVector(14, 14, 10));
		B->SetRelativeLocation(FVector(0, 0, 10));
		B->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		B->SetCollisionResponseToAllChannels(ECR_Ignore);
		B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
		B->RegisterComponent();
		I.Hit = B;
		I.Root = S;
		Actions.Add(B, FName(*(TEXT("item:") + Part.ToString())));
		Hunt.Add(MoveTemp(I));
	};
	const float Counter = FZ + 95.f;
	Add("bus_can", TEXT("Empty soda can"), TEXT("Kitchen counter"), FVector(1150, 555, Counter), true, nullptr, 1.f);
	Add("cpu_ard", TEXT("ESP32 microcontroller"), TEXT("Office desk"), FVector(262, DeskTopY - 30.f, DeskTopZ), true, TEXT("esp32_wroom32"), 1.f);
	Add("com_433", TEXT("LoRa radio module"), TEXT("Garage shelf"), FVector(GarageShelfX, GarageShelfBox.GetCenter().Y - GarageShelfBox.GetExtent().Y * 0.45f, GarageShelfTop), true, TEXT("radio_zeta433"), 1.f);
	Add("pl_temp", TEXT("DHT11 temperature sensor"), TEXT("Electronics kit on the office shelf"), FVector(OfficeShelfX, 915, OfficeShelfTop), true, TEXT("dht11"), 1.f);
	Add("bat_aa", TEXT("18650 battery + holder"), TEXT("Mudroom shelf (old flashlight)"), FVector(MudShelfX, MudShelfY, MudShelfTop), true, TEXT("battery_18650"), 1.f);
	Add("ex_chute_l", TEXT("Parachute (ripstop nylon)"), TEXT("Folded on the bed"), FVector(BedX, BedY, BedTopZ), true, nullptr, 1.f);
	Add("ex_wires", TEXT("Jumper wires + pin headers"), TEXT("Garage workbench"), BenchTop + FVector(-30, 10, 0), true, TEXT("header_1x15"), 1.f);
	Add("ex_buzzer", TEXT("Buzzer (recovery beeper)"), TEXT("Box of old electronics in the garage"), FVector(-690, 130, JunkBoxTop), false, TEXT("buzzer"), 1.f);
	Add("pl_gnss_mini", TEXT("GNSS module"), TEXT("Old car sat-nav on the garage shelf"), FVector(GarageShelfX, GarageShelfBox.GetCenter().Y + GarageShelfBox.GetExtent().Y * 0.45f, GarageShelfTop), false, nullptr, 1.f);
	Add("pl_cam_mini", TEXT("Camera module (old webcam)"), TEXT("Office desk, behind the laptop"), FVector(DeskBox.Max.X - 12.f, DeskTopY + 22.f, DeskTopZ), false, nullptr, 1.f);
}

int32 AISHomeWorld::FoundCount(bool bRequiredOnly) const
{
	int32 N = 0;
	for (const FISHuntItem& H : Hunt) if (H.bFound && (!bRequiredOnly || H.bRequired)) ++N;
	return N;
}

int32 AISHomeWorld::RequiredCount() const
{
	int32 N = 0;
	for (const FISHuntItem& H : Hunt) if (H.bRequired) ++N;
	return N;
}

bool AISHomeWorld::IsFound(FName Part) const
{
	for (const FISHuntItem& H : Hunt) if (H.Part == Part) return H.bFound;
	return true;   // parts that are not part of the hunt are always available
}

void AISHomeWorld::BuildGround2()
{
	UStaticMeshComponent* G = NewObject<UStaticMeshComponent>(this);
	G->SetStaticMesh(ISMat::Cube());
	G->SetupAttachment(Root);
	G->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector(800, 5000, -50), FVector(3000, 3000, 1)));
	G->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	G->SetCollisionResponseToAllChannels(ECR_Block);
	G->RegisterComponent();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Ground.M_IS_Ground"))) G->SetMaterial(0, M);
	AllComps.Add(G);
	// mowed lawn around the house: slightly brighter
	Paint(FVector(700, 300, 0.4f), FVector(4200, 6600, 0.6f), FLinearColor(0.42f, 0.7f, 0.28f), 0.95f, false);
}

// Logs furniture that blocks doors/windows, overlaps other furniture, floats, or pokes through walls.
void AISHomeWorld::ValidateLayout()
{
	int32 Issues = 0;
	auto Log = [&Issues](const FString& S) { ++Issues; UE_LOG(LogTemp, Warning, TEXT("[LAYOUT] %s"), *S); };
	for (int32 i = 0; i < Furniture.Num(); ++i)
	{
		const FISPlaced& A = Furniture[i];
		if (!A.C || A.bSurface) continue;
		const FBox BA = A.C->Bounds.GetBox().ExpandBy(-2.f);
		const FVector Ca = BA.GetCenter();
		for (int32 o = 0; o < Openings.Num() && !A.bOnTop; ++o)
			if (BA.Intersect(Openings[o]))
				Log(FString::Printf(TEXT("%s at (%.0f,%.0f) blocks opening %d centred (%.0f,%.0f)"), *A.Name, Ca.X, Ca.Y, o, Openings[o].GetCenter().X, Openings[o].GetCenter().Y));
		for (int32 j = i + 1; j < Furniture.Num(); ++j)
		{
			const FISPlaced& B = Furniture[j];
			if (!B.C || B.bSurface) continue;
			if (A.bOnTop != B.bOnTop) continue;   // a lamp on a table is fine
			const FBox BB = B.C->Bounds.GetBox().ExpandBy(-2.f);
			if (BA.Intersect(BB))
			{
				const FBox X = BA.Overlap(BB);
				const FVector S = X.GetSize();
				if (S.X > 4.f && S.Y > 4.f && S.Z > 4.f)
					Log(FString::Printf(TEXT("%s (%.0f,%.0f) overlaps %s (%.0f,%.0f) by %.0fx%.0f cm"), *A.Name, Ca.X, Ca.Y, *B.Name, BB.GetCenter().X, BB.GetCenter().Y, S.X, S.Y));
			}
		}
		if (!A.bOnTop && BA.Min.Z > FZ + 6.f)
			Log(FString::Printf(TEXT("%s (%.0f,%.0f) floats %.0f cm above the floor"), *A.Name, Ca.X, Ca.Y, BA.Min.Z - FZ));
		// walls: furniture should stay inside the house rooms (x 0..1500, y 0..1050) or the garage (x -760..0, y 0..700)
		const bool bInHouse = Ca.X > 0 && Ca.X < 1500 && Ca.Y > 0 && Ca.Y < 1050;
		const bool bInGarage = Ca.X < 0 && Ca.X > -760 && Ca.Y > 0 && Ca.Y < 700;
		if (bInHouse && (BA.Min.X < 7 || BA.Max.X > 1493 || BA.Min.Y < 7 || BA.Max.Y > 1043))
			Log(FString::Printf(TEXT("%s (%.0f,%.0f) pokes into an outer wall"), *A.Name, Ca.X, Ca.Y));
		if (bInGarage && (BA.Min.X < -753 || BA.Max.X > -7 || BA.Min.Y < 7 || BA.Max.Y > 693))
			Log(FString::Printf(TEXT("%s (%.0f,%.0f) pokes into a garage wall"), *A.Name, Ca.X, Ca.Y));
		// interior walls
		const float WallsX[] = { 800.f, 500.f, 1000.f, 1250.f };
		const float WallsY[] = { 600.f, 760.f };
		for (float Wy : WallsY) if (bInHouse && BA.Min.Y < Wy + 7.f && BA.Max.Y > Wy - 7.f)
			Log(FString::Printf(TEXT("%s (%.0f,%.0f) crosses the wall at y=%.0f"), *A.Name, Ca.X, Ca.Y, Wy));
		for (float Wx : WallsX)
		{
			const bool bSpan = (Wx == 800.f) ? (BA.Max.Y < 600.f) : (BA.Min.Y > 760.f);
			if (bInHouse && bSpan && BA.Min.X < Wx + 7.f && BA.Max.X > Wx - 7.f)
				Log(FString::Printf(TEXT("%s (%.0f,%.0f) crosses the wall at x=%.0f"), *A.Name, Ca.X, Ca.Y, Wx));
		}
	}
	// support: things standing on other things must rest on them: something directly under the centre (else it floats),
	// and the footprint must stay inside the supporting surface (else it hangs over the edge). Hunt items too.
	auto Check = [this, &Log](const FString& What, const FBox& Bx, const TArray<const UPrimitiveComponent*>& Ignore)
	{
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ISSupport), true);
		for (const UPrimitiveComponent* Ig : Ignore) Q.AddIgnoredComponent(Ig);
		FHitResult H;
		const FVector C = Bx.GetCenter();
		if (!GetWorld()->LineTraceSingleByChannel(H, FVector(C.X, C.Y, Bx.Min.Z + 2.f), FVector(C.X, C.Y, Bx.Min.Z - 4.f), ECC_Visibility, Q) || !H.GetComponent())
		{
			Log(FString::Printf(TEXT("%s (%.0f,%.0f,%.0f) floats"), *What, C.X, C.Y, Bx.Min.Z));
			return;
		}
		const FBox S = H.GetComponent()->Bounds.GetBox();
		const float Over = FMath::Max(FMath::Max(S.Min.X - Bx.Min.X, Bx.Max.X - S.Max.X), FMath::Max(S.Min.Y - Bx.Min.Y, Bx.Max.Y - S.Max.Y));
		if (Over > 3.f) Log(FString::Printf(TEXT("%s (%.0f,%.0f) hangs %.0f cm over the edge of %s"), *What, C.X, C.Y, Over, *H.GetComponent()->GetName()));
	};
	for (const FISPlaced& A : Furniture)
		if (A.C && A.bOnTop && !A.Name.Contains(TEXT("Upper"))) Check(A.Name, A.C->Bounds.GetBox(), { A.C });
	for (const FISHuntItem& H : Hunt)
	{
		FBox Bx(ForceInit);
		TArray<const UPrimitiveComponent*> Ig;
		for (UStaticMeshComponent* Pc : H.Model.Parts) if (Pc) { Bx += Pc->Bounds.GetBox(); Ig.Add(Pc); }
		if (H.Hit) Ig.Add(H.Hit);
		if (Bx.IsValid) Check(TEXT("hunt item ") + H.Name, Bx, Ig);
		if (Bx.IsValid && Bx.Min.Z < 10.f && H.Where.Contains(TEXT("shelf")) ) Log(FString::Printf(TEXT("hunt item %s fell to the floor (meant for: %s)"), *H.Name, *H.Where));
	}
	UE_LOG(LogTemp, Warning, TEXT("[LAYOUT] done: %d issues, %d pieces checked"), Issues, Furniture.Num());
}

// Drop every decoration standing on furniture and every hunt part straight down onto the surface below it,
// so nothing floats above or sinks into a table, shelf or bed. Runs once, after the world is activated
// (Chaos only answers traces for bodies after a physics update).
void AISHomeWorld::SettleProps()
{
	bSettled = true;
	auto DropTo = [this](USceneComponent* Mover, const FBox& Bx, const TArray<const UPrimitiveComponent*>& Ignore, float MaxDrop) -> bool
	{
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ISSettle), true);
		for (const UPrimitiveComponent* Ig : Ignore) Q.AddIgnoredComponent(Ig);
		FHitResult H;
		const FVector Start(Bx.GetCenter().X, Bx.GetCenter().Y, Bx.Min.Z + 3.f);
		if (!GetWorld()->LineTraceSingleByChannel(H, Start, Start - FVector(0, 0, MaxDrop), ECC_Visibility, Q)) return false;
		Mover->AddWorldOffset(FVector(0, 0, H.ImpactPoint.Z - Bx.Min.Z + 0.05f));
		return true;
	};
	int32 Moved = 0;
	for (FISPlaced& A : Furniture)
	{
		if (!A.C || !A.bOnTop || A.Name.Contains(TEXT("Upper"))) continue;   // wall cabinets hang on the wall on purpose
		if (DropTo(A.C, A.C->Bounds.GetBox(), { A.C }, 120.f)) ++Moved;
	}
	for (FISHuntItem& H : Hunt)
	{
		if (!H.Root) continue;
		FBox Bx(ForceInit);
		TArray<const UPrimitiveComponent*> Ig;
		for (UStaticMeshComponent* Pc : H.Model.Parts) if (Pc) { Bx += Pc->Bounds.GetBox(); Ig.Add(Pc); }
		if (H.Hit) Ig.Add(H.Hit);
		if (Bx.IsValid && DropTo(H.Root, Bx, Ig, 150.f))
		{
			H.Pos = Root->GetComponentTransform().InverseTransformPosition(H.Root->GetComponentLocation());
			++Moved;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[LAYOUT-INFO] settled %d props onto their surfaces"), Moved);
}

void AISHomeWorld::ApplyLightQuality()
{
	for (int32 i = 0; i < RoomLights.Num(); ++i)
		if (RoomLights[i]) RoomLights[i]->SetCastShadows(ISQuality::Level >= 3 && i < 2);
}
