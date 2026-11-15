// Infinisat - deep-space missions in the 3D view. From the parking orbit the rocket really reached: the upper stage relights
// to leave Earth, the satellite springs off and unfolds, then flies the real transfer (ISTrajectory) on the mission clock.
//   Moon: one Earth-centred true-scale scene (the Moon moves on its real orbit) - departure, cruise, insertion, science orbit.
//   Jupiter / interstellar: departure in the Earth scene, the cruise in the solar-system (Cosmos) view locked to the mission
//   clock, Jupiter's arrival in a Jupiter-centred true-scale scene (Galilean moons on their real orbits).
#include "ISScene.h"
#include "ISOrbit.h"
#include "ISSim.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Camera/CameraActor.h"

namespace
{
	const double KmCm = 1.0e5;     // true scale
	const double DaySec = 86400.0;
	FVector Cm(const FVector3d& Km) { return FVector(Km * KmCm); }
	const FLinearColor Flown(1.f, 0.55f, 0.15f), Ahead(0.3f, 0.85f, 1.f);
}

int32 AISSpaceScene::VoyCentre() const { return ISOrbit::FindBody(bJupiterScene ? TEXT("Jupiter") : TEXT("Earth")); }

int32 AISSpaceScene::VoyagePhase() const
{
	if (!bVoyage || !Voy.bValid || !bVoySepDone || ViewDest == EISDest::Moon) return 0;   // the Moon trip stays in one scene
	const double T = MissionDay * DaySec;
	if (Voy.Target >= 0 && T >= Voy.ArriveT - 20.0 * DaySec) return 2;
	return Voy.Legs.IsValidIndex(1) && T >= Voy.Legs[1].T0 + DaySec ? 1 : 0;   // a day after the departure burn: far from Earth
}

void AISSpaceScene::PlanVoyage()
{
	if (!Sim.L) return;
	// the parking orbit in the scene's Earth-centred frame, exactly where the rocket is drawn (Earth-fixed view, see TickFlight)
	const double SpinA = FISRocketSim::OmegaE * FMath::Cos(FMath::DegreesToRadians(Sim.L->SiteLatDeg)) * FMath::Sin(FMath::DegreesToRadians(Sim.LaunchAzDeg)) * FMath::Max(0.0, Sim.T);
	const double C = FMath::Cos(SpinA), S = FMath::Sin(SpinA);
	auto Fixed = [C, S](const FVector2D& Q) { return FVector2D(Q.X * C - Q.Y * S, Q.Y * C + Q.X * S); };
	const FVector2D R2 = Fixed(Sim.R), V2 = Fixed(Sim.V);
	const FVector3d R = FVector3d(FlightU * R2.Y + FlightDn * R2.X) / 1000.0 + FVector3d(FlightBase) / 1.0e5;   // m -> km; FlightBase: the real (Cesium) ground offset, cm
	const FVector3d V = FVector3d(FlightU * V2.Y + FlightDn * V2.X) / 1000.0;

	const double Dep = ISSim::DepartDay(ViewDest);
	if (ViewDest == EISDest::Moon) Voy = FISTrajectory::Moon(R, V, Dep, ISSim::CruiseDays(ViewDest), VoyAltKm);
	else if (ViewDest == EISDest::Jupiter) Voy = FISTrajectory::Jupiter(R, V, Dep, ISSim::CruiseDays(ViewDest), VoyAltKm);
	else Voy = FISTrajectory::Interstellar(R, V, Dep, VoyEndDay);

	// burn lengths from the real hardware: the upper stage's vacuum thrust on what is left of the vehicle; the satellite's own
	// insertion engine is small (~0.5 m/s^2, like LRO's or Juno's main engine)
	const FISEngineStage& St = Sim.L->Stages[FMath::Clamp(Sim.StageIdx, 0, Sim.L->Stages.Num() - 1)];
	const double Acc = St.ThrustVacN / FMath::Max(1.0, Sim.MassKg);
	VoyBurnDur = Voy.Burns.Num() ? FMath::Clamp(Voy.Burns[0].DvMs / FMath::Max(Acc, 0.1), 60.0, 1800.0) : 300.0;
	VoyInsDur = Voy.Target >= 0 && Voy.Burns.Num() > 1 ? FMath::Clamp(Voy.Burns.Last().DvMs / 0.5, 300.0, 3600.0) : 0.0;
	bVoyPlannedNew = Voy.bValid;
	bVoySepDone = false;
	VoySepStartT = -1.0;
	VoyCamMode = -1;

	FString B;
	for (const FISBurn& X : Voy.Burns) B += FString::Printf(TEXT(" %s %.0f m/s @ day %.3f;"), *X.What, X.DvMs, X.T / DaySec);
	UE_LOG(LogTemp, Log, TEXT("[TRAJ] dest %d plan %s: parking %.0f km, burn %.0f s, insertion %.0f s;%s"), (int32)ViewDest, Voy.bValid ? TEXT("ok") : TEXT("FAILED"),
		R.Size() - 6371.0, VoyBurnDur, VoyInsDur, *B);
}

