#include "ISTrajectory.h"
#include "ISOrbit.h"

namespace
{
	const double DaySec = 86400.0;
	// angle flown from the TLI burn to the Moon. Measured: 140 deg -> 4.32 km/s, 150 -> 3.82, 160 -> 3.42, 170 -> 3.18
	// (Apollo: ~3.1); closer to 180 the transfer plane becomes undefined
	const double MoonTransferDeg = 170.0;
	const FVector3d AxX(1, 0, 0), AxZ(0, 0, 1);

	double MuOf(int32 B) { return ISOrbit::Bodies()[B].Mu; }
	FVector3d Unit(const FVector3d& V, const FVector3d& Fallback) { return V.SizeSquared() > 1e-24 ? V.GetSafeNormal() : Fallback; }
	FVector3d Prop(double Mu, FVector3d R, FVector3d V, double Dt, FVector3d* OutV = nullptr)
	{
		ISOrbit::Kepler(Mu, R, V, Dt);
		if (OutV) *OutV = V;
		return R;
	}

	// Arrival hyperbola with periapsis radius Rp, incoming excess velocity Vinf (body frame) and orbit normal Hn
	// (unit, perpendicular to Vinf). Far in the past the craft moves along Vinf; it returns the periapsis state.
	void ArrivalHyperbola(double Mu, double Rp, const FVector3d& Vinf, const FVector3d& Hn, FVector3d& PR, FVector3d& PV)
	{
		const double Vi = Vinf.Size();
		const double Nu = FMath::Acos(-1.0 / (1.0 + Rp * Vi * Vi / Mu));   // true anomaly of the asymptote
		const FVector3d S = Vinf / Vi, Sp = FVector3d::CrossProduct(Hn, S);
		const FVector3d P = -S * FMath::Cos(Nu) - Sp * FMath::Sin(Nu);
		PR = P * Rp;
		PV = FVector3d::CrossProduct(Hn, P) * FMath::Sqrt(Vi * Vi + 2.0 * Mu / Rp);
	}
	// a polar science orbit: its normal lies in the body's equator (perpendicular to the spin axis) and contains the approach
	FVector3d PolarNormal(const FVector3d& S, const FVector3d& Spin)
	{
		return Unit(FVector3d::CrossProduct(S, Spin), Unit(FVector3d::CrossProduct(S, AxX), FVector3d(0, 1, 0)));
	}

	// Departure: periapsis at the parking-orbit point Rd; the escape velocity is perpendicular to the radius, on the side of
	// the wanted excess velocity, so the outgoing asymptote leans toward it (exact when Rd was picked by BestDeparture).
	FVector3d EscapeVelocity(double Mu, const FVector3d& Rd, const FVector3d& Vpark, const FVector3d& Vinf)
	{
		const FVector3d P = Rd.GetSafeNormal();
		const FVector3d Q = Unit(Vinf - P * FVector3d::DotProduct(Vinf, P), Vpark.GetSafeNormal());
		return Q * FMath::Sqrt(Vinf.SizeSquared() + 2.0 * Mu / Rd.Size());
	}
	// the parking-orbit moment (within [T0, T1]) whose escape asymptote points closest to Dir
	double BestDeparture(double Mu, const FVector3d& ParkR, const FVector3d& ParkV, double Vinf, const FVector3d& Dir, double T0, double T1)
	{
		const FVector3d H = Unit(FVector3d::CrossProduct(ParkR, ParkV), AxZ);
		double Best = T0, BestDot = -2.0;
		for (double T = T0; T <= T1; T += 5.0)
		{
			const FVector3d R = Prop(Mu, ParkR, ParkV, T);
			const double Rp = R.Size(), Nu = FMath::Acos(-1.0 / (1.0 + Rp * Vinf * Vinf / Mu));
			const FVector3d P = R / Rp, Q = FVector3d::CrossProduct(H, P);
			const double D = FVector3d::DotProduct(P * FMath::Cos(Nu) + Q * FMath::Sin(Nu), Dir);
			if (D > BestDot) { BestDot = D; Best = T; }
		}
		return Best;
	}
	FVector3d BodyP(int32 B, double T) { return ISOrbit::BodyPos(B, T); }
	FVector3d BodyV(int32 B, double T) { return ISOrbit::BodyVel(B, T); }
	bool Finite(const FISTrajectory& T)
	{
		for (const FISLeg& L : T.Legs) if (L.R.ContainsNaN() || L.V.ContainsNaN()) return false;
		return true;
	}
	// ISOrbit time at which Earth's velocity points along +-N (so an escape in the parking plane leaves along Earth's path)
	TArray<double> EarthAlignedTimes(const FVector3d& N, int32 Count)
	{
		const FISBody& E = ISOrbit::Bodies()[1];
		const double nE = FMath::Sqrt(MuOf(0) / (E.OrbitR * E.OrbitR * E.OrbitR)), Year = 2.0 * PI / nE;
		TArray<double> Out;
		for (int32 k = 0; k < Count; ++k)
		{
			const double S = (k % 2) ? -1.0 : 1.0;
			double Th = FMath::Atan2(-S * N.X, S * N.Y) - E.Phase0;   // velocity (-sin, cos) = S * N
			Th = FMath::Fmod(Th, 2.0 * PI); if (Th < 0.0) Th += 2.0 * PI;
			Out.Add(Th / nE + (k / 2) * Year);
		}
		return Out;
	}
}

