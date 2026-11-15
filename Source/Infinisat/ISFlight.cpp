// Infinisat - the launch you fly: FISRocketSim physics drawn at true scale (1 uu = 1 cm) over the real-size Earth.
// The rocket is built from the launcher's real stage dimensions; spent stages, boosters and the fairing drop away;
// the plume grows wider as the air thins (under-expanded exhaust in vacuum).
#include "ISScene.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/LineBatchComponent.h"

namespace
{
	const FLinearColor EngineGrey(0.12f, 0.12f, 0.13f), Band(0.06f, 0.06f, 0.07f);
}

bool AISSpaceScene::StartFlight(FName VehicleId, double PayloadKg, double TargetAltKm, double IncDeg)
{
	const FISLauncher* L = FISLauncher::Find(VehicleId);
	if (!L || View != EISView::Orbit || !bTrueScale) return false;
	FlightVehicle = VehicleId; FlightPayload = PayloadKg; FlightAlt = TargetAltKm; FlightInc = IncDeg;
	if (!RocketRoot)
	{
		RocketRoot = NewObject<USceneComponent>(this);
		RocketRoot->SetupAttachment(SceneRoot);
		RocketRoot->RegisterComponent();
	}
	if (!FlightLines)
	{
		FlightLines = NewObject<ULineBatchComponent>(this);
		FlightLines->SetupAttachment(SceneRoot);
		FlightLines->RegisterComponent();
	}
	// launch plane = the spacecraft's orbital plane; the pad sits ~25 deg "behind" the spacecraft so the ascent ends near it
	const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(20.f)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(OrbitInc));
	const FVector N = Q.RotateVector(FVector::UpVector);
	FVector SatDir = SatRoot->GetRelativeLocation().GetSafeNormal();
	if (SatDir.IsNearlyZero()) SatDir = Q.RotateVector(FVector::ForwardVector);
	// pad on the sunlit side of the orbital plane (daylight launch)
	const FVector ToSun = -SunLight->GetForwardVector();
	FlightU = (ToSun - N * FVector::DotProduct(N, ToSun)).GetSafeNormal();
	if (FlightU.IsNearlyZero()) FlightU = FQuat(N, -0.45f).RotateVector(SatDir).GetSafeNormal();
	FlightU = FQuat(N, -0.35f).RotateVector(FlightU).GetSafeNormal();   // morning side: the rocket flies toward the sun
	FlightDn = FVector::CrossProduct(N, FlightU).GetSafeNormal();
	RetryFlight();
	return true;
}