void AISSpaceScene::EnsureThruster()
{
	// the satellite's own engine for the orbit insertion: a small pale glow at the bottom of the bus
	if (SatThruster || !MatGlow) return;
	SatThruster = AddMesh(SatRoot, MCone, FTransform(FRotator(180, 0, 0), FVector(0, 0, -SatHalfH - 40.f), FVector(0.3f, 0.3f, 0.9f)), FLinearColor(0, 0, 0, 0), true, 0.f, nullptr);
	if (!SatThruster) return;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatGlow, SatThruster);
	M->SetTextureParameterValue(TEXT("Tex"), CosmosTex(TEXT("white"), 4, 4, [](float, float) { return FLinearColor::White; }));
	M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.75f, 0.85f, 1.f));   // hydrazine/MMH: pale, nearly invisible in reality
	M->SetScalarParameterValue(TEXT("Intensity"), 2.f);
	SatThruster->SetMaterial(0, M);
	SatThruster->CastShadow = false;
	SatThruster->SetVisibility(false);
}

// Jupiter at true scale: the planet, the four Galilean moons on their real orbits, sunlight from where the Sun really is
void AISSpaceScene::BuildJupiterScene()
{
	bJupiterScene = true;
	bTrueScale = true;
	ViewK = (float)KmCm;
	const int32 J = ISOrbit::FindBody(TEXT("Jupiter"));
	const FISBody& Jb = ISOrbit::Bodies()[J];
	Sky->SetWorldScale3D(FVector(1.0e11f));   // star sphere radius 50 million km: the approach starts millions of km out
	SetPlanet(Planet, TEXT("/Game/Infinisat/Textures/T_Jupiter2.T_Jupiter2"), (float)(Jb.Radius * KmCm), FVector::ZeroVector);
	if (UStaticMesh* Hi = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi"))) Planet->SetStaticMesh(Hi);
	const FVector3d ToSun = -ISOrbit::BodyPos(J, Voy.Epoch + MissionDay * DaySec);
	SunLight->SetWorldRotation((-FVector(ToSun)).Rotation());   // light travels from the Sun to Jupiter
	if (UStaticMesh* Hi = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi")))
		for (const ISNBody::FMoon& M : ISNBody::Moons())
		{
			if (FString(M.Parent) != TEXT("Jupiter")) continue;
			UStaticMeshComponent* C = AddMesh(SceneRoot, Hi, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector((float)(M.R * KmCm / 50.0))), FLinearColor::White, false, 0.f, nullptr);
			if (!C) continue;
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MatPlanet, C);
			if (UTexture2D* T = MoonTexture(M.Name)) MID->SetTextureParameterValue(TEXT("Tex"), T);
			C->SetMaterial(0, MID);
			C->CastShadow = false;
			VoyMoons.Add(C);
		}
	const double RJ = Jb.Radius * KmCm;
	AddLabel(TEXT("Jupiter: 11 Earths across, 318 Earth masses"), FVector(0, 0, RJ * 1.25), FColor::White);
	AddLabel(TEXT("Radiation belts: deadly for electronics"), FVector(-RJ * 3.2, 0, RJ * 0.3), FColor(255, 110, 110));
	UE_LOG(LogTemp, Log, TEXT("[TRAJ] Jupiter scene: planet visible %d at %s scale %.3g, mesh %s, %d moons"), Planet->IsVisible() ? 1 : 0, *Planet->GetComponentLocation().ToString(),
		Planet->GetComponentScale().X, Planet->GetStaticMesh() ? *Planet->GetStaticMesh()->GetName() : TEXT("none"), VoyMoons.Num());
	OrbitRadiusUU = (float)((Jb.Radius + VoyAltKm) * KmCm);
	PlanetRUU = (float)RJ;
	OrbitInc = 90.f;
	SetCameraTarget(FVector::ZeroVector, (float)(RJ * 40.0), 210.f, -20.f, (float)(RJ * 0.2), (float)(RJ * 2000.0));
}

