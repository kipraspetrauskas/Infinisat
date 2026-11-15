// Infinisat - the player's home and neighbourhood (stylised, Astroneer-like palette).
// Architecture is built from coloured blocks + a procedural roof; furniture, houses, roads,
// trees and flowers are Kenney CC0 kits; electronics are KiCad parts converted in Fusion.
#include "ISWorld.h"
#include "ISBit.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "ISEarth.h"
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

static UStaticMeshComponent* PlaceMesh(AISHomeWorld* W, USceneComponent* Root, TArray<UPrimitiveComponent*>& All, const FString& Path, const FVector& Center, float FaceYaw, float Scale, bool bCollide, bool bKenney)
{
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
	const bool bKit = KitReady();   // handmade Kenney Building Kit pieces draw the wall; the boxes below only collide

	auto Piece = [&](float U1, float U2, float Z1, float Z2)
	{
		if (U2 - U1 < 0.5f || Z2 - Z1 < 0.5f) return;
		UStaticMeshComponent* Core = Paint(P((U1 + U2) * 0.5f, 0, FZ + (Z1 + Z2) * 0.5f), S(U2 - U1, WallT, Z2 - Z1), CoreCol, 0.85f);
		if (bKit) { if (Core) { Core->SetVisibility(false); Core->SetCastShadow(false); } return; }
		if (OutSide != 0.f)   // interior paint layer on the inside face
			Paint(P((U1 + U2) * 0.5f, -OutSide * (WallT * 0.5f + 0.6f), FZ + (Z1 + Z2) * 0.5f), S(U2 - U1, 1.f, Z2 - Z1), In, 0.9f, false);
	};
	auto Baseboard = [&](float U1, float U2)
	{
		if (bKit || !bBaseboard || U2 - U1 < 2.f) return;
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
		// frame trims (both faces) - the kit pieces bring their own frames
		const float FT = WallT + 3.f;
		Paint(P(O.From - 3.f, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(6.f, FT, O.Top - O.Bottom + 6.f), Trim, 0.5f, false);
		Paint(P(O.To + 3.f, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(6.f, FT, O.Top - O.Bottom + 6.f), Trim, 0.5f, false);
		Paint(P(Mid, 0, FZ + O.Top + 3.f), S(W + 12.f, FT, 6.f), Trim, 0.5f, false);
		if (O.Kind == 1)
		{
			// window: glass, cross mullions, sill
			UStaticMeshComponent* G = ISBuild::Glass(this, Root, ISMat::Cube(), P(Mid, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(W, 1.5f, O.Top - O.Bottom), FLinearColor(0.75f, 0.9f, 1.f), 0.18f, nullptr);
			if (G) { G->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); G->SetCollisionResponseToAllChannels(ECR_Block); AllComps.Add(G); }
			{
				Paint(P(Mid, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(4.f, 4.f, O.Top - O.Bottom), Trim, 0.5f, false);
				Paint(P(Mid, 0, FZ + (O.Bottom + O.Top) * 0.5f), S(W, 4.f, 4.f), Trim, 0.5f, false);
				Paint(P(Mid, 0, FZ + O.Bottom - 2.f), S(W + 16.f, WallT + 12.f, 4.f), Trim, 0.5f, false);
			}
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

	// ---- handmade wall: Kenney Building Kit pieces (200 cm wide, 240 tall; window hole 90 wide at 70..200, door hole
	// 90 wide 0..220, wide versions 400 / 290). Each framed piece is scaled so its hole matches the opening exactly;
	// where a frame doesn't fit between neighbours, plain wall pieces surround the opening instead.
	if (bKit)
	{
		const float PieceYaw = bAlongX ? 90.f : 0.f;   // kit pieces run along their local Y
		auto Slab = [&](float U1, float U2, float Z1, float Z2)
		{
			if (U2 - U1 < 1.f || Z2 - Z1 < 1.f) return;
			KitPiece(TEXT("KB"), TEXT("wall"), P((U1 + U2) * 0.5f, 0, FZ + Z1), PieceYaw, FVector(WallT / 10.f, (U2 - U1) / 200.f, (Z2 - Z1) / 240.f));
		};
		auto Run = [&](float U1, float U2)
		{
			if (U2 - U1 < 1.f) return;
			const int32 N = FMath::Max(1, FMath::RoundToInt((U2 - U1) / 200.f));
			const float L = (U2 - U1) / N;
			for (int32 k = 0; k < N; ++k) Slab(U1 + k * L, U1 + (k + 1) * L, 0.f, WallH);
		};
		float C0 = 0.f;
		for (int32 i = 0; i < Sorted.Num(); ++i)
		{
			const FISOpening& O = Sorted[i];
			const float Next = i + 1 < Sorted.Num() ? Sorted[i + 1].From : Len;
			const float W = O.To - O.From, Mid = (O.From + O.To) * 0.5f;
			bool bFramed = false;
			if (O.Kind != 2 && false)   // the kit frame holes never lined up with the openings: plain kit wall around every opening + white trims instead
			{
				const bool bWide = W > 160.f;
				const float SU = W / (bWide ? 290.f : 90.f), PL = (bWide ? 400.f : 200.f) * SU;
				const float HB = O.Kind == 1 ? 70.f : 0.f, HT = O.Kind == 1 ? 200.f : 220.f;
				const float SZ = (O.Top - O.Bottom) / (HT - HB);
				const float Z0 = O.Bottom - HB * SZ, Z1 = Z0 + 240.f * SZ;
				if (Mid - PL * 0.5f >= C0 - 0.5f && Mid + PL * 0.5f <= Next + 0.5f && Z0 > -1.f && Z1 <= WallH + 1.f)
				{
					const TCHAR* Name = O.Kind == 1 ? (bWide ? TEXT("wall_window_wide_square") : TEXT("wall_window_square"))
					                                : (bWide ? TEXT("wall_doorway_wide_square") : TEXT("wall_doorway_square"));
					KitPiece(TEXT("KB"), Name, P(Mid, 0, FZ + Z0), PieceYaw, FVector(WallT / 14.f, SU, SZ));
					Run(C0, Mid - PL * 0.5f);
					Slab(Mid - PL * 0.5f, Mid + PL * 0.5f, 0.f, Z0);
					Slab(Mid - PL * 0.5f, Mid + PL * 0.5f, Z1, WallH);
					C0 = Mid + PL * 0.5f;
					bFramed = true;
				}
			}
			if (!bFramed)
			{
				Run(C0, O.From);
				Slab(O.From, O.To, 0.f, O.Bottom);
				Slab(O.From, O.To, O.Top, WallH);
				C0 = O.To;
			}
		}
		Run(C0, Len);
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

// Neighbour house built exactly like ours (same walls, trimmed windows, gabled roof, same flat paint style), door to the street.
void AISHomeWorld::HouseShell(const FVector& Origin, float SX, float SY, float Yaw, const FLinearColor& WallCol, const FLinearColor& DoorCol, const TCHAR*, const FLinearColor& RoofCol)
{
	const float X0 = Origin.X - SX * 0.5f, X1 = Origin.X + SX * 0.5f, Y0 = Origin.Y - SY * 0.5f, Y1 = Origin.Y + SY * 0.5f;
	const bool bDoorFront = Yaw < 0.f;          // -90: faces -Y (street below), +90: faces +Y
	Paint(FVector(Origin.X, Origin.Y, FZ * 0.5f - 15.f), FVector(SX + 30.f, SY + 30.f, FZ + 30.f), FLinearColor(0.75f, 0.73f, 0.7f));   // plinth
	auto Win = [](float From) { return FISOpening{ From, From + 120.f, 90.f, 210.f, 1 }; };
	const float Mid = SX * 0.5f;
	TArray<FISOpening> Front = { Win(120.f), Win(SX - 240.f) }, Back = { Win(150.f), Win(SX - 270.f) };
	(bDoorFront ? Front : Back).Add(FISOpening{ Mid - 50.f, Mid + 50.f, 0.f, 215.f, 0 });
	Wall(FVector2D(X0, Y0), FVector2D(X1, Y0), Front, nullptr, Cream, nullptr, WallCol, -1.f);
	Wall(FVector2D(X0, Y1), FVector2D(X1, Y1), Back, nullptr, Cream, nullptr, WallCol, 1.f);
	Wall(FVector2D(X0, Y0), FVector2D(X0, Y1), { Win(SY * 0.5f - 60.f) }, nullptr, Cream, nullptr, WallCol, -1.f);
	Wall(FVector2D(X1, Y0), FVector2D(X1, Y1), { Win(SY * 0.5f - 60.f) }, nullptr, Cream, nullptr, WallCol, 1.f);
	Paint(FVector(Origin.X, bDoorFront ? Y0 - 2.f : Y1 + 2.f, FZ + 107.f), FVector(98.f, 6.f, 214.f), DoorCol, 0.5f);           // closed door
	Paint(FVector(Origin.X, bDoorFront ? Y0 - 60.f : Y1 + 60.f, FZ * 0.5f), FVector(160.f, 100.f, FZ), FLinearColor(0.75f, 0.74f, 0.72f), 0.9f);   // step
	Roof(FVector(Origin.X, Origin.Y, FZ + WallH + 10.f), SX, SY, 30.f, 50.f, nullptr, RoofCol, nullptr, WallCol);
	// nobody home: a black, empty interior so the windows read as dark glass (no seeing through the house)
	if (UStaticMeshComponent* Dark = Paint(FVector(Origin.X, Origin.Y, FZ + WallH * 0.5f), FVector(SX - 40.f, SY - 40.f, WallH - 2.f), FLinearColor(0.004f, 0.004f, 0.005f), 1.f, false))
		Dark->SetCastShadow(false);
}

// Picket-rail fence from A to B in the house's wood colour: posts every 1.5 m and two rails; optional gap (a gate opening).
void AISHomeWorld::WoodFence(const FVector& A, const FVector& B, float GapFrom, float GapTo)
{
	const FVector D = B - A;
	const float Len = D.Size2D();
	if (Len < 10.f) return;
	const FVector U = D.GetSafeNormal2D();
	const float Yaw = U.Rotation().Yaw;
	const FLinearColor FenceCol = Wood * 1.15f;
	for (float T = 0.f; T <= Len + 1.f; T += 150.f)
	{
		if (GapTo > GapFrom && T > GapFrom && T < GapTo) continue;
		Paint(A + U * T + FVector(0, 0, 55.f), FVector(10.f, 10.f, 110.f), FenceCol, 0.8f, true, Yaw);
	}
	if (GapTo > GapFrom)   // gate posts exactly at the opening
	{
		Paint(A + U * GapFrom + FVector(0, 0, 60.f), FVector(12.f, 12.f, 120.f), FenceCol, 0.8f, true, Yaw);
		Paint(A + U * GapTo + FVector(0, 0, 60.f), FVector(12.f, 12.f, 120.f), FenceCol, 0.8f, true, Yaw);
	}
	auto Rail = [&](float T0, float T1)
	{
		if (T1 - T0 < 5.f) return;
		for (float Z : { 40.f, 88.f }) Paint(A + U * ((T0 + T1) * 0.5f) + FVector(0, 0, Z), FVector(T1 - T0, 3.f, 12.f), FenceCol, 0.8f, true, Yaw);
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
	Paint(FVector(400, 300, FZ - 1.f), FVector(800, 600, 2), Wood, 0.6f);               // living
	Paint(FVector(1150, 300, FZ - 1.f), FVector(700, 600, 2), TileCol, 0.4f);              // kitchen
	Paint(FVector(750, 680, FZ - 1.f), FVector(1500, 160, 2), Wood * 0.95f, 0.6f);      // hall
	Paint(FVector(250, 905, FZ - 1.f), FVector(500, 290, 2), Wood, 0.6f);               // office
	Paint(FVector(750, 905, FZ - 1.f), FVector(500, 290, 2), Wood * 1.05f, 0.6f);       // bedroom
	Paint(FVector(1125, 905, FZ - 1.f), FVector(250, 290, 2), Aqua, 0.3f);              // bath
	Paint(FVector(1375, 905, FZ - 1.f), FVector(250, 290, 2), GarageGrey, 0.8f);        // mudroom
	Paint(FVector(-380, 350, 1.f), FVector(760, 700, 2), GarageGrey * 0.85f, 0.9f);     // garage
	Interiors.Add(FBox(FVector(-760, 0, -50), FVector(0, 700, 400)));                     // garage + house: indoor exposure
	Interiors.Add(FBox(FVector(0, 0, -50), FVector(1500, 1050, 700)));
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
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof")) && !KitReady()) Paint(FVector(-380, 350, FZ + WallH + 22.f), FVector(800, 740, 18), FLinearColor(0.35f, 0.36f, 0.4f), 0.8f);   // garage flat roof
	Paint(FVector(-380, -4, FZ + 255.f), FVector(640, 30, 40), FLinearColor(0.55f, 0.66f, 0.76f), 0.6f);          // rolled-up door box

	// ---- roof + chimney
	if (KitReady())
	{
		// handmade flat roofs (Kenney Building Kit): roof tiles + a border trim all round, house and garage
		auto KitRoof = [&](const FBox2D& R, float Z)
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) return;
			KitTile(TEXT("KB"), TEXT("roof_flat_center"), R, Z, 200.f, 200.f);
			for (int32 Side = 0; Side < 4; ++Side)
			{
				const bool bX = Side < 2;   // edges running along X (y = min / max)
				const float L = bX ? R.GetSize().X : R.GetSize().Y;
				const int32 N = FMath::Max(1, FMath::RoundToInt(L / 200.f));
				for (int32 k = 0; k < N; ++k)
				{
					const float U = (k + 0.5f) * L / N;
					const FVector C = bX ? FVector(R.Min.X + U, Side == 0 ? R.Min.Y : R.Max.Y, Z) : FVector(Side == 2 ? R.Min.X : R.Max.X, R.Min.Y + U, Z);
					KitPiece(TEXT("KB"), TEXT("border"), C, bX ? 90.f : 0.f, FVector(1.f, (L / N) / 200.f, 1.f));
				}
			}
		};
		KitRoof(FBox2D(FVector2D(-10, -10), FVector2D(1510, 1060)), FZ + WallH + 10.f);
		KitRoof(FBox2D(FVector2D(-770, -10), FVector2D(-10, 710)), WallH + 13.f);
	}
	else
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Roof(FVector(750, 525, FZ + WallH + 10.f), 1500, 1050, 30.f, 55.f, nullptr, Coral, nullptr, Cream);
		Paint(FVector(1180, 760, FZ + WallH + 260.f), FVector(70, 70, 330), FLinearColor(0.62f, 0.3f, 0.25f), 0.9f);
	}
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
	K(TEXT("laptop"), FVector(297, DeskY - 17.f, DeskTop), -90.f, KF, false);
	AddInteract(FVector(297, DeskY - 17.f, DeskTop + 15.f), FVector(28, 25, 20), "laptop");
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
	K(TEXT("cardboardBoxClosed"), FVector(-610, 650, 2.f), 15.f);   // clear of the steel shelving (DressGarage)
	K(TEXT("trashcan"), FVector(-45, 650, 2.f), 0.f);
	AddSign(TEXT("WORKBENCH"), BenchTop + FVector(0, 30, KitReady() ? 145.f : 110.f), -90.f, 14.f, FColor(40, 40, 50));   // above the kit wall shelf
	if (KitReady()) { DressHouse(); DressGarage(); }
}

// =====================================================================================================
//  street, neighbours, garden
// =====================================================================================================
void AISHomeWorld::BuildNeighbourhood()
{
	auto C = [this](const TCHAR* Kit, const TCHAR* Name, const FVector& P, float Face, float Scale, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(Kit, Name), P, Face, Scale, bCollide, true); };

	// road along X: one clean asphalt strip with a dashed centre line, kerbed sidewalks, simple street lights
	Paint(FVector(1000, -1800, 0.6f), FVector(19000, 960, 1.2f), FLinearColor(0.2f, 0.2f, 0.21f), 0.9f, false);
	for (int32 i = -9; i <= 10; ++i) Paint(FVector(i * 1000.f + 250.f, -1800.f, 1.3f), FVector(300.f, 12.f, 0.4f), FLinearColor(0.92f, 0.9f, 0.82f), 0.8f, false);
	Paint(FVector(1000, -1240, 4), FVector(19000, 160, 8), FLinearColor(0.82f, 0.81f, 0.79f), 0.9f);
	Paint(FVector(1000, -2360, 4), FVector(19000, 160, 8), FLinearColor(0.82f, 0.81f, 0.79f), 0.9f);
	for (int32 i = -3; i <= 4; ++i)
		for (int32 s = 0; s < 2; ++s)
		{
			const float Lx = i * 2500.f + s * 1250.f, Ly = s == 0 ? -1210.f : -2390.f, Arm = s == 0 ? -90.f : 90.f;
			Paint(FVector(Lx, Ly, 300.f), FVector(10.f, 10.f, 600.f), FLinearColor(0.55f, 0.57f, 0.6f), 0.5f, true, 0.f, 0.6f);
			Paint(FVector(Lx, Ly + Arm * 0.6f, 595.f), FVector(10.f, 110.f, 8.f), FLinearColor(0.55f, 0.57f, 0.6f), 0.5f, false, 0.f, 0.6f);
			Paint(FVector(Lx, Ly + Arm * 1.15f, 588.f), FVector(30.f, 18.f, 8.f), FLinearColor(1.f, 0.95f, 0.8f), 0.3f, false);
		}
	// our front garden: path, driveway, mailbox, flower beds, car
	Paint(FVector(610, -700, 1.f), FVector(140, 1200, 2), FLinearColor(0.85f, 0.83f, 0.8f), 0.9f, false);   // path
	Paint(FVector(-380, -650, 1.f), FVector(340, 1300, 2), FLinearColor(0.7f, 0.69f, 0.67f), 0.9f, false);  // driveway
	if (!PlaceCar(FVector(-380, -520, 2.f), -90.f)) C(TEXT("car"), TEXT("sedan"), FVector(-380, -520, 2.f), -90.f, 1.6f);
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
	PlaceTree(FVector(1250, -700, 0), 30.f);
	PlaceTree(FVector(-1100, 1400, 0), 0.f);

	// neighbours: built the same way as our house (same walls, windows, roof, flat paint), each in its own colours
	struct FN { float X, Y, Face; FLinearColor Wall, Door, RoofC; };
	const FN Hs[] = {
		{ -2700, 500, -90, Mint, FLinearColor(0.2f, 0.3f, 0.45f), FLinearColor(0.35f, 0.42f, 0.5f) },
		{ 3600, 500, -90, SoftBlue, Coral, FLinearColor(0.55f, 0.3f, 0.26f) },
		{ 6900, 500, -90, Peach, Teal, FLinearColor(0.3f, 0.33f, 0.38f) },
		{ -1600, -3700, 90, Lilac, FLinearColor(0.3f, 0.2f, 0.12f), FLinearColor(0.6f, 0.32f, 0.28f) },
		{ 1800, -3700, 90, Cream, FLinearColor(0.1f, 0.25f, 0.4f), Coral },
		{ 5200, -3700, 90, Aqua, FLinearColor(0.5f, 0.35f, 0.15f), FLinearColor(0.35f, 0.36f, 0.4f) },
		{ -5200, -3700, 90, WarmWhite, Teal, FLinearColor(0.5f, 0.3f, 0.25f) } };
	for (const FN& H : Hs)
	{
		HouseShell(FVector(H.X, H.Y, 0), 1100.f, 850.f, H.Face, H.Wall, H.Door, nullptr, H.RoofC);
		const float Dir = H.Y > 0 ? -1.f : 1.f;
		Paint(FVector(H.X + 750.f, H.Y + Dir * 900.f, 1.f), FVector(320, 900, 2), FLinearColor(0.7f, 0.69f, 0.67f), 0.9f, false);   // driveway
		R.RandRange(0, 1);   // keeps the random sequence of the old Kenney pick
		PlaceTree(FVector(H.X - 800.f, H.Y + Dir * 800.f, 0), R.FRandRange(0.f, 360.f));
		WoodFence(FVector(H.X - 600.f, H.Y + Dir * 1150.f, 0), FVector(H.X + 500.f, H.Y + Dir * 1150.f, 0));
	}
}

void AISHomeWorld::BuildBackyard()
{
	auto C = [this](const TCHAR* Kit, const TCHAR* Name, const FVector& P, float Face, float Scale, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(Kit, Name), P, Face, Scale, bCollide, true); };
	// wooden fence around the back garden with a gate toward the field
	const float Y1 = 3400.f, X0 = -900.f, X1 = 2300.f;
	// one continuous fence; the gate opening is lined up exactly with the path to the launch field (x = 800)
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
	PlaceTree(FVector(1900, 2600, 0), 0.f);
	PlaceTree(FVector(-500, 2900, 0), 0.f);
	C(TEXT("nat"), TEXT("plant_bushLarge"), FVector(2150, 1400, 0), 0.f, 3.f);
	C(TEXT("nat"), TEXT("plant_bushLarge"), FVector(-750, 1500, 0), 0.f, 3.f);
	// path from the gate to the launch field
	const FVector Gate(800, Y1, 0);
	const FVector Seg = PadPos - Gate;
	UStaticMeshComponent* Path = Paint((Gate + PadPos) * 0.5f + FVector(0, 0, 0.6f), FVector(Seg.Size2D(), 160, 1.2f), FLinearColor(0.55f, 0.42f, 0.3f), 0.95f, false);
	Path->SetRelativeRotation(FRotator(0, Seg.Rotation().Yaw, 0));

	// rolling hills on the horizon (soft Astroneer-like silhouettes); on the real Earth the real hills are there instead
	FRandomStream R(11);
	for (int32 i = 0; i < (AISEarth::Enabled() ? 0 : 14); ++i)
	{
		const float A = R.FRandRange(0.f, 2.f * PI);
		const float D = R.FRandRange(60000.f, 110000.f);
		const float Rad = R.FRandRange(15000.f, 40000.f);
		const FLinearColor Hc = KitReady() ? FMath::Lerp(FLinearColor(0.24f, 0.46f, 0.08f), FLinearColor(0.3f, 0.52f, 0.1f), R.FRand())   // same matte green as the grass ground
		                                   : FMath::Lerp(FLinearColor(0.32f, 0.58f, 0.25f), FLinearColor(0.5f, 0.68f, 0.32f), R.FRand());
		ISBuild::Prim(this, Root, ISMat::Sphere(), FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, -Rad * 0.75f), FVector(Rad * 2.f, Rad * 2.4f, Rad * 2.f), Hc, nullptr, FRotator(0, R.FRandRange(0.f, 180.f), 0), 0.f, 1.f);
	}

	// instanced nature (trees, bushes, grass, flowers), density follows the graphics preset
	auto Scatter = [&](const TCHAR* Name, int32 N, float MinR, float MaxR, const FVector& Ctr, float MinS, float MaxS, float Cull)
	{
		// a name with a '/' is a full asset path; then MinS/MaxS are heights in cm (scale from the mesh's own height)
		const bool bPath = FString(Name).Contains(TEXT("/"));
		UStaticMesh* Mesh = ISMat::Mesh(bPath ? FString(Name) : KPath(TEXT("nat"), Name));
		if (!Mesh) return;
		const FBox MB = Mesh->GetBoundingBox();
		const float HScale = bPath ? 1.f / FMath::Max(1.f, MB.GetSize().Z) : 1.f;
		const float Base = bPath ? -MB.Min.Z : 0.f;
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
			if (P.X > 12200.f && P.X < 23900.f && P.Y > -1300.f && P.Y < 4400.f) continue;   // space centre grounds
			if (AISEarth::Enabled() && !AISEarth::InSite(P)) continue;   // beyond the site plate the real ground has its own trees
			{
				static const FVector2D Nb[] = { { -2700, 500 }, { 3600, 500 }, { 6900, 500 }, { -1600, -3700 }, { 1800, -3700 }, { 5200, -3700 }, { -5200, -3700 } };
				bool bIn = false;
				for (const FVector2D& Nn : Nb) if (FMath::Abs(P.X - Nn.X) < 750.f && FMath::Abs(P.Y - Nn.Y) < 650.f) { bIn = true; break; }
				if (bIn) continue;
			}
			const float S = R.FRandRange(MinS, MaxS) * HScale;
			H->AddInstance(FTransform(FRotator(0, R.FRandRange(0.f, 360.f), 0), P + FVector(0, 0, Base * S - 4.f), FVector(S)));
			++Added;
		}
		AllComps.Add(H);
	};
	const FVector Mid(800, 6000, 0);
	if (KitReady())   // Quaternius Ultimate Nature (handmade low-poly), heights in cm
	{
		Scatter(TEXT("/Game/Infinisat/QN/CommonTree_1/SM_CommonTree_1.SM_CommonTree_1"), 110, 6000.f, 40000.f, Mid, 650.f, 1000.f, 150000.f);
		Scatter(TEXT("/Game/Infinisat/QN/CommonTree_2/SM_CommonTree_2.SM_CommonTree_2"), 90, 6000.f, 40000.f, Mid, 600.f, 900.f, 150000.f);
		Scatter(TEXT("/Game/Infinisat/QN/CommonTree_4/SM_CommonTree_4.SM_CommonTree_4"), 80, 6000.f, 40000.f, Mid, 700.f, 1000.f, 150000.f);
		Scatter(TEXT("/Game/Infinisat/QN/PineTree_1/SM_PineTree_1.SM_PineTree_1"), 110, 9000.f, 45000.f, Mid, 800.f, 1300.f, 150000.f);
		Scatter(TEXT("/Game/Infinisat/QN/PineTree_3/SM_PineTree_3.SM_PineTree_3"), 70, 9000.f, 45000.f, Mid, 800.f, 1200.f, 150000.f);
		Scatter(TEXT("/Game/Infinisat/QN/BirchTree_2/SM_BirchTree_2.SM_BirchTree_2"), 60, 7000.f, 40000.f, Mid, 700.f, 1000.f, 150000.f);
	}
	else
	{
		Scatter(TEXT("tree_default"), 160, 6000.f, 40000.f, Mid, 3.5f, 6.f, 150000.f);
		Scatter(TEXT("tree_oak"), 120, 6000.f, 40000.f, Mid, 3.5f, 5.5f, 150000.f);
		Scatter(TEXT("tree_pineRoundB"), 120, 9000.f, 45000.f, Mid, 4.f, 7.f, 150000.f);
		Scatter(TEXT("tree_fat"), 80, 7000.f, 40000.f, Mid, 3.5f, 5.5f, 150000.f);
	}
	if (KitReady())   // faceted rocks + bushes/ferns (KayKit, Quaternius); the grass itself is the GrassCarpet
	{
		Scatter(TEXT("/Game/Infinisat/KF/Rock_1_A_Color1/SM_Rock_1_A_Color1.SM_Rock_1_A_Color1"), 50, 2500.f, 35000.f, Mid, 60.f, 160.f, 60000.f);
		Scatter(TEXT("/Game/Infinisat/KF/Rock_1_D_Color1/SM_Rock_1_D_Color1.SM_Rock_1_D_Color1"), 40, 2500.f, 35000.f, Mid, 80.f, 220.f, 60000.f);
		Scatter(TEXT("/Game/Infinisat/KF/Rock_1_H_Color1/SM_Rock_1_H_Color1.SM_Rock_1_H_Color1"), 30, 2500.f, 35000.f, Mid, 50.f, 120.f, 60000.f);
		Scatter(TEXT("/Game/Infinisat/QN/Bush_1/SM_Bush_1.SM_Bush_1"), 120, 2500.f, 30000.f, Mid, 70.f, 140.f, 40000.f);
		Scatter(TEXT("/Game/Infinisat/QN/Bush_2/SM_Bush_2.SM_Bush_2"), 120, 2500.f, 30000.f, Mid, 60.f, 120.f, 40000.f);
		Scatter(TEXT("/Game/Infinisat/QN/Plant_2/SM_Plant_2.SM_Plant_2"), 120, 2500.f, 20000.f, Mid, 40.f, 90.f, 20000.f);
		Scatter(TEXT("/Game/Infinisat/QN/Flowers/SM_Flowers.SM_Flowers"), 160, 2500.f, 16000.f, Mid, 25.f, 45.f, 9000.f);
	}
	else
	{
		Scatter(TEXT("plant_bush"), 260, 2500.f, 30000.f, Mid, 2.5f, 4.5f, 40000.f);
		Scatter(TEXT("rock_largeA"), 40, 6000.f, 35000.f, Mid, 1.5f, 3.5f, 60000.f);
		Scatter(TEXT("grass_large"), 2800, 2500.f, 16000.f, Mid, 1.4f, 2.2f, 9000.f);
		Scatter(TEXT("grass"), 2800, 2500.f, 16000.f, Mid, 1.4f, 2.2f, 9000.f);
		Scatter(TEXT("flower_yellowA"), 500, 2500.f, 14000.f, Mid, 1.2f, 1.7f, 7000.f);
		Scatter(TEXT("flower_purpleB"), 500, 2500.f, 14000.f, Mid, 1.2f, 1.7f, 7000.f);
	}
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
	if (AISEarth::Enabled())
	{
		// on the real Earth: only the site itself is our ground, a levelled plate on the real coast at Mahia (a deep
		// block, so wherever the real ground dips at its edge there is an embankment, not a hole); beyond it the
		// real terrain and imagery take over
		Paint(FVector(7500, 750, -2000), FVector(35000, 13500, 4000), FLinearColor(0.35f, 0.55f, 0.215f), 0.95f);   // the colour of the real pasture around it
		Paint(FVector(700, 300, 0.4f), FVector(4200, 6600, 0.6f), FLinearColor(0.42f, 0.7f, 0.28f), 0.95f, false);
		return;
	}
	UStaticMeshComponent* G = NewObject<UStaticMeshComponent>(this);
	G->SetStaticMesh(ISMat::Cube());
	G->SetupAttachment(Root);
	G->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector(800, 5000, -50), FVector(3000, 3000, 1)));
	G->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	G->SetCollisionResponseToAllChannels(ECR_Block);
	G->RegisterComponent();
	if (KitReady())
	{
		// matte root-green under the stylised grass carpet: no shine, and it reads as grass where the blades fade out
		UMaterialInstanceDynamic* GM = UMaterialInstanceDynamic::Create(ISMat::Color(), G);
		GM->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.46f, 0.08f));
		GM->SetScalarParameterValue(TEXT("Roughness"), 1.f);
		GM->SetScalarParameterValue(TEXT("Metallic"), 0.f);
		G->SetMaterial(0, GM);
	}
	else if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Ground.M_IS_Ground"))) G->SetMaterial(0, M);
	AllComps.Add(G);
	if (!KitReady()) Paint(FVector(700, 300, 0.4f), FVector(4200, 6600, 0.6f), FLinearColor(0.42f, 0.7f, 0.28f), 0.95f, false);   // mowed lawn around the house
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
	ValidateDress();
}