void AISSpaceScene::RetryFlight()
{
	const FISLauncher* L = FISLauncher::Find(FlightVehicle);
	if (!L) return;
	Sim = FISRocketSim();
	Sim.Setup(*L, FlightPayload, FlightAlt, FlightInc);
	UE_LOG(LogTemp, Log, TEXT("[FLIGHT] start %s payload %.1f kg target %.0f km inc %.1f (ascent target %.0f km)"), *L->Name, FlightPayload, FlightAlt, FlightInc, Sim.TargetAltKm);
	bFlight = true;
	FlightWarp = 1.f;
	FlightEndT = 0.f;
	BuildFlightRocket();
	RocketRoot->SetVisibility(true, true);
	// launch site: concrete pad, flame trench, tower and a patch of ground at the true radius (the Earth mesh is faceted at this scale)
	for (UStaticMeshComponent* C : PadVis) if (IsValid(C)) C->DestroyComponent();
	PadVis.Reset();
	{
		const FVector Site = FlightU * (float)(FISRocketSim::Re * 100.0);
		const FRotator Rot = FRotationMatrix::MakeFromZX(FlightU, FlightDn).Rotator();
		auto Pad = [&](UStaticMesh* M, const FVector& LocalM, const FVector& SizeM, const FLinearColor& C)
		{
			UStaticMeshComponent* P = AddMesh(SceneRoot, M, FTransform(Rot, Site + Rot.RotateVector(LocalM * 100.f), SizeM), C, false, 0.f, nullptr);
			if (P) PadVis.Add(P);
		};
		const float Len = RocketLenCm / 100.f, D = (float)Sim.L->Stages[0].DiameterM;
		// real surfaces: the same tiled photo textures the house uses (concrete, grass, gravel)
		UMaterialInterface* World = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_World.M_IS_World"));
		auto Tex = [&](const TCHAR* T, float Tile, const FLinearColor& Tint)
		{
			UStaticMeshComponent* P = PadVis.Num() ? PadVis.Last() : nullptr;
			UTexture* Tx = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Infinisat/PHTex/%s_diff.%s_diff"), T, T));
			if (!P || !World || !Tx) return;
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(World, P);
			M->SetTextureParameterValue(TEXT("Tex"), Tx);
			M->SetScalarParameterValue(TEXT("Tile"), Tile);
			M->SetVectorParameterValue(TEXT("Tint"), Tint);
			P->SetMaterial(0, M);
		};
		Pad(MCyl, FVector(0, 0, -6.0f), FVector(8000.f, 8000.f, 12.f), FLinearColor(0.2f, 0.3f, 0.13f));              // ground (4 km)
		// (world-aligned textures break at true planet scale: 6,371 km from the origin is beyond their precision -> plain colours)
		Pad(MCyl, FVector(0, 0, -0.45f), FVector(160.f, 160.f, 1.f), FLinearColor(0.42f, 0.41f, 0.38f));                 // gravel apron
		Pad(MCyl, FVector(0, 0, -0.4f), FVector(90.f, 90.f, 1.f), FLinearColor(0.55f, 0.55f, 0.53f));                  // concrete pad
		Pad(MCube, FVector(0, 0, -0.9f), FVector(D * 2.2f, D * 6.f, 0.4f), FLinearColor(0.08f, 0.08f, 0.08f));            // flame trench
		Pad(MCube, FVector(-D * 1.2f, 0, 0.6f), FVector(0.6f, D * 6.f, 1.4f), FLinearColor(0.35f, 0.35f, 0.34f));          // trench walls
		Pad(MCube, FVector(D * 1.2f, 0, 0.6f), FVector(0.6f, D * 6.f, 1.4f), FLinearColor(0.35f, 0.35f, 0.34f));
		// support area beside the pad: propellant spheres, water tower for the sound-suppression deluge, control bunker
		Pad(MSphere, FVector(70.f, 45.f, 6.f), FVector(12.f, 12.f, 12.f), FLinearColor(0.92f, 0.92f, 0.9f));            // LOX sphere
		Pad(MSphere, FVector(70.f, 65.f, 6.f), FVector(12.f, 12.f, 12.f), FLinearColor(0.92f, 0.92f, 0.9f));            // fuel sphere
		for (int32 k = 0; k < 4; ++k) Pad(MCyl, FVector(-60.f + (k % 2) * 6.f, -60.f + (k / 2) * 6.f, 12.f), FVector(0.5f, 0.5f, 24.f), FLinearColor(0.5f, 0.5f, 0.5f));
		Pad(MCyl, FVector(-57.f, -57.f, 27.f), FVector(9.f, 9.f, 7.f), FLinearColor(0.85f, 0.85f, 0.82f));             // water tower
		Pad(MCube, FVector(-80.f, 70.f, 2.f), FVector(14.f, 9.f, 4.f), FLinearColor(0.6f, 0.6f, 0.58f));               // control bunker
		// launch tower: an open steel lattice (4 legs + cross beams every 4 m), not a solid block
		{
			const FVector TC(-D * 2.6f - 2.5f, 0, 0);
			const float TH = Len * 1.1f, Leg = 0.35f, W = 4.f;
			const FLinearColor Steel(0.5f, 0.17f, 0.1f);
			for (int32 k = 0; k < 4; ++k)
				Pad(MCube, TC + FVector((k < 2 ? -1.f : 1.f) * W * 0.5f, (k % 2 ? -1.f : 1.f) * W * 0.5f, TH * 0.5f), FVector(Leg, Leg, TH), Steel);
			for (float Z = 2.f; Z < TH; Z += 4.f)
			{
				Pad(MCube, TC + FVector(0, -W * 0.5f, Z), FVector(W, 0.2f, 0.2f), Steel);
				Pad(MCube, TC + FVector(0, W * 0.5f, Z), FVector(W, 0.2f, 0.2f), Steel);
				Pad(MCube, TC + FVector(-W * 0.5f, 0, Z), FVector(0.2f, W, 0.2f), Steel);
				Pad(MCube, TC + FVector(W * 0.5f, 0, Z), FVector(0.2f, W, 0.2f), Steel);
			}
		}
		{
			const FVector TC(-D * 2.6f - 2.5f, 0, 0);
			const float TH = Len * 1.1f, W = 4.f;
			const FLinearColor Steel(0.5f, 0.17f, 0.1f), Grate(0.3f, 0.3f, 0.32f);
			// diagonal bracing on the two long faces (each 4 m bay), service platforms every 12 m, crane jib on top
			for (float Z = 2.f; Z + 4.f < TH; Z += 4.f)
				for (float S : { -1.f, 1.f })
				{
					UStaticMeshComponent* Br = nullptr;
					Pad(MCube, TC + FVector(0, S * W * 0.5f, Z + 2.f), FVector(5.6f, 0.16f, 0.16f), Steel);
					Br = PadVis.Last();
					if (Br) Br->AddLocalRotation(FRotator(((int32)(Z / 4.f) % 2 ? 45.f : -45.f), 0, 0));
				}
			for (float Z = 12.f; Z < TH; Z += 12.f) Pad(MCube, TC + FVector(0, 0, Z), FVector(W + 1.2f, W + 1.2f, 0.25f), Grate);
			Pad(MCube, TC + FVector(0, 0, TH + 1.f), FVector(1.f, 1.f, 2.f), Steel);
			Pad(MCube, TC + FVector(2.5f, 0, TH + 2.f), FVector(9.f, 0.5f, 0.5f), FLinearColor(0.85f, 0.7f, 0.15f));   // crane jib
			Pad(MCyl, TC + FVector(0, 0, TH + 6.f), FVector(0.15f, 0.15f, 8.f), FLinearColor(0.6f, 0.6f, 0.62f));    // lightning rod
			// umbilical arms reaching to each stage (propellant, power, data), with a hose drooping to the rocket
			const int32 NArm = FMath::Clamp(Sim.L->Stages.Num() + 1, 2, 4);
			for (int32 a = 0; a < NArm; ++a)
			{
				const float Z = Len * (0.18f + 0.62f * a / FMath::Max(1, NArm - 1));
				const float Reach = FMath::Abs(TC.X) - D * 0.5f - W * 0.5f;
				Pad(MCube, FVector(TC.X + W * 0.5f + Reach * 0.5f, 0, Z), FVector(Reach, 0.8f, 0.5f), FLinearColor(0.42f, 0.42f, 0.44f));
				Pad(MCyl, FVector(-D * 0.5f - 0.3f, 0, Z - 0.5f), FVector(0.25f, 0.25f, 1.2f), FLinearColor(0.12f, 0.12f, 0.12f));
			}
		}
		// launch mount: raised table over the trench with hold-down clamps around the first stage
		Pad(MCyl, FVector(0, 0, -0.3f), FVector(D * 1.9f, D * 1.9f, 0.6f), FLinearColor(0.38f, 0.38f, 0.38f));
		for (int32 k = 0; k < 4; ++k)
		{
			const FVector Dk = FRotator(0, 45.f + 90.f * k, 0).Vector();
			Pad(MCube, Dk * (D * 0.62f) + FVector(0, 0, 0.6f), FVector(0.5f, 0.5f, 1.2f), FLinearColor(0.25f, 0.25f, 0.27f));
			Pad(MCube, Dk * (D * 0.85f) + FVector(0, 0, 0.3f), FVector(0.4f, 0.4f, 0.6f), FLinearColor(0.85f, 0.7f, 0.15f));
		}
		// flame deflector ramp and deluge water pipes
		Pad(MCube, FVector(0, D * 2.2f, -1.4f), FVector(D * 2.f, D * 2.f, 0.3f), FLinearColor(0.2f, 0.2f, 0.2f));
		if (PadVis.Num() && PadVis.Last()) PadVis.Last()->AddLocalRotation(FRotator(0, 0, 25.f));
		for (float S : { -1.f, 1.f }) Pad(MCyl, FVector(S * D * 1.5f, 0, 0.4f), FVector(0.5f, 0.5f, D * 5.f), FLinearColor(0.3f, 0.45f, 0.6f));
		if (PadVis.Num() >= 2) { PadVis[PadVis.Num() - 1]->AddLocalRotation(FRotator(0, 0, 90.f)); PadVis[PadVis.Num() - 2]->AddLocalRotation(FRotator(0, 0, 90.f)); }
		// propellant lines from the tank farm to the pad (on low supports)
		for (float Y : { 45.f, 65.f })
		{
			const FVector From(70.f, Y, 0.8f), To(0, 0, 0.8f);
			const FVector Mid = (From + To) * 0.5f;
			Pad(MCyl, Mid, FVector(0.6f, 0.6f, (From - To).Size()), FLinearColor(0.82f, 0.82f, 0.8f));
			if (PadVis.Last()) PadVis.Last()->SetRelativeRotation((FRotationMatrix::MakeFromZX(FlightU, FlightDn).ToQuat() * FRotationMatrix::MakeFromZ((From - To).GetSafeNormal()).ToQuat()).Rotator());
		}
		// assembly hangar, access road, perimeter fence, floodlight towers
		Pad(MCube, FVector(-130.f, -40.f, 12.f), FVector(60.f, 30.f, 24.f), FLinearColor(0.82f, 0.83f, 0.85f));
		Pad(MCube, FVector(-130.f, -40.f, 25.f), FVector(62.f, 32.f, 2.f), FLinearColor(0.45f, 0.47f, 0.5f));
		Pad(MCube, FVector(-99.5f, -40.f, 10.f), FVector(0.4f, 22.f, 20.f), FLinearColor(0.35f, 0.37f, 0.4f));   // hangar door
		Pad(MCube, FVector(-60.f, -40.f, -0.4f), FVector(80.f, 10.f, 1.f), FLinearColor(0.25f, 0.25f, 0.26f));   // road to the pad
		Pad(MCube, FVector(-400.f, -40.f, -0.4f), FVector(600.f, 10.f, 1.f), FLinearColor(0.25f, 0.25f, 0.26f));
		for (int32 k = 0; k < 24; ++k)
		{
			const float A = 360.f * k / 24.f;
			const FVector Pk = FRotator(0, A, 0).Vector() * 190.f;
			Pad(MCyl, Pk + FVector(0, 0, 1.2f), FVector(0.15f, 0.15f, 2.4f), FLinearColor(0.55f, 0.55f, 0.55f));
		}
		for (int32 k = 0; k < 4; ++k)
		{
			const FVector Pk = FRotator(0, 20.f + 90.f * k, 0).Vector() * 85.f;
			Pad(MCyl, Pk + FVector(0, 0, 15.f), FVector(0.5f, 0.5f, 30.f), FLinearColor(0.6f, 0.6f, 0.62f));
			Pad(MCube, Pk + FVector(0, 0, 30.5f), FVector(3.f, 3.f, 1.5f), FLinearColor(0.95f, 0.95f, 0.85f));
		}
		for (int32 k = 0; k < 4; ++k) Pad(MCyl, FVector((k < 2 ? -1.f : 1.f) * 30.f, (k % 2 ? -1.f : 1.f) * 30.f, 25.f), FVector(1.2f, 1.2f, 50.f), FLinearColor(0.6f, 0.6f, 0.62f)); // lightning masts
	}
	SatRoot->SetVisibility(false, true);
	LaunchT = 0.5f;           // "launching": the follow camera tracks RocketRoot, mission time waits
	bFollow = true;
	CamMin = RocketLenCm * 0.4f;
	CamDistGoal = RocketLenCm * 3.6f; CamDist = CamDistGoal;
	FollowYaw = 150.f; FollowPitch = -4.f;   // down-range side: the tower stands behind the rocket, not in front of the lens
	bFlightCamUser = false;
	TickFlight(0.f);
}