int32 FISTrajectory::LegAt(double T) const
{
	for (int32 i = Legs.Num() - 1; i > 0; --i) if (T >= Legs[i].T0) return i;
	return 0;
}

int32 FISTrajectory::State(double T, FVector3d& R, FVector3d& V) const
{
	if (Legs.Num() == 0) return -1;
	const FISLeg& L = Legs[LegAt(T)];
	R = L.R; V = L.V;
	ISOrbit::Kepler(MuOf(L.Body), R, V, T - L.TRef);
	return L.Body;
}

FVector3d FISTrajectory::HelioPos(double T) const
{
	FVector3d R, V;
	const int32 B = State(T, R, V);
	return B > 0 ? BodyP(B, Epoch + T) + R : R;
}

FVector3d FISTrajectory::PosAround(int32 Body, double T) const
{
	FVector3d R, V;
	const int32 B = State(T, R, V);
	if (B == Body) return R;   // same frame: exact, no big heliocentric numbers
	if (B > 0 && ISOrbit::Bodies()[B].Parent == Body) return ISOrbit::BodyPos(B, Epoch + T, true) + R;   // e.g. Moon orbit seen from Earth
	return HelioPos(T) - (Body > 0 ? BodyP(Body, Epoch + T) : FVector3d::ZeroVector);
}

