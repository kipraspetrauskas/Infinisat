#include "ISRocket.h"

// ------------------------------------------------------------------------------------------------ real launchers
// Rounded public figures (masses kg, thrust N, Isp s). Upper-stage "SL" thrust is only used if lit in the air.
namespace
{
	FISEngineStage S(const TCHAR* Name, double Dry, double Prop, double TSL, double TVac, double IspSL, double IspVac, double MinThr, bool bSolid, double D, double Len, FLinearColor C)
	{
		FISEngineStage St;
		St.Name = Name; St.DryKg = Dry; St.PropKg = Prop; St.ThrustSLN = TSL; St.ThrustVacN = TVac; St.IspSL = IspSL; St.IspVac = IspVac;
		St.MinThrottle = MinThr; St.bSolid = bSolid; St.DiameterM = D; St.LengthM = Len; St.Color = C;
		return St;
	}
	void Boost(FISEngineStage& St, int32 N, double Dry, double Prop, double TSL, double TVac, double IspSL, double IspVac, double D, double Len, bool bSolid)
	{
		St.Boosters = N; St.BoosterDryKg = Dry; St.BoosterPropKg = Prop; St.BoosterThrustSLN = TSL; St.BoosterThrustVacN = TVac;
		St.BoosterIspSL = IspSL; St.BoosterIspVac = IspVac; St.BoosterDiameterM = D; St.BoosterLengthM = Len; St.bBoosterSolid = bSolid;
	}
	const FLinearColor White(0.92f, 0.93f, 0.95f), Black(0.05f, 0.05f, 0.06f), Grey(0.55f, 0.56f, 0.6f), Orange(0.85f, 0.42f, 0.12f);

	TArray<FISLauncher> MakeAll()
	{
		TArray<FISLauncher> A;
		{
			FISLauncher L; L.Id = "lv_electron"; L.Name = TEXT("Electron"); L.Site = TEXT("Mahia, New Zealand"); L.SiteLatDeg = -39.26; L.MaxQPa = 50000;
			L.Stages.Add(S(TEXT("Stage 1 (9 Rutherford)"), 950, 9250, 224e3, 239e3, 293, 311, 0.6, false, 1.2, 12.1, Black));
			L.Stages.Add(S(TEXT("Stage 2 (Rutherford Vacuum)"), 250, 2050, 18e3, 25.8e3, 240, 343, 0.6, false, 1.2, 2.4, Black));
			L.FairingKg = 50; L.FairingDiameterM = 1.2; L.FairingLengthM = 2.5;
			A.Add(L);
		}
		{
			FISLauncher L; L.Id = "lv_f9"; L.Name = TEXT("Falcon 9 Block 5"); L.Site = TEXT("Cape Canaveral SLC-40, Florida"); L.SiteLatDeg = 28.56; L.MaxQPa = 50000;
			L.Stages.Add(S(TEXT("Stage 1 (9 Merlin 1D)"), 25600, 395700, 7607e3, 8227e3, 283, 312, 0.4, false, 3.66, 47.0, White));
			L.Stages.Add(S(TEXT("Stage 2 (Merlin Vacuum)"), 4000, 92670, 600e3, 981e3, 200, 348, 0.39, false, 3.66, 13.8, White));
			L.FairingKg = 1900; L.FairingDiameterM = 5.2; L.FairingLengthM = 13.1;
			A.Add(L);
			FISLauncher R = L; R.Id = "lv_rideshare"; R.Name = TEXT("Falcon 9 rideshare (Transporter)"); R.Site = TEXT("Vandenberg SLC-4E, California"); R.SiteLatDeg = 34.63;
			A.Add(R);
		}
		{
			FISLauncher L; L.Id = "lv_vega"; L.Name = TEXT("Vega-C"); L.Site = TEXT("Guiana Space Centre ELV, Kourou"); L.SiteLatDeg = 5.24; L.MaxQPa = 55000;
			L.Stages.Add(S(TEXT("P120C (solid)"), 11000, 141600, 2700e3, 3000e3, 255, 279, 1.0, true, 3.4, 13.5, White));
			L.Stages.Add(S(TEXT("Zefiro 40 (solid)"), 3800, 36200, 1100e3, 1304e3, 250, 293, 1.0, true, 2.4, 8.3, White));
			L.Stages.Add(S(TEXT("Zefiro 9 (solid)"), 1300, 10500, 230e3, 317e3, 230, 296, 1.0, true, 1.9, 4.1, White));
			L.Stages.Add(S(TEXT("AVUM+ (liquid)"), 700, 740, 2e3, 2.45e3, 200, 315, 1.0, false, 2.2, 1.7, Grey));
			L.FairingKg = 860; L.FairingDiameterM = 3.3; L.FairingLengthM = 9.0;
			A.Add(L);
		}
		{
			FISLauncher L; L.Id = "lv_a6"; L.Name = TEXT("Ariane 6 (A64)"); L.Site = TEXT("Guiana Space Centre ELA-4, Kourou"); L.SiteLatDeg = 5.24; L.MaxQPa = 50000;
			FISEngineStage Core = S(TEXT("Core stage (Vulcain 2.1)"), 17000, 150000, 1000e3, 1370e3, 318, 431, 0.8, false, 5.4, 30.0, White);
			Boost(Core, 4, 11000, 141600, 2700e3, 3000e3, 255, 279, 3.4, 13.5, true);
			L.Stages.Add(Core);
			L.Stages.Add(S(TEXT("Upper stage (Vinci)"), 5000, 31000, 100e3, 180e3, 300, 457, 0.6, false, 5.4, 11.6, White));
			L.FairingKg = 2600; L.FairingDiameterM = 5.4; L.FairingLengthM = 20.0;
			A.Add(L);
		}
		{
			FISLauncher L; L.Id = "lv_fh"; L.Name = TEXT("Falcon Heavy"); L.Site = TEXT("Kennedy Space Center LC-39A, Florida"); L.SiteLatDeg = 28.61; L.MaxQPa = 50000;
			FISEngineStage Core = S(TEXT("Centre core (9 Merlin 1D)"), 28000, 395700, 7607e3, 8227e3, 283, 312, 0.4, false, 3.66, 47.0, White);
			Boost(Core, 2, 25600, 395700, 7607e3, 8227e3, 283, 312, 3.66, 44.0, false);
			L.Stages.Add(Core);
			L.Stages.Add(S(TEXT("Stage 2 (Merlin Vacuum)"), 4000, 107500, 600e3, 981e3, 200, 348, 0.39, false, 3.66, 13.8, White));
			L.FairingKg = 1900; L.FairingDiameterM = 5.2; L.FairingLengthM = 13.1;
			A.Add(L);
		}
		{
			FISLauncher L; L.Id = "lv_sls"; L.Name = TEXT("SLS Block 1"); L.Site = TEXT("Kennedy Space Center LC-39B, Florida"); L.SiteLatDeg = 28.63; L.MaxQPa = 45000;
			FISEngineStage Core = S(TEXT("Core stage (4 RS-25)"), 85000, 987000, 7440e3, 9100e3, 366, 452, 0.67, false, 8.4, 65.0, Orange);
			Boost(Core, 2, 98000, 628000, 14600e3, 16000e3, 242, 269, 3.7, 54.0, true);
			L.Stages.Add(Core);
			L.Stages.Add(S(TEXT("ICPS (RL10)"), 3500, 28500, 60e3, 110e3, 300, 462, 1.0, false, 5.0, 13.7, White));
			L.FairingKg = 0; L.FairingDiameterM = 5.0; L.FairingLengthM = 8.0;
			A.Add(L);
		}
		return A;
	}
}

