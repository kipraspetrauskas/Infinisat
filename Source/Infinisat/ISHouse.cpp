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
	const float KYaw = 90.f;    // Kenney models face -Y; +90 makes yaw 0 face +X

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
	L->SetCastShadows(ISQuality::Level >= 1);
	L->RegisterComponent();
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
		Paint(P((U1 + U2) * 0.5f, 0, FZ + (Z1 + Z2) * 0.5f), S(U2 - U1, WallT, Z2 - Z1), CoreCol, 0.85f);
		if (OutSide != 0.f)   // interior paint layer on the inside face
			Paint(P((U1 + U2) * 0.5f, -OutSide * (WallT * 0.5f + 0.6f), FZ + (Z1 + Z2) * 0.5f), S(U2 - U1, 1.f, Z2 - Z1), In, 0.9f, false);
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

void AISHomeWorld::HouseShell(const FVector& Origin, float, float, float Yaw, const FLinearColor&, const FLinearColor&, const TCHAR* Type, const FLinearColor&)
{
	PlaceMesh(this, Root, AllComps, KPath(TEXT("sub"), Type), Origin, Yaw, KCity, true, true);
}

// =====================================================================================================
//  the home
// =====================================================================================================
void AISHomeWorld::BuildHome()
{
	auto K = [this](const TCHAR* Name, const FVector& C, float Face, float Scale = KF, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(TEXT("furn"), Name), C, Face, Scale, bCollide, true); };
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
	// front steps
	Paint(FVector(610, -45, 5.f), FVector(180, 70, 10), FLinearColor(0.8f, 0.78f, 0.74f));
	Paint(FVector(610, -20, FZ - 5.f), FVector(200, 40, FZ + 10.f), FLinearColor(0.8f, 0.78f, 0.74f));
	// ceilings
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Paint(FVector(750, 525, FZ + WallH + 5.f), FVector(1500, 1050, 10), FLinearColor(0.97f, 0.96f, 0.94f), 0.95f);
	if (!FParse::Param(FCommandLine::Get(), TEXT("ISNoRoof"))) Paint(FVector(-380, 350, WallH + 8.f), FVector(760, 700, 10), FLinearColor(0.9f, 0.9f, 0.9f), 0.95f);

	// ---- walls (openings: From, To, Bottom, Top, Kind 0=door 1=window 2=arch 3=exterior door)
	Wall({ 0, 0 }, { 1500, 0 }, { { 120, 420, 85, 220, 1 }, { 560, 660, 0, 212, 3 }, { 980, 1300, 95, 220, 1 } }, nullptr, WarmWhite, nullptr, Cream, -1.f);
	Wall({ 0, 1050 }, { 1500, 1050 }, { { 130, 380, 85, 220, 1 }, { 620, 880, 85, 220, 1 }, { 1080, 1180, 150, 220, 1 }, { 1330, 1430, 0, 212, 3 } }, nullptr, SoftBlue, nullptr, Cream, 1.f);
	Wall({ 0, 0 }, { 0, 1050 }, { { 440, 530, 0, 212, 0 }, { 830, 1000, 85, 220, 1 } }, nullptr, WarmWhite, nullptr, Cream, -1.f);
	Wall({ 1500, 0 }, { 1500, 1050 }, { { 160, 440, 95, 220, 1 } }, nullptr, Mint, nullptr, Cream, 1.f);
	Wall({ 0, 600 }, { 1500, 600 }, { { 640, 780, 0, 225, 2 }, { 900, 1150, 0, 225, 2 } }, nullptr, Peach, nullptr, Peach, 0.f);
	Wall({ 0, 760 }, { 1500, 760 }, { { 200, 290, 0, 212, 0 }, { 600, 690, 0, 212, 0 }, { 1040, 1120, 0, 212, 0 }, { 1290, 1460, 0, 225, 2 } }, nullptr, Peach, nullptr, Peach, 0.f);
	Wall({ 800, 0 }, { 800, 600 }, { { 150, 450, 0, 225, 2 } }, nullptr, Mint, nullptr, Mint, 0.f);
	Wall({ 500, 760 }, { 500, 1050 }, {}, nullptr, Lilac, nullptr, Lilac, 0.f);
	Wall({ 1000, 760 }, { 1000, 1050 }, {}, nullptr, Aqua, nullptr, Aqua, 0.f);
	Wall({ 1250, 760 }, { 1250, 1050 }, {}, nullptr, GarageGrey, nullptr, GarageGrey, 0.f);
	// bathroom door: locked
	if (Doors.Num() >= 3)
	{
		for (FISDoor& D : Doors)
			if (D.Hinge && FVector::Dist2D(D.Hinge->GetRelativeLocation(), FVector(1040, 760, 0)) < 5.f)
			{
				D.bLocked = true; D.Target = D.Angle = 0.f;
				D.Hinge->SetRelativeRotation(FRotator(0, 0, 0));
				D.LockMsg = TEXT("Occupied! (BIT: robots don't need the bathroom anyway.)");
			}
	}
	// garage walls (front opening is the big garage door)
	Wall({ -760, 0 }, { 0, 0 }, { { 80, 680, 0, 235, 2 } }, nullptr, GarageGrey, nullptr, FLinearColor(0.62f, 0.75f, 0.86f), -1.f, false);
	Wall({ -760, 700 }, { 0, 700 }, { { 260, 460, 140, 220, 1 } }, nullptr, GarageGrey, nullptr, FLinearColor(0.62f, 0.75f, 0.86f), 1.f, false);
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

	// ---- living room
	K(TEXT("rugRectangle"), FVector(300, 330, FZ), 90.f, KF * 1.3f, false);
	K(TEXT("cabinetTelevision"), FVector(300, 570, FZ), -90.f);
	UStaticMeshComponent* TVCab = nullptr;
	TVCab = K(TEXT("televisionModern"), FVector(300, 572, FZ + 72.f), -90.f, KF, false);
	K(TEXT("loungeSofa"), FVector(300, 150, FZ), 90.f);
	K(TEXT("tableCoffee"), FVector(300, 350, FZ), 90.f);
	K(TEXT("loungeChair"), FVector(620, 300, FZ), 180.f);
	K(TEXT("sideTable"), FVector(560, 130, FZ), 90.f);
	K(TEXT("lampRoundTable"), FVector(560, 130, FZ + 58.f), 90.f, KF, false);
	UStaticMeshComponent* Shelf = K(TEXT("bookcaseOpen"), FVector(30, 300, FZ), 0.f);
	K(TEXT("books"), FVector(30, 290, TopOf(Shelf, FZ + 180.f)), 0.f, KF, false);
	K(TEXT("pottedPlant"), FVector(740, 60, FZ), 0.f);
	K(TEXT("pottedPlant"), FVector(40, 560, FZ), 0.f);
	K(TEXT("speaker"), FVector(120, 575, FZ), -90.f);
	K(TEXT("speaker"), FVector(480, 575, FZ), -90.f);
	K(TEXT("lampRoundFloor"), FVector(60, 80, FZ), 0.f);

	// ---- kitchen: counters along the hall wall
	const TCHAR* Run[] = { TEXT("kitchenCabinetDrawer"), TEXT("kitchenCabinet"), TEXT("kitchenStoveElectric"), TEXT("kitchenCabinet"), TEXT("kitchenSink"), TEXT("kitchenCabinetDrawer") };
	float CounterTop = FZ + 95.f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Run); ++i)
	{
		UStaticMeshComponent* C = K(Run[i], FVector(880.f + i * 90.f, 553.f, FZ), -90.f);
		if (i == 1 && C) CounterTop = TopOf(C, CounterTop);
		K(TEXT("kitchenCabinetUpper"), FVector(880.f + i * 90.f, 575.f, FZ + 155.f), -90.f, KF, false);
	}
	K(TEXT("kitchenFridgeLarge"), FVector(1440, 540, FZ), -90.f);
	K(TEXT("kitchenMicrowave"), FVector(890, 560, CounterTop), -90.f, KF, false);
	K(TEXT("kitchenCoffeeMachine"), FVector(1330, 560, CounterTop), -90.f, KF, false);
	K(TEXT("toaster"), FVector(1060, 560, CounterTop), -90.f, KF, false);
	K(TEXT("table"), FVector(1160, 250, FZ), 0.f);
	K(TEXT("chair"), FVector(1060, 250, FZ), 0.f);
	K(TEXT("chair"), FVector(1260, 250, FZ), 180.f);
	K(TEXT("chair"), FVector(1160, 150, FZ), 90.f);
	K(TEXT("chair"), FVector(1160, 350, FZ), -90.f);
	K(TEXT("pottedPlant"), FVector(1470, 40, FZ), 0.f);
	K(TEXT("trashcan"), FVector(830, 470, FZ), 0.f);

	// ---- office (BIT's room): desk + order laptop, charging dock
	UStaticMeshComponent* Desk = K(TEXT("desk"), FVector(250, 1005, FZ), -90.f);
	const float DeskTop = TopOf(Desk, FZ + 80.f);
	K(TEXT("chairDesk"), FVector(250, 920, FZ), 90.f);
	K(TEXT("computerScreen"), FVector(200, 1025, DeskTop), -90.f, KF, false);
	K(TEXT("computerKeyboard"), FVector(200, 990, DeskTop), -90.f, KF, false);
	UStaticMeshComponent* Lap = K(TEXT("laptop"), FVector(320, 1000, DeskTop), -90.f, KF, false);
	AddInteract(FVector(320, 1000, DeskTop + 15.f), FVector(28, 25, 20), "laptop");
	K(TEXT("bookcaseClosedWide"), FVector(30, 900, FZ), 0.f);
	K(TEXT("lampSquareFloor"), FVector(460, 1010, FZ), 0.f);
	K(TEXT("plantSmall2"), FVector(120, 1025, DeskTop), 0.f, KF, false);
	// BIT's charging pad
	Paint(FVector(400, 820, FZ + 2.f), FVector(90, 90, 4), FLinearColor(0.2f, 0.22f, 0.26f), 0.4f, false);
	ISBuild::Unlit(this, Root, ISMat::Cyl(), FVector(400, 820, FZ + 4.5f), FVector(70, 70, 1), FLinearColor(0.1f, 0.85f, 1.f), 2.5f, nullptr);
	(void)Lap;

	// ---- bedroom
	K(TEXT("rugRound"), FVector(750, 850, FZ), 0.f, KF, false);
	K(TEXT("bedDouble"), FVector(750, 925, FZ), -90.f);
	K(TEXT("cabinetBedDrawerTable"), FVector(610, 1020, FZ), -90.f);
	K(TEXT("cabinetBedDrawerTable"), FVector(890, 1020, FZ), -90.f);
	K(TEXT("lampRoundTable"), FVector(610, 1020, FZ + 50.f), -90.f, KF, false);
	UStaticMeshComponent* Wardrobe = K(TEXT("bookcaseClosedDoors"), FVector(525, 800, FZ), 0.f);
	(void)Wardrobe;

	// ---- bathroom (seen through the window)
	K(TEXT("bathtub"), FVector(1125, 1000, FZ), -90.f);
	K(TEXT("toilet"), FVector(1220, 800, FZ), 180.f);
	K(TEXT("bathroomSink"), FVector(1030, 880, FZ), 0.f);

	// ---- mudroom
	K(TEXT("washerDryerStacked"), FVector(1465, 1000, FZ), -90.f);
	UStaticMeshComponent* Mud = K(TEXT("bookcaseOpenLow"), FVector(1300, 1015, FZ), -90.f);
	MudShelfTop = TopOf(Mud, FZ + 90.f);
	K(TEXT("coatRackStanding"), FVector(1470, 790, FZ), 180.f);
	K(TEXT("cardboardBoxClosed"), FVector(1300, 800, FZ), 30.f);

	// ---- garage: workbench, shelves, ground-station radio
	UStaticMeshComponent* Bench = K(TEXT("desk"), FVector(-380, 655, 2.f), -90.f, KF * 1.2f);
	if (Bench)
	{
		const FBox B = Bench->Bounds.GetBox();
		BenchTop = FVector(B.GetCenter().X, B.GetCenter().Y, B.Max.Z);
	}
	else BenchTop = FVector(-380, 655, 90);
	AddInteract(BenchTop + FVector(0, 0, 10), FVector(70, 30, 20), "bench");
	K(TEXT("radio"), BenchTop + FVector(70, 15, 0), -90.f, KF, false);
	AddInteract(BenchTop + FVector(70, 15, 15), FVector(25, 15, 18), "radio");
	K(TEXT("lampSquareTable"), BenchTop + FVector(-70, 20, 0), -90.f, KF, false);
	UStaticMeshComponent* GShelf = K(TEXT("bookcaseOpen"), FVector(-735, 300, 2.f), 0.f);
	GarageShelfTop = TopOf(GShelf, 180.f) - 40.f;
	K(TEXT("cardboardBoxOpen"), FVector(-735, 180, 2.f), 0.f);
	K(TEXT("cardboardBoxClosed"), FVector(-700, 560, 2.f), 15.f);
	K(TEXT("trashcan"), FVector(-60, 640, 2.f), 0.f);
	AddSign(TEXT("WORKBENCH"), BenchTop + FVector(0, 30, 110), -90.f, 14.f, FColor(40, 40, 50));
}