// ------------------------------------------------------------------------------------------------ Moon
FISTrajectory FISTrajectory::Moon(const FVector3d& ParkR, const FVector3d& ParkV, double DepartDay, double ArriveDay, double AltKm)
{
	using namespace ISOrbit;
	FISTrajectory T;
	const int32 E = FindBody(TEXT("Earth")), M = FindBody(TEXT("Moon"));
	T.Target = M;
	const FISBody& Mb = Bodies()[M];
	const double MuE = MuOf(E), MuM = Mb.Mu, Rp = Mb.Radius + AltKm;
	const FVector3d Hp = Unit(FVector3d::CrossProduct(ParkR, ParkV), AxZ);
	const FVector3d Hm = Unit(FVector3d::CrossProduct(BodyPos(M, 0, true), BodyVel(M, 0, true)), AxZ);
	const FVector3d Node = Unit(FVector3d::CrossProduct(Hp, Hm), Unit(ParkR, AxX));

	// The Moon is met where the parking-orbit plane crosses the Moon's orbit plane (a point in both planes), so the whole
	// transfer stays in the parking plane - no costly plane change at the burn. Departure: the moment the parking orbit is
	// MoonTransferDeg short of that point (whichever node gives a burn time nearest the planned day).
	const double Dd = DepartDay * DaySec;
	double TD = Dd, BestGap = 1e30;
	FVector3d U = Node;
	for (const double Sgn : { 1.0, -1.0 })
	{
		const FVector3d D = FQuat4d(Hp, FMath::DegreesToRadians(-MoonTransferDeg)).RotateVector(Node * Sgn);   // burn direction
		const FVector3d W = FVector3d::CrossProduct(Hp, D);
		double PrevT = 0.5 * Dd, PrevS = FVector3d::DotProduct(Prop(MuE, ParkR, ParkV, PrevT), W);
		for (double t = PrevT + 10.0; t <= 1.5 * Dd; t += 10.0)
		{
			const FVector3d Rt = Prop(MuE, ParkR, ParkV, t);
			const double S = FVector3d::DotProduct(Rt, W);
			if ((S > 0.0) != (PrevS > 0.0) && FVector3d::DotProduct(Rt, D) > 0.0)
			{
				double A = PrevT, B = t, Sa = PrevS;
				for (int32 i = 0; i < 40; ++i)
				{
					const double Mid = 0.5 * (A + B), Sm = FVector3d::DotProduct(Prop(MuE, ParkR, ParkV, Mid), W);
					if ((Sm > 0.0) == (Sa > 0.0)) { A = Mid; Sa = Sm; } else B = Mid;
				}
				const double Tc = 0.5 * (A + B);
				if (FMath::Abs(Tc - Dd) < BestGap) { BestGap = FMath::Abs(Tc - Dd); TD = Tc; U = Node * Sgn; }
			}
			PrevT = t; PrevS = S;
		}
	}
	FVector3d VPark;
	const FVector3d Rd = Prop(MuE, ParkR, ParkV, TD, &VPark);
	const double TA = ArriveDay * DaySec;

	// phase the Moon: at arrival it stands on that crossing point U
	const double I = FMath::DegreesToRadians(Mb.OrbitInc), Nm = FMath::Sqrt(MuE / (Mb.OrbitR * Mb.OrbitR * Mb.OrbitR));
	const double ThU = FMath::Atan2(FVector3d::DotProduct(U, FVector3d(0, FMath::Cos(I), FMath::Sin(I))), U.X);
	T.Epoch = FMath::Fmod((ThU - Mb.Phase0) / Nm - TA, 2.0 * PI / Nm);
	if (T.Epoch < 0.0) T.Epoch += 2.0 * PI / Nm;
	auto MoonP = [&](double t) { return BodyPos(M, T.Epoch + t, true); };
	auto MoonV = [&](double t) { return BodyVel(M, T.Epoch + t, true); };

	// Aim at the point where the arrival hyperbola enters the Moon's sphere of influence (iterate: aim -> v_inf -> hyperbola).
	// The translunar burn aims at that point flattened into the parking plane (no plane change at 11 km/s); a mid-course
	// correction at 60% of the way, where the craft is slow, puts it on the real entry point - as Apollo did.
	FVector3d Aim = MoonP(TA), V1 = FVector3d::ZeroVector, V2 = FVector3d::ZeroVector, PR = FVector3d::ZeroVector, PV = FVector3d::ZeroVector;
	FVector3d RM = FVector3d::ZeroVector, VM = FVector3d::ZeroVector, VA = FVector3d::ZeroVector;
	auto InPlane = [&](const FVector3d& X) { return (X - Hp * FVector3d::DotProduct(X, Hp)).GetSafeNormal() * X.Size(); };
	double TS = TA, TM = TD;
	bool bOk = true;
	auto Shoot = [&]()
	{
		TM = TD + 0.6 * (TS - TD);
		FVector3d Unused;
		bOk = bOk && Lambert(MuE, Rd, InPlane(Aim), TS - TD, Hp, V1, Unused);
		RM = Prop(MuE, Rd, V1, TM - TD, &VA);
		bOk = bOk && Lambert(MuE, RM, Aim, TS - TM, Hp, VM, V2);   // ends exactly where the hyperbola begins
	};
	for (int32 It = 0; It < 8 && bOk; ++It)
	{
		Shoot();
		const FVector3d Rel = V2 - MoonV(TS);
		const double Vi = It == 0 ? Rel.Size() : FMath::Sqrt(FMath::Max(Rel.SizeSquared() - 2.0 * MuM / Mb.Soi, 1e-4));
		const FVector3d S = Rel.GetSafeNormal();
		ArrivalHyperbola(MuM, Rp, S * Vi, PolarNormal(S, Hm), PR, PV);
		const double Dt = HyperbolicTime(MuM, Rp, Vi, Mb.Soi);
		TS = TA - Dt;
		Aim = MoonP(TS) + Prop(MuM, PR, PV, -Dt);
	}
	Shoot();

	const double Vc = FMath::Sqrt(MuM / Rp);
	T.DepartT = TD; T.ArriveT = TA;
	T.Legs.Add({ E, 0.0, TD, 0.0, ParkR, ParkV });
	T.Legs.Add({ E, TD, TM, TD, Rd, V1 });
	T.Legs.Add({ E, TM, TS, TM, RM, VM });
	T.Legs.Add({ M, TS, TA, TA, PR, PV });
	T.Legs.Add({ M, TA, 1e30, TA, PR, PV.GetSafeNormal() * Vc });
	T.Burns.Add({ TD, (V1 - VPark).Size() * 1000.0, TEXT("Translunar injection") });
	T.Burns.Add({ TM, (VM - VA).Size() * 1000.0, TEXT("Mid-course correction") });
	T.Burns.Add({ TA, (PV.Size() - Vc) * 1000.0, TEXT("Lunar orbit insertion") });
	T.bValid = bOk && Finite(T);
	return T;
}

