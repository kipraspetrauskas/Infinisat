#include "ISOrbit.h"

namespace
{
	double StumpC(double Z)
	{
		if (Z > 1e-8) return (1.0 - FMath::Cos(FMath::Sqrt(Z))) / Z;
		if (Z < -1e-8) return (FMath::Cosh(FMath::Sqrt(-Z)) - 1.0) / (-Z);
		return 0.5;
	}
	double StumpS(double Z)
	{
		if (Z > 1e-8) { const double s = FMath::Sqrt(Z); return (s - FMath::Sin(s)) / (s * s * s); }
		if (Z < -1e-8) { const double s = FMath::Sqrt(-Z); return (FMath::Sinh(s) - s) / (s * s * s); }
		return 1.0 / 6.0;
	}
	FISBody B(const TCHAR* N, int32 Parent, double Mu, double R, double OrbitR, double Soi, double Phase, const FLinearColor& C, double Inc = 0, double Atmo = 0, const TCHAR* Tex = TEXT(""))
	{
		FISBody X; X.Name = N; X.Parent = Parent; X.Mu = Mu; X.Radius = R; X.OrbitR = OrbitR; X.Soi = Soi; X.Phase0 = Phase; X.Color = C; X.OrbitInc = Inc; X.AtmoTop = Atmo; X.Texture = Tex;
		return X;
	}
}

namespace ISOrbit
{
	const TArray<FISBody>& Bodies()
	{
		static TArray<FISBody> L = {
			B(TEXT("Sun"), -1, 1.32712440018e11, 695700.0, 0, 1e12, 0, FLinearColor(1.f, 0.85f, 0.4f)),
			B(TEXT("Earth"), 0, 398600.4418, 6371.0, 149.598e6, 924000.0, 0.0, FLinearColor(0.3f, 0.55f, 1.f), 0, 100.0, TEXT("/Game/Infinisat/Textures/T_Earth.T_Earth")),
			B(TEXT("Moon"), 1, 4902.8, 1737.4, 384400.0, 66100.0, 2.0, FLinearColor(0.75f, 0.75f, 0.75f), 5.14, 0, TEXT("/Game/Infinisat/Textures/T_Moon.T_Moon")),
			B(TEXT("Mercury"), 0, 22032.0, 2439.7, 57.909e6, 112000.0, 1.1, FLinearColor(0.6f, 0.55f, 0.5f)),
			B(TEXT("Venus"), 0, 324859.0, 6051.8, 108.21e6, 616000.0, 2.5, FLinearColor(0.95f, 0.8f, 0.5f), 0, 250.0),
			B(TEXT("Mars"), 0, 42828.0, 3389.5, 227.92e6, 577000.0, 4.0, FLinearColor(0.9f, 0.4f, 0.2f), 0, 80.0),
			B(TEXT("Jupiter"), 0, 126686534.0, 69911.0, 778.57e6, 48.2e6, 0.6, FLinearColor(0.9f, 0.75f, 0.55f), 0, 1000.0, TEXT("/Game/Infinisat/Textures/T_Jupiter.T_Jupiter")),
			B(TEXT("Saturn"), 0, 37931187.0, 58232.0, 1433.53e6, 54.8e6, 3.3, FLinearColor(0.9f, 0.8f, 0.55f), 0, 1000.0),
			B(TEXT("Uranus"), 0, 5793939.0, 25362.0, 2872.46e6, 51.8e6, 5.1, FLinearColor(0.6f, 0.9f, 0.95f), 0, 1000.0),
			B(TEXT("Neptune"), 0, 6836529.0, 24622.0, 4495.06e6, 86.8e6, 0.2, FLinearColor(0.3f, 0.45f, 1.f), 0, 1000.0),
		};
		return L;
	}

	int32 FindBody(const FString& Name)
	{
		const TArray<FISBody>& L = Bodies();
		for (int32 i = 0; i < L.Num(); ++i) if (L[i].Name == Name) return i;
		return -1;
	}

	static FVector3d LocalPos(const FISBody& Bd, double T)
	{
		const FISBody& P = Bodies()[Bd.Parent];
		const double N = FMath::Sqrt(P.Mu / (Bd.OrbitR * Bd.OrbitR * Bd.OrbitR));
		const double Th = Bd.Phase0 + N * T;
		const double I = FMath::DegreesToRadians(Bd.OrbitInc);
		return FVector3d(FMath::Cos(Th), FMath::Sin(Th) * FMath::Cos(I), FMath::Sin(Th) * FMath::Sin(I)) * Bd.OrbitR;
	}
	static FVector3d LocalVel(const FISBody& Bd, double T)
	{
		const FISBody& P = Bodies()[Bd.Parent];
		const double N = FMath::Sqrt(P.Mu / (Bd.OrbitR * Bd.OrbitR * Bd.OrbitR));
		const double Th = Bd.Phase0 + N * T;
		const double I = FMath::DegreesToRadians(Bd.OrbitInc);
		return FVector3d(-FMath::Sin(Th), FMath::Cos(Th) * FMath::Cos(I), FMath::Cos(Th) * FMath::Sin(I)) * (N * Bd.OrbitR);
	}

