#include "ISNBody.h"
#include "ISOrbit.h"

int32 FISNBodySim::Add(const FString& Name, double Mu, double Radius, const FVector3d& R, const FVector3d& V)
{
	FISNBody X;
	X.Name = Name; X.Mu = Mu; X.Radius = Radius; X.R = R; X.V = V;
	bAccValid = false;
	return B.Add(X);
}

// pairwise accelerations, and the next substep from the tightest pair (orbit, fly-by or impact course alike)
void FISNBodySim::ComputeAcc()
{
	const int32 N = B.Num();
	Acc.Init(FVector3d::ZeroVector, N);
	double MinT2 = TNumericLimits<double>::Max();
	for (int32 i = 0; i < N; ++i)
	{
		if (!B[i].bAlive) continue;
		for (int32 j = i + 1; j < N; ++j)
		{
			if (!B[j].bAlive) continue;
			const FVector3d D = B[j].R - B[i].R;
			const double D2 = D.SizeSquared();
			if (D2 <= 0.0) continue;
			const double D1 = FMath::Sqrt(D2);
			const FVector3d U = D / (D2 * D1);
			Acc[i] += U * B[j].Mu;
			Acc[j] -= U * B[i].Mu;
			const double M = B[i].Mu + B[j].Mu;
			if (M > 0.0) MinT2 = FMath::Min(MinT2, D2 * D1 / M);
		}
	}
	NextDt = MinT2 < TNumericLimits<double>::Max() ? FMath::Max(1e-3, StepFraction * FMath::Sqrt(MinT2)) : 3600.0;
	bAccValid = true;
}

// touching bodies merge into the heavier one: mass and momentum kept, volume added
bool FISNBodySim::Collide()
{
	bool bAny = false;
	for (int32 i = 0; i < B.Num(); ++i)
	{
		for (int32 j = i + 1; j < B.Num() && B[i].bAlive; ++j)
		{
			if (!B[j].bAlive) continue;
			const double Rs = B[i].Radius + B[j].Radius;
			if ((B[j].R - B[i].R).SizeSquared() >= Rs * Rs) continue;
			const int32 W = B[i].Mu >= B[j].Mu ? i : j, L = W == i ? j : i;
			FISNBody& A = B[W];
			FISNBody& C = B[L];
			const double M = A.Mu + C.Mu;
			if (M > 0.0)
			{
				A.R = (A.R * A.Mu + C.R * C.Mu) / M;
				A.V = (A.V * A.Mu + C.V * C.Mu) / M;
			}
			A.Mu = M;
			A.Radius = FMath::Pow(A.Radius * A.Radius * A.Radius + C.Radius * C.Radius * C.Radius, 1.0 / 3.0);
			C.bAlive = false;
			LastEvent = FString::Printf(TEXT("%s crashed into %s."), *C.Name, *A.Name);
			bAny = true;
		}
	}
	return bAny;
}

void FISNBodySim::Advance(double Dt, int32 MaxSubsteps)
{
	bLimited = false;
	if (Dt <= 0.0 || B.Num() == 0) return;
	if (!bAccValid) ComputeAcc();
	double Left = Dt;
	int32 Steps = 0;
	while (Left > 1e-9)
	{
		if (Steps++ >= MaxSubsteps) { bLimited = true; return; }
		const double H = FMath::Min(Left, NextDt);
		for (int32 i = 0; i < B.Num(); ++i) if (B[i].bAlive) { B[i].V += Acc[i] * (0.5 * H); B[i].R += B[i].V * H; }
		Collide();
		ComputeAcc();
		for (int32 i = 0; i < B.Num(); ++i) if (B[i].bAlive) B[i].V += Acc[i] * (0.5 * H);
		T += H;
		Left -= H;
	}
}

int32 FISNBodySim::Heaviest() const
{
	int32 H = -1;
	for (int32 i = 0; i < B.Num(); ++i) if (B[i].bAlive && (H < 0 || B[i].Mu > B[H].Mu)) H = i;
	return H;
}