// ------------------------------------------------------------------------------------------------ Jupiter
FISTrajectory FISTrajectory::Jupiter(const FVector3d& ParkR, const FVector3d& ParkV, double DepartDay, double ArriveDay, double PeriAltKm)
{
	using namespace ISOrbit;
	FISTrajectory T;
	const int32 E = FindBody(TEXT("Earth")), J = FindBody(TEXT("Jupiter"));
	T.Target = J;
	const FISBody& Eb = Bodies()[E];
	const FISBody& Jb = Bodies()[J];
	const double MuS = MuOf(0), MuE = Eb.Mu, MuJ = Jb.Mu, Rp = Jb.Radius + PeriAltKm;
	const double Dd = DepartDay * DaySec, TA = ArriveDay * DaySec;
	const FVector3d Hp = Unit(FVector3d::CrossProduct(ParkR, ParkV), AxZ);

	// phasing: departure dates when Earth's velocity lies in the parking plane (escape stays in-plane); of those,
	// the one whose Earth -> Jupiter transfer needs the least excess speed
	double BestVi = 1e30;
	for (const double Tabs : EarthAlignedTimes(Unit(FVector3d::CrossProduct(Hp, AxZ), AxX), 14))
	{
		const double Ep = Tabs - Dd;
		FVector3d V1, V2;
		if (!Lambert(MuS, BodyP(E, Ep + Dd), BodyP(J, Ep + TA), TA - Dd, AxZ, V1, V2)) continue;
		const double Vi = (V1 - BodyV(E, Ep + Dd)).Size();
		if (Vi < BestVi) { BestVi = Vi; T.Epoch = Ep; }
	}
	auto JupP = [&](double t) { return BodyP(J, T.Epoch + t); };
	auto JupV = [&](double t) { return BodyV(J, T.Epoch + t); };

	// heliocentric transfer from Earth's centre to the point where the arrival hyperbola enters Jupiter's sphere of influence
	FVector3d Aim = JupP(TA), V1 = FVector3d::ZeroVector, V2 = FVector3d::ZeroVector, PR = FVector3d::ZeroVector, PV = FVector3d::ZeroVector;
	double TS = TA;
	bool bOk = true;
	auto ArriveFrom = [&](const FVector3d& Vhelio, int32 It)
	{
		const FVector3d Rel = Vhelio - JupV(TS);
		const double Vi = It == 0 ? Rel.Size() : FMath::Sqrt(FMath::Max(Rel.SizeSquared() - 2.0 * MuJ / Jb.Soi, 1e-4));
		const FVector3d S = Rel.GetSafeNormal();
		ArrivalHyperbola(MuJ, Rp, S * Vi, PolarNormal(S, AxZ), PR, PV);
		const double Dt = HyperbolicTime(MuJ, Rp, Vi, Jb.Soi);
		TS = TA - Dt;
		Aim = JupP(TS) + Prop(MuJ, PR, PV, -Dt);
	};
	for (int32 It = 0; It < 6 && bOk; ++It)
	{
		bOk = Lambert(MuS, BodyP(E, T.Epoch + Dd), Aim, TS - Dd, AxZ, V1, V2);
		ArriveFrom(V2, It);
	}
	const FVector3d VinfDep = V1 - BodyV(E, T.Epoch + Dd);

	// leave the parking orbit where the escape asymptote points along the wanted excess velocity
	const double TD = BestDeparture(MuE, ParkR, ParkV, VinfDep.Size(), VinfDep.GetSafeNormal(), 0.5 * Dd, 1.5 * Dd);
	FVector3d VPark;
	const FVector3d Rd = Prop(MuE, ParkR, ParkV, TD, &VPark);
	const FVector3d V0 = EscapeVelocity(MuE, Rd, VPark, VinfDep);
	const double TX = TD + HyperbolicTime(MuE, Rd.Size(), VinfDep.Size(), Eb.Soi);
	FVector3d ExV;
	const FVector3d Ex = Prop(MuE, Rd, V0, TX - TD, &ExV);
	const FVector3d HR = BodyP(E, T.Epoch + TX) + Ex, HV = BodyV(E, T.Epoch + TX) + ExV;

	// cruise, then a mid-course correction re-aims at the entry point (absorbs the small departure errors)
	const double TM = FMath::Clamp(200.0 * DaySec, TX + DaySec, TS - 30.0 * DaySec);
	FVector3d MV = FVector3d::ZeroVector, Vmcc = FVector3d::ZeroVector, V2b = FVector3d::ZeroVector;
	const FVector3d MR = Prop(MuS, HR, HV, TM - TX, &MV);
	for (int32 It = 0; It < 4 && bOk; ++It)
	{
		bOk = Lambert(MuS, MR, Aim, TS - TM, AxZ, Vmcc, V2b);
		if (It < 3) ArriveFrom(V2b, 1);
	}

	// Juno's science orbit: 53.5-day polar ellipse with periapsis just above the clouds
	const double Ajuno = FMath::Pow(MuJ * FMath::Square(53.5 * DaySec / (2.0 * PI)), 1.0 / 3.0);
	const double Vp = FMath::Sqrt(MuJ * (2.0 / Rp - 1.0 / Ajuno));
	T.DepartT = TD; T.ArriveT = TA;
	T.Legs.Add({ E, 0.0, TD, 0.0, ParkR, ParkV });
	T.Legs.Add({ E, TD, TX, TD, Rd, V0 });
	T.Legs.Add({ 0, TX, TM, TX, HR, HV });
	T.Legs.Add({ 0, TM, TS, TM, MR, Vmcc });
	T.Legs.Add({ J, TS, TA, TA, PR, PV });
	T.Legs.Add({ J, TA, 1e30, TA, PR, PV.GetSafeNormal() * Vp });
	T.Burns.Add({ TD, (V0 - VPark).Size() * 1000.0, TEXT("Earth departure") });
	T.Burns.Add({ TM, (Vmcc - MV).Size() * 1000.0, TEXT("Mid-course correction") });
	T.Burns.Add({ TA, (PV.Size() - Vp) * 1000.0, TEXT("Jupiter orbit insertion") });
	T.bValid = bOk && Finite(T);
	return T;
}