void AISSpaceScene::BuildFlightRocket()
{
	for (TArray<UStaticMeshComponent*>& S : StageVis) for (UStaticMeshComponent* C : S) if (IsValid(C)) C->DestroyComponent();
	for (TArray<UStaticMeshComponent*>* A : { &BoosterVis, &FairingVis, &PlumeCore, &PlumeBoost }) { for (UStaticMeshComponent* C : *A) if (IsValid(C)) C->DestroyComponent(); A->Reset(); }
	StageVis.Reset(); StageBaseZ.Reset();
	if (!Sim.L) return;
	const FISLauncher& L = *Sim.L;
	if (!MatGlow) MatGlow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Glow.M_IS_Glow"));
	UTexture2D* White = CosmosTex(TEXT("white"), 4, 4, [](float, float) { return FLinearColor::White; });

	// meters -> engine shapes (cylinder/cone are 100 uu tall, radius 50: scale D,D,L in metres gives cm)
	auto Part = [&](UStaticMesh* M, float Dm, float Lm, float ZcM, const FVector& OffM, const FLinearColor& C, TArray<UStaticMeshComponent*>& Into, float Metal = 0.2f)
	{
		UStaticMeshComponent* P = AddMesh(RocketRoot, M, FTransform(FRotator::ZeroRotator, (OffM + FVector(0, 0, ZcM)) * 100.f, FVector(Dm, Dm, Lm)), C, false, 0.f, nullptr);
		Into.Add(P);
		return P;
	};
	auto Plume = [&](TArray<UStaticMeshComponent*>& Into, const FVector& OffM)
	{
		for (int32 k = 0; k < 2; ++k)
		{
			UStaticMeshComponent* P = AddMesh(RocketRoot, MCone, FTransform(FRotator(180, 0, 0), OffM * 100.f, FVector(1)), FLinearColor(0, 0, 0, 0), true, 0.f, nullptr);
			if (!P) continue;
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatGlow, P);
			M->SetTextureParameterValue(TEXT("Tex"), White);
			M->SetVectorParameterValue(TEXT("Color"), k == 0 ? FLinearColor(1.f, 0.48f, 0.12f) : FLinearColor(1.f, 0.9f, 0.72f));
			M->SetScalarParameterValue(TEXT("Intensity"), k == 0 ? 1.1f : 2.6f);
			P->SetMaterial(0, M);
			P->CastShadow = false;
			P->SetVisibility(false);
			Into.Add(P);
		}
	};

	float Z = 0.f;
	for (int32 i = 0; i < L.Stages.Num(); ++i)
	{
		const FISEngineStage& St = L.Stages[i];
		TArray<UStaticMeshComponent*> Parts;
		const float D = (float)St.DiameterM, Len = (float)St.LengthM;
		StageBaseZ.Add(Z);
		Part(MCyl, D, Len, Z + Len * 0.5f, FVector::ZeroVector, St.Color, Parts);
		// engines: the real count from the stage name ("9 Rutherford", "9 Merlin") - a ring of bells around a centre one
		int32 NEng = 1;
		for (int32 c = 0; c < St.Name.Len(); ++c) if (FChar::IsDigit(St.Name[c])) { NEng = FMath::Clamp(FCString::Atoi(*St.Name.Mid(c)), 1, 33); break; }
		const float Bell = NEng > 1 ? D * 0.24f : D * 0.6f;
		Part(MCyl, D * 0.92f, 0.12f, Z - 0.06f, FVector::ZeroVector, EngineGrey, Parts, 0.8f);                       // thrust plate
		for (int32 e = 0; e < NEng; ++e)
		{
			const FVector O = (NEng == 1 || e == 0) ? FVector::ZeroVector : FRotator(0, 360.f * (e - 1) / (NEng - 1), 0).Vector() * (D * 0.33f);
			Part(MCone, Bell, Bell * 1.25f, Z - Bell * 0.62f, O, EngineGrey, Parts, 0.85f);
		}
		if (i > 0) Part(MCyl, D * 1.01f, FMath::Min(1.5f, Len * 0.1f), Z + FMath::Min(0.75f, Len * 0.05f), FVector::ZeroVector, Band, Parts); // interstage
		Part(MCyl, D * 1.006f, 0.12f, Z + Len * 0.62f, FVector::ZeroVector, Band, Parts);                          // tank weld line
		Part(MCyl, D * 1.006f, 0.12f, Z + Len * 0.25f, FVector::ZeroVector, Band, Parts);
		Part(MCube, D * 0.12f, Len * 0.86f, Z + Len * 0.5f, FVector(0, D * 0.51f, 0), Band, Parts);                   // cable raceway
		if (i == 0 && L.Name.Contains(TEXT("Falcon")))
			for (int32 g = 0; g < 4; ++g)   // grid fins at the top of the first stage
				Part(MCube, D * 0.32f, D * 0.32f, Z + Len - D * 0.4f, FRotator(0, 90.f * g, 0).Vector() * (D * 0.6f), EngineGrey, Parts, 0.6f)->SetRelativeScale3D(FVector(D * 0.32f, 0.06f, D * 0.32f) / 1.f);
		StageVis.Add(Parts);
		Z += Len;
	}
	// payload fairing (or a capsule-like nose)
	const float FD = (float)FMath::Max(L.FairingDiameterM, L.Stages.Last().DiameterM), FL = (float)L.FairingLengthM;
	// ogive fairing: cylinder, two tapering cone sections and a rounded tip, with the split line visible
	const FLinearColor FairC(0.95f, 0.95f, 0.96f);
	Part(MCyl, FD, FL * 0.5f, Z + FL * 0.25f, FVector::ZeroVector, FairC, FairingVis);
	Part(MCone, FD, FL * 0.32f, Z + FL * 0.5f + FL * 0.16f, FVector::ZeroVector, FairC, FairingVis);
	Part(MCone, FD * 0.55f, FL * 0.3f, Z + FL * 0.82f - FL * 0.04f, FVector::ZeroVector, FairC, FairingVis);
	Part(MSphere, FD * 0.16f, FD * 0.16f, Z + FL * 0.93f, FVector::ZeroVector, FairC, FairingVis);
	Part(MCube, 0.06f, FL * 0.82f, Z + FL * 0.41f, FVector(FD * 0.5f, 0, 0), Band, FairingVis)->SetRelativeScale3D(FVector(0.06f, FD * 0.02f, FL * 0.82f));
	RocketLenCm = (Z + FL) * 100.f;
	// strap-on boosters around the first stage
	const FISEngineStage& S0 = L.Stages[0];
	for (int32 b = 0; b < S0.Boosters; ++b)
	{
		const FVector Dir = FRotator(0, 360.f * b / S0.Boosters + 90.f, 0).Vector();
		const FVector Off = Dir * (float)(S0.DiameterM * 0.5 + S0.BoosterDiameterM * 0.5);
		const float Bd = (float)S0.BoosterDiameterM, Bl = (float)S0.BoosterLengthM;
		Part(MCyl, Bd, Bl, Bl * 0.5f, Off, S0.bBoosterSolid ? FLinearColor(0.93f, 0.93f, 0.92f) : S0.Color, BoosterVis);
		Part(MCone, Bd, Bd * 1.6f, Bl + Bd * 0.8f, Off, S0.bBoosterSolid ? FLinearColor(0.93f, 0.93f, 0.92f) : Band, BoosterVis);
		Part(MCone, Bd * 0.6f, Bd * 0.3f, -Bd * 0.1f, Off, EngineGrey, BoosterVis, 0.8f);
		Plume(PlumeBoost, Off);
	}
	Plume(PlumeCore, FVector::ZeroVector);
}