const FISLauncher* FISLauncher::Find(FName VehicleId)
{
	static const TArray<FISLauncher> All = MakeAll();
	for (const FISLauncher& L : All) if (L.Id == VehicleId) return &L;
	return nullptr;   // hobby rocket / balloon: back-yard flight, not an orbital launcher
}

// ------------------------------------------------------------------------------------------------ atmosphere
double FISRocketSim::Density(double H)
{
	if (H < 0) H = 0;
	if (H > 150000) return 0;
	// piecewise exponential fit to the US Standard Atmosphere 1976
	if (H < 11000) return 1.225 * FMath::Pow(1.0 - 2.2558e-5 * H, 4.2559);
	if (H < 25000) return 0.36392 * FMath::Exp(-(H - 11000) / 6341.6);
	return 0.04008 * FMath::Exp(-(H - 25000) / 7600.0) * (H > 80000 ? FMath::Exp(-(H - 80000) / 12000.0) : 1.0);
}

double FISRocketSim::PressureRatio(double H)
{
	if (H < 0) H = 0;
	if (H < 11000) return FMath::Pow(1.0 - 2.2558e-5 * H, 5.2559);
	return 0.2234 * FMath::Exp(-(H - 11000) / 6341.6);
}

double FISRocketSim::SoundSpeed(double H)
{
	const double Tk = H < 11000 ? 288.15 - 0.0065 * H : (H < 20000 ? 216.65 : FMath::Min(270.65, 216.65 + 0.001 * (H - 20000)));
	return FMath::Sqrt(1.4 * 287.05 * Tk);
}

// ------------------------------------------------------------------------------------------------ setup
void FISRocketSim::Setup(const FISLauncher& InL, double InPayloadKg, double InTargetAltKm, double InIncDeg)
{
	L = &InL;
	PayloadKg = InPayloadKg;
	IncDeg = InIncDeg;
	// high orbits are reached later by the upper stage from a parking orbit: the ascent targets 200-600 km
	TargetAltKm = InTargetAltKm > 2000 ? 250.0 : FMath::Clamp(InTargetAltKm, 180.0, 1200.0);
	StageIdx = 0;
	StageProp = L->Stages[0].PropKg;
	bBoostersOn = L->Stages[0].Boosters > 0;
	BoosterProp = L->Stages[0].Boosters * L->Stages[0].BoosterPropKg;
	bFairingOn = L->FairingKg > 0;
	Phase = EISFlightPhase::OnPad;
	T = -10.0;
	Events.Reset();
	FailReason.Reset();
	MaxQSeen = 0; bMaxQAnnounced = bMaxQPassed = bCircBurn = bCircActive = false;
	Throttle = 1.0; PitchDeg = 0;
	// launch azimuth for the target inclination: sin(az) = cos(i) / cos(lat)
	const double Lat = FMath::DegreesToRadians(L->SiteLatDeg), Inc = FMath::DegreesToRadians(IncDeg);
	const double SinAz = FMath::Clamp(FMath::Cos(Inc) / FMath::Max(0.05, FMath::Cos(Lat)), -1.0, 1.0);
	LaunchAzDeg = FMath::RadiansToDegrees(FMath::Asin(SinAz));
	EarthBoostMs = OmegaE * Re * FMath::Cos(Lat) * SinAz;   // Earth's spin helps eastward launches, hurts retrograde (SSO)
	R = FVector2D(0, Re);
	V = FVector2D(EarthBoostMs, 0);
	ThetaI = 0; OmegaI = 0; AttDeg = 0; RateDps = 0; GimbalDeg = 0;
	History.Reset(); HistT = -1e9;
	UpdateMassProperties();
	UpdateDerived();
	LiftoffDv = DeltaVLeft();
}