// ------------------------------------------------------------------------------------------------ interstellar
FISTrajectory FISTrajectory::Interstellar(const FVector3d& ParkR, const FVector3d& ParkV, double DepartDay, double EndDay)
{
	using namespace ISOrbit;
	FISTrajectory T;
	const int32 E = FindBody(TEXT("Earth"));
	const FISBody& Eb = Bodies()[E];
	const double MuS = MuOf(0), MuE = Eb.Mu, Dd = DepartDay * DaySec, TEnd = EndDay * DaySec;
	const FVector3d Hp = Unit(FVector3d::CrossProduct(ParkR, ParkV), AxZ);
	T.Epoch = EarthAlignedTimes(Unit(FVector3d::CrossProduct(Hp, AxZ), AxX), 1)[0] - Dd;

	// leave along Earth's path, 5 deg out of the planets' plane (like Voyager 1); bisect the speed so the probe
	// crosses the heliopause (120 AU) on the last day of the mission
	const FVector3d RE = BodyP(E, T.Epoch + Dd), VE = BodyV(E, T.Epoch + Dd);
	const FVector3d Dir = (VE.GetSafeNormal() * FMath::Cos(FMath::DegreesToRadians(5.0)) + AxZ * FMath::Sin(FMath::DegreesToRadians(5.0))).GetSafeNormal();
	const double R1 = RE.Size(), R120 = 120.0 * 1.495978707e8, Vesc = FMath::Sqrt(2.0 * MuS / R1);
	double Lo = Vesc + 0.01, Hi = 400.0;
	for (int32 i = 0; i < 80; ++i)
	{
		const double Mid = 0.5 * (Lo + Hi);
		const double Tof = HyperbolicTime(MuS, R1, FMath::Sqrt(Mid * Mid - Vesc * Vesc), R120);
		if (Tof > TEnd - Dd) Lo = Mid; else Hi = Mid;
	}
	const FVector3d VinfDep = Dir * (0.5 * (Lo + Hi)) - VE;

	const double TD = BestDeparture(MuE, ParkR, ParkV, VinfDep.Size(), VinfDep.GetSafeNormal(), 0.5 * Dd, 1.5 * Dd);
	FVector3d VPark;
	const FVector3d Rd = Prop(MuE, ParkR, ParkV, TD, &VPark);
	const FVector3d V0 = EscapeVelocity(MuE, Rd, VPark, VinfDep);
	const double TX = TD + HyperbolicTime(MuE, Rd.Size(), VinfDep.Size(), Eb.Soi);
	FVector3d ExV;
	const FVector3d Ex = Prop(MuE, Rd, V0, TX - TD, &ExV);
	T.DepartT = TD; T.ArriveT = TEnd;
	T.Legs.Add({ E, 0.0, TD, 0.0, ParkR, ParkV });
	T.Legs.Add({ E, TD, TX, TD, Rd, V0 });
	T.Legs.Add({ 0, TX, 1e30, TX, BodyP(E, T.Epoch + TX) + Ex, BodyV(E, T.Epoch + TX) + ExV });
	T.Burns.Add({ TD, (V0 - VPark).Size() * 1000.0, TEXT("Earth escape") });
	T.bValid = Finite(T);
	return T;
}