void AISHomeWorld::ApplyLightQuality()
{
	for (int32 i = 0; i < RoomLights.Num(); ++i)
		if (RoomLights[i]) RoomLights[i]->SetCastShadows(ISQuality::Level >= 3 && i < 2);
	ApplyLook();   // Lumen in the demo looks follows the graphics preset too
}

// =====================================================================================================
//  the space centre at the end of the street (Kerbal Space Center idea: one place, a building per job)
//  Same flat style as the houses. Each building has a console: walk in and press E.
void AISHomeWorld::BuildSpaceCentre()
{
	auto F = [this](const TCHAR* Name, const FVector& P, float Face) { return PlaceMesh(this, Root, AllComps, KPath(TEXT("furn"), Name), P, Face, KF, true, true); };
	const FLinearColor WallW(0.9f, 0.91f, 0.92f), Blue(0.18f, 0.3f, 0.48f), Orange(1.f, 0.56f, 0.12f), Glass(0.25f, 0.4f, 0.55f), FloorC(0.78f, 0.79f, 0.8f), Concrete(0.68f, 0.68f, 0.66f);
	// the street carries on east to the centre's gate; concrete grounds; fence with a gate on the road
	Paint(FVector(17500, -1800, 0.6f), FVector(14000, 960, 1.2f), FLinearColor(0.2f, 0.2f, 0.21f), 0.9f, false);
	for (int32 i = 11; i <= 24; ++i) Paint(FVector(i * 1000.f + 250.f, -1800.f, 1.3f), FVector(300.f, 12.f, 0.4f), FLinearColor(0.92f, 0.9f, 0.82f), 0.8f, false);
	Paint(FVector(18000, 1500, 2.f), FVector(11000, 5200, 4.f), Concrete, 0.9f, false);
	WoodFence(FVector(12400, -1150, 0), FVector(23600, -1150, 0), 600.f, 1400.f);
	AddSign(TEXT("INFINISAT SPACE CENTRE"), FVector(13400, -1185, 245), -90.f, 70.f, FColor(255, 145, 30));
	Paint(FVector(13400, -1170, 245), FVector(1400, 12, 90), Blue, 0.5f);
	Paint(FVector(12820, -1170, 100), FVector(14, 14, 200), Blue, 0.5f);
	Paint(FVector(13980, -1170, 100), FVector(14, 14, 200), Blue, 0.5f);

	// one building: floor, walls (door on the road side), window bands, flat roof, orange band, sign, lights
	auto Bldg = [&](const FVector2D& C, float SX, float SY, float H, float DoorW, float DoorH, const TCHAR* Name)
	{
		const float T = 30.f, X0 = C.X - SX * 0.5f, X1 = C.X + SX * 0.5f, Y0 = C.Y - SY * 0.5f, Y1 = C.Y + SY * 0.5f;
		Interiors.Add(FBox(FVector(X0, Y0, -50.f), FVector(X1, Y1, H)));
		Paint(FVector(C.X, C.Y, 6.f), FVector(SX, SY, 12.f), FloorC, 0.8f);
		Paint(FVector(C.X, Y1, H * 0.5f), FVector(SX, T, H), WallW, 0.7f);
		Paint(FVector(X0, C.Y, H * 0.5f), FVector(T, SY, H), WallW, 0.7f);
		Paint(FVector(X1, C.Y, H * 0.5f), FVector(T, SY, H), WallW, 0.7f);
		const float Side = (SX - DoorW) * 0.5f;
		Paint(FVector(X0 + Side * 0.5f, Y0, H * 0.5f), FVector(Side, T, H), WallW, 0.7f);
		Paint(FVector(X1 - Side * 0.5f, Y0, H * 0.5f), FVector(Side, T, H), WallW, 0.7f);
		Paint(FVector(C.X, Y0, DoorH + (H - DoorH) * 0.5f), FVector(DoorW, T, H - DoorH), WallW, 0.7f);
		for (float S : { -1.f, 1.f })   // window bands on the long sides
		{
			Paint(FVector(S < 0 ? X0 - 16.f : X1 + 16.f, C.Y, H * 0.58f), FVector(4.f, SY * 0.8f, H * 0.22f), Glass, 0.15f, false, 0.f, 0.3f);
			Paint(FVector(C.X + S * (Side * 0.5f + DoorW * 0.5f), Y0 - 16.f, H * 0.58f), FVector(Side * 0.6f, 4.f, H * 0.22f), Glass, 0.15f, false, 0.f, 0.3f);
		}
		Paint(FVector(C.X, C.Y, H + 15.f), FVector(SX + 60.f, SY + 60.f, 30.f), Blue, 0.7f);
		Paint(FVector(C.X, Y0 - 32.f, H - 20.f), FVector(SX + 60.f, 4.f, 26.f), Orange, 0.5f, false);
		AddSign(Name, FVector(C.X, Y0 - 40.f, H - 90.f), -90.f, FMath::Clamp(SX / 18.f, 40.f, 120.f), FColor(30, 50, 80));
		for (float Lx = X0 + 300.f; Lx < X1 - 100.f; Lx += 500.f)
			for (float Ly = Y0 + 300.f; Ly < Y1 - 100.f; Ly += 500.f)
				RoomLight(FVector(Lx, Ly, H - 60.f), 2500.f, false);
	};
	auto Console = [&](const FVector& P, float Face, FName Action, const TCHAR* Label)
	{
		F(TEXT("desk"), P, Face);
		F(TEXT("computerScreen"), P + FVector(0, 0, 78.f), Face + 180.f);   // on the desk top, facing the chair
		F(TEXT("chairDesk"), P + FRotator(0, Face, 0).RotateVector(FVector(-90.f, 0, 0)), Face);
		AddSign(Label, P + FVector(0, 0, 175.f), Face - 90.f, 18.f, FColor(255, 145, 30));
		AddInteract(P + FVector(0, 0, 120.f), FVector(80, 80, 80), Action);
	};

	// MISSION CONTROL: the customers' orders and the big screens
	Bldg(FVector2D(14200, 900), 1500, 1100, 520, 260, 300, TEXT("MISSION CONTROL"));
	Console(FVector(14200, 1200, 12), 270.f, "sc_orders", TEXT("ORDERS"));
	for (int32 k = -1; k <= 1; k += 2) { F(TEXT("desk"), FVector(14200 + k * 420.f, 900, 12), 270.f); F(TEXT("computerScreen"), FVector(14200 + k * 420.f, 900, 172), 270.f); F(TEXT("chairDesk"), FVector(14200 + k * 420.f, 810, 12), 270.f); }
	Paint(FVector(14200, 1425, 330), FVector(1100, 10, 260), FLinearColor(0.02f, 0.05f, 0.1f), 0.3f, false);   // wall of screens
	Paint(FVector(14200, 1418, 330), FVector(500, 6, 200), FLinearColor(0.1f, 0.35f, 0.8f) * 1.4f, 0.2f, false);

	// LAB: test sensors and radios, read about every part (the part library)
	Bldg(FVector2D(16300, 900), 1500, 1100, 520, 260, 300, TEXT("LAB"));
	Console(FVector(16300, 1200, 12), 270.f, "sc_lab", TEXT("PART LIBRARY"));
	for (int32 k = 0; k < 3; ++k) { F(TEXT("desk"), FVector(15800 + k * 500.f, 700, 12), 0.f); F(TEXT("bookcaseOpen"), FVector(15750 + k * 500.f, 1380, 12), 180.f); }
	F(TEXT("pottedPlant"), FVector(15650, 520, 12), 0.f);

	// ASSEMBLY HALL: the clean room where satellites are designed and built
	Bldg(FVector2D(19300, 1600), 2800, 2400, 1500, 900, 1100, TEXT("ASSEMBLY HALL"));
	Paint(FVector(19300, 1600, 14), FVector(1800, 1600, 4), FLinearColor(0.93f, 0.94f, 0.95f), 0.4f, false);   // clean-room floor
	Paint(FVector(19300, 1600, 60), FVector(260, 260, 100), FLinearColor(0.3f, 0.32f, 0.35f), 0.5f);            // build stand
	Paint(FVector(19300, 1600, 170), FVector(140, 140, 120), FLinearColor(0.85f, 0.7f, 0.25f), 0.3f, true, 0.f, 0.8f);   // satellite under construction (gold MLI)
	Paint(FVector(19300, 1600, 170), FVector(400, 8, 100), FLinearColor(0.05f, 0.08f, 0.2f), 0.2f, false);       // folded solar wings
	for (int32 k = 0; k < 4; ++k) Paint(FVector(18500 + k * 530.f, 2700, 500), FVector(14, 14, 1000), FLinearColor(0.85f, 0.7f, 0.15f), 0.5f);   // crane rail posts
	Paint(FVector(19300, 2700, 1000), FVector(1700, 30, 30), FLinearColor(0.85f, 0.7f, 0.15f), 0.5f);
	Console(FVector(18300, 1100, 12), 0.f, "sc_design", TEXT("DESIGN A SATELLITE"));

	// TRACKING STATION: the dish and the map of space
	Bldg(FVector2D(22500, 700), 1000, 900, 450, 240, 300, TEXT("TRACKING"));
	Console(FVector(22500, 950, 12), 270.f, "sc_track", TEXT("MAP OF SPACE"));
	Paint(FVector(22500, 2200, 400), FVector(120, 120, 800), Concrete, 0.8f);                                       // dish pedestal
	if (UStaticMeshComponent* D = Paint(FVector(22500, 2200, 950), FVector(1100, 1100, 120), FLinearColor(0.92f, 0.93f, 0.94f), 0.5f))
	{
		D->SetStaticMesh(ISMat::Cyl());
		D->SetRelativeRotation(FRotator(-35.f, 90.f, 0.f));
	}
	Paint(FVector(22500, 2000, 1250), FVector(20, 20, 500), FLinearColor(0.6f, 0.6f, 0.62f), 0.5f, false);       // feed

	// parked cars and trees make it feel lived in
	for (float X : { 15200.f, 15700.f })
		if (!PlaceCar(FVector(X, -500, 2), 0.f)) PlaceMesh(this, Root, AllComps, KPath(TEXT("car"), TEXT("sedan")), FVector(X, -500, 2), 0.f, 1.6f, true, true);
	for (int32 k = 0; k < 6; ++k) PlaceTree(FVector(12800 + k * 1900.f, -900, 0), k * 50.f);
}