void FISRocketSim::StartCountdown(double Seconds)
{
	if (Phase != EISFlightPhase::OnPad) return;
	Phase = EISFlightPhase::Countdown;
	T = -Seconds;
	Event(FString::Printf(TEXT("Countdown started: T-%.0f s. Launch azimuth %.1f deg, Earth's rotation gives %+.0f m/s."), Seconds, LaunchAzDeg, EarthBoostMs));
}

double FISRocketSim::StackMass() const
{
	if (!L) return 0;
	double M = PayloadKg + (bFairingOn ? L->FairingKg : 0.0);
	for (int32 i = StageIdx; i < L->Stages.Num(); ++i)
	{
		const FISEngineStage& St = L->Stages[i];
		M += St.DryKg + (i == StageIdx ? StageProp : St.PropKg);
		if (i == StageIdx && bBoostersOn) M += St.Boosters * St.BoosterDryKg + BoosterProp;
	}
	return M;
}

double FISRocketSim::DeltaVLeft() const
{
	if (!L) return 0;
	double Dv = 0, M = StackMass();
	for (int32 i = StageIdx; i < L->Stages.Num(); ++i)
	{
		const FISEngineStage& St = L->Stages[i];
		const double Prop = (i == StageIdx ? StageProp : St.PropKg) + (i == StageIdx && bBoostersOn ? BoosterProp : 0.0);
		const double Isp = (i == StageIdx && bBoostersOn) ? (St.IspVac * St.ThrustVacN + St.BoosterIspVac * St.Boosters * St.BoosterThrustVacN) / (St.ThrustVacN + St.Boosters * St.BoosterThrustVacN) : St.IspVac;
		Dv += Isp * G0 * FMath::Loge(M / FMath::Max(1.0, M - Prop));
		M -= Prop + St.DryKg + (i == StageIdx && bBoostersOn ? St.Boosters * St.BoosterDryKg : 0.0);
		if (i == StageIdx && bFairingOn && L->FairingKg > 0) M -= 0;   // fairing leaves during stage 2 (ignored here)
	}
	return Dv;
}

FVector2D FISRocketSim::NoseDir() const
{
	return FVector2D(FMath::Sin(ThetaI), FMath::Cos(ThetaI));
}

FVector2D FISRocketSim::ThrustDir() const
{
	// a gimballed engine pushes the tail sideways: the thrust leans opposite to the turn it makes
	const double A = ThetaI - FMath::DegreesToRadians(GimbalDeg);
	return FVector2D(FMath::Sin(A), FMath::Cos(A));
}

// What is still attached, bottom to top: the current stage (+ its boosters beside it), the stages above, then the payload
// under its fairing. Each piece is a uniform cylinder; the parallel-axis theorem adds them about the common centre of mass
// (Principia's PileUp does the same with full inertia tensors via MechanicalSystem::AddRigidBody).
void FISRocketSim::UpdateMassProperties()
{
	if (!L) return;
	struct FSeg { double M, Z0, Len, Diam; };
	TArray<FSeg, TInlineAllocator<10>> Segs;
	double Z = 0;
	for (int32 i = StageIdx; i < L->Stages.Num(); ++i)
	{
		const FISEngineStage& St = L->Stages[i];
		Segs.Add({ St.DryKg + (i == StageIdx ? StageProp : St.PropKg), Z, St.LengthM, St.DiameterM });
		if (i == StageIdx && bBoostersOn) Segs.Add({ St.Boosters * St.BoosterDryKg + BoosterProp, 0.0, St.BoosterLengthM, St.BoosterDiameterM });
		Z += St.LengthM;
	}
	const double PayLen = bFairingOn ? FMath::Max(1.0, L->FairingLengthM) : 1.5;
	Segs.Add({ PayloadKg + (bFairingOn ? L->FairingKg : 0.0), Z, PayLen, bFairingOn ? L->FairingDiameterM : 1.0 });
	StackLenM = Z + PayLen;
	double Mt = 0, Mz = 0;
	for (const FSeg& S : Segs) { Mt += S.M; Mz += S.M * (S.Z0 + S.Len * 0.5); }
	CoMHeightM = Mz / FMath::Max(1.0, Mt);
	double I = 0;
	for (const FSeg& S : Segs)
	{
		const double Rr = S.Diam * 0.5, D = S.Z0 + S.Len * 0.5 - CoMHeightM;
		I += S.M * (3.0 * Rr * Rr + S.Len * S.Len) / 12.0 + S.M * D * D;
	}
	InertiaKgm2 = FMath::Max(1.0, I);
}