	FVector3d BodyPos(int32 Index, double T, bool bLocal)
	{
		const FISBody& Bd = Bodies()[Index];
		if (Bd.Parent < 0) return FVector3d::ZeroVector;
		const FVector3d L = LocalPos(Bd, T);
		return bLocal ? L : L + BodyPos(Bd.Parent, T, false);
	}

	FVector3d BodyVel(int32 Index, double T, bool bLocal)
	{
		const FISBody& Bd = Bodies()[Index];
		if (Bd.Parent < 0) return FVector3d::ZeroVector;
		const FVector3d L = LocalVel(Bd, T);
		return bLocal ? L : L + BodyVel(Bd.Parent, T, false);
	}

	double CircularSpeed(double Mu, double R) { return FMath::Sqrt(Mu / R); }

	// Universal-variable Kepler propagation (Curtis, Orbital Mechanics for Engineering Students, alg. 3.3/3.4)
	void Kepler(double Mu, FVector3d& R0, FVector3d& V0, double Dt)
	{
		if (FMath::Abs(Dt) < 1e-9) return;
		const double SqMu = FMath::Sqrt(Mu);
		const double r0 = R0.Length();
		const double v0 = V0.Length();
		const double vr0 = FVector3d::DotProduct(R0, V0) / r0;
		const double Alpha = 2.0 / r0 - v0 * v0 / Mu;
		double X = SqMu * FMath::Abs(Alpha) * Dt;
		if (FMath::Abs(Alpha) < 1e-10) X = SqMu * Dt / r0;
		for (int32 It = 0; It < 60; ++It)
		{
			const double Z = Alpha * X * X;
			const double C = StumpC(Z), S = StumpS(Z);
			const double F = r0 * vr0 / SqMu * X * X * C + (1.0 - Alpha * r0) * X * X * X * S + r0 * X - SqMu * Dt;
			const double dF = r0 * vr0 / SqMu * X * (1.0 - Alpha * X * X * S) + (1.0 - Alpha * r0) * X * X * C + r0;
			const double Ratio = F / dF;
			X -= Ratio;
			if (FMath::Abs(Ratio) < 1e-9) break;
		}
		const double Z = Alpha * X * X;
		const double C = StumpC(Z), S = StumpS(Z);
		const double f = 1.0 - X * X / r0 * C;
		const double g = Dt - X * X * X * S / SqMu;
		const FVector3d R = R0 * f + V0 * g;
		const double r = R.Length();
		const double fd = SqMu / (r * r0) * (Alpha * X * X * X * S - X);
		const double gd = 1.0 - X * X / r * C;
		V0 = R0 * fd + V0 * gd;
		R0 = R;
	}

	FISElements Elements(double Mu, double BodyR, const FVector3d& R, const FVector3d& V)
	{
		FISElements E;
		const double r = R.Length(), v = V.Length();
		const FVector3d H = FVector3d::CrossProduct(R, V);
		const double h = H.Length();
		const FVector3d Ev = ((v * v - Mu / r) * R - FVector3d::DotProduct(R, V) * V) / Mu;
		E.E = Ev.Length();
		const double Energy = v * v / 2.0 - Mu / r;
		E.A = FMath::Abs(Energy) > 1e-12 ? -Mu / (2.0 * Energy) : 1e30;
		E.Inc = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(H.Z / FMath::Max(h, 1e-12), -1.0, 1.0)));
		const FVector3d N = FVector3d::CrossProduct(FVector3d(0, 0, 1), H);
		const double n = N.Length();
		E.Raan = n > 1e-9 ? FMath::RadiansToDegrees(FMath::Atan2(N.Y, N.X)) : 0.0;
		if (n > 1e-9 && E.E > 1e-9)
		{
			E.ArgP = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector3d::DotProduct(N, Ev) / (n * E.E), -1.0, 1.0)));
			if (Ev.Z < 0) E.ArgP = 360.0 - E.ArgP;
		}
		if (E.E > 1e-9)
		{
			E.Nu = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector3d::DotProduct(Ev, R) / (E.E * r), -1.0, 1.0)));
			if (FVector3d::DotProduct(R, V) < 0) E.Nu = 360.0 - E.Nu;
		}
		const double P = h * h / Mu;
		E.Periapsis = P / (1.0 + E.E) - BodyR;
		if (E.E < 1.0)
		{
			E.Apoapsis = P / (1.0 - E.E) - BodyR;
			E.Period = 2.0 * PI * FMath::Sqrt(E.A * E.A * E.A / Mu);
		}
		else { E.Apoapsis = -1.0; E.Period = 0.0; }
		return E;
	}
}

