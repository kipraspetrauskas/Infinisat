// Infinisat - detailed procedural spacecraft (NanoAvionics-style): open aluminium skeleton with visible
// avionics stack, cell-level solar panels, patch/tape/dish antennas, star trackers, thrusters, MLI.
// Local frame: +X = nadir (faces Earth), -X = zenith, +-Y = solar wings, Z = along the bus.
// The bus is 100 units wide; the scene scales the whole model to the real bus size.
#include "ISScene.h"
#include "ISSim.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace
{
	const FLinearColor Anod(0.035f, 0.035f, 0.04f), Alu(0.78f, 0.79f, 0.81f), Gold(1.f, 0.72f, 0.28f), Copper(0.85f, 0.45f, 0.22f);
	const FLinearColor Cell(0.03f, 0.06f, 0.22f), Busbar(0.75f, 0.76f, 0.8f), PCB(0.02f, 0.2f, 0.07f), PCBBlack(0.03f, 0.03f, 0.035f);
	const FLinearColor Chip(0.01f, 0.01f, 0.012f), White(0.92f, 0.92f, 0.9f), Radiator(0.95f, 0.95f, 0.97f), Glass(0.05f, 0.12f, 0.25f);

	struct FBusShape { float W = 100, D = 100, H = 110; bool bCube = false; bool bRound = false; int32 Units = 1; };

	FBusShape ShapeFor(const FISDesign& D)
	{
		FBusShape S;
		for (const FName& Id : D.Parts)
		{
			const FString B = Id.ToString();
			if (B == TEXT("bus_1u")) { S = { 100, 100, 113, true, false, 1 }; }
			else if (B == TEXT("bus_3u")) { S = { 100, 100, 340, true, false, 3 }; }
			else if (B == TEXT("bus_6u")) { S = { 200, 100, 340, true, false, 3 }; }
			else if (B == TEXT("bus_can")) { S = { 100, 100, 175, false, true, 1 }; }
			else if (B == TEXT("bus_small")) { S = { 100, 100, 115, false, false, 1 }; }
			else if (B == TEXT("bus_med")) { S = { 100, 100, 160, false, false, 1 }; }
			else if (B == TEXT("bus_geo")) { S = { 100, 110, 170, false, false, 1 }; }
			else if (B == TEXT("bus_deep")) { S = { 110, 110, 90, false, true, 1 }; }
			else if (B == TEXT("bus_ultra")) { S = { 80, 80, 40, false, false, 1 }; }
		}
		return S;
	}
}