// Pitch dynamics: I * dOmega/dt = gimbal torque + aerodynamic torque + attitude thrusters.
// Burnt propellant leaves with its own angular momentum, so losing mass does not spin the vehicle up
// (the same assumption Principia makes for solid motors: the angular velocity is kept, not the angular momentum).
void FISRocketSim::StepAttitude(double Dt)
{
	UpdateMassProperties();
	const double Phi = FMath::Atan2(R.X, R.Y);            // inertial angle of the local vertical
	const FVector2D U = Up(), E = East();
	const double Rn = R.Size();
	const double FrameRate = FVector2D::DotProduct(V, E) / Rn;   // the horizon turns as the vehicle flies downrange
	const FVector2D Air = E * (OmegaE * Rn * FMath::Cos(FMath::DegreesToRadians(L->SiteLatDeg)) * FMath::Sin(FMath::DegreesToRadians(LaunchAzDeg)));
	const FVector2D Vrel = V - Air;
	const double Vr = Vrel.Size();
	if (AltM < 1.0 && FVector2D::DotProduct(Vrel, U) < 0.5)
	{
		// still clamped to the pad: vertical, not turning
		ThetaI = Phi; OmegaI = FrameRate; GimbalDeg = 0; AeroTorqueNm = 0; bRcs = false;
		AttDeg = 0; RateDps = 0;
		return;
	}
	// a finless rocket is aerodynamically unstable: the centre of pressure sits ahead of the centre of mass, so the air
	// pushes the nose further away from the airflow. Normal-force slope ~2 per radian; pitch damping from the same lift.
	const FISEngineStage& St = L->Stages[StageIdx];
	double Diam = St.DiameterM;
	if (bFairingOn) Diam = FMath::Max(Diam, L->FairingDiameterM);
	const double Area = PI * Diam * Diam * 0.25;
	const double Q = 0.5 * Density(AltM) * Vr * Vr;
	const double Dcp = 0.12 * StackLenM;
	AeroTorqueNm = 0;
	double Damp = 0;
	if (Vr > 20.0 && Q > 1.0)
	{
		const double Alpha = FMath::UnwindRadians(ThetaI - FMath::Atan2(Vrel.X, Vrel.Y));
		AeroTorqueNm = Q * Area * 2.0 * FMath::Sin(Alpha) * Dcp;
		Damp = -Q * Area * 2.0 * Dcp * Dcp * (OmegaI - FrameRate) / Vr;
	}
	// flight computer: a critically damped turn toward the commanded pitch (feed-forward cancels the known air torque)
	const double Cmd = FMath::DegreesToRadians(PitchDeg) + Phi;
	const double Err = FMath::UnwindRadians(Cmd - ThetaI);
	const double Wn = 0.9, Zeta = 0.95;
	const double AccWant = Wn * Wn * Err + 2.0 * Zeta * Wn * (FrameRate - OmegaI);
	const double TauWant = AccWant * InertiaKgm2 - AeroTorqueNm - Damp;
	const double Arm = FMath::Max(1.0, CoMHeightM);       // engines sit at the bottom of the current stage
	GimbalMaxDeg = St.bSolid ? 6.0 : 5.0;
	double Tau = 0;
	if (ThrustN > 1.0)
	{
		const double SinG = FMath::Clamp(TauWant / (ThrustN * Arm), -FMath::Sin(FMath::DegreesToRadians(GimbalMaxDeg)), FMath::Sin(FMath::DegreesToRadians(GimbalMaxDeg)));
		GimbalDeg = FMath::RadiansToDegrees(FMath::Asin(SinG));
		Tau = ThrustN * Arm * SinG;
	}
	else GimbalDeg = 0;
	// cold-gas attitude thrusters: weak, but they turn a coasting stage (and help the gimbal above the thick air)
	bRcs = false;
	if (ThrustN <= 1.0 || AltM > 60000.0)
	{
		const double RcsMax = InertiaKgm2 * 0.03;
		const double Extra = FMath::Clamp(TauWant - Tau, -RcsMax, RcsMax);
		Tau += Extra;
		bRcs = ThrustN <= 1.0 && FMath::Abs(Extra) > 0.02 * RcsMax;
	}
	OmegaI += (Tau + AeroTorqueNm + Damp) / InertiaKgm2 * Dt;
	ThetaI = FMath::UnwindRadians(ThetaI + OmegaI * Dt);
	AttDeg = FMath::RadiansToDegrees(FMath::UnwindRadians(ThetaI - Phi));
	RateDps = FMath::RadiansToDegrees(OmegaI);
}

// The prediction: no thrust from now on, gravity + drag, velocity-Verlet (symplectic, so an orbit closes on itself
// instead of spiralling out like with plain Euler steps). Stops at the ground or after one revolution.
void FISRocketSim::Predict(TArray<FVector2D>& Out, int32 MaxPoints) const
{
	Out.Reset();
	if (!L) return;
	FVector2D Rp = R, Vp = V;
	const double M = FMath::Max(1.0, MassKg);
	const double En = Vp.SizeSquared() * 0.5 - Mu / Rp.Size();
	double Horizon = 3600.0;
	if (En < 0) { const double A = -Mu / (2.0 * En); Horizon = FMath::Min(2.0 * PI * FMath::Sqrt(A * A * A / Mu), 30000.0); }
	const int32 Sub = 6;
	const double H = Horizon / (MaxPoints * Sub);
	const FISEngineStage& St = L->Stages[StageIdx];
	const double Diam = bFairingOn ? FMath::Max(St.DiameterM, L->FairingDiameterM) : St.DiameterM;
	const double CdA = 0.3 * PI * Diam * Diam * 0.25;
	const double Spin = OmegaE * FMath::Cos(FMath::DegreesToRadians(L->SiteLatDeg)) * FMath::Sin(FMath::DegreesToRadians(LaunchAzDeg));
	auto Accel = [&](const FVector2D& Rr, const FVector2D& Vv)
	{
		const double Rn = Rr.Size();
		FVector2D A = -Rr * (Mu / (Rn * Rn * Rn));
		const double Alt = Rn - Re;
		if (Alt < 120000.0)
		{
			const FVector2D U = Rr / Rn, E(U.Y, -U.X);
			const FVector2D Vrel = Vv - E * (Spin * Rn);
			A -= Vrel * (0.5 * Density(Alt) * Vrel.Size() * CdA / M);
		}
		return A;
	};
	Out.Add(Rp);
	FVector2D Acc = Accel(Rp, Vp);
	for (int32 i = 0; i < MaxPoints; ++i)
	{
		for (int32 s = 0; s < Sub; ++s)
		{
			Vp += Acc * (H * 0.5);
			Rp += Vp * H;
			Acc = Accel(Rp, Vp);
			Vp += Acc * (H * 0.5);
		}
		if (Rp.Size() < Re) { Out.Add(Rp.GetSafeNormal() * Re); break; }
		Out.Add(Rp);
	}
}