// ==================================================================================================
FISElements FISCraft::Elements() const
{
	const FISBody& Bd = ISOrbit::Bodies()[Body];
	return ISOrbit::Elements(Bd.Mu, Bd.Radius, R, V);
}

void FISCraft::Place(int32 InBody, double Alt, double IncDeg, double LongDeg)
{
	Body = InBody;
	const FISBody& Bd = ISOrbit::Bodies()[Body];
	const double r = Bd.Radius + Alt;
	const double L = FMath::DegreesToRadians(LongDeg);
	const double vc = ISOrbit::CircularSpeed(Bd.Mu, r);
	FVector3d Rr(FMath::Cos(L) * r, FMath::Sin(L) * r, 0);
	FVector3d Vv(-FMath::Sin(L) * vc, FMath::Cos(L) * vc, 0);
	const double I = FMath::DegreesToRadians(IncDeg);
	auto RotX = [I](const FVector3d& P) { return FVector3d(P.X, P.Y * FMath::Cos(I) - P.Z * FMath::Sin(I), P.Y * FMath::Sin(I) + P.Z * FMath::Cos(I)); };
	R = RotX(Rr); V = RotX(Vv);
	bCrashed = false; DvUsed = 0;
}

void FISCraft::Burn(double Pro, double Nor, double Rad)
{
	const FVector3d P = V.GetSafeNormal();
	const FVector3d N = FVector3d::CrossProduct(R, V).GetSafeNormal();
	const FVector3d Rd = R.GetSafeNormal();
	V += P * Pro + N * Nor + Rd * Rad;
	DvUsed += FMath::Sqrt(Pro * Pro + Nor * Nor + Rad * Rad);
}

void FISCraft::Step(double T, double Dt)
{
	if (bCrashed) return;
	const TArray<FISBody>& L = ISOrbit::Bodies();
	double Left = Dt;
	int32 Guard = 0;
	while (Left > 1e-6 && Guard++ < 5000)
	{
		const FISBody& Bd = L[Body];
		// step size: a small fraction of the local orbital timescale
		const double r = R.Length();
		const double Timescale = r / FMath::Max(V.Length(), 1e-6);
		const double H = FMath::Min(Left, FMath::Max(1.0, Timescale * 0.05));
		ISOrbit::Kepler(Bd.Mu, R, V, H);
		T += H; Left -= H;

		const double rr = R.Length();
		if (rr < Bd.Radius + Bd.AtmoTop * 0.5)
		{
			bCrashed = true;
			LastEvent = Bd.AtmoTop > 0 ? FString::Printf(TEXT("Re-entered %s's atmosphere and burned up."), *Bd.Name) : FString::Printf(TEXT("Impacted %s."), *Bd.Name);
			return;
		}
		// leaving the sphere of influence -> parent frame
		if (Bd.Parent >= 0 && rr > Bd.Soi)
		{
			R += ISOrbit::BodyPos(Body, T, true);
			V += ISOrbit::BodyVel(Body, T, true);
			LastEvent = FString::Printf(TEXT("Left %s's sphere of influence."), *Bd.Name);
			Body = Bd.Parent;
			continue;
		}
		// entering a child body's sphere of influence
		for (int32 i = 0; i < L.Num(); ++i)
		{
			if (L[i].Parent != Body) continue;
			const FVector3d D = R - ISOrbit::BodyPos(i, T, true);
			if (D.Length() < L[i].Soi)
			{
				R = D;
				V -= ISOrbit::BodyVel(i, T, true);
				Body = i;
				LastEvent = FString::Printf(TEXT("Entered %s's sphere of influence!"), *L[i].Name);
				break;
			}
		}
	}
}

void FISCraft::Predict(double T, int32 Samples, TArray<FVector3d>& Out, int32& NextBody) const
{
	Out.Reset();
	NextBody = -1;
	const TArray<FISBody>& L = ISOrbit::Bodies();
	const FISBody& Bd = L[Body];
	const FISElements E = Elements();
	FVector3d r = R, v = V;
	const bool bClosed = E.E < 1.0 && (E.Apoapsis + Bd.Radius) < Bd.Soi;
	const double Span = bClosed ? E.Period : FMath::Max(3600.0, 4.0 * Bd.Soi / FMath::Max(V.Length(), 0.01));
	const double Dt = Span / Samples;
	Out.Add(r);
	double t = T;
	for (int32 i = 0; i < Samples; ++i)
	{
		ISOrbit::Kepler(Bd.Mu, r, v, Dt);
		t += Dt;
		Out.Add(r);
		const double rr = r.Length();
		if (rr < Bd.Radius) { NextBody = -2; break; }
		if (Bd.Parent >= 0 && rr > Bd.Soi) { NextBody = Bd.Parent; break; }
		bool bHit = false;
		for (int32 c = 0; c < L.Num(); ++c)
			if (L[c].Parent == Body && (r - ISOrbit::BodyPos(c, t, true)).Length() < L[c].Soi) { NextBody = c; bHit = true; break; }
		if (bHit) break;
	}
}