// =====================================================================================================
//  street, neighbours, garden
// =====================================================================================================
void AISHomeWorld::BuildNeighbourhood()
{
	auto C = [this](const TCHAR* Kit, const TCHAR* Name, const FVector& P, float Face, float Scale, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(Kit, Name), P, Face, Scale, bCollide, true); };

	// road along X with sidewalks
	for (int32 i = -8; i <= 10; ++i)
		C(TEXT("road"), TEXT("road-straight"), FVector(i * 1000.f, -1800.f, 0.5f), 0.f, KCity, false);
	Paint(FVector(1000, -1240, 4), FVector(19000, 160, 8), FLinearColor(0.82f, 0.81f, 0.79f), 0.9f);
	Paint(FVector(1000, -2360, 4), FVector(19000, 160, 8), FLinearColor(0.82f, 0.81f, 0.79f), 0.9f);
	for (int32 i = -3; i <= 4; ++i)
	{
		C(TEXT("road"), TEXT("light-square"), FVector(i * 2500.f, -1190.f, 8.f), -90.f, KCity * 0.8f);
		C(TEXT("road"), TEXT("light-square"), FVector(i * 2500.f + 1250.f, -2410.f, 8.f), 90.f, KCity * 0.8f);
	}
	// our front garden: path, driveway, mailbox, flower beds, car
	for (int32 i = 0; i < 3; ++i) C(TEXT("sub"), TEXT("path-long"), FVector(610, -300.f - i * 400.f, 1.f), 0.f, KCity, false);
	for (int32 i = 0; i < 3; ++i) C(TEXT("sub"), TEXT("driveway-long"), FVector(-380, -250.f - i * 400.f, 1.f), 0.f, KCity, false);
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

	// neighbours (Kenney suburban houses) with their own driveways and trees
	struct FN { const TCHAR* Type; float X, Y, Face; };
	const FN Hs[] = { { TEXT("building-type-c"), -2700, 500, -90 }, { TEXT("building-type-k"), 3600, 500, -90 }, { TEXT("building-type-f"), 6900, 500, -90 },
		{ TEXT("building-type-h"), -1600, -3700, 90 }, { TEXT("building-type-a"), 1800, -3700, 90 }, { TEXT("building-type-m"), 5200, -3700, 90 }, { TEXT("building-type-p"), -5200, -3700, 90 } };
	for (const FN& H : Hs)
	{
		C(TEXT("sub"), H.Type, FVector(H.X, H.Y, 0), H.Face, KCity);
		const float Dir = H.Y > 0 ? -1.f : 1.f;
		C(TEXT("sub"), TEXT("driveway-long"), FVector(H.X + 450.f, H.Y + Dir * 900.f, 1.f), 0.f, KCity, false);
		C(TEXT("nat"), R.RandRange(0, 1) ? TEXT("tree_default") : TEXT("tree_fat"), FVector(H.X - 600.f, H.Y + Dir * 700.f, 0), R.FRandRange(0.f, 360.f), KNat);
		C(TEXT("sub"), TEXT("fence-1x3"), FVector(H.X - 300.f, H.Y + Dir * 1150.f, 0), 0.f, KCity * 0.7f, false);
	}
}