void FISRocketSim::Stage()
{
	if (Phase == EISFlightPhase::OnPad) { StartCountdown(); return; }
	if (!IsFlying() || !L) return;
	if (bBoostersOn) { bBoostersOn = false; BoosterProp = 0; Event(TEXT("Boosters separated.")); return; }
	if (StageIdx + 1 >= L->Stages.Num()) return;
	Event(FString::Printf(TEXT("Stage separation: %s dropped with %.0f kg of propellant left. Next: %s."), *L->Stages[StageIdx].Name, StageProp, *L->Stages[StageIdx + 1].Name));
	++StageIdx;
	StageProp = L->Stages[StageIdx].PropKg;
}

void FISRocketSim::Abort(const FString& Why)
{
	if (Phase == EISFlightPhase::Failed || Phase == EISFlightPhase::Orbit) return;
	Phase = EISFlightPhase::Failed;
	FailReason = Why;
	Event(TEXT("FAILURE: ") + Why);
}

// ------------------------------------------------------------------------------------------------ guidance
double FISRocketSim::TimeToApoapsis() const
{
	const double Rn = R.Size(), Vn2 = V.SizeSquared();
	const double En = Vn2 * 0.5 - Mu / Rn;
	if (En >= 0) return 1e9;
	const double A = -Mu / (2.0 * En);
	const FVector2D Ev = (R * (Vn2 - Mu / Rn) - V * FVector2D::DotProduct(R, V)) / Mu;
	const double Ecc = Ev.Size();
	if (Ecc < 1e-4) return 0.0;
	double Nu = FMath::Acos(FMath::Clamp(FVector2D::DotProduct(Ev, R) / (Ecc * Rn), -1.0, 1.0));
	if (FVector2D::DotProduct(R, V) < 0) Nu = 2.0 * PI - Nu;
	const double Ea = 2.0 * FMath::Atan(FMath::Sqrt((1.0 - Ecc) / (1.0 + Ecc)) * FMath::Tan(Nu * 0.5));
	double Ma = Ea - Ecc * FMath::Sin(Ea);
	if (Ma < 0) Ma += 2.0 * PI;
	const double N = FMath::Sqrt(Mu / (A * A * A));
	double Dm = PI - Ma;
	if (Dm < 0) Dm += 2.0 * PI;
	return Dm / N;
}