// ------------------------------------------------------------------------------------------------ self-test
// Dev check (-ISTrajectoryTest): every plan from a typical Cape Canaveral parking orbit must join up, arrive on time at the
// right height and cost a believable delta-v.
void ISTrajectorySelfTest()
{
	auto Log = [](bool bOk, const FString& Msg) { UE_LOG(LogTemp, Warning, TEXT("[TRAJ] %s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *Msg); };
	const double R = 6371.0 + 250.0, Vc = FMath::Sqrt(398600.4418 / R);
	const FQuat4d Q = FQuat4d(AxZ, FMath::DegreesToRadians(20.0)) * FQuat4d(AxX, FMath::DegreesToRadians(28.5));
	const FVector3d PR = Q.RotateVector(FVector3d(R, 0, 0)), PV = Q.RotateVector(FVector3d(0, Vc, 0));

	// largest position jump where one leg hands over to the next (both expressed around the Sun)
	auto Joins = [](const FISTrajectory& T)
	{
		double Worst = 0;
		for (int32 i = 1; i < T.Legs.Num(); ++i)
		{
			const FISLeg& A = T.Legs[i - 1];
			const FISLeg& B = T.Legs[i];
			FVector3d Ra = A.R, Va = A.V, Rb = B.R, Vb = B.V;
			ISOrbit::Kepler(MuOf(A.Body), Ra, Va, B.T0 - A.TRef);
			ISOrbit::Kepler(MuOf(B.Body), Rb, Vb, B.T0 - B.TRef);
			if (A.Body > 0) Ra += ISOrbit::BodyPos(A.Body, T.Epoch + B.T0);
			if (B.Body > 0) Rb += ISOrbit::BodyPos(B.Body, T.Epoch + B.T0);
			Worst = FMath::Max(Worst, (Ra - Rb).Size());
		}
		return Worst;
	};
	auto Burns = [](const FISTrajectory& T)
	{
		FString S;
		for (const FISBurn& B : T.Burns) S += FString::Printf(TEXT(" %s %.0f m/s @ day %.2f;"), *B.What, B.DvMs, B.T / DaySec);
		return S;
	};
	auto Peri = [](const FISTrajectory& T)
	{
		const FISLeg& L = T.Legs.Last();
		return L.R.Size() - ISOrbit::Bodies()[L.Body].Radius;
	};

	{	// Moon: 4.5 days, 50 km polar orbit
		const FISTrajectory T = FISTrajectory::Moon(PR, PV, 0.1, 4.5, 50.0);
		const double J = Joins(T), Alt = Peri(T);
		const bool bOk = T.bValid && J < 1.0 && FMath::Abs(Alt - 50.0) < 1.0 && FMath::Abs(T.ArriveT / DaySec - 4.5) < 1e-6
			&& FMath::IsWithin(T.Burns[0].DvMs, 3000.0, 3300.0) && T.Burns[1].DvMs < 250.0 && FMath::IsWithin(T.Burns[2].DvMs, 600.0, 1100.0);
		Log(bOk, FString::Printf(TEXT("Moon: joins within %.3f km, periapsis %.2f km, arrive day %.2f;%s"), J, Alt, T.ArriveT / DaySec, *Burns(T)));
	}
	{	// Jupiter: direct transfer, Juno-like 53.5-day orbit 4,200 km above the clouds
		const double Arr = 0.1 + PI * FMath::Sqrt(FMath::Pow(0.5 * (149.598e6 + 778.57e6), 3) / MuOf(0)) / DaySec;
		const FISTrajectory T = FISTrajectory::Jupiter(PR, PV, 0.1, Arr, 4200.0);
		const double J = Joins(T), Alt = Peri(T);
		const bool bOk = T.bValid && J < 1.0 && FMath::Abs(Alt - 4200.0) < 1.0
			&& FMath::IsWithin(T.Burns[0].DvMs, 6000.0, 7500.0) && T.Burns[1].DvMs < 1000.0 && FMath::IsWithin(T.Burns[2].DvMs, 300.0, 1200.0);
		Log(bOk, FString::Printf(TEXT("Jupiter: joins within %.3f km, periapsis %.1f km, arrive day %.1f;%s"), J, Alt, T.ArriveT / DaySec, *Burns(T)));
	}
	{	// interstellar: 120 AU after 35 years
		const FISTrajectory T = FISTrajectory::Interstellar(PR, PV, 0.1, 35.0 * 365.25);
		const double J = Joins(T), Au = T.HelioPos(T.ArriveT).Size() / 1.495978707e8;
		const bool bOk = T.bValid && J < 1.0 && FMath::Abs(Au - 120.0) < 6.0;
		Log(bOk, FString::Printf(TEXT("Interstellar: joins within %.3f km, %.1f AU at the end;%s"), J, Au, *Burns(T)));
	}
	{	// the arrival hyperbola really comes in along the asked direction
		FVector3d Rp, Vp;
		const FVector3d S = FVector3d(0.3, 0.9, 0.2).GetSafeNormal();
		ArrivalHyperbola(4902.8, 1787.4, S * 0.8, PolarNormal(S, AxZ), Rp, Vp);
		FVector3d Rf = Rp, Vf = Vp;
		ISOrbit::Kepler(4902.8, Rf, Vf, -10.0 * DaySec);
		const double D = FVector3d::DotProduct(Vf.GetSafeNormal(), S);
		Log(D > 0.995, FString::Printf(TEXT("arrival hyperbola: incoming direction matches (cos %.4f)"), D));
	}
}