void AISSpaceScene::TickVoyage(float Dt)
{
	if (!Voy.bValid || !RocketRoot) { BodyC = FollowN = FVector::ZeroVector; return; }   // still on the pad / in the launch
	const double T = MissionDay * DaySec;
	const int32 EarthB = ISOrbit::FindBody(TEXT("Earth")), MoonB = ISOrbit::FindBody(TEXT("Moon")), Centre = VoyCentre();
	const FISBody& Cb = ISOrbit::Bodies()[Centre];

	// the Moon on its real orbit (the plan's epoch put it where the transfer meets it) / the Galilean moons
	FVector MoonPos = FVector::ZeroVector;
	if (!bJupiterScene)
	{
		MoonPos = Cm(ISOrbit::BodyPos(MoonB, Voy.Epoch + T, true));
		Moon->SetRelativeLocation(MoonPos);
		if (MoonLabel) MoonLabel->SetRelativeLocation(MoonPos + FVector(0, 0, 4000.0 * KmCm));
	}
	else
	{
		int32 k = 0;
		for (const ISNBody::FMoon& M : ISNBody::Moons())
		{
			if (FString(M.Parent) != TEXT("Jupiter") || !VoyMoons.IsValidIndex(k)) continue;
			const double Th = M.Phase + FMath::Sqrt(Cb.Mu / (M.A * M.A * M.A)) * (Voy.Epoch + T), I = FMath::DegreesToRadians(M.IncDeg);
			VoyMoons[k++]->SetRelativeLocation(Cm(FVector3d(FMath::Cos(Th), FMath::Sin(Th) * FMath::Cos(I), FMath::Sin(Th) * FMath::Sin(I)) * M.A));
		}
	}

	// the craft, seen from the centre of this scene
	FVector3d Rr, Vv;
	const int32 B = Voy.State(T, Rr, Vv);
	const FVector P = Cm(Voy.PosAround(Centre, T));
	FVector3d Ve = Vv;
	if (B == MoonB) Ve += ISOrbit::BodyVel(MoonB, Voy.Epoch + T, true);
	else if (B != Centre) Ve += (B > 0 ? ISOrbit::BodyVel(B, Voy.Epoch + T) : FVector3d::ZeroVector) - ISOrbit::BodyVel(Centre, Voy.Epoch + T);
	const FVector Dir = FVector(Ve).GetSafeNormal();
	BodyC = B == MoonB ? MoonPos : FVector::ZeroVector;
	FollowN = FVector(FVector3d::CrossProduct(Rr, Vv)).GetSafeNormal();
	const bool bFar = P.Size() > 3.0e7 * KmCm;   // beyond ~30 million km the Earth scene cannot hold it (that part is the Cosmos view's)

	// timeline
	const FISBurn& DepB = Voy.Burns[0];
	const double B0 = DepB.T - VoyBurnDur * 0.5, B1 = DepB.T + VoyBurnDur * 0.5, SepAt = B1 + 60.0;
	const bool bIns = Voy.Target >= 0 && Voy.Burns.Num() > 1;
	const double I0 = Voy.ArriveT - VoyInsDur * 0.5, I1 = Voy.ArriveT + VoyInsDur * 0.5;
	const float Sc = SatRoot->GetRelativeScale3D().Z;
	const float TopCm = StageBaseZ.Num() ? (StageBaseZ.Last() + (float)Sim.L->Stages.Last().LengthM) * 100.f : RocketLenCm * 0.85f;
	const float AdapterCm = TopCm + (SatHalfH + 13.f) * Sc;

	// only the upper stage flies on: lower stages, boosters and the fairing are long gone (a recursive show would bring them back)
	auto ShowUpperStage = [this](bool bOn)
	{
		RocketRoot->SetVisibility(bOn, true);
		if (!bOn) return;
		for (int32 i = 0; i + 1 < StageVis.Num(); ++i) for (UStaticMeshComponent* C : StageVis[i]) if (IsValid(C)) C->SetVisibility(false);
		for (TArray<UStaticMeshComponent*>* A : { &FairingVis, &BoosterVis, &PlumeBoost, &PlumeCore }) for (UStaticMeshComponent* C : *A) if (IsValid(C)) C->SetVisibility(false);
	};

	if (!bVoySepDone)
	{
		// the stack, nose along the flight direction: stage below, the stowed satellite on its adapter on top
		if (SepT < 0.f) SepT = 0.f;
		if (SepRoot && !SepRoot->IsVisible()) SepRoot->SetVisibility(true, true);
		const bool bLeftovers = (FairingVis.Num() && IsValid(FairingVis[0]) && FairingVis[0]->IsVisible())
			|| (StageVis.Num() > 1 && StageVis[0].Num() && IsValid(StageVis[0][0]) && StageVis[0][0]->IsVisible());
		if (!RocketRoot->IsVisible() || bLeftovers) ShowUpperStage(true);
		if (SatRoot->IsVisible()) SatRoot->SetVisibility(false, true);
		if (T >= SepAt && VoySepStartT < 0.0) { VoySepStartT = SepAt; UE_LOG(LogTemp, Log, TEXT("[TRAJ] separation at day %.3f"), MissionDay); }   // when it really happened (fast-forward safe)
		if (VoySepStartT >= 0.0 && T > SepAt + 3600.0) SepT = FMath::Max(SepT, 8.f);   // fast-forwarded past it: finish at once
		const float Free = FMath::Max(0.f, SepT - 1.f) * 100.f;
		RocketRoot->SetRelativeLocationAndRotation(P - Dir * (AdapterCm + Free), FRotationMatrix::MakeFromZX(Dir, P.GetSafeNormal()).ToQuat());
		VoyCamOff = RocketRoot->GetUpVector() * -0.5f * (AdapterCm + Free - (StageBaseZ.Num() ? StageBaseZ.Last() * 100.f : 0.f));
		TickSeparation(VoySepStartT >= 0.0 ? Dt : 0.f);   // springs + wing accordion; the copy ends exactly on the trajectory point

		// departure burn: the upper stage's vacuum plume (wide, the exhaust balloons out in vacuum)
		const bool bBurn = T >= B0 && T < B1;
		const int32 Si = Sim.L->Stages.Num() - 1;
		const float D = (float)Sim.L->Stages[Si].DiameterM, BaseZ = StageBaseZ.IsValidIndex(Si) ? StageBaseZ[Si] : 0.f;
		const float Flick = 0.93f + 0.14f * FMath::FRand();
		for (int32 k = 0; k < PlumeCore.Num(); ++k)
		{
			UStaticMeshComponent* C = PlumeCore[k];
			if (!IsValid(C)) continue;
			C->SetVisibility(bBurn);
			if (!bBurn) continue;
			const float Len = D * (k == 0 ? 5.f : 2.f) * 1.35f * Flick, Wid = D * (k == 0 ? 0.9f : 0.45f) * 1.9f;
			C->SetRelativeLocation(FVector(0, 0, (BaseZ - D * 0.25f - Len * 0.5f) * 100.f));
			C->SetRelativeScale3D(FVector(Wid, Wid, Len));
		}
		if (SepT >= 8.f)
		{
			// hand over from the animated copy to the real satellite, same place, same pose, wings out
			bVoySepDone = true;
			VoyAtt = SepRoot ? SepRoot->GetComponentQuat() : FQuat::Identity;
			SatRoot->SetWorldRotation(VoyAtt);
			if (SepRoot) SepRoot->SetVisibility(false, true);
			SepT = -1.f;
		}
	}
	if (bVoySepDone)
	{
		if (SatRoot->IsVisible() == bFar) SatRoot->SetVisibility(!bFar, true);
		SatRoot->SetRelativeLocation(bFar ? FVector::ZeroVector : P);
		EnsureThruster();
		// spent stage: keeps drifting back at ~1 m/s for half an hour, then it is out of the picture
		const double Since = T - FMath::Max(VoySepStartT, 0.0);
		const bool bStage = Since < 1800.0 && !bJupiterScene && !bFar;
		if (RocketRoot->IsVisible() != bStage) ShowUpperStage(bStage);
		for (UStaticMeshComponent* C : PlumeCore) if (IsValid(C)) C->SetVisibility(false);
		if (bStage) RocketRoot->SetRelativeLocation(P - Dir * (AdapterCm + 700.f + (float)Since * 100.f));
		// attitude: as it separated during cruise; engine forward (thrust against the motion) before the insertion burn;
		// instruments down at the destination afterwards
		FQuat Want = VoyAtt;
		if (bIns && T >= I0 - 600.0 && T < I1) Want = FRotationMatrix::MakeFromZ(-Dir).ToQuat();
		else if (bIns && T >= I1) Want = (BodyC - P).Rotation().Quaternion();
		SatRoot->SetWorldRotation(FQuat::Slerp(SatRoot->GetComponentQuat(), Want, FMath::Clamp(Dt * 0.8f, 0.f, 1.f)));
		if (SatThruster)
		{
			const bool bOn = bIns && T >= I0 && T < I1;
			SatThruster->SetVisibility(bOn);
			if (bOn) SatThruster->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.9f) * (0.9f + 0.2f * FMath::FRand()));
		}
	}

	if (bVoySepDone) VoyCamOff = FMath::VInterpTo(VoyCamOff, FVector::ZeroVector, Dt, 1.5f);   // glide from the stack to the satellite

	// camera: close on the stack for the burn and the separation; the whole system for the cruise (Earth + Moon, or Jupiter +
	// the approach); close again for the arrival. The player can still switch with "Orbit map".
	const bool bMoonTrip = ViewDest == EISDest::Moon;
	int32 Mode = bVoySepDone ? 0 : 2;   // 2: the stack (stage + stowed satellite) seen from the side
	if (bJupiterScene) Mode = T < Voy.ArriveT - 2.0 * DaySec ? 1 : 0;
	else if (bVoySepDone && T > SepAt + 1800.0 && (!bIns || !bMoonTrip || T < Voy.ArriveT - 6.0 * 3600.0)) Mode = 1;
	// Moon trip map frame: Earth, the Moon now and the Moon on arrival day (the route ends there, wherever the launch site
	// put the transfer plane)
	FBox MoonRoute(ForceInit);
	FVector MoonArr = FVector::ZeroVector;
	if (bMoonTrip)
	{
		MoonArr = Cm(ISOrbit::BodyPos(MoonB, Voy.Epoch + Voy.ArriveT, true));
		MoonRoute += FVector::ZeroVector; MoonRoute += MoonPos; MoonRoute += MoonArr;
	}
	if (Mode != VoyCamMode)
	{
		VoyCamMode = Mode;
		if (Mode == 1)
		{
			bFollow = false;
			MapDist = (float)FMath::Min(bMoonTrip ? MoonRoute.GetSize().GetMax() * 2.6 : FMath::Max(P.Size() * 2.4, 2.0e6 * KmCm), 5.0e7 * KmCm);
			CamMin = (float)(2000.0 * KmCm); CamMax = (float)(5.0e7 * KmCm);
			CamDistGoal = MapDist; CamPitch = -60.f;   // from above the orbit plane: the whole route in view
			FVector Nm = FVector::CrossProduct(MoonPos, MoonArr).GetSafeNormal();
			if (bMoonTrip && !Nm.IsNearlyZero())
			{
				// look down on the Moon's orbit plane (20 deg off its axis), not down the world's Z: launched from the real
				// site, that plane can be tilted far from the world's horizontal
				if (Nm.Z < 0.f) Nm = -Nm;
				const FVector L = (-Nm * 0.94f + MoonRoute.GetCenter().GetSafeNormal() * 0.34f).GetSafeNormal();
				CamPitch = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(L.Z, -1.f, 1.f)));
				CamYaw = FMath::RadiansToDegrees(FMath::Atan2(L.Y, L.X));
			}
		}
		else
		{
			bFollow = true;
			CamMin = LastSatSize * 1.2f; CamMax = (float)(5.0e7 * KmCm);
			// the stack: far enough for stage + satellite (the stage is ~10 m, the satellite ~2 m)
			CamDistGoal = Mode == 2 ? FMath::Max(LastSatSize * 5.f, (AdapterCm - (StageBaseZ.Num() ? StageBaseZ.Last() * 100.f : 0.f)) * 3.5f) : LastSatSize * 5.f;
		}
		UE_LOG(LogTemp, Log, TEXT("[TRAJ] view: mode %d around body %d, craft %.0f km from it, camera %.0f m (now %.0f m), sat %.0f cm, adapter %.0f cm"), Mode, Centre, P.Size() / KmCm, CamDistGoal / 100.f, CamDist / 100.f, LastSatSize, AdapterCm);
	}
	// the stack is filmed from the side (from behind, the stage would hide the satellite), turning there gently
	if (Mode == 2 && !bFlightCamUser)
	{
		FollowYaw = FMath::FInterpTo(FollowYaw, 100.f, Dt, 1.2f);
		FollowPitch = FMath::FInterpTo(FollowPitch, -12.f, Dt, 1.2f);
	}
	// at Jupiter: look down past the probe so the planet's limb sits in the lower part of the picture (it fills
	// 130 deg of sky at periapsis, so "straight at it" would show only cloud tops)
	// the satellite alone: back to the usual follow angle (from the side, a deployed wing would sit in front of the lens)
	if (Mode == 0 && !bJupiterScene && !bFlightCamUser)
	{
		FollowYaw = FMath::FInterpTo(FollowYaw, 25.f, Dt, 0.8f);
		FollowPitch = FMath::FInterpTo(FollowPitch, -18.f, Dt, 0.8f);
	}
	if (Mode == 0 && bJupiterScene && !bFlightCamUser)
	{
		const double Limb = -90.0 + FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(69911.0 * KmCm / FMath::Max(P.Size(), 1.0), 0.0, 1.0)));
		FollowPitch = FMath::FInterpTo(FollowPitch, (float)FMath::Max(Limb - 12.0, -78.0), Dt, 1.2f);
	}
	VoyFocus = bMoonTrip ? MoonRoute.GetCenter() * 0.8f : (bFar ? FVector::ZeroVector : P * 0.5f);

	// the path in this scene's frame (flown orange, ahead cyan) + the science orbit around the destination
	if (FlightLines)
	{
		FlightLines->Flush();
		const double PathStart = bJupiterScene ? Voy.ArriveT - 25.0 * DaySec : FMath::Max(0.0, Voy.DepartT - 5400.0);
		const double PathEnd = bMoonTrip || bJupiterScene ? Voy.ArriveT : (Voy.Legs.IsValidIndex(1) ? Voy.Legs[1].T1 : Voy.ArriveT);
		for (const FISLeg& L : Voy.Legs)
		{
			const double A = FMath::Max(L.T0, PathStart), Bt = FMath::Min(L.T1, PathEnd);
			if (Bt <= A) continue;
			const int32 N = 160;
			FVector Prev = Cm(Voy.PosAround(Centre, A));
			for (int32 i = 1; i <= N; ++i)
			{
				const double t = A + (Bt - A) * i / N;
				const FVector Q = Cm(Voy.PosAround(Centre, t));
				FlightLines->DrawLine(Prev, Q, t <= T ? Flown : Ahead, SDPG_World, 0.f, 0.f);
				Prev = Q;
			}
		}
		if (bIns && (bMoonTrip || bJupiterScene))
		{
			// one period of the science orbit around the destination, where the destination is now
			const FISLeg& L = Voy.Legs.Last();
			const double Mu = ISOrbit::Bodies()[L.Body].Mu;
			const FISElements E = ISOrbit::Elements(Mu, 0.0, L.R, L.V);
			const double Per = E.Period > 0.0 ? E.Period : 2.0 * PI * FMath::Sqrt(FMath::Pow(L.R.Size(), 3) / Mu);
			const FVector Org = bMoonTrip ? MoonPos : FVector::ZeroVector;
			FVector3d R0 = L.R, V0 = L.V;
			FVector Prev = Org + Cm(R0);
			for (int32 i = 1; i <= 192; ++i)
			{
				ISOrbit::Kepler(Mu, R0, V0, Per / 192.0);
				const FVector Q = Org + Cm(R0);
				FlightLines->DrawLine(Prev, Q, T >= Voy.ArriveT ? Flown : Ahead * 0.7f, SDPG_World, 0.f, 0.f);
				Prev = Q;
			}
		}
		if (!bFollow && !bFar) FlightLines->DrawPoint(P, Flown, 10.f, SDPG_Foreground, 0.f);
		if (!bFollow && (bMoonTrip || bJupiterScene)) FlightLines->DrawPoint(bMoonTrip ? MoonPos : FVector::ZeroVector, FLinearColor(0.85f, 0.85f, 0.8f), 8.f, SDPG_Foreground, 0.f);   // where the target is: a dot at map scale
	}
}