// Ascent autopilot, the way real/KSP ascent guidance flies it:
//   1) straight up off the pad, 2) pitch kick + gravity turn (nose on the airflow) in the thick air,
//   3) prograde until the apoapsis reaches the target, 4) coast, 5) circularise around the apoapsis.
void FISRocketSim::Guidance(double Dt)
{
	const FVector2D U = Up(), E = East();
	const FVector2D Air = E * (OmegaE * R.Size() * FMath::Cos(FMath::DegreesToRadians(L->SiteLatDeg)) * FMath::Sin(FMath::DegreesToRadians(LaunchAzDeg)));
	const FVector2D Vrel = V - Air;
	const double Vz = FVector2D::DotProduct(V, U), Vh = FVector2D::DotProduct(V, E);
	const FISEngineStage& St = L->Stages[StageIdx];
	const double Target = TargetAltKm;

	if (!bCircBurn)
	{
		Throttle = St.bSolid ? 1.0 : FMath::Clamp(1.0 - (QPa - 0.55 * L->MaxQPa) / (0.2 * L->MaxQPa), St.MinThrottle, 1.0);   // liquids throttle down through max-Q (Falcon 9 "bucket")
		if (AltM < 250 || SurfSpeed < 60)
			PitchDeg = 0;
		else
		{
			const double Bl = FMath::Clamp((AltM - 30000.0) / 30000.0, 0.0, 1.0);
			const double GammaAir = FMath::RadiansToDegrees(FMath::Atan2(FVector2D::DotProduct(Vrel, E), FVector2D::DotProduct(Vrel, U)));
			const FVector2D Vg = Vrel * (1.0 - Bl) + V * Bl;      // airflow in the thick air, blending to orbital velocity by 60 km
			const double Gamma = FMath::RadiansToDegrees(FMath::Atan2(FVector2D::DotProduct(Vg, E), FVector2D::DotProduct(Vg, U)));
			// pitch program: vertical -> horizontal by ~120 km (square-root profile); in thick air stay within 3 deg of the airflow
			const double Profile = 90.0 * FMath::Pow(FMath::Clamp(AltM / 150000.0, 0.0, 1.0), 0.5);
			PitchDeg = QPa > 3000.0 ? FMath::Clamp(Profile, GammaAir - 1.5, GammaAir + 1.5) : Profile;   // q-alpha: angle to the real airflow
			PitchDeg = FMath::Clamp(PitchDeg, 0.0, 89.0);
			if (QPa < 3000.0 && AltM > 45000.0)
			{
				// out of the thick air: keep the apoapsis ~50 s ahead while it climbs to the target (pitch up if it is too close)
				const double Tapo = Vz < 0.0 ? -60.0 : FMath::Min(TimeToApoapsis(), 400.0);
				PitchDeg = FMath::Clamp(75.0 + (Tapo - 50.0) * 0.6, 35.0, 92.0);
			}
			// never lean across the airflow while the air still pushes: q x angle-of-attack stays under ~15 kPa*deg (break-up is ~150-300)
			const double AllowA = FMath::Clamp(15000.0 / FMath::Max(1.0, QPa), 1.5, 180.0);
			PitchDeg = FMath::Clamp(PitchDeg, GammaAir - AllowA, GammaAir + AllowA);
		}
		if (ApoKm >= Target) { Throttle = 0.0; bCircBurn = true; Event(FString::Printf(TEXT("Apoapsis %.0f km reached: engine cut-off, coasting to apoapsis (%.0f s)."), ApoKm, TimeToApoapsis())); }
	}
	else
	{
		// coast, then burn horizontally around the apoapsis; small pitch corrections keep the apoapsis in place
		const double Rn = R.Size();
		const double Vcirc = FMath::Sqrt(Mu / Rn);
		const double DvNeed = FMath::Max(0.0, Vcirc - Vh);
		const double Acc = FMath::Max(0.05, (St.ThrustVacN + (bBoostersOn ? St.Boosters * St.BoosterThrustVacN : 0.0)) / FMath::Max(1.0, MassKg));
		const double BurnT = DvNeed / Acc;
		const double Tapo = TimeToApoapsis();
		if (ApoKm < Target - 25.0 && AltM < 70000.0 && !bCircActive) { bCircBurn = false; return; }   // drag ate the apoapsis: back to prograde
		// low-thrust upper stages cannot circularise in one short burn around the apoapsis: they burn continuously instead
		const bool bLong = BurnT > 90.0;
		if (!bCircActive && (bLong || Tapo < BurnT * 0.5 + 1.0 || Vz < -5.0)) { bCircActive = true; Event(FString::Printf(TEXT("Circularisation burn: %.0f m/s needed, about %.0f s."), DvNeed, BurnT)); }
		Throttle = bCircActive ? 1.0 : 0.0;
		if (bLong)
		{
			// hold the target altitude: just enough upward thrust to cancel gravity-minus-centrifugal, the rest sideways
			const double Geff = Mu / (Rn * Rn) - Vh * Vh / Rn;
			const double VzWant = FMath::Clamp((Target * 1000.0 - AltM) / 60.0, -60.0, 200.0);
			const double Need = Geff + (VzWant - Vz) / 8.0;
			PitchDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Need / Acc, -0.35, 1.0)));
		}
		else PitchDeg = FMath::Clamp(90.0 + Vz * 0.3 + (ApoKm - Target) * 0.1, 60.0, 120.0);   // climbing or apoapsis too high: nose slightly down
		if (PeriKm >= FMath::Min(Target - 10.0, Target * 0.95) || (PeriKm > 150.0 && ApoKm > Target + 150.0)) Throttle = 0.0;
	}
	if (StageProp <= 0.0 && StageIdx + 1 < L->Stages.Num() && (bCircBurn ? Throttle > 0.0 : true) && !bBoostersOn) Stage();
	(void)Dt;
}

// ------------------------------------------------------------------------------------------------ physics
void FISRocketSim::UpdateDerived()
{
	if (!L) return;
	const FVector2D U = Up(), E = East();
	AltM = R.Size() - Re;
	Speed = V.Size();
	const FVector2D Air = E * (OmegaE * R.Size() * FMath::Cos(FMath::DegreesToRadians(L->SiteLatDeg)) * FMath::Sin(FMath::DegreesToRadians(LaunchAzDeg)));
	const FVector2D Vrel = V - Air;
	SurfSpeed = Vrel.Size();
	Mach = SurfSpeed / SoundSpeed(AltM);
	QPa = 0.5 * Density(AltM) * SurfSpeed * SurfSpeed;
	MassKg = StackMass();
	const FVector2D Nd = NoseDir();
	AoADeg = SurfSpeed > 20 ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector2D::DotProduct(Nd, Vrel.GetSafeNormal()), -1.0, 1.0))) : 0.0;
	// orbit from the state vector
	const double Rn = R.Size();
	const double H = R.X * V.Y - R.Y * V.X;
	const double En = Speed * Speed * 0.5 - Mu / Rn;
	if (En < 0)
	{
		const double A = -Mu / (2.0 * En);
		const double Ecc = FMath::Sqrt(FMath::Max(0.0, 1.0 + 2.0 * En * H * H / (Mu * Mu)));
		ApoKm = (A * (1.0 + Ecc) - Re) / 1000.0;
		PeriKm = (A * (1.0 - Ecc) - Re) / 1000.0;
	}
	else { ApoKm = 1e9; PeriKm = (H * H / Mu / 2.0 - Re) / 1000.0; }
	DownrangeKm = FMath::Atan2(R.X, R.Y) * Re / 1000.0;
	TWR = ThrustN / FMath::Max(1.0, MassKg * Mu / (Rn * Rn));
}