void AISSpaceScene::TickFlight(float Dt)
{
	if (!bFlight || !Sim.L) return;
	// time: slow motion to 10x physics warp under thrust / in the air; up to 100x when coasting in vacuum
	const bool bQuiet = Sim.ThrustN <= 0.0 && Sim.AltM > 90000.0;
	FlightWarp = FMath::Clamp(FlightWarp, 0.05f, bQuiet ? 100.f : 10.f);
	double Left = (double)Dt * FlightWarp;
	int32 Guard = 0;
	while (Left > 1e-6 && Guard++ < 6000) { const double H = FMath::Min(Left, 0.02); Sim.Step(H); Left -= H; }

	// launch camera: watch lift-off from the open side, then swing in behind the climbing rocket (unless the player
	// turned the view themselves)
	if (!bFlightCamUser && Sim.AltM > 400.0)
	{
		FollowYaw = FMath::FInterpTo(FollowYaw, 65.f, Dt, 0.35f);   // side-behind: rocket and plume both in view
		FollowPitch = FMath::FInterpTo(FollowPitch, -10.f, Dt, 0.35f);
	}
	// pose (metres in the launch plane -> world cm)
	const FVector P = (FlightU * (float)Sim.R.Y + FlightDn * (float)Sim.R.X) * 100.f;
	const FVector2D Nd = Sim.NoseDir();
	const FVector Dir = (FlightU * (float)Nd.Y + FlightDn * (float)Nd.X).GetSafeNormal();
	RocketRoot->SetRelativeLocation(P);
	// vibration: engines rumble near the ground (sound reflecting off the pad), the air buffets it through max-Q
	{
		const double Q01 = FMath::Clamp(Sim.QPa / FMath::Max(1.0, Sim.L->MaxQPa), 0.0, 1.2);
		const double Ground = FMath::Clamp(1.0 - Sim.AltM / 1500.0, 0.0, 1.0);
		const float Goal = Sim.IsFlying() && Sim.ThrustN > 0.0 ? (float)FMath::Clamp(0.15 + 0.6 * Ground + 0.7 * Q01 * Q01 + 0.02 * FMath::Abs(Sim.AoADeg), 0.0, 1.0) : 0.f;
		FlightShake = FMath::FInterpTo(FlightShake, Goal, Dt, 3.f);
	}
	const float Tn = (float)Sim.T;
	const FRotator Wob(FlightShake * 0.35f * FMath::Sin(Tn * 23.f) * FMath::Sin(Tn * 3.1f), 0.f, FlightShake * 0.35f * FMath::Sin(Tn * 19.f + 1.f) * FMath::Sin(Tn * 2.3f));
	RocketRoot->SetRelativeRotation((FRotationMatrix::MakeFromZX(Dir, FlightDn).ToQuat() * Wob.Quaternion()).Rotator());

	// trajectory lines: where it has been (history) and where it goes if the engines stopped now (prediction)
	if (FlightLines)
	{
		FlightLines->Flush();
		const FTransform& Xf = SceneRoot->GetComponentTransform();
		auto W = [&](const FVector2D& Q) { return Xf.TransformPosition((FlightU * (float)Q.Y + FlightDn * (float)Q.X) * 100.f); };
		if (Sim.AltM > 2000.0)
		{
			const TArray<FVector2D>& H = Sim.History;
			for (int32 i = 1; i < H.Num(); ++i) FlightLines->DrawLine(W(H[i - 1]), W(H[i]), FLinearColor(1.f, 0.55f, 0.15f), SDPG_World, 0.f, 0.f);
			if (H.Num()) FlightLines->DrawLine(W(H.Last()), W(Sim.R), FLinearColor(1.f, 0.55f, 0.15f), SDPG_World, 0.f, 0.f);
			if (Sim.IsFlying() || Sim.Phase == EISFlightPhase::Orbit)
			{
				Sim.Predict(FlightPred, 300);
				for (int32 i = 1; i < FlightPred.Num(); ++i) FlightLines->DrawLine(W(FlightPred[i - 1]), W(FlightPred[i]), FLinearColor(0.3f, 0.85f, 1.f), SDPG_World, 0.f, 0.f);
			}
		}
	}

	// what is still attached
	for (int32 i = 0; i < StageVis.Num(); ++i) for (UStaticMeshComponent* C : StageVis[i]) if (IsValid(C)) C->SetVisibility(i >= Sim.StageIdx && Sim.Phase != EISFlightPhase::Failed);
	for (UStaticMeshComponent* C : BoosterVis) if (IsValid(C)) C->SetVisibility(Sim.bBoostersOn && Sim.Phase != EISFlightPhase::Failed);
	for (UStaticMeshComponent* C : FairingVis) if (IsValid(C)) C->SetVisibility(Sim.bFairingOn && Sim.Phase != EISFlightPhase::Failed);

	// plumes: length with throttle, width with altitude (the exhaust balloons out in vacuum)
	const FISEngineStage& St = Sim.L->Stages[FMath::Clamp(Sim.StageIdx, 0, Sim.L->Stages.Num() - 1)];
	const double Pr = FISRocketSim::PressureRatio(Sim.AltM);
	const bool bCoreOn = Sim.IsFlying() && Sim.StageProp > 0.0 && (St.bSolid || Sim.Throttle > 0.0) && Sim.ThrustN > 0.0;
	const float Flick = 0.93f + 0.14f * FMath::FRand();
	const float BaseZ = StageBaseZ.IsValidIndex(Sim.StageIdx) ? StageBaseZ[Sim.StageIdx] : 0.f;
	for (int32 k = 0; k < PlumeCore.Num(); ++k)
	{
		UStaticMeshComponent* C = PlumeCore[k];
		if (!IsValid(C)) continue;
		C->SetVisibility(bCoreOn);
		if (!bCoreOn) continue;
		const float Thr = St.bSolid ? 1.f : (float)FMath::Clamp(Sim.Throttle, St.MinThrottle, 1.0);
		const float Len = (float)St.DiameterM * (k == 0 ? 5.f : 2.f) * (0.55f + 0.45f * Thr) * (1.f + 0.35f * (float)(1.0 - Pr)) * Flick;
		const float Wid = (float)St.DiameterM * (k == 0 ? 0.9f : 0.45f) * (1.f + 0.9f * (float)(1.0 - Pr));   // a vacuum plume spreads, but must not swallow the rocket
		C->SetRelativeLocation(FVector(0, 0, (BaseZ - (float)St.DiameterM * 0.25f - Len * 0.5f) * 100.f));
		C->SetRelativeScale3D(FVector(Wid, Wid, Len));
	}
	const FISEngineStage& S0 = Sim.L->Stages[0];
	for (int32 k = 0; k < PlumeBoost.Num(); ++k)
	{
		UStaticMeshComponent* C = PlumeBoost[k];
		if (!IsValid(C)) continue;
		const bool bOn = Sim.bBoostersOn && Sim.BoosterProp > 0.0 && Sim.IsFlying();
		C->SetVisibility(bOn);
		if (!bOn) continue;
		const float Len = (float)S0.BoosterDiameterM * ((k % 2) == 0 ? 5.f : 2.f) * (1.f + 0.35f * (float)(1.0 - Pr)) * Flick;
		const float Wid = (float)S0.BoosterDiameterM * ((k % 2) == 0 ? 0.9f : 0.45f) * (1.f + 0.9f * (float)(1.0 - Pr));
		const FVector Base = C->GetRelativeLocation();
		C->SetRelativeLocation(FVector(Base.X, Base.Y, (-(float)S0.BoosterDiameterM * 0.25f - Len * 0.5f) * 100.f));
		C->SetRelativeScale3D(FVector(Wid, Wid, Len));
	}

	// stars only above the blue sky
	if (Sky) { const bool bStars = Sim.AltM > 50000.0; if (Sky->IsVisible() != bStars) Sky->SetVisibility(bStars); }
	// the air: a real atmosphere (Rayleigh/Mie) around the true-scale Earth, visible near the ground
	if (SkyAtmo && !SkyAtmo->IsVisible()) SkyAtmo->SetVisibility(true);

	if (Sim.Phase == EISFlightPhase::Orbit)
	{
		FlightEndT += Dt;
		if (FlightEndT > 5.f) EndFlight();
	}
}