float AISSpaceScene::WarpCap() const
{
	if (!bVoyage || !Voy.bValid || bFlight || Voy.Burns.Num() == 0) return 0.f;
	const double T = MissionDay * DaySec;
	const FISBurn& DepB = Voy.Burns[0];
	const double B0 = DepB.T - VoyBurnDur * 0.5, B1 = DepB.T + VoyBurnDur * 0.5, SepAt = B1 + 60.0;
	if (!bVoySepDone && T >= SepAt - 20.0) return 1.f;              // the separation is filmed in real time
	if (T >= B0 - 60.0 && T < B1) return 10.f;                        // burns at 10x: a few minutes become ~30 s
	double Next = !bVoySepDone ? (T < B0 - 60.0 ? B0 - 60.0 : SepAt - 20.0) : 1e30;
	if (Voy.Target >= 0 && Voy.Burns.Num() > 1)
	{
		const double I0 = Voy.ArriveT - VoyInsDur * 0.5 - 600.0, I1 = Voy.ArriveT + VoyInsDur * 0.5;
		if (T >= I0 && T < I1) return 10.f;
		if (T < I0) Next = FMath::Min(Next, I0);
	}
	// slow down smoothly before the next event instead of jumping over it in one frame (1,000,000x = 11.6 days per second)
	return Next < 1e29 ? (float)FMath::Max(10.0, (Next - T) * 0.8) : 0.f;
}