void FISRocketSim::Step(double Dt)
{
	if (!L || Phase == EISFlightPhase::Failed || Phase == EISFlightPhase::OnPad) return;
	T += Dt;
	if (Phase == EISFlightPhase::Countdown)
	{
		if (T < 0) return;
		Phase = EISFlightPhase::Ascent;
		Event(FString::Printf(TEXT("Ignition: %s. Lift-off mass %.1f t, thrust %.0f kN."), *L->Name, StackMass() / 1000.0,
			(L->Stages[0].ThrustSLN + L->Stages[0].Boosters * L->Stages[0].BoosterThrustSLN) / 1000.0));
	}
	if (bAutopilot && Phase != EISFlightPhase::Orbit) Guidance(Dt);

	const FISEngineStage& St = L->Stages[StageIdx];
	const double Pr = PressureRatio(AltM);
	// core / current stage
	double Thrust = 0, Mdot = 0;
	const bool bCoreFuel = StageProp > 0;
	const double Eff = St.bSolid ? (bCoreFuel ? 1.0 : 0.0) : (Throttle <= 0.0 || !bCoreFuel ? 0.0 : FMath::Clamp(Throttle, St.MinThrottle, 1.0));
	if (Eff > 0)
	{
		Thrust += Eff * (St.ThrustVacN - (St.ThrustVacN - St.ThrustSLN) * Pr);
		Mdot += Eff * St.ThrustVacN / (St.IspVac * G0);
	}
	double Bmdot = 0;
	if (bBoostersOn && BoosterProp > 0)
	{
		const double N = St.Boosters;
		Thrust += N * (St.BoosterThrustVacN - (St.BoosterThrustVacN - St.BoosterThrustSLN) * Pr);
		Bmdot = N * St.BoosterThrustVacN / (St.BoosterIspVac * G0);
	}
	ThrustN = Thrust;
	StageProp = FMath::Max(0.0, StageProp - Mdot * Dt);
	BoosterProp = FMath::Max(0.0, BoosterProp - Bmdot * Dt);
	if (bBoostersOn && BoosterProp <= 0.0) { bBoostersOn = false; Event(FString::Printf(TEXT("Booster burn-out and separation at %.1f km, %.0f m/s."), AltM / 1000.0, Speed)); }
	if (bCoreFuel && StageProp <= 0.0) Event(FString::Printf(TEXT("%s burn-out (%s) at %.1f km, %.0f m/s."), *St.Name, StageIdx == 0 ? TEXT("MECO") : TEXT("SECO"), AltM / 1000.0, Speed));
	if (bFairingOn && AltM > 110000.0) { bFairingOn = false; Event(FString::Printf(TEXT("Fairing jettisoned at %.0f km: %.0f kg less to carry."), AltM / 1000.0, L->FairingKg)); }

	// attitude first: the thrust then points where the vehicle really points (not where it was told to point)
	StepAttitude(Dt);

	// forces
	const double M = StackMass();
	const FVector2D U = Up(), E = East();
	const double Rn = R.Size();
	const FVector2D Air = E * (OmegaE * Rn * FMath::Cos(FMath::DegreesToRadians(L->SiteLatDeg)) * FMath::Sin(FMath::DegreesToRadians(LaunchAzDeg)));
	const FVector2D Vrel = V - Air;
	const double Vr = Vrel.Size();
	const double MachN = Vr / SoundSpeed(AltM);
	const double Cd = MachN < 0.8 ? 0.30 : (MachN < 1.2 ? 0.30 + (MachN - 0.8) * 0.5 : FMath::Max(0.22, 0.50 - (MachN - 1.2) * 0.06));
	double Diam = St.DiameterM;
	if (bFairingOn) Diam = FMath::Max(Diam, L->FairingDiameterM);
	const double Area = PI * Diam * Diam * 0.25 + (bBoostersOn ? St.Boosters * PI * St.BoosterDiameterM * St.BoosterDiameterM * 0.25 : 0.0);
	const FVector2D Drag = Vr > 0.1 ? -Vrel * (0.5 * Density(AltM) * Vr * Cd * Area) : FVector2D::ZeroVector;
	const FVector2D Grav = -R * (Mu / (Rn * Rn * Rn)) * M;
	FVector2D F = Grav + Drag + ThrustDir() * Thrust;

	// held on the pad until thrust beats weight
	const bool bOnGround = AltM <= 0.5 && FVector2D::DotProduct(V - Air, U) <= 0.1;
	if (bOnGround && FVector2D::DotProduct(F, U) <= 0.0)
	{
		V = Air;
		UpdateDerived();
		return;
	}
	if (bOnGround && T < 20.0 && !Events.ContainsByPredicate([](const FISFlightEvent& Ev) { return Ev.Text.StartsWith(TEXT("Lift-off")); }))
		Event(FString::Printf(TEXT("Lift-off! Thrust-to-weight %.2f."), Thrust / (M * G0)));

	// velocity Verlet (kick-drift-kick): gravity is re-evaluated at the new position, so energy does not drift when coasting
	const FVector2D NonGrav = (Drag + ThrustDir() * Thrust) / M;
	V += (NonGrav + Grav / M) * (Dt * 0.5);
	R += V * Dt;
	const double Rn1 = R.Size();
	V += (NonGrav - R * (Mu / (Rn1 * Rn1 * Rn1))) * (Dt * 0.5);
	UpdateDerived();
	if (T - HistT >= 1.0 && History.Num() < 20000) { History.Add(R); HistT = T; }

	// structural limits and ground
	MaxQSeen = FMath::Max(MaxQSeen, QPa);
	if (QPa > 2000.0) MaxAoASeen = FMath::Max(MaxAoASeen, AoADeg);
	MaxGimbalSeen = FMath::Max(MaxGimbalSeen, FMath::Abs(GimbalDeg));
	if (!bMaxQPassed && QPa < MaxQSeen * 0.97 && MaxQSeen > 5000.0)
	{
		bMaxQPassed = true;
		Event(FString::Printf(TEXT("Max-Q passed: %.1f kPa at %.1f km (Mach %.1f). The worst aerodynamic load is over."), MaxQSeen / 1000.0, AltM / 1000.0, Mach));
	}
	if (QPa > L->MaxQPa * 1.25) Abort(FString::Printf(TEXT("Structural failure: dynamic pressure %.0f kPa (limit %.0f kPa). Pitch over more gently or throttle down through max-Q."), QPa / 1000.0, L->MaxQPa / 1000.0));
	else if (QPa / 1000.0 * AoADeg > 300.0 && QPa > 5000.0) Abort(FString::Printf(TEXT("Aerodynamic break-up: %.0f deg angle of attack at %.0f kPa. In thick air the nose must follow the airflow."), AoADeg, QPa / 1000.0));
	else if (AltM < -1.0 && T > 1.0) Abort(TEXT("The vehicle hit the ground."));

	// phases
	if (Phase == EISFlightPhase::Ascent && ThrustN <= 0.0) Phase = EISFlightPhase::Coast;
	if (Phase == EISFlightPhase::Coast && ThrustN > 0.0) Phase = EISFlightPhase::Ascent;
	if (IsFlying() && ThrustN <= 0.0 && PeriKm > 140.0)
	{
		Phase = EISFlightPhase::Orbit;
		Event(FString::Printf(TEXT("ORBIT! %.0f x %.0f km, %.0f m/s. Delta-v left in the upper stage: %.0f m/s."), ApoKm, PeriKm, Speed, DeltaVLeft()));
	}
	if (IsFlying() && ThrustN <= 0.0 && StageProp <= 0.0 && StageIdx + 1 >= L->Stages.Num() && PeriKm < 140.0 && FVector2D::DotProduct(V, U) < 0.0 && AltM < 120000.0)
		{
		// low orbit needs ~7.8 km/s plus ~1.5 km/s of gravity and drag losses, minus what Earth's spin gives
		const double Need = 9400.0 - EarthBoostMs;
		Abort(LiftoffDv < Need
			? FString::Printf(TEXT("Out of propellant before orbit: this rocket had %.0f m/s of delta-v with your %.0f kg payload, but low orbit needs about %.0f m/s. The payload is too heavy for this rocket: lighten the satellite or pick a bigger rocket."), LiftoffDv, PayloadKg, Need)
			: FString::Printf(TEXT("Out of propellant before orbit (periapsis %.0f km), although the rocket had enough delta-v (%.0f m/s, need about %.0f). The path wasted it: climb steeply first, lean over gradually, then burn sideways near the top. Try the autopilot (T)."), PeriKm, LiftoffDv, Need));
	}
}