void AISSpaceScene::EndFlight()
{
	if (!bFlight) return;
	UE_LOG(LogTemp, Log, TEXT("[FLIGHT] end phase %d  %.0f x %.0f km  %s"), (int32)Sim.Phase, Sim.ApoKm, Sim.PeriKm, *Sim.FailReason);
	bFlight = false;
	LaunchT = -1.f;
	if (FlightLines) FlightLines->Flush();
	if (RocketRoot) RocketRoot->SetVisibility(false, true);
	SatRoot->SetVisibility(true, true);
	Sky->SetVisibility(true);
	for (UStaticMeshComponent* C : PadVis) if (IsValid(C)) C->SetVisibility(false);
	CamMin = LastSatSize * 1.2f;
	CamDistGoal = LastSatSize * 5.f;
	FollowPitch = -28.f; FollowYaw = 25.f; bFlightCamUser = false;   // after separation: spacecraft in front, Earth filling the lower part of the view
}

void AISSpaceScene::SkipLaunch()
{
	if (bFlight)
	{
		for (int32 i = 0; i < 400000 && (Sim.Phase == EISFlightPhase::Countdown || Sim.Phase == EISFlightPhase::OnPad || Sim.IsFlying()); ++i)
		{
			if (Sim.Phase == EISFlightPhase::OnPad) Sim.StartCountdown(0.0);
			Sim.Step(0.02);
		}
		EndFlight();
		return;
	}
	if (LaunchT >= 0.f) LaunchT = 0.999f;
}