// heaviest first, so each body's Hill sphere is measured around its own primary (a moon's around its planet, not the Sun)
void FISNBodySim::Primaries(TArray<int32>& P) const
{
	P.Init(-1, B.Num());
	TArray<int32> Order;
	for (int32 i = 0; i < B.Num(); ++i) if (B[i].bAlive) Order.Add(i);
	Order.Sort([this](int32 X, int32 Y) { return B[X].Mu > B[Y].Mu; });
	TArray<double> Hill;
	Hill.Init(TNumericLimits<double>::Max(), B.Num());
	for (int32 k = 1; k < Order.Num(); ++k)
	{
		const int32 i = Order[k];
		int32 Best = Order[0];
		double BestHill = TNumericLimits<double>::Max();
		for (int32 m = 1; m < k; ++m)
		{
			const int32 j = Order[m];
			if (B[j].Mu > B[i].Mu && Hill[j] < BestHill && (B[i].R - B[j].R).Size() < Hill[j]) { Best = j; BestHill = Hill[j]; }
		}
		P[i] = Best;
		Hill[i] = (B[i].R - B[Best].R).Size() * FMath::Pow(B[i].Mu / (3.0 * B[Best].Mu), 1.0 / 3.0);
	}
}

int32 FISNBodySim::Primary(int32 i) const
{
	TArray<int32> P;   // ponytail: O(n^2) per call, fine for ~20 bodies; cache per frame if the sandbox grows to hundreds
	Primaries(P);
	return P.IsValidIndex(i) ? P[i] : -1;
}

double FISNBodySim::Energy() const
{
	double E = 0.0;
	for (int32 i = 0; i < B.Num(); ++i)
	{
		if (!B[i].bAlive) continue;
		E += 0.5 * B[i].Mu * B[i].V.SizeSquared();
		for (int32 j = i + 1; j < B.Num(); ++j)
			if (B[j].bAlive) E -= B[i].Mu * B[j].Mu / FMath::Max((B[j].R - B[i].R).Size(), 1e-9);
	}
	return E;
}

FVector3d FISNBodySim::Momentum() const
{
	FVector3d P = FVector3d::ZeroVector;
	for (const FISNBody& X : B) if (X.bAlive) P += X.V * X.Mu;
	return P;
}

TArrayView<const ISNBody::FMoon> ISNBody::Moons()
{
	static const FMoon L[] = {
		{ TEXT("Io"),       TEXT("Jupiter"), 5959.9, 1821.6,  421700.0,   3.13, 0.4 },
		{ TEXT("Europa"),   TEXT("Jupiter"), 3202.7, 1560.8,  671034.0,   3.13, 2.1 },
		{ TEXT("Ganymede"), TEXT("Jupiter"), 9887.8, 2634.1, 1070412.0,   3.13, 4.0 },
		{ TEXT("Callisto"), TEXT("Jupiter"), 7179.3, 2410.3, 1882709.0,   3.13, 5.5 },
		{ TEXT("Titan"),    TEXT("Saturn"),  8978.1, 2574.7, 1221870.0,  26.73, 1.0 },
		{ TEXT("Triton"),   TEXT("Neptune"), 1427.6, 1353.4,  354759.0, 185.32, 3.0 },
	};
	return L;
}

void ISNBody::SeedSolarSystem(FISNBodySim& S, double T0)
{
	S = FISNBodySim();
	S.T = T0;
	const TArray<FISBody>& L = ISOrbit::Bodies();
	for (int32 i = 0; i < L.Num(); ++i) S.Add(L[i].Name, L[i].Mu, L[i].Radius, ISOrbit::BodyPos(i, T0), ISOrbit::BodyVel(i, T0));

	for (const FMoon& M : Moons())
	{
		const int32 P = S.Find(M.Parent);
		if (P < 0) continue;
		const double I = FMath::DegreesToRadians(M.IncDeg), C = FMath::Cos(M.Phase), Sn = FMath::Sin(M.Phase);
		const double Vc = ISOrbit::CircularSpeed(S.B[P].Mu + M.Mu, M.A);
		const FVector3d R(C * M.A, Sn * M.A * FMath::Cos(I), Sn * M.A * FMath::Sin(I));
		const FVector3d V(-Sn * Vc, C * Vc * FMath::Cos(I), C * Vc * FMath::Sin(I));
		S.Add(M.Name, M.Mu, M.R, S.B[P].R + R, S.B[P].V + V);
	}

	// barycentric frame: otherwise the whole system slowly drifts away
	double Total = 0.0;
	for (const FISNBody& X : S.B) Total += X.Mu;
	const FVector3d Drift = S.Momentum() / Total;
	for (FISNBody& X : S.B) X.V -= Drift;
	S.Changed();
}