// Dev check: fly every launcher with the autopilot and log the result (-ISRocketTest).
void ISRocketSelfTest()
{
	struct FCase { const char* Id; double Kg, Alt, Inc; } Cases[] = {
		{ "lv_electron", 150, 500, 97.4 }, { "lv_electron", 300, 500, 45 }, { "lv_electron", 600, 500, 45 },
		{ "lv_f9", 500, 550, 53 }, { "lv_f9", 15000, 400, 51.6 }, { "lv_f9", 25000, 400, 51.6 }, { "lv_rideshare", 200, 525, 97.5 },
		{ "lv_vega", 1500, 700, 98 }, { "lv_a6", 10000, 400, 6 }, { "lv_fh", 30000, 400, 28.5 }, { "lv_sls", 40000, 300, 28.5 } };
	for (const FCase& C : Cases)
	{
		const FISLauncher* L = FISLauncher::Find(FName(C.Id));
		if (!L) continue;
		FISRocketSim S;
		S.Setup(*L, C.Kg, C.Alt, C.Inc);
		S.StartCountdown(1.0);
		double WorstQA = 0, WAlt = 0, WQ = 0, WA = 0, WCmd = 0, WAtt = 0, WT = 0;
		for (int32 i = 0; i < 200000 && (S.Phase == EISFlightPhase::Countdown || S.IsFlying()); ++i)
		{
			S.Step(0.02);
			if (S.QPa * S.AoADeg > WorstQA) { WorstQA = S.QPa * S.AoADeg; WAlt = S.AltM; WQ = S.QPa; WA = S.AoADeg; WCmd = S.PitchDeg; WAtt = S.AttDeg; WT = S.T; }
		}
		UE_LOG(LogTemp, Warning, TEXT("[ROCKET]   worst q*AoA %.0f kPa*deg at t=%.0f s, %.1f km, q %.1f kPa, AoA %.1f (cmd %.1f, actual %.1f)"), WorstQA / 1000.0, WT, WAlt / 1000.0, WQ / 1000.0, WA, WCmd, WAtt);
		UE_LOG(LogTemp, Warning, TEXT("[ROCKET] %-14s %6.0f kg -> %s  t=%.0fs apo %.0f peri %.0f km  maxQ %.1f kPa  maxAoA %.1f  maxGimbal %.1f  dv left %.0f  %s"),
			ANSI_TO_TCHAR(C.Id), C.Kg, S.Phase == EISFlightPhase::Orbit ? TEXT("ORBIT ") : TEXT("FAILED"), S.T, S.ApoKm, S.PeriKm, S.MaxQSeen / 1000.0, S.MaxAoASeen, S.MaxGimbalSeen, S.DeltaVLeft(), *S.FailReason);
	}
}
