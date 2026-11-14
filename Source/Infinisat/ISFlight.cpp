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
		Pad(MCyl, FVector(0, 0, -6.0f), FVector(8000.f, 8000.f, 12.f), FLinearColor(0.16f, 0.22f, 0.1f));              // ground (4 km)
		Pad(MCyl, FVector(0, 0, -0.5f), FVector(90.f, 90.f, 1.f), FLinearColor(0.55f, 0.55f, 0.53f));                  // concrete pad
		Pad(MCube, FVector(0, 0, -0.9f), FVector(D * 2.2f, 30.f, 0.4f), FLinearColor(0.08f, 0.08f, 0.08f));            // flame trench
		Pad(MCube, FVector(-D * 2.6f, 0, Len * 0.55f), FVector(5.f, 5.f, Len * 1.1f), FLinearColor(0.45f, 0.18f, 0.1f)); // launch tower
		Pad(MCube, FVector(-D * 1.6f, 0, Len * 0.8f), FVector(D * 1.8f, 1.5f, 1.2f), FLinearColor(0.4f, 0.4f, 0.42f));   // crew/umbilical arm
		for (int32 k = 0; k < 4; ++k) Pad(MCyl, FVector((k < 2 ? -1.f : 1.f) * 30.f, (k % 2 ? -1.f : 1.f) * 30.f, 25.f), FVector(1.2f, 1.2f, 50.f), FLinearColor(0.6f, 0.6f, 0.62f)); // lightning masts
	}
	SatRoot->SetVisibility(false, true);
	LaunchT = 0.5f;           // "launching": the follow camera tracks RocketRoot, mission time waits
	bFollow = true;
	CamMin = RocketLenCm * 0.4f;
	CamDistGoal = RocketLenCm * 3.6f; CamDist = CamDistGoal;
	FollowYaw = 35.f; FollowPitch = -6.f;
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
		Part(MCone, D * 0.62f, D * 0.32f, Z - D * 0.12f, FVector::ZeroVector, EngineGrey, Parts, 0.8f);          // engine bell(s)
		if (i > 0) Part(MCyl, D * 1.01f, FMath::Min(1.2f, Len * 0.08f), Z + 0.3f, FVector::ZeroVector, Band, Parts); // interstage band
		Part(MCyl, D * 1.005f, 0.25f, Z + Len * 0.62f, FVector::ZeroVector, Band, Parts);                           // tank ring line
		StageVis.Add(Parts);
		Z += Len;
	}
	// payload fairing (or a capsule-like nose)
	const float FD = (float)FMath::Max(L.FairingDiameterM, L.Stages.Last().DiameterM), FL = (float)L.FairingLengthM;
	Part(MCyl, FD, FL * 0.55f, Z + FL * 0.275f, FVector::ZeroVector, FLinearColor(0.95f, 0.95f, 0.96f), FairingVis);
	Part(MCone, FD, FL * 0.45f, Z + FL * 0.55f + FL * 0.225f, FVector::ZeroVector, FLinearColor(0.95f, 0.95f, 0.96f), FairingVis);
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

	// pose (metres in the launch plane -> world cm)
	const FVector P = (FlightU * (float)Sim.R.Y + FlightDn * (float)Sim.R.X) * 100.f;
	const FVector2D Td = Sim.ThrustDir();
	const FVector Dir = (FlightU * (float)Td.Y + FlightDn * (float)Td.X).GetSafeNormal();
	RocketRoot->SetRelativeLocation(P);
	RocketRoot->SetRelativeRotation(FRotationMatrix::MakeFromZX(Dir, FlightDn).Rotator());

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
		const float Len = (float)St.DiameterM * (k == 0 ? 5.f : 2.f) * (0.55f + 0.45f * Thr) * (1.f + 1.2f * (float)(1.0 - Pr)) * Flick;
		const float Wid = (float)St.DiameterM * (k == 0 ? 0.9f : 0.45f) * (1.f + 3.5f * (float)(1.0 - Pr));
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
		const float Len = (float)S0.BoosterDiameterM * ((k % 2) == 0 ? 5.f : 2.f) * (1.f + 1.2f * (float)(1.0 - Pr)) * Flick;
		const float Wid = (float)S0.BoosterDiameterM * ((k % 2) == 0 ? 0.9f : 0.45f) * (1.f + 3.5f * (float)(1.0 - Pr));
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
	bFlight = false;
	LaunchT = -1.f;
	if (RocketRoot) RocketRoot->SetVisibility(false, true);
	SatRoot->SetVisibility(true, true);
	Sky->SetVisibility(true);
	for (UStaticMeshComponent* C : PadVis) if (IsValid(C)) C->SetVisibility(false);
	CamMin = LastSatSize * 1.2f;
	CamDistGoal = LastSatSize * 5.f;
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