void AISHomeWorld::BuildBackyard()
{
	auto C = [this](const TCHAR* Kit, const TCHAR* Name, const FVector& P, float Face, float Scale, bool bCollide = true)
	{ return PlaceMesh(this, Root, AllComps, KPath(Kit, Name), P, Face, Scale, bCollide, true); };
	// wooden fence around the back garden with a gate toward the field
	const float Y1 = 3400.f, X0 = -900.f, X1 = 2300.f;
	for (float X = X0; X < X1 - 10.f; X += 300.f)
	{
		if (X > 600.f && X < 1000.f) { if (X < 700.f) C(TEXT("nat"), TEXT("fence_gate"), FVector(X + 150.f, Y1, 0), 0.f, 3.f, false); continue; }
		C(TEXT("nat"), TEXT("fence_simple"), FVector(X + 150.f, Y1, 0), 0.f, 3.f);
	}
	for (float Y = 1100.f; Y < Y1 - 10.f; Y += 300.f)
	{
		C(TEXT("nat"), TEXT("fence_simple"), FVector(X0, Y + 150.f, 0), 90.f, 3.f);
		C(TEXT("nat"), TEXT("fence_simple"), FVector(X1, Y + 150.f, 0), 90.f, 3.f);
	}
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
		UStaticMesh* Mesh = ISMat::Mesh(KPath(TEXT("nat"), Name));
		if (!Mesh) return;
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
	auto Add = [this](FName Part, const TCHAR* Name, const TCHAR* Where, const FVector& Pos, bool bReq, const TCHAR* Cad, float CadScale)
	{
		FISHuntItem I;
		I.Part = Part; I.Name = Name; I.Where = Where; I.Pos = Pos; I.bRequired = bReq;
		USceneComponent* S = NewObject<USceneComponent>(this);
		S->SetupAttachment(Root);
		S->SetRelativeLocation(Pos);
		S->RegisterComponent();
		// shown 3x real size so parts are visible in a room; tiny glow so they read as collectables
		USceneComponent* Scaled = NewObject<USceneComponent>(this);
		Scaled->SetupAttachment(S);
		Scaled->SetRelativeScale3D(FVector(3.f));
		Scaled->RegisterComponent();
		const FString CadP = Cad ? CadPath(Cad) : FString();
		if (Cad && ISMat::Mesh(CadP)) ISBuild::Asset(this, Scaled, CadP, FVector::ZeroVector, CadScale, &I.Model);
		else ISBuild::CanSatPart(this, Scaled, Part, FVector::ZeroVector, &I.Model);
		if (Part == "bus_can") ISBuild::CanShell(this, Scaled, FVector::ZeroVector, false, &I.Model);
		ISBuild::Unlit(this, S, ISMat::Cyl(), FVector(0, 0, 0.5f), FVector(26, 26, 0.4f), FLinearColor(0.2f, 0.9f, 1.f), 1.6f, &I.Model);
		UBoxComponent* B = NewObject<UBoxComponent>(this);
		B->SetupAttachment(S);
		B->SetBoxExtent(FVector(18, 18, 14));
		B->SetRelativeLocation(FVector(0, 0, 10));
		B->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		B->SetCollisionResponseToAllChannels(ECR_Ignore);
		B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		B->RegisterComponent();
		I.Hit = B;
		Actions.Add(B, FName(*(TEXT("item:") + Part.ToString())));
		Hunt.Add(MoveTemp(I));
	};
	const float Counter = FZ + 95.f;
	Add("bus_can", TEXT("Empty soda can"), TEXT("Kitchen counter"), FVector(1150, 555, Counter), true, nullptr, 1.f);
	Add("cpu_ard", TEXT("ESP32 microcontroller"), TEXT("Office desk"), FVector(150, 985, FZ + 80.f), true, TEXT("esp32_wroom32"), 1.f);
	Add("com_433", TEXT("LoRa radio module"), TEXT("Garage shelf"), FVector(-725, 330, GarageShelfTop), true, TEXT("radio_zeta433"), 1.f);
	Add("pl_temp", TEXT("DHT11 temperature sensor"), TEXT("Living-room bookcase"), FVector(40, 360, FZ + 95.f), true, TEXT("dht11"), 1.f);
	Add("bat_aa", TEXT("18650 battery + holder"), TEXT("Mudroom shelf (old flashlight)"), FVector(1300, 1010, MudShelfTop), true, TEXT("battery_18650"), 1.f);
	Add("ex_chute_l", TEXT("Parachute (ripstop nylon)"), TEXT("Bedroom wardrobe"), FVector(560, 820, FZ + 2.f), true, nullptr, 1.f);
	Add("ex_wires", TEXT("Jumper wires + pin headers"), TEXT("Garage workbench"), BenchTop + FVector(-30, 10, 0), true, TEXT("header_1x15"), 1.f);
	Add("ex_buzzer", TEXT("Buzzer (recovery beeper)"), TEXT("Living-room TV cabinet"), FVector(420, 560, FZ + 72.f), false, TEXT("buzzer"), 1.f);
	Add("pl_gnss", TEXT("GNSS module"), TEXT("Car in the driveway"), FVector(-380, -520, 95.f), false, nullptr, 1.f);
	Add("pl_cam_mini", TEXT("Camera module"), TEXT("Kitchen table"), FVector(1160, 250, FZ + 80.f), false, nullptr, 1.f);
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