void AISSpaceScene::BuildSatellite(USceneComponent* Root, const FISDesign& D, TArray<UStaticMeshComponent*>& Track)
{
	// helper: box / cylinder / sphere / cone with a PBR finish
	auto Part = [&](UStaticMesh* Mesh, const FVector& Pos, const FVector& Size, const FLinearColor& Col, float Metal, float Rough, const FRotator& Rot = FRotator::ZeroRotator, float Emis = 0.f)
	{
		UStaticMeshComponent* C = AddMesh(Root, Mesh, FTransform(Rot, Pos, Size / 100.f), Col, false, Emis, &Track);
		if (C)
			if (UMaterialInstanceDynamic* M = Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))
			{
				M->SetScalarParameterValue(TEXT("Metallic"), Metal);
				M->SetScalarParameterValue(TEXT("Roughness"), Rough);
			}
		return C;
	};
	auto Box = [&](const FVector& P, const FVector& S, const FLinearColor& C, float Me = 0.f, float Ro = 0.5f, const FRotator& R = FRotator::ZeroRotator) { return Part(MCube, P, S, C, Me, Ro, R); };
	auto Cyl = [&](const FVector& P, float Dia, float Len, const FLinearColor& C, float Me = 0.f, float Ro = 0.5f, const FRotator& R = FRotator::ZeroRotator) { return Part(MCyl, P, FVector(Dia, Dia, Len), C, Me, Ro, R); };

	TArray<const FISPart*> P;
	for (const FName& Id : D.Parts) if (const FISPart* X = ISData::FindPart(Id)) P.Add(X);
	auto Has = [&](const TCHAR* Id) { return D.Has(FName(Id)); };
	auto HasTag = [&](FName T) { return D.HasTag(T); };

	const FBusShape S = ShapeFor(D);
	const float HW = S.W * 0.5f, HD = S.D * 0.5f, HH = S.H * 0.5f;
	const bool bMain = &Track == &SatParts;   // the player's satellite: remember its deployable panels for the separation animation
	if (bMain) { SatWings.Reset(); SatHalfH = HH; }
	const FLinearColor Frame = S.bCube ? Anod : Alu;
	const bool bMLI = HasTag(ISTag::Heaters) || S.H >= 150.f;

	// ---------------------------------------------------------------- skeleton frame
	if (S.bRound && Has(TEXT("bus_can")))
	{
		// CanSat: open aluminium can frame with rings and three longerons
		for (float Z : { -HH + 3.f, -HH * 0.3f, HH * 0.3f, HH - 3.f }) Cyl(FVector(0, 0, Z), S.W, 4.f, Alu, 1.f, 0.3f);
		for (int32 i = 0; i < 3; ++i) { const FVector Dir = FRotator(0, i * 120.f, 0).Vector(); Cyl(Dir * (HW - 3.f), 5.f, S.H, Alu, 1.f, 0.3f); }
	}
	else if (S.bRound)
	{
		// deep-space probe: decagonal ring bus (Voyager / Galileo style)
		for (int32 i = 0; i < 10; ++i)
		{
			const float A = i * 36.f;
			const FVector Dir = FRotator(0, A, 0).Vector();
			Box(Dir * HW * 0.95f, FVector(4.f, HW * 0.62f, S.H), (i % 2) ? Gold : Alu, 1.f, (i % 2) ? 0.25f : 0.35f, FRotator(0, A, 0));
		}
		Cyl(FVector(0, 0, HH), S.W * 1.02f, 4.f, Alu, 1.f, 0.35f);
		Cyl(FVector(0, 0, -HH), S.W * 1.02f, 4.f, Alu, 1.f, 0.35f);
	}
	else
	{
		const float T = S.bCube ? 8.5f : 5.f;   // CubeSat rails are 8.5 mm on a 100 mm cube
		for (int32 sx : { -1, 1 }) for (int32 sy : { -1, 1 })
			Box(FVector(sx * (HW - T * 0.5f), sy * (HD - T * 0.5f), 0), FVector(T, T, S.H), Frame, S.bCube ? 0.4f : 1.f, S.bCube ? 0.55f : 0.3f);
		// frame rings at the ends and at every U (or every third for big buses)
		const int32 Rings = S.bCube ? S.Units + 1 : 3;
		for (int32 r = 0; r < Rings; ++r)
		{
			const float Z = -HH + 4.f + (S.H - 8.f) * r / FMath::Max(1, Rings - 1);
			for (int32 s : { -1, 1 })
			{
				Box(FVector(s * (HW - 2.f), 0, Z), FVector(4.f, S.D, 5.f), Frame, S.bCube ? 0.4f : 1.f, 0.4f);
				Box(FVector(0, s * (HD - 2.f), Z), FVector(S.W, 4.f, 5.f), Frame, S.bCube ? 0.4f : 1.f, 0.4f);
			}
		}
		// side panels: two open faces show the inside; the others get skin
		if (!S.bCube)
		{
			// -X (zenith): white radiator with stripes; +Y/-Y: honeycomb or gold MLI
			Box(FVector(-HW + 1.f, 0, 0), FVector(1.5f, S.D - 10.f, S.H - 10.f), Radiator, 0.f, 0.25f);
			for (int32 k = 0; k < 6; ++k) Box(FVector(-HW + 0.f, 0, -HH + 12.f + k * (S.H - 24.f) / 5.f), FVector(1.f, S.D - 14.f, 1.2f), FLinearColor(0.7f, 0.7f, 0.72f), 0.5f, 0.4f);
			for (int32 s : { -1, 1 })
				Box(FVector(0, s * (HD - 1.f), 0), FVector(S.W - 10.f, 1.5f, S.H - 10.f), bMLI ? Gold : FLinearColor(0.55f, 0.56f, 0.58f), 1.f, bMLI ? 0.22f : 0.55f);
		}
		// separation ring / launch adapter on the bottom
		Cyl(FVector(0, 0, -HH - 4.f), S.W * 0.62f, 8.f, Alu, 1.f, 0.3f);
		Cyl(FVector(0, 0, -HH - 9.f), S.W * 0.5f, 3.f, FLinearColor(0.2f, 0.2f, 0.22f), 0.8f, 0.4f);
	}

	// ---------------------------------------------------------------- internals (visible through the skeleton)
	const float InnerW = S.W - 22.f, InnerD = S.D - 22.f;
	if (S.bCube || S.bRound)
	{
		// avionics card stack: PCBs on standoffs with chips and connectors
		const int32 Cards = FMath::Clamp((int32)(S.H / 22.f), 3, 14);
		for (int32 c = 0; c < Cards; ++c)
		{
			const float Z = -HH + 14.f + c * (S.H - 28.f) / FMath::Max(1, Cards - 1);
			Box(FVector(0, 0, Z), FVector(InnerW, InnerD, 1.6f), (c % 3 == 0) ? PCBBlack : PCB, 0.1f, 0.6f);
			Box(FVector(InnerW * 0.15f, -InnerD * 0.1f, Z + 1.6f), FVector(14.f, 14.f, 2.f), Chip, 0.2f, 0.4f);
			Box(FVector(-InnerW * 0.2f, InnerD * 0.2f, Z + 1.4f), FVector(9.f, 6.f, 1.6f), Chip, 0.2f, 0.4f);
			Box(FVector(-InnerW * 0.42f, 0, Z + 2.f), FVector(4.f, InnerD * 0.7f, 3.f), Gold, 1.f, 0.3f);      // stack connector
			for (int32 sx : { -1, 1 }) for (int32 sy : { -1, 1 })
				Cyl(FVector(sx * InnerW * 0.42f, sy * InnerD * 0.42f, Z + 4.f), 3.f, 7.f, Alu, 1.f, 0.3f);
		}
	}
	else
	{
		// bigger buses: propellant tank, avionics boxes, harness
		if (HasTag(ISTag::SolarSail) == false)
			Part(MSphere, FVector(0, 0, -HH * 0.25f), FVector(InnerW * 0.62f, InnerW * 0.62f, InnerW * 0.62f), Alu, 1.f, 0.25f);
		for (int32 k = 0; k < 4; ++k)
			Box(FVector(InnerW * 0.22f, (k % 2 ? -1 : 1) * InnerD * 0.25f, HH * 0.15f + (k / 2) * 20.f), FVector(InnerW * 0.4f, InnerD * 0.35f, 15.f), (k % 2) ? FLinearColor(0.15f, 0.16f, 0.18f) : FLinearColor(0.75f, 0.6f, 0.2f), 0.6f, 0.4f);
		Cyl(FVector(-InnerW * 0.3f, 0, 0), 3.f, S.H * 0.8f, FLinearColor(0.9f, 0.9f, 0.85f), 0.f, 0.7f);    // harness bundle
	}

	// ---------------------------------------------------------------- components by subsystem
	int32 NadirSlot = 0;
	auto NadirPos = [&](float Size)
	{
		const int32 Col = NadirSlot % 2, Row = NadirSlot / 2; ++NadirSlot;
		return FVector(HW + Size * 0.5f, (Col ? 1.f : -1.f) * S.D * 0.22f, HH * 0.4f - Row * S.H * 0.3f);
	};
	for (const FISPart* X : P)
	{
		switch (X->Cat)
		{
		case EISCat::Power:
		{
			if (X->RtgW > 0)
			{
				const int32 N = X->Id == "pwr_gphs" ? 3 : 1;
				for (int32 i = 0; i < N; ++i)
				{
					const FVector C(-HW - 60.f - i * 42.f, -HD - 25.f, -HH * 0.3f);
					Cyl(C, 22.f, 40.f, FLinearColor(0.22f, 0.22f, 0.24f), 0.6f, 0.45f, FRotator(90, 0, 0));
					for (int32 f = 0; f < 8; ++f) Box(C, FVector(40.f, 1.2f, 44.f), FLinearColor(0.25f, 0.25f, 0.27f), 0.6f, 0.45f, FRotator(0, 0, f * 22.5f));
					Cyl(C + FVector(0, 0, 22.f), 16.f, 2.f, Gold, 1.f, 0.25f, FRotator(90, 0, 0));
				}
				Cyl(FVector(-HW - 30.f, -HD - 12.f, -HH * 0.3f), 3.f, 70.f, Alu, 1.f, 0.3f, FRotator(90, 0, 0));
				break;
			}
			if (X->SolarW <= 0) break;
			const bool bBody = X->Id == "pwr_body";
			// cell grid on a panel: Rows x Cols cells with busbars, panel frame, gold back
			// a flat panel defined by its normal and in-plane axes; cells in a grid with busbars, gold back
			auto Panel = [&](const FVector& C, float Len, float Hgt, const FVector& Normal, const FVector& LenAxis)
			{
				const FRotator R = FRotationMatrix::MakeFromXY(Normal, LenAxis).Rotator();
				const FVector An = Normal.GetSafeNormal(), Ax = R.RotateVector(FVector(0, 1, 0)), Az = R.RotateVector(FVector(0, 0, 1));
				Box(C, FVector(1.2f, Len, Hgt), FLinearColor(0.08f, 0.08f, 0.1f), 0.4f, 0.5f, R);
				Box(C - An * 0.9f, FVector(0.6f, Len - 2.f, Hgt - 2.f), Gold, 1.f, 0.3f, R);
				const int32 Cols = FMath::Clamp((int32)(Len / 10.f), 2, 24), Rows = FMath::Clamp((int32)(Hgt / 14.f), 1, 10);
				const float CW = (Len - 4.f) / Cols, CH = (Hgt - 4.f) / Rows;
				for (int32 r = 0; r < Rows; ++r) for (int32 c = 0; c < Cols; ++c)
				{
					const FVector P2 = C + An * 0.8f + Ax * (-Len * 0.5f + 2.f + CW * (c + 0.5f)) + Az * (-Hgt * 0.5f + 2.f + CH * (r + 0.5f));
					Box(P2, FVector(0.5f, CW - 1.2f, CH - 1.2f), Cell, 0.5f, 0.12f, R);
				}
				for (int32 r = 0; r <= Rows; ++r)
					Box(C + An * 1.f + Az * (-Hgt * 0.5f + 2.f + CH * r), FVector(0.3f, Len - 3.f, 0.5f), Busbar, 1.f, 0.3f, R);
			};
			if (bBody || S.bCube)
			{
				for (int32 s : { -1, 1 })
					Panel(FVector(0, s * (HD + 0.8f), 0), S.W - 10.f, S.H - 14.f, FVector(0, s, 0), FVector(1, 0, 0));
				if (bBody) break;
			}
			// deployable wings: panel count and size grow with power
			const float Area = FMath::Clamp(X->SolarW / 20.f, 1.f, 700.f);
			const int32 Panels = FMath::Clamp((int32)FMath::Sqrt(Area) , 1, 5);
			const float PanelLen = S.bCube ? S.H * 0.95f : FMath::Clamp(S.H * 0.9f, 60.f, 160.f);
			const float PanelHgt = S.bCube ? S.W * 0.95f : FMath::Clamp(30.f * FMath::Sqrt(Area) / Panels, 40.f, 120.f);
			const int32 Wings = X->Id == "pwr_juno" ? 3 : 2;
			for (int32 w = 0; w < Wings; ++w)
			{
				const float Yaw = Wings == 3 ? w * 120.f : (w == 0 ? 0.f : 180.f);
				const FRotator WR(0, Yaw, 0);
				const FVector Out = WR.RotateVector(FVector(0, 1, 0));
				// yoke boom and hinges
				Cyl(Out * (HD + 12.f), 3.f, 22.f, Alu, 1.f, 0.3f, FRotator(0, Yaw, 90));
				for (int32 k = 0; k < Panels; ++k)
				{
					const FVector C = Out * (HD + 24.f + PanelHgt * 0.5f + k * (PanelHgt + 3.f));
					const int32 First = Track.Num();
					Panel(C, PanelLen, PanelHgt, FVector(-1, 0, 0), FVector(0, 0, 1));   // cells face the Sun (zenith side), panel spans bus length x outward
					Box(C - Out * (PanelHgt * 0.5f + 1.5f), FVector(3.f, PanelLen * 0.2f, 3.f), Alu, 1.f, 0.3f, FRotator(0, Yaw + 90.f, 0));
					if (bMain) SatWings.Add({ First, Track.Num() - First, k, w, Out, Out * (HD + 24.f), PanelHgt + 3.f, PanelHgt * 0.5f });
				}
			}
			break;
		}
		case EISCat::Comms:
		{
			if (X->DishM > 1.f || X->Tag == ISTag::HGA)
			{
				// parabolic high-gain antenna on the zenith side with feed tripod
				const float Dia = FMath::Clamp(S.W * (0.8f + X->DishM * 0.35f), 60.f, 260.f);
				const FVector C(-HW - Dia * 0.18f, 0, 0);
				Part(MSphere, C, FVector(Dia * 0.32f, Dia, Dia), White, 0.f, 0.35f);
				Cyl(C + FVector(-Dia * 0.16f, 0, 0), Dia * 1.0f, 2.f, Alu, 1.f, 0.3f, FRotator(90, 0, 0));
				for (int32 k = 0; k < 3; ++k)
				{
					const FVector Rim = C + FRotator(0, 0, k * 120.f).RotateVector(FVector(-Dia * 0.16f, 0, Dia * 0.45f));
					const FVector Feed = C + FVector(-Dia * 0.55f, 0, 0);
					Cyl((Rim + Feed) * 0.5f, 1.6f, (Feed - Rim).Size(), Alu, 1.f, 0.3f, FRotationMatrix::MakeFromZ((Feed - Rim).GetSafeNormal()).Rotator());
				}
				Part(MCone, C + FVector(-Dia * 0.58f, 0, 0), FVector(9.f, 9.f, 12.f), Gold, 1.f, 0.25f, FRotator(-90, 0, 0));
			}
			else if (X->RadioMbps < 0.1f)
			{
				// deployable UHF tape-measure antennas (turnstile)
				for (int32 k = 0; k < 4; ++k)
				{
					const FRotator R(0, 45.f + k * 90.f, 0);
					const FVector Dir = R.Vector();
					Box(FVector(-HW + 2.f, 0, HH - 2.f) + FVector(0, Dir.Y, Dir.X) * 60.f, FVector(1.f, 0.4f, 120.f), Alu, 1.f, 0.3f, FRotator(0, 0, 0) + FRotator(0, 0, 45.f + k * 90.f) + FRotator(90, 0, 0));
				}
				Box(FVector(-HW + 1.f, 0, HH - 6.f), FVector(4.f, S.D * 0.5f, 10.f), Anod, 0.4f, 0.5f);
			}
			else
			{
				// patch antennas on the nadir face (S-band / X-band), small horn for Ka
				const int32 N = X->RadioMbps > 100 ? 4 : 2;
				for (int32 k = 0; k < N; ++k)
				{
					const FVector Pp(HW + 1.f, (k % 2 ? 1 : -1) * S.D * 0.2f, -HH * 0.55f + (k / 2) * 26.f);
					Box(Pp, FVector(1.5f, 22.f, 22.f), White, 0.f, 0.5f);
					Box(Pp + FVector(1.f, 0, 0), FVector(0.6f, 14.f, 14.f), Gold, 1.f, 0.25f);
				}
				if (X->RadioMbps > 500)
					Part(MCone, FVector(HW + 14.f, 0, -HH * 0.8f), FVector(18.f, 18.f, 26.f), Gold, 1.f, 0.25f, FRotator(-90, 0, 0));
			}
			break;
		}
		case EISCat::ADCS:
			if (X->PointingDeg > 0 && X->PointingDeg < 0.1f)
				for (int32 s : { -1, 1 })
				{
					// star tracker with light baffle, angled away from the Sun
					const FVector C(-HW - 8.f, s * HD * 0.45f, HH - 18.f);
					Box(C + FVector(6.f, 0, 0), FVector(12.f, 16.f, 16.f), Anod, 0.4f, 0.5f);
					Cyl(C + FVector(-6.f, 0, 4.f), 12.f, 18.f, Anod, 0.3f, 0.6f, FRotator(-60, 0, s * 20.f));
					Cyl(C + FVector(-12.f, 0, 8.f), 8.f, 1.f, Glass, 0.9f, 0.05f, FRotator(-60, 0, s * 20.f));
				}
			if (X->PointingDeg > 0 && X->PointingDeg < 1.f && (S.bCube || S.bRound))
				for (int32 k = 0; k < 3; ++k)   // reaction wheels inside the stack
					Cyl(FVector(InnerW * 0.1f, 0, -HH * 0.5f + k * 9.f), InnerW * 0.55f, 6.f, FLinearColor(0.6f, 0.6f, 0.62f), 1.f, 0.25f, FRotator(k * 90.f, k * 45.f, 0));
			if (X->Id == "adcs_mtq" || X->Id == "adcs_mag")
				for (int32 k = 0; k < 2; ++k) Cyl(FVector(0, (k ? 1 : -1) * InnerD * 0.3f, HH * 0.6f), 4.f, InnerW * 0.8f, Copper, 1.f, 0.3f, FRotator(0, 0, 90));
			// coarse sun sensors on the corners
			for (int32 s : { -1, 1 }) Box(FVector(-HW - 1.f, s * HD * 0.75f, -HH + 10.f), FVector(3.f, 6.f, 6.f), Glass, 0.8f, 0.1f);
			break;
		case EISCat::Propulsion:
			if (X->Tag == ISTag::SolarSail)
			{
				const float Side = 260.f * (1.f + FMath::LogX(10.f, X->SailM2 / 80.f + 1.f));
				for (int32 k = 0; k < 4; ++k)
				{
					const float A = 45.f + k * 90.f;
					const FVector Dir = FRotator(0, 0, A).RotateVector(FVector(0, 1, 0));
					Cyl(FVector(0, 0, HH + 3.f) + FVector(0, Dir.Y, Dir.Z) * Side * 0.35f, 2.f, Side * 0.7f, FLinearColor(0.3f, 0.25f, 0.2f), 0.4f, 0.5f, FRotator(0, 0, A + 90.f));
				}
				Part(MCube, FVector(0, 0, HH + 3.f), FVector(Side, Side, 0.4f), FLinearColor(0.92f, 0.84f, 0.62f), 0.6f, 0.25f, FRotator(0, 45.f, 0));
			}
			else if (X->IspS > 1000.f)
			{
				Cyl(FVector(0, 0, -HH - 10.f), 28.f, 12.f, FLinearColor(0.3f, 0.3f, 0.33f), 0.8f, 0.4f);
				Cyl(FVector(0, 0, -HH - 17.f), 24.f, 1.f, FLinearColor(0.35f, 0.55f, 1.f), 0.f, 0.5f, FRotator::ZeroRotator);
				AddMesh(Root, MCyl, FTransform(FRotator::ZeroRotator, FVector(0, 0, -HH - 18.f), FVector(0.22f, 0.22f, 0.01f)), FLinearColor(0.3f, 0.5f, 1.f), true, 6.f, &Track);
			}
			else if (X->PropKg > 0)
			{
				const int32 N = X->PropKg > 100 ? 4 : 2;
				for (int32 k = 0; k < N; ++k)
				{
					const FVector Nz((k % 2 ? 1 : -1) * HW * 0.5f, (k / 2 ? 1 : -1) * HD * 0.5f, -HH - 8.f);
					Part(MCone, Nz, FVector(7.f, 7.f, 10.f), Gold, 1.f, 0.25f, FRotator(180, 0, 0));
				}
				if (X->PropKg > 1000) Part(MCone, FVector(0, 0, -HH - 22.f), FVector(30.f, 30.f, 40.f), FLinearColor(0.35f, 0.32f, 0.3f), 0.9f, 0.4f, FRotator(180, 0, 0));
			}
			break;
		case EISCat::Payload:
		{
			const float M3 = FMath::Pow(FMath::Max(0.01f, X->MassKg), 0.33f);
			const float Sz = FMath::Clamp(14.f + M3 * 6.f, 14.f, 70.f);
			if (X->Tag == ISTag::Imager || X->Tag == ISTag::Camera || X->Tag == ISTag::WeatherImager || X->Tag == ISTag::Altimeter)
			{
				// telescope barrel: black tube, gold MLI collar, glass front, baffle ring
				const FVector C = NadirPos(Sz);
				Cyl(C, Sz, Sz * 1.2f, Anod, 0.3f, 0.6f, FRotator(90, 0, 0));
				Cyl(C - FVector(Sz * 0.3f, 0, 0), Sz * 1.15f, Sz * 0.45f, Gold, 1.f, 0.22f, FRotator(90, 0, 0));
				Cyl(C + FVector(Sz * 0.61f, 0, 0), Sz * 0.8f, 0.6f, Glass, 0.9f, 0.04f, FRotator(90, 0, 0));
				Cyl(C + FVector(Sz * 0.62f, 0, 0), Sz * 1.02f, 2.f, Alu, 1.f, 0.3f, FRotator(90, 0, 0));
			}
			else if (X->Tag == ISTag::Transponder)
			{
				// flat phased-array panel on the nadir face
				const FVector C(HW + 2.f, 0, 0);
				Box(C, FVector(2.f, S.D * 0.9f, S.H * 0.7f), FLinearColor(0.85f, 0.85f, 0.88f), 0.2f, 0.4f);
				for (int32 r = 0; r < 6; ++r) for (int32 c = 0; c < 6; ++c)
					Box(C + FVector(1.2f, -S.D * 0.35f + c * S.D * 0.14f, -S.H * 0.28f + r * S.H * 0.11f), FVector(0.6f, S.D * 0.1f, S.D * 0.1f), Gold, 1.f, 0.3f);
			}
			else if (X->Tag == ISTag::EntryProbe)
			{
				Part(MCone, FVector(HW + 40.f, 0, -HH * 0.4f), FVector(80.f, 80.f, 45.f), FLinearColor(0.45f, 0.3f, 0.18f), 0.1f, 0.8f, FRotator(-90, 0, 0));
				Cyl(FVector(HW + 18.f, 0, -HH * 0.4f), 82.f, 6.f, Alu, 1.f, 0.3f, FRotator(90, 0, 0));
			}
			else if (X->Tag == ISTag::Record)
				Cyl(FVector(0, HD + 2.f, -HH * 0.4f), 30.f, 1.5f, Gold, 1.f, 0.15f, FRotator(0, 0, 90));
			else if (X->Tag == ISTag::Fields)
			{
				// magnetometer on a long boom
				Cyl(FVector(0, -HD - 70.f, HH * 0.4f), 2.f, 140.f, FLinearColor(0.85f, 0.8f, 0.6f), 0.3f, 0.5f, FRotator(0, 0, 90));
				Box(FVector(0, -HD - 142.f, HH * 0.4f), FVector(8.f, 8.f, 8.f), Anod, 0.4f, 0.5f);
			}
			else
			{
				const FVector C = NadirPos(Sz);
				Box(C, FVector(Sz, Sz, Sz), (X->Color.R + X->Color.G + X->Color.B > 2.f) ? White : FLinearColor(0.2f, 0.2f, 0.22f), 0.3f, 0.5f);
				Box(C + FVector(Sz * 0.5f + 0.5f, 0, 0), FVector(0.6f, Sz * 0.6f, Sz * 0.6f), Glass, 0.8f, 0.1f);
			}
			break;
		}
		case EISCat::Extra:
			if (X->Tag == ISTag::Parachute)
				Part(MCone, FVector(0, 0, HH + 15.f), FVector(S.W * 0.7f, S.W * 0.7f, 30.f), FLinearColor(1.f, 0.35f, 0.08f), 0.f, 0.9f);
			else if (X->Tag == ISTag::Vault)
				Box(FVector(0, 0, HH * 0.1f), FVector(InnerW * 0.8f, InnerD * 0.8f, S.H * 0.35f), FLinearColor(0.55f, 0.55f, 0.6f), 1.f, 0.4f);
			else if (X->Tag == ISTag::DragSail)
				Box(FVector(-HW - 6.f, 0, HH - 15.f), FVector(10.f, 30.f, 30.f), Alu, 1.f, 0.35f);
			else if (X->Tag == ISTag::Retro)
				for (int32 k = 0; k < 7; ++k)
					Cyl(FVector(HW + 1.5f, -S.D * 0.3f + (k % 3) * 7.f, HH * 0.75f - (k / 3) * 7.f), 6.f, 2.f, Glass, 1.f, 0.05f, FRotator(90, 0, 0));
			else if (X->Tag == ISTag::Buzzer)
				Cyl(FVector(0, 0, HH - 6.f), 14.f, 9.f, Anod, 0.2f, 0.5f);
			break;
		default: break;
		}
	}
	// CanSat: small parts inside the open frame (reuse the bench models at real scale)
	if (Has(TEXT("bus_can")))
	{
		float Z = -HH + 6.f;
		for (const FISPart* X : P)
			if (X->Cat == EISCat::Battery || X->Cat == EISCat::Computer || X->Cat == EISCat::Comms || (X->Cat == EISCat::Payload && X->MassKg < 0.1f))
			{
				Box(FVector(0, 0, Z), FVector(InnerW, InnerD * 0.8f, 10.f), X->Cat == EISCat::Battery ? FLinearColor(0.15f, 0.3f, 0.7f) : PCB, 0.2f, 0.5f);
				Z += 22.f;
			}
	}
}