FString AISSpaceScene::VoyageCaption() const
{
	if (!bVoyage || !Voy.bValid || bFlight || Voy.Burns.Num() == 0) return FString();
	const double T = MissionDay * DaySec;
	const FISBurn& DepB = Voy.Burns[0];
	const double B0 = DepB.T - VoyBurnDur * 0.5, B1 = DepB.T + VoyBurnDur * 0.5;
	if (T < B0) return FString::Printf(TEXT("Parking orbit. Departure burn in %s."), *ISSim::FmtDays((B0 - T) / DaySec));
	if (T < B1) return FString::Printf(TEXT("%s: the upper stage adds %.1f km/s."), *DepB.What, DepB.DvMs / 1000.0);
	if (!bVoySepDone) return TEXT("Separation: springs push the satellite off the spent stage.");
	if (Voy.Target >= 0 && Voy.Burns.Num() > 1)
	{
		const FISBurn& Ins = Voy.Burns.Last();
		const double I0 = Voy.ArriveT - VoyInsDur * 0.5, I1 = Voy.ArriveT + VoyInsDur * 0.5;
		if (T < I0 - 600.0) return FString::Printf(TEXT("Cruise. %s in %s."), *Ins.What, *ISSim::FmtDays((I0 - T) / DaySec));
		if (T < I0) return TEXT("Turning around: the engine must point forward to brake.");
		if (T < I1) return FString::Printf(TEXT("%s: braking %.2f km/s so gravity can catch the satellite."), *Ins.What, Ins.DvMs / 1000.0);
		return FString();
	}
	// interstellar: how far out
	return FString::Printf(TEXT("%.1f AU from the Sun. Interstellar space begins at the heliopause, about 120 AU."), Voy.HelioPos(T).Size() / 1.495978707e8);
}