// Dev check (-ISNBodyTest): integrator accuracy, long-run stability, merge conservation, extreme edits.
void ISNBodySelfTest()
{
	auto Log = [](bool bOk, const FString& Msg) { UE_LOG(LogTemp, Warning, TEXT("[NBODY] %s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *Msg); };
	const double Day = 86400.0;

	{	// 1. Earth-Moon two-body: back to the start after one period
		FISNBodySim S;
		const double Me = 398600.4418, Mm = 4902.8, A = 384400.0;
		S.Add(TEXT("Earth"), Me, 6371.0, FVector3d::ZeroVector, FVector3d::ZeroVector);
		S.Add(TEXT("Moon"), Mm, 1737.4, FVector3d(A, 0, 0), FVector3d(0, ISOrbit::CircularSpeed(Me + Mm, A), 0));
		const double P = 2.0 * PI * FMath::Sqrt(A * A * A / (Me + Mm));
		S.Advance(P, 1000000);
		const double Err = ((S.B[1].R - S.B[0].R) - FVector3d(A, 0, 0)).Size() / A;
		Log(Err < 0.01, FString::Printf(TEXT("Earth-Moon one orbit (%.2f d): back within %.3f%% of the start"), P / Day, Err * 100.0));
	}
	{	// 2. whole solar system for 10 years: energy kept, nobody wandered off
		FISNBodySim S;
		ISNBody::SeedSolarSystem(S);
		const double E0 = S.Energy();
		for (int32 d = 0; d < 3653; ++d) S.Advance(Day, 1000000);
		const double Drift = FMath::Abs((S.Energy() - E0) / E0);
		const TMap<FString, FString> Expect = {
			{ TEXT("Moon"), TEXT("Earth") }, { TEXT("Io"), TEXT("Jupiter") }, { TEXT("Europa"), TEXT("Jupiter") }, { TEXT("Ganymede"), TEXT("Jupiter") },
			{ TEXT("Callisto"), TEXT("Jupiter") }, { TEXT("Titan"), TEXT("Saturn") }, { TEXT("Triton"), TEXT("Neptune") } };
		FString Bad;
		for (int32 i = 0; i < S.B.Num(); ++i)
		{
			if (S.B[i].Name == TEXT("Sun")) continue;
			const FString* E = Expect.Find(S.B[i].Name);
			const FString Want = E ? *E : FString(TEXT("Sun"));
			const int32 P = S.Primary(i);
			const FString Got = !S.B[i].bAlive ? FString(TEXT("(merged)")) : P >= 0 ? S.B[P].Name : FString(TEXT("(none)"));
			if (Got != Want) Bad += FString::Printf(TEXT(" %s->%s"), *S.B[i].Name, *Got);
		}
		Log(Drift < 1e-5 && Bad.IsEmpty(), FString::Printf(TEXT("solar system 10 years: energy drift %.2e, hierarchy %s"), Drift, Bad.IsEmpty() ? TEXT("intact") : *Bad));
	}
	{	// 3. a merge keeps momentum
		FISNBodySim S;
		S.Add(TEXT("A"), 100.0, 10.0, FVector3d::ZeroVector, FVector3d(1, 0, 0));
		S.Add(TEXT("B"), 50.0, 10.0, FVector3d(15, 0, 0), FVector3d(-1, 0.5, 0));
		const FVector3d P0 = S.Momentum();
		S.Advance(1.0);
		const double Err = (S.Momentum() - P0).Size() / P0.Size();
		Log(!S.B[1].bAlive && Err < 1e-9, FString::Printf(TEXT("merge: %s, momentum error %.1e"), *S.LastEvent, Err));
	}
	{	// 4. sandbox abuse: Jupiter x100 for a year must not blow up
		FISNBodySim S;
		ISNBody::SeedSolarSystem(S);
		const int32 J = S.Find(TEXT("Jupiter"));
		S.B[J].Mu *= 100.0;
		S.B[J].Radius *= FMath::Pow(100.0, 1.0 / 3.0);
		S.Changed();
		for (int32 d = 0; d < 365; ++d) S.Advance(Day, 1000000);
		bool bOk = true;
		int32 Alive = 0;
		for (const FISNBody& X : S.B) if (X.bAlive) { ++Alive; bOk &= !X.R.ContainsNaN() && !X.V.ContainsNaN(); }
		Log(bOk, FString::Printf(TEXT("Jupiter x100 for 1 year: finite, %d bodies left%s%s"), Alive, S.LastEvent.IsEmpty() ? TEXT("") : TEXT(", last event: "), *S.LastEvent));
	}
}
